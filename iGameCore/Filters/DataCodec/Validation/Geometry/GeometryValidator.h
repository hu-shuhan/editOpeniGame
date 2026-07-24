#ifndef DATACODEC_VALIDATION_GEOMETRY_GEOMETRYVALIDATOR_H
#define DATACODEC_VALIDATION_GEOMETRY_GEOMETRYVALIDATOR_H

#include "DataCodec/API/Params/CodecStorageParams.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedGeometryCache.h"
#include "DataCodec/Validation/Result/ValidationResult.h"
#include "DataCodec/Validation/Common/FiniteNumericValidation.h"

#include <algorithm>
#include <cstddef>

namespace datacodec::validation {

class GeometryValidator final {
public:
    [[nodiscard]] static ValidationResult ValidateDecodedCacheShape(
        const CodecStorageParams& params,
        const DecodedGeometryCache& geometry) {
        std::size_t expectedPoints = 0u;
        if (!TryParamSizeToSizeT(params.geomParams.elementCount, expectedPoints)) {
            return ValidationResult::Failure(
                CodecErrorCode::InvalidInput,
                ValidationDomain::Geometry,
                "geometry.params.size",
                "geometry params exceed this platform size limit");
        }
        const auto expectedDimension = static_cast<std::size_t>(
            std::max(params.geomParams.dimension, 0));
        if (expectedPoints == 0u || expectedDimension == 0u) {
            return ValidationResult::Success(
                ValidationDomain::Geometry,
                "geometry.cache.shape");
        }
        if (!geometry.complete ||
            geometry.pointCount != expectedPoints ||
            geometry.dimension != expectedDimension) {
            return ValidationResult::Failure(
                CodecErrorCode::DecodeFailure,
                ValidationDomain::Geometry,
                "geometry.cache.shape",
                "decoded geometry cache shape does not match params");
        }
        return ValidationResult::Success(
            ValidationDomain::Geometry,
            "geometry.cache.shape");
    }

    [[nodiscard]] static ValidationResult ValidateFiniteValues(
        const CodecStorageParams& params,
        const DecodedGeometryCache& geometry) {
        if (params.geomParams.dataType != DataType::Float32 &&
            params.geomParams.dataType != DataType::Float64) {
            return ValidationResult::Success(
                ValidationDomain::Geometry,
                "geometry.values.finite");
        }
        std::uint64_t valueCount = 0u;
        if (!CheckedMulU64(
                params.geomParams.elementCount,
                static_cast<std::uint64_t>(std::max(params.geomParams.dimension, 0)),
                valueCount,
                "geometry finite validation value count")) {
            return ValidationResult::Failure(
                CodecErrorCode::InvalidInput,
                ValidationDomain::Geometry,
                "geometry.values.finite",
                "geometry finite validation value count overflows");
        }
        if (params.geomParams.dataType == DataType::Float64) {
            return ValidateFiniteNumericStore<double>(
                geometry.bytes,
                valueCount,
                ValidationDomain::Geometry,
                "geometry.values.finite");
        }
        return ValidateFiniteNumericStore<float>(
            geometry.bytes,
            valueCount,
            ValidationDomain::Geometry,
            "geometry.values.finite");
    }
};

} // namespace datacodec::validation

#endif
