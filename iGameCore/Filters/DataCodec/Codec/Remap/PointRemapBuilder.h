#ifndef DATACODEC_CODEC_REMAP_POINTREMAPBUILDER_H
#define DATACODEC_CODEC_REMAP_POINTREMAPBUILDER_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Codec/Remap/RemapProvider.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Codec/Remap/Common/MortonRemapBuilder.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
namespace datacodec {
namespace pointremap {

inline constexpr std::uint32_t kMortonBitsPerAxis = 10;
inline constexpr std::uint32_t kMortonBucketCount = 1u << kMortonBitsPerAxis;
inline constexpr float kMortonMaxCoordinate = static_cast<float>(kMortonBucketCount - 1u);

template<typename TScalar>
inline void CopyPointTupleBytesToF32(const std::uint8_t* bytes, float* output) {
    TScalar tuple[3]{};
    std::memcpy(tuple, bytes, sizeof(tuple));
    output[0] = static_cast<float>(tuple[0]);
    output[1] = static_cast<float>(tuple[1]);
    output[2] = static_cast<float>(tuple[2]);
}

inline bool ConvertPointTupleBytesToF32(
    const ScalarType scalarType,
    const std::uint8_t* bytes,
    float* output,
    std::string* error = nullptr) {
    if (bytes == nullptr || output == nullptr) {
        return validation::AssignError(error, "point tuple conversion received a null buffer");
    }
    switch (scalarType) {
        case ScalarType::Float32:
            CopyPointTupleBytesToF32<float>(bytes, output);
            return true;
        case ScalarType::Float64:
            CopyPointTupleBytesToF32<double>(bytes, output);
            return true;
        case ScalarType::Int8:
            CopyPointTupleBytesToF32<std::int8_t>(bytes, output);
            return true;
        case ScalarType::UInt8:
            CopyPointTupleBytesToF32<std::uint8_t>(bytes, output);
            return true;
        case ScalarType::Int16:
            CopyPointTupleBytesToF32<std::int16_t>(bytes, output);
            return true;
        case ScalarType::UInt16:
            CopyPointTupleBytesToF32<std::uint16_t>(bytes, output);
            return true;
        case ScalarType::Int32:
            CopyPointTupleBytesToF32<std::int32_t>(bytes, output);
            return true;
        case ScalarType::UInt32:
            CopyPointTupleBytesToF32<std::uint32_t>(bytes, output);
            return true;
        case ScalarType::Int64:
            CopyPointTupleBytesToF32<std::int64_t>(bytes, output);
            return true;
        case ScalarType::UInt64:
            CopyPointTupleBytesToF32<std::uint64_t>(bytes, output);
            return true;
    }
    return validation::AssignError(error, "point tuple scalar type is unsupported");
}

inline bool ReadGeometryTupleAsF32(
    const NumericArrayView& geometry,
    const std::size_t pointIndex,
    float* output,
    std::string* error = nullptr) {
    const auto valueSize = geometry.ComponentSize();
    if (valueSize == 0u) {
        return validation::AssignError(error, "geometry scalar type is unsupported");
    }
    if (valueSize > sizeof(std::uint64_t)) {
        return validation::AssignError(error, "geometry scalar type is too large for point remap");
    }
    std::array<std::uint8_t, sizeof(std::uint64_t) * 3u> tupleBytes{};
    if (!ReadNumericArrayTupleBytes(geometry, pointIndex, 3u, tupleBytes.data(), error)) {
        return false;
    }
    return ConvertPointTupleBytesToF32(geometry.scalarType, tupleBytes.data(), output, error);
}

inline std::uint32_t MortonPart1By2(std::uint32_t value) {
    value &= 0x000003ffu;
    value = (value ^ (value << 16)) & 0xff0000ffu;
    value = (value ^ (value << 8)) & 0x0300f00fu;
    value = (value ^ (value << 4)) & 0x030c30c3u;
    value = (value ^ (value << 2)) & 0x09249249u;
    return value;
}

inline std::uint32_t BuildMortonKey(
    const float* value,
    const float* minValues,
    const float scale) {
    const auto quantizedX = static_cast<std::uint32_t>(
        (value[0] - minValues[0]) * scale * kMortonMaxCoordinate + 0.5f);
    const auto quantizedY = static_cast<std::uint32_t>(
        (value[1] - minValues[1]) * scale * kMortonMaxCoordinate + 0.5f);
    const auto quantizedZ = static_cast<std::uint32_t>(
        (value[2] - minValues[2]) * scale * kMortonMaxCoordinate + 0.5f);
    return MortonPart1By2(quantizedX) |
        (MortonPart1By2(quantizedY) << 1u) |
        (MortonPart1By2(quantizedZ) << 2u);
}

struct RemapProviders {
    std::shared_ptr<IRemapProvider> orderProvider;
    std::shared_ptr<IRemapProvider> inverseProvider;
};

struct BuildOptions {
    WritableRemapProviderFactory providerFactory{};
    bytestore::ByteStoreSession* byteStoreSession{nullptr};
    resource::ActiveByteBudget* scratchBudget{nullptr};
    std::size_t mortonLeafBudgetBytes{mortonremap::kMortonDefaultLeafBudgetBytes};
    std::size_t mortonRunBufferBytes{mortonremap::kMortonRunBufferBytes};
    bool useMemoryScratchStore{false};
};

template<typename TGetter>
inline bool BuildPointMortonRemapProviders(
    const std::size_t pointCount,
    TGetter&& reader,
    RemapProviders& result,
    std::string* error = nullptr,
    const BuildOptions& remapOptions = {}) {
    result = {};

    float minValues[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float maxValues[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    float value[3]{0.0f, 0.0f, 0.0f};
    for (std::size_t pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
        if (!reader(pointIndex, value, error)) {
            return false;
        }
        for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
            minValues[axisIndex] = std::min(minValues[axisIndex], value[axisIndex]);
            maxValues[axisIndex] = std::max(maxValues[axisIndex], value[axisIndex]);
        }
    }

    float extent = 0.0f;
    extent = std::max(extent, maxValues[0] - minValues[0]);
    extent = std::max(extent, maxValues[1] - minValues[1]);
    extent = std::max(extent, maxValues[2] - minValues[2]);
    const float scale = extent == 0.0f ? 0.0f : 1.0f / extent;

    mortonremap::MortonRemapResult remapResult;
    mortonremap::MortonRemapOptions options;
    options.resourcePrefix = "point_remap.morton";
    options.providerFactory = remapOptions.providerFactory;
    options.byteStoreSession = remapOptions.byteStoreSession;
    options.scratchBudget = remapOptions.scratchBudget;
    options.leafBudgetBytes = remapOptions.mortonLeafBudgetBytes;
    options.runBufferBytes = remapOptions.mortonRunBufferBytes;
    options.buildInverse = true;
    options.useMemoryScratchStore = remapOptions.useMemoryScratchStore;
    bool readFailed = false;
    std::string readError;
    const auto keyGetter = [&](const std::size_t pointIndex) {
        float localValue[3]{0.0f, 0.0f, 0.0f};
        if (!reader(pointIndex, localValue, &readError)) {
            readFailed = true;
            return std::uint32_t{0u};
        }
        return BuildMortonKey(localValue, minValues, scale);
    };
    if (!mortonremap::BuildMortonRemapProvider(pointCount, keyGetter, remapResult, options, error)) {
        return false;
    }
    if (readFailed) {
        return validation::AssignError(
            error,
            readError.empty() ? "failed to read geometry tuple for point remap" : readError);
    }
    result.orderProvider = std::move(remapResult.orderProvider);
    result.inverseProvider = std::move(remapResult.inverseProvider);
    return result.orderProvider != nullptr && result.inverseProvider != nullptr;
}

inline bool BuildPointMortonRemapProviders(
    const float* positions,
    const std::size_t pointCount,
    RemapProviders& result,
    std::string* error = nullptr,
    const BuildOptions& remapOptions = {}) {
    result = {};
    if (positions == nullptr) {
        return validation::AssignError(error, "point positions are null");
    }
    return BuildPointMortonRemapProviders(
        pointCount,
        [&](const std::size_t pointIndex, float* value, std::string*) {
            const auto* source = positions + pointIndex * 3u;
            value[0] = source[0];
            value[1] = source[1];
            value[2] = source[2];
            return true;
        },
        result,
        error,
        remapOptions);
}

inline bool BuildPointMortonRemapProviders(
    const NumericArrayView& geometry,
    RemapProviders& result,
    std::string* error = nullptr,
    const BuildOptions& remapOptions = {}) {
    result = {};
    const auto pointCount = geometry.tupleCount;
    if (!geometry.IsValid()) {
        return validation::AssignError(error, "geometry view is invalid for point remap");
    }
    if (geometry.componentCount < 3) {
        return validation::AssignError(error, "geometry view requires at least 3 components for point remap");
    }
    return BuildPointMortonRemapProviders(
        pointCount,
        [&](const std::size_t pointIndex, float* value, std::string* readError) {
            return ReadGeometryTupleAsF32(geometry, pointIndex, value, readError);
        },
        result,
        error,
        remapOptions);
}

} // namespace pointremap
} // namespace datacodec

#endif
