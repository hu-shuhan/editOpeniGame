#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKENCODE_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKENCODE_H

#include "DataCodec/Storage/ByteIO/ScratchByteBuffer.h"
#include "DataCodec/Codec/NumericArray/IntegerResidualCodec.h"
#include "DataCodec/Codec/NumericArray/NumericArrayCodec.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockDecode.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockFormat.h"
#include "DataCodec/Codec/NumericArray/NumericArrayRegionPlan.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {
namespace numericarray {

inline bool ResolveEncodedNumericArrayComponentBytes(
    const NumericArrayBlockParams& params,
    const CompressorConfig& compressor,
    const std::uint32_t elementCount,
    const std::span<const std::uint8_t> rawBytes,
    NumericArrayEncodedBytes& encodedBytes,
    NumericArrayBytesCodec& bytesCodec,
    std::string* error = nullptr) {
    encodedBytes.Reset();
    bytesCodec = NumericArrayBytesCodec::RawBytes;
    if (!ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }

    std::size_t rawByteCount = 0u;
    if (!ResolveNumericArrayComponentRawByteCount(params, elementCount, rawByteCount, error)) {
        return false;
    }
    if (rawBytes.size() != rawByteCount) {
        return validation::AssignError(error, "numeric array component bytes size does not match block range");
    }
    if (rawBytes.empty()) {
        return true;
    }

    if (IsIntegerNumericArrayDataType(params.dataType)) {
        std::vector<std::uint8_t> integerEncodedBytes;
        if (!EncodeIntegerDeltaLiteralRunVarintComponentBytes(
                params,
                elementCount,
                rawBytes,
                integerEncodedBytes,
                error)) {
            return false;
        }
        encodedBytes.TakeVector(std::move(integerEncodedBytes));
        bytesCodec = NumericArrayBytesCodec::IntegerDeltaLiteralRunVarint;
        return true;
    }

    // SZ3 会将长度为 1 的维度归一化为空维度，退化浮点块直接保留原始字节
    if (elementCount == 1u) {
        encodedBytes.TakeVector(std::vector<std::uint8_t>(rawBytes.begin(), rawBytes.end()));
        return true;
    }

    NumericArrayEncodedBytes compressed;
    std::string compressError;
    if (!NumericArrayEncode::Compress(
            NumericArrayBufferView{
                rawBytes.data(),
                MakeNumericArrayComponentLayout(
                    params.dataType,
                    params.valueSize,
                    static_cast<std::size_t>(elementCount)),
            },
            compressor,
            compressed,
            &compressError)) {
        return validation::AssignError(
            error,
            compressError.empty()
                ? "numeric array component codec compression failed"
                : compressError);
    }
    if (compressed.Empty()) {
        return validation::AssignError(
            error,
            "numeric array component codec returned an empty payload");
    }

    encodedBytes = std::move(compressed);
    bytesCodec = NumericArrayBytesCodec::NumericArrayCodec;
    return true;
}

inline bool ResolveEncodedNumericArrayBlockBytes(
    const NumericArrayBlockParams& params,
    const CompressorConfig& compressor,
    const std::uint32_t elementCount,
    const std::span<const std::uint8_t> rawBytes,
    std::vector<std::uint8_t>& encodedBytes,
    NumericArrayBytesCodec& bytesCodec,
    std::string* error = nullptr,
    ScratchByteBufferPool* scratchBytePool = nullptr,
    std::vector<NumericArrayComponentLayoutParams>* componentLayouts = nullptr) {
    encodedBytes.clear();
    bytesCodec = NumericArrayBytesCodec::NumericArrayCodec;
    if (!ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }

    std::size_t rawByteCount = 0u;
    if (!ResolveNumericArrayBlockRawByteCount(params, elementCount, rawByteCount, error)) {
        return false;
    }
    if (rawBytes.size() != rawByteCount) {
        return validation::AssignError(error, "numeric array block raw bytes size does not match block range");
    }
    const auto componentCount = params.componentCount;
    std::size_t tupleBytes = 0u;
    std::size_t componentByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            params.valueSize,
            tupleBytes,
            "numeric array tuple bytes",
            error) ||
        !validation::CheckedMulSizeT(
            static_cast<std::size_t>(elementCount),
            params.valueSize,
            componentByteCount,
            "numeric array component raw bytes",
            error)) {
        return false;
    }
    if (scratchBytePool == nullptr) {
        return validation::AssignError(error, "numeric array block encoding requires a memory pool");
    }

    std::vector<EncodedNumericArrayComponentByteView> componentViews;
    std::vector<ScratchByteBuffer> componentRawBuffers;
    std::vector<NumericArrayEncodedBytes> componentEncodedBytes;
    componentViews.reserve(componentCount);
    componentRawBuffers.reserve(componentCount);
    componentEncodedBytes.reserve(componentCount);
    for (std::size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex) {
        std::vector<std::uint8_t>* componentBytes = nullptr;
        componentRawBuffers.push_back(scratchBytePool->Acquire(componentByteCount));
        componentBytes = &componentRawBuffers.back().Bytes();
        std::fill(componentBytes->begin(), componentBytes->end(), 0u);
        for (std::size_t elementIndex = 0; elementIndex < static_cast<std::size_t>(elementCount); ++elementIndex) {
            std::memcpy(
                componentBytes->data() + elementIndex * params.valueSize,
                rawBytes.data() + elementIndex * tupleBytes + componentIndex * params.valueSize,
                params.valueSize);
        }
        NumericArrayBytesCodec componentBytesCodec{NumericArrayBytesCodec::RawBytes};
        componentEncodedBytes.emplace_back();
        auto& componentEncodedOwner = componentEncodedBytes.back();
        if (!ResolveEncodedNumericArrayComponentBytes(
                params,
                compressor,
                elementCount,
                std::span<const std::uint8_t>(componentBytes->data(), componentBytes->size()),
                componentEncodedOwner,
                componentBytesCodec,
                error)) {
            return false;
        }
        componentViews.push_back(EncodedNumericArrayComponentByteView{
            .componentIndex = static_cast<std::uint32_t>(componentIndex),
            .bytesCodec = componentBytesCodec,
            .bytes = componentEncodedOwner.Bytes(),
        });
    }

    return AppendNumericArrayComponentBundle(
        encodedBytes,
        std::span<const EncodedNumericArrayComponentByteView>(componentViews.data(), componentViews.size()),
        componentLayouts,
        error);
}

template<typename TValue>
inline bool BuildResidualComponentRawBytes(
    const std::span<const std::uint8_t> sourceComponentBytes,
    const std::span<const std::uint8_t> baseComponentBytes,
    const std::span<const NumericArrayRegionRunLayoutParams> runs,
    const std::size_t valueSize,
    const std::uint32_t elementCount,
    std::vector<std::uint8_t>& residualBytes,
    std::string* error = nullptr) {
    ParamSize refinedElementCount = 0u;
    for (const auto& run : runs) {
        if (run.begin > elementCount || run.count > static_cast<ParamSize>(elementCount) - run.begin) {
            return validation::AssignError(error, "region residual encode run exceeds block range");
        }
        if (!validation::CheckedAddU64(
                refinedElementCount,
                run.count,
                refinedElementCount,
                "region residual refined element count",
                error)) {
            return false;
        }
    }
    std::size_t localRefinedElementCount = 0u;
    std::size_t residualByteCount = 0u;
    if (!validation::CheckedCastSizeT(
            refinedElementCount,
            localRefinedElementCount,
            "region residual refined element count",
            error) ||
        !validation::CheckedMulSizeT(
            localRefinedElementCount,
            valueSize,
            residualByteCount,
            "region residual raw bytes",
            error)) {
        return false;
    }
    residualBytes.assign(residualByteCount, 0u);
    std::size_t residualElementIndex = 0u;
    for (const auto& run : runs) {
        for (ParamSize runIndex = 0u; runIndex < run.count; ++runIndex) {
            const auto sourceElementIndex = static_cast<std::size_t>(run.begin + runIndex);
            TValue sourceValue{};
            TValue baseValue{};
            std::memcpy(&sourceValue, sourceComponentBytes.data() + sourceElementIndex * valueSize, sizeof(TValue));
            std::memcpy(&baseValue, baseComponentBytes.data() + sourceElementIndex * valueSize, sizeof(TValue));
            const auto residualValue = static_cast<TValue>(sourceValue - baseValue);
            std::memcpy(residualBytes.data() + residualElementIndex * valueSize, &residualValue, sizeof(TValue));
            ++residualElementIndex;
        }
    }
    return true;
}

template<typename TValue>
inline bool AddResidualComponentBytesInPlace(
    std::vector<std::uint8_t>& baseComponentBytes,
    const std::span<const std::uint8_t> residualComponentBytes,
    const std::span<const NumericArrayRegionRunLayoutParams> runs,
    const std::size_t valueSize,
    const std::uint32_t elementCount,
    std::string* error = nullptr) {
    ParamSize refinedElementCount = 0u;
    for (const auto& run : runs) {
        if (run.begin > elementCount || run.count > static_cast<ParamSize>(elementCount) - run.begin) {
            return validation::AssignError(error, "region residual update run exceeds block range");
        }
        if (!validation::CheckedAddU64(
                refinedElementCount,
                run.count,
                refinedElementCount,
                "region residual refined element count",
                error)) {
            return false;
        }
    }
    std::size_t localRefinedElementCount = 0u;
    std::size_t expectedResidualBytes = 0u;
    if (!validation::CheckedCastSizeT(
            refinedElementCount,
            localRefinedElementCount,
            "region residual refined element count",
            error) ||
        !validation::CheckedMulSizeT(
            localRefinedElementCount,
            valueSize,
            expectedResidualBytes,
            "region residual raw bytes",
            error)) {
        return false;
    }
    if (residualComponentBytes.size() != expectedResidualBytes) {
        return validation::AssignError(error, "decoded region residual component bytes do not match run-list");
    }
    std::size_t residualElementIndex = 0u;
    for (const auto& run : runs) {
        for (ParamSize runIndex = 0u; runIndex < run.count; ++runIndex) {
            const auto targetElementIndex = static_cast<std::size_t>(run.begin + runIndex);
            TValue targetValue{};
            TValue residualValue{};
            std::memcpy(&targetValue, baseComponentBytes.data() + targetElementIndex * valueSize, sizeof(TValue));
            std::memcpy(&residualValue, residualComponentBytes.data() + residualElementIndex * valueSize, sizeof(TValue));
            targetValue = static_cast<TValue>(targetValue + residualValue);
            std::memcpy(baseComponentBytes.data() + targetElementIndex * valueSize, &targetValue, sizeof(TValue));
            ++residualElementIndex;
        }
    }
    return true;
}

inline bool ResolveEncodedLayeredResidualNumericArrayBlockBytes(
    const NumericArrayBlockParams& params,
    const NumericArrayRegionControlParams& regionControl,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const std::span<const std::uint8_t> rawBytes,
    const std::span<const RegionRun> regionRuns,
    std::vector<std::uint8_t>& encodedBytes,
    NumericArrayBytesCodec& bytesCodec,
    NumericArrayBlockLayoutParams& layout,
    std::string* error = nullptr,
    ScratchByteBufferPool* scratchBytePool = nullptr) {
    encodedBytes.clear();
    bytesCodec = NumericArrayBytesCodec::NumericArrayCodec;
    layout = {};
    if (!ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }
    if (!ValidateRegionControlForEncode(regionControl, error)) {
        return false;
    }
    if (scratchBytePool == nullptr) {
        return validation::AssignError(error, "layered residual block encoding requires a memory pool");
    }
    std::size_t rawByteCount = 0u;
    if (!ResolveNumericArrayBlockRawByteCount(params, elementCount, rawByteCount, error)) {
        return false;
    }
    if (rawBytes.size() != rawByteCount) {
        return validation::AssignError(error, "layered residual raw bytes size does not match block range");
    }

    LayeredResidualRegionPlan layeredPlan;
    if (!BuildNormalizedRegionPlansFromRegionRuns(
            regionRuns,
            params.elementCount,
            elementOffset,
            elementCount,
            regionControl,
            layeredPlan,
            error)) {
        return false;
    }
    if (!layeredPlan.layers.empty() &&
        params.dataType != DataType::Float32 &&
        params.dataType != DataType::Float64) {
        return validation::AssignError(error, "region residual refinement requires float32 or float64 data");
    }

    const auto componentCount = params.componentCount;
    std::size_t tupleBytes = 0u;
    std::size_t componentByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            params.valueSize,
            tupleBytes,
            "layered residual tuple bytes",
            error) ||
        !validation::CheckedMulSizeT(
            static_cast<std::size_t>(elementCount),
            params.valueSize,
            componentByteCount,
            "layered residual component raw bytes",
            error)) {
        return false;
    }
    auto backgroundParams = params;
    backgroundParams.regionControl = nullptr;

    std::vector<NumericArrayEncodedBytes> backgroundOwners;
    std::vector<EncodedNumericArrayComponentByteView> backgroundViews;
    backgroundOwners.reserve(componentCount);
    backgroundViews.reserve(componentCount);

    std::vector<NumericArrayRegionLayerLayoutParams> regionLayers;
    regionLayers.reserve(layeredPlan.layers.size());
    for (const auto& plan : layeredPlan.layers) {
        regionLayers.push_back(MakeRegionLayerLayoutFromPlan(plan));
    }
    std::vector<std::vector<NumericArrayEncodedBytes>> regionOwners(layeredPlan.layers.size());
    std::vector<std::vector<EncodedNumericArrayComponentByteView>> regionViews(layeredPlan.layers.size());
    for (std::size_t layerIndex = 0u; layerIndex < layeredPlan.layers.size(); ++layerIndex) {
        regionOwners[layerIndex].reserve(componentCount);
        regionViews[layerIndex].reserve(componentCount);
    }

    for (std::size_t componentIndex = 0u; componentIndex < componentCount; ++componentIndex) {
        auto componentRaw = scratchBytePool->Acquire(componentByteCount);
        auto& componentRawBytes = componentRaw.Bytes();
        std::fill(componentRawBytes.begin(), componentRawBytes.end(), 0u);
        for (std::size_t elementIndex = 0u; elementIndex < static_cast<std::size_t>(elementCount); ++elementIndex) {
            std::memcpy(
                componentRawBytes.data() + elementIndex * params.valueSize,
                rawBytes.data() + elementIndex * tupleBytes + componentIndex * params.valueSize,
                params.valueSize);
        }

        NumericArrayBytesCodec backgroundComponentCodec{NumericArrayBytesCodec::RawBytes};
        backgroundOwners.emplace_back();
        auto& backgroundOwner = backgroundOwners.back();
        if (!ResolveEncodedNumericArrayComponentBytes(
                backgroundParams,
                layeredPlan.backgroundCompressor,
                elementCount,
                std::span<const std::uint8_t>(componentRawBytes.data(), componentRawBytes.size()),
                backgroundOwner,
                backgroundComponentCodec,
                error)) {
            return false;
        }
        std::vector<std::uint8_t> baseDecodedComponent;
        if (!DecodeNumericArrayComponentBytes(
                backgroundParams,
                layeredPlan.backgroundCompressor,
                elementCount,
                static_cast<std::uint32_t>(componentIndex),
                backgroundComponentCodec,
                backgroundOwner.Bytes(),
                baseDecodedComponent,
                error)) {
            return false;
        }
        backgroundViews.push_back(EncodedNumericArrayComponentByteView{
            .componentIndex = static_cast<std::uint32_t>(componentIndex),
            .bytesCodec = backgroundComponentCodec,
            .bytes = backgroundOwner.Bytes(),
        });

        for (std::size_t layerIndex = 0u; layerIndex < layeredPlan.layers.size(); ++layerIndex) {
            const auto& plan = layeredPlan.layers[layerIndex];
            if (plan.refinedElementCount > static_cast<ParamSize>(std::numeric_limits<std::uint32_t>::max())) {
                return validation::AssignError(error, "region residual element count exceeds current block format");
            }
            std::vector<std::uint8_t> residualRawBytes;
            const auto runSpan = std::span<const NumericArrayRegionRunLayoutParams>(
                plan.runs.data(),
                plan.runs.size());
            const auto sourceSpan = std::span<const std::uint8_t>(
                componentRawBytes.data(),
                componentRawBytes.size());
            const auto baseSpan = std::span<const std::uint8_t>(
                baseDecodedComponent.data(),
                baseDecodedComponent.size());
            const bool residualOk = params.dataType == DataType::Float32
                ? BuildResidualComponentRawBytes<float>(
                    sourceSpan,
                    baseSpan,
                    runSpan,
                    params.valueSize,
                    elementCount,
                    residualRawBytes,
                    error)
                : BuildResidualComponentRawBytes<double>(
                    sourceSpan,
                    baseSpan,
                    runSpan,
                    params.valueSize,
                    elementCount,
                    residualRawBytes,
                    error);
            if (!residualOk) {
                return false;
            }
            auto residualParams = params;
            residualParams.regionControl = nullptr;
            NumericArrayBytesCodec residualComponentCodec{NumericArrayBytesCodec::RawBytes};
            regionOwners[layerIndex].emplace_back();
            auto& residualOwner = regionOwners[layerIndex].back();
            if (!ResolveEncodedNumericArrayComponentBytes(
                    residualParams,
                    plan.refineCompressor,
                    static_cast<std::uint32_t>(plan.refinedElementCount),
                    std::span<const std::uint8_t>(residualRawBytes.data(), residualRawBytes.size()),
                    residualOwner,
                    residualComponentCodec,
                    error)) {
                return false;
            }
            std::vector<std::uint8_t> decodedResidualComponent;
            if (!DecodeNumericArrayComponentBytes(
                    residualParams,
                    plan.refineCompressor,
                    static_cast<std::uint32_t>(plan.refinedElementCount),
                    static_cast<std::uint32_t>(componentIndex),
                    residualComponentCodec,
                    residualOwner.Bytes(),
                    decodedResidualComponent,
                    error)) {
                return false;
            }
            const bool updateOk = params.dataType == DataType::Float32
                ? AddResidualComponentBytesInPlace<float>(
                    baseDecodedComponent,
                    std::span<const std::uint8_t>(decodedResidualComponent.data(), decodedResidualComponent.size()),
                    runSpan,
                    params.valueSize,
                    elementCount,
                    error)
                : AddResidualComponentBytesInPlace<double>(
                    baseDecodedComponent,
                    std::span<const std::uint8_t>(decodedResidualComponent.data(), decodedResidualComponent.size()),
                    runSpan,
                    params.valueSize,
                    elementCount,
                    error);
            if (!updateOk) {
                return false;
            }
            regionViews[layerIndex].push_back(EncodedNumericArrayComponentByteView{
                .componentIndex = static_cast<std::uint32_t>(componentIndex),
                .bytesCodec = residualComponentCodec,
                .bytes = residualOwner.Bytes(),
            });
        }
    }

    std::vector<std::uint8_t> backgroundBundleBytes;
    if (!AppendNumericArrayComponentBundle(
            backgroundBundleBytes,
            std::span<const EncodedNumericArrayComponentByteView>(backgroundViews.data(), backgroundViews.size()),
            &layout.componentLayouts,
            error)) {
        return false;
    }
    encodedBytes.insert(encodedBytes.end(), backgroundBundleBytes.begin(), backgroundBundleBytes.end());

    layout.regionLayers = std::move(regionLayers);
    for (std::size_t layerIndex = 0u; layerIndex < layeredPlan.layers.size(); ++layerIndex) {
        std::vector<std::uint8_t> residualBundleBytes;
        if (!AppendNumericArrayComponentBundle(
                residualBundleBytes,
                std::span<const EncodedNumericArrayComponentByteView>(
                    regionViews[layerIndex].data(),
                    regionViews[layerIndex].size()),
                &layout.regionLayers[layerIndex].componentLayouts,
                error)) {
            return false;
        }
        layout.regionLayers[layerIndex].residualBytesCodec = NumericArrayBytesCodec::NumericArrayCodec;
        layout.regionLayers[layerIndex].residualEncodedByteLength =
            static_cast<ParamSize>(residualBundleBytes.size());
        encodedBytes.insert(encodedBytes.end(), residualBundleBytes.begin(), residualBundleBytes.end());
    }

    layout.mode = NumericArrayBlockMode::LayeredResidual;
    layout.referenceKind = NumericArrayReferenceKind::None;
    layout.codecId = NumericArrayReferenceCodecId::NonReference;
    layout.localParentFieldIndex = 0xFFFFu;
    layout.elementOffset = elementOffset;
    layout.elementCount = elementCount;
    layout.encodedByteLength = static_cast<ParamSize>(encodedBytes.size());
    layout.bytesCodec = NumericArrayBytesCodec::NumericArrayCodec;
    layout.backgroundCompressor = layeredPlan.backgroundCompressor;
    layout.backgroundEncodedByteLength = static_cast<ParamSize>(backgroundBundleBytes.size());
    bytesCodec = layout.bytesCodec;
    return true;
}

} // namespace numericarray
} // namespace datacodec

#endif
