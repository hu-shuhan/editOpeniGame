#ifndef DATACODEC_VALIDATION_ATTRIBUTE_ATTRIBUTEVALIDATOR_H
#define DATACODEC_VALIDATION_ATTRIBUTE_ATTRIBUTEVALIDATOR_H

#include "DataCodec/API/Params/CodecStorageParams.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedAttributeCacheSet.h"
#include "DataCodec/Validation/Result/ValidationResult.h"
#include "DataCodec/Validation/Common/FiniteNumericValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace datacodec::validation {

class AttributeValidator final {
public:
    [[nodiscard]] static ValidationResult ValidateDecodedCacheShape(
        const CodecStorageParams& params,
        const DecodedAttributeCacheSet& attributes,
        const std::span<const std::size_t> attrIndices) {
        if (attrIndices.empty()) {
            return ValidationResult::Success(
                ValidationDomain::Attribute,
                "attribute.cache.shape");
        }
        if (!attributes.IsInitialized() ||
            attributes.FieldCount() != params.attrParams.size()) {
            return ValidationResult::Failure(
                CodecErrorCode::DecodeFailure,
                ValidationDomain::Attribute,
                "attribute.cache.shape",
                "decoded attribute cache shape does not match params");
        }
        for (const auto attrIndex : attrIndices) {
            if (attrIndex >= params.attrParams.size() ||
                !attributes.Complete(attrIndex)) {
                return ValidationResult::Failure(
                    CodecErrorCode::DecodeFailure,
                    ValidationDomain::Attribute,
                    "attribute.cache.coverage",
                    "requested decoded attribute cache shape does not match params");
            }
        }
        return ValidationResult::Success(
            ValidationDomain::Attribute,
            "attribute.cache.shape");
    }

    [[nodiscard]] static ValidationResult ValidateFiniteValues(
        const CodecStorageParams& params,
        const DecodedAttributeCacheSet& attributes,
        const std::span<const std::size_t> attrIndices) {
        for (const auto attrIndex : attrIndices) {
            if (attrIndex >= params.attrParams.size()) {
                return ValidationResult::Failure(
                    CodecErrorCode::InvalidInput,
                    ValidationDomain::Attribute,
                    "attribute.values.finite",
                    "attribute finite validation index is out of range");
            }
            const auto& meta = params.attrParams[attrIndex];
            if (meta.dataType != DataType::Float32 && meta.dataType != DataType::Float64) {
                continue;
            }
            std::uint64_t valueCount = 0u;
            if (!CheckedMulU64(
                    meta.elementCount,
                    static_cast<std::uint64_t>(std::max(meta.dimension, 0)),
                    valueCount,
                    "attribute finite validation value count")) {
                return ValidationResult::Failure(
                    CodecErrorCode::InvalidInput,
                    ValidationDomain::Attribute,
                    "attribute.values.finite",
                    "attribute finite validation value count overflows");
            }
            auto validationResult = meta.dataType == DataType::Float64
                ? ValidateFiniteNumericStore<double>(
                    attributes.Bytes(attrIndex),
                    valueCount,
                    ValidationDomain::Attribute,
                    "attribute.values.finite")
                : ValidateFiniteNumericStore<float>(
                    attributes.Bytes(attrIndex),
                    valueCount,
                    ValidationDomain::Attribute,
                    "attribute.values.finite");
            if (!validationResult) { return validationResult; }
        }
        return ValidationResult::Success(
            ValidationDomain::Attribute,
            "attribute.values.finite");
    }
};

} // namespace datacodec::validation

#endif
