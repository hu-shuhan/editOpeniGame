#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKDECODE_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKDECODE_H

#include "DataCodec/Codec/NumericArray/IntegerResidualCodec.h"
#include "DataCodec/Codec/NumericArray/NumericArrayCodec.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockFormat.h"
#include "DataCodec/Codec/SubCodec/VarintCodec.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <vector>
namespace datacodec {
namespace numericarray {

inline bool DecodeIntegerDeltaRunVarintComponentBytes(
    const NumericArrayBlockParams& params,
    const std::uint32_t elementCount,
    const std::span<const std::uint8_t> bytes,
    std::vector<std::uint8_t>& decodedComponent,
    std::string* error = nullptr) {
    decodedComponent.clear();
    if (!IsIntegerNumericArrayDataType(params.dataType)) {
        return validation::AssignError(error, "integer delta-run varint requires an integer data type");
    }

    std::size_t rawByteCount = 0u;
    if (!ResolveNumericArrayComponentRawByteCount(params, elementCount, rawByteCount, error)) {
        return false;
    }
    decodedComponent.assign(rawByteCount, 0u);
    if (elementCount == 0u) {
        if (!bytes.empty()) {
            validation::AssignError(error, "empty integer component has payload bytes");
        }
        return bytes.empty();
    }

    const auto valueSize = params.valueSize;
    const auto mask = IntegerStorageMask(valueSize);
    std::size_t cursor = 0u;
    std::uint32_t written = 0u;
    std::uint64_t previousValue = 0u;
    while (cursor < bytes.size()) {
        std::uint64_t delta = 0u;
        std::uint64_t runLength = 0u;
        if (!codec::DecodeVarint64(bytes, cursor, delta, error) ||
            !codec::DecodeVarint64(bytes, cursor, runLength, error)) {
            decodedComponent.clear();
            return false;
        }
        if (runLength == 0u ||
            runLength > static_cast<std::uint64_t>(elementCount - written)) {
            decodedComponent.clear();
            return validation::AssignError(error, "integer delta-run varint run length is invalid");
        }
        for (std::uint64_t runIndex = 0u; runIndex < runLength; ++runIndex) {
            const auto value = (previousValue + delta) & mask;
            WriteIntegerStorageValue(
                value,
                decodedComponent.data() + static_cast<std::size_t>(written) * valueSize,
                valueSize);
            previousValue = value;
            ++written;
        }
    }
    if (written != elementCount) {
        decodedComponent.clear();
        return validation::AssignError(error, "integer delta-run varint did not decode the expected element count");
    }
    return true;
}

inline bool DecodeNumericArrayComponentBytes(
    const NumericArrayBlockParams& params,
    const CompressorConfig& compressor,
    const std::uint32_t elementCount,
    const std::uint32_t componentIndex,
    const NumericArrayBytesCodec bytesCodec,
    const std::span<const std::uint8_t> bytes,
    std::vector<std::uint8_t>& decodedComponent,
    std::string* error = nullptr) {
    decodedComponent.clear();
    if (!ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }
    if (componentIndex >= static_cast<std::uint32_t>(params.componentCount)) {
        return validation::AssignError(error, "numeric array component bytes index is out of range");
    }

    std::size_t rawByteCount = 0u;
    if (!ResolveNumericArrayComponentRawByteCount(params, elementCount, rawByteCount, error)) {
        return false;
    }
    if (bytesCodec == NumericArrayBytesCodec::IntegerDeltaRunVarint) {
        return DecodeIntegerDeltaRunVarintComponentBytes(
            params,
            elementCount,
            bytes,
            decodedComponent,
            error);
    }
    if (bytesCodec == NumericArrayBytesCodec::IntegerDeltaLiteralRunVarint) {
        return DecodeIntegerDeltaLiteralRunVarintComponentBytes(
            params,
            elementCount,
            bytes,
            decodedComponent,
            error);
    }
    if (bytesCodec == NumericArrayBytesCodec::RawBytes) {
        if (bytes.size() != rawByteCount) {
            return validation::AssignError(error, "numeric array raw component byte count mismatch");
        }
        decodedComponent.assign(bytes.begin(), bytes.end());
        return true;
    }
    if (bytesCodec != NumericArrayBytesCodec::NumericArrayCodec) {
        return validation::AssignError(error, "unsupported numeric array component bytes codec");
    }

    const auto componentLayout = MakeNumericArrayComponentLayout(
        params.dataType,
        params.valueSize,
        static_cast<std::size_t>(elementCount));

    decodedComponent.resize(rawByteCount);
    if (!NumericArrayDecode::Decompress(
            bytes,
            componentLayout,
            compressor,
            MutableNumericArrayBufferView{decodedComponent.data(), componentLayout},
            error)) {
        decodedComponent.clear();
        return false;
    }
    return true;
}

inline bool DecodeNumericArrayComponentBundle(
    const NumericArrayBlockParams& params,
    const CompressorConfig& compressor,
    const std::uint32_t elementCount,
    const std::span<const NumericArrayComponentLayoutParams> componentLayouts,
    const std::span<const std::uint8_t> bytes,
    std::vector<std::uint8_t>& decodedBytes,
    std::string* error = nullptr) {
    decodedBytes.clear();
    if (!ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }
    const auto componentCount = params.componentCount;
    std::size_t tupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            params.valueSize,
            tupleBytes,
            "numeric array tuple bytes",
            error)) {
        return false;
    }
    std::size_t rawByteCount = 0u;
    if (!ResolveNumericArrayBlockRawByteCount(params, elementCount, rawByteCount, error)) {
        return false;
    }
    decodedBytes.assign(rawByteCount, 0u);

    std::size_t cursor = 0u;
    if (componentLayouts.size() != componentCount) {
        decodedBytes.clear();
        return validation::AssignError(error, "numeric array component layout count does not match field dimension");
    }

    std::vector<bool> seen(componentCount, false);
    std::vector<std::uint8_t> decodedComponent;
    for (const auto& componentLayout : componentLayouts) {
        const auto componentIndex = componentLayout.componentIndex;
        const auto encodedByteLength = componentLayout.encodedByteLength;
        if (componentIndex >= componentCount || seen[componentIndex] ||
            encodedByteLength > bytes.size() - cursor) {
            decodedBytes.clear();
            return validation::AssignError(error, "invalid numeric array component layout entry");
        }
        seen[componentIndex] = true;
        const auto componentBytes = std::span<const std::uint8_t>(
            bytes.data() + cursor,
            static_cast<std::size_t>(encodedByteLength));
        if (!validation::CheckedAddSizeT(
                cursor,
                static_cast<std::size_t>(encodedByteLength),
                cursor,
                "numeric array component bundle cursor",
                error)) {
            decodedBytes.clear();
            return false;
        }

        if (!DecodeNumericArrayComponentBytes(
                params,
                compressor,
                elementCount,
                componentIndex,
                componentLayout.bytesCodec,
                componentBytes,
                decodedComponent,
                error)) {
            decodedBytes.clear();
            return false;
        }
        for (std::size_t elementIndex = 0; elementIndex < static_cast<std::size_t>(elementCount); ++elementIndex) {
            std::memcpy(
                decodedBytes.data() + elementIndex * tupleBytes + componentIndex * params.valueSize,
                decodedComponent.data() + elementIndex * params.valueSize,
                params.valueSize);
        }
    }

    if (cursor != bytes.size()) {
        decodedBytes.clear();
        return validation::AssignError(error, "numeric array component bundle has trailing bytes");
    }
    return true;
}

inline bool ResolveDecodedNumericArrayBlockBytes(
    const NumericArrayBlockParams& params,
    const CompressorConfig& compressor,
    const std::uint32_t elementCount,
    const NumericArrayBytesCodec bytesCodec,
    const std::span<const NumericArrayComponentLayoutParams> componentLayouts,
    const std::span<const std::uint8_t> bytes,
    std::vector<std::uint8_t>& decodedBytes,
    std::string* error = nullptr) {
    decodedBytes.clear();
    if (!ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }
    std::size_t rawByteCount = 0u;
    if (!ResolveNumericArrayBlockRawByteCount(params, elementCount, rawByteCount, error)) {
        return false;
    }
    if (bytesCodec == NumericArrayBytesCodec::RawBytes) {
        if (rawByteCount == 0u && bytes.empty()) {
            return true;
        }
        return validation::AssignError(error, "numeric array block requires component bundle bytes");
    }
    if (bytesCodec != NumericArrayBytesCodec::NumericArrayCodec) {
        return validation::AssignError(error, "unsupported numeric array bytes codec");
    }
    return DecodeNumericArrayComponentBundle(params, compressor, elementCount, componentLayouts, bytes, decodedBytes, error);
}

template<typename TValue>
inline void AddResidualBytesInPlace(
    std::vector<std::uint8_t>& decodedBytes,
    const std::span<const std::uint8_t> residualBytes,
    const std::size_t componentCount,
    const std::size_t valueSize,
    const std::span<const NumericArrayRegionRunLayoutParams> runs) {
    const auto tupleBytes = validation::SaturatingMulSizeT(componentCount, valueSize);
    std::size_t residualElementIndex = 0u;
    for (const auto& run : runs) {
        for (ParamSize runIndex = 0u; runIndex < run.count; ++runIndex) {
            const auto targetElementIndex = static_cast<std::size_t>(run.begin + runIndex);
            for (std::size_t componentIndex = 0u; componentIndex < componentCount; ++componentIndex) {
                const auto targetOffset = targetElementIndex * tupleBytes + componentIndex * valueSize;
                const auto residualOffset = residualElementIndex * tupleBytes + componentIndex * valueSize;
                TValue targetValue{};
                TValue residualValue{};
                std::memcpy(&targetValue, decodedBytes.data() + targetOffset, sizeof(TValue));
                std::memcpy(&residualValue, residualBytes.data() + residualOffset, sizeof(TValue));
                targetValue += residualValue;
                std::memcpy(decodedBytes.data() + targetOffset, &targetValue, sizeof(TValue));
            }
            ++residualElementIndex;
        }
    }
}

inline bool ValidateRegionLayerDecodeRuns(
    const NumericArrayRegionLayerLayoutParams& layer,
    const std::uint32_t blockElementCount,
    std::string* error = nullptr) {
    ParamSize consumedElements = 0u;
    ParamSize previousEnd = 0u;
    bool hasPrevious = false;
    for (const auto& run : layer.runs) {
        if (run.count == 0u) {
            return validation::AssignError(error, "region residual layer contains an empty run");
        }
        if (run.begin > blockElementCount || run.count > static_cast<ParamSize>(blockElementCount) - run.begin) {
            return validation::AssignError(error, "region residual run exceeds the decoded block range");
        }
        if (hasPrevious && run.begin < previousEnd) {
            return validation::AssignError(error, "region residual runs overlap");
        }
        if (!validation::CheckedAddU64(run.begin, run.count, previousEnd, "region residual run end", error)) {
            return false;
        }
        hasPrevious = true;
        if (!validation::CheckedAddU64(
                consumedElements,
                run.count,
                consumedElements,
                "region residual consumed elements",
                error)) {
            return false;
        }
    }
    if (consumedElements != layer.refinedElementCount) {
        return validation::AssignError(error, "region residual run count does not match refined element count");
    }
    return true;
}

inline bool ResolveDecodedLayeredResidualNumericArrayBlockBytes(
    const NumericArrayBlockParams& params,
    const std::uint32_t elementCount,
    const CompressorConfig& backgroundCompressor,
    const ParamSize backgroundEncodedByteLength,
    const std::span<const NumericArrayComponentLayoutParams> backgroundComponentLayouts,
    const std::span<const NumericArrayRegionLayerLayoutParams> regionLayers,
    const std::span<const std::uint8_t> bytes,
    std::vector<std::uint8_t>& decodedBytes,
    std::string* error = nullptr) {
    decodedBytes.clear();
    if (!ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }
    if (!regionLayers.empty() &&
        params.dataType != DataType::Float32 &&
        params.dataType != DataType::Float64) {
        return validation::AssignError(error, "layered residual refinement requires float32 or float64 data");
    }
    std::size_t backgroundByteCount = 0u;
    if (!TryParamSizeToSizeT(backgroundEncodedByteLength, backgroundByteCount) ||
        backgroundByteCount > bytes.size()) {
        return validation::AssignError(error, "layered residual background payload length is invalid");
    }

    auto backgroundParams = params;
    backgroundParams.regionControl = nullptr;
    if (!ResolveDecodedNumericArrayBlockBytes(
            backgroundParams,
            backgroundCompressor,
            elementCount,
            NumericArrayBytesCodec::NumericArrayCodec,
            backgroundComponentLayouts,
            bytes.first(backgroundByteCount),
            decodedBytes,
            error)) {
        return false;
    }

    std::size_t cursor = backgroundByteCount;
    const auto componentCount = params.componentCount;
    std::size_t tupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            params.valueSize,
            tupleBytes,
            "layered residual tuple bytes",
            error)) {
        return false;
    }
    for (const auto& layer : regionLayers) {
        if (layer.regionId == 0u) {
            return validation::AssignError(error, "region residual layer id is invalid");
        }
        if (layer.residualBytesCodec != NumericArrayBytesCodec::NumericArrayCodec) {
            return validation::AssignError(error, "region residual layer uses an unsupported bytes codec");
        }
        if (!ValidateRegionLayerDecodeRuns(layer, elementCount, error)) {
            return false;
        }
        std::size_t residualByteLength = 0u;
        if (!TryParamSizeToSizeT(layer.residualEncodedByteLength, residualByteLength) ||
            residualByteLength > bytes.size() - cursor) {
            return validation::AssignError(error, "region residual payload length is invalid");
        }
        if (layer.refinedElementCount > static_cast<ParamSize>(std::numeric_limits<std::uint32_t>::max())) {
            return validation::AssignError(error, "region residual element count exceeds current block limits");
        }
        auto residualParams = params;
        residualParams.regionControl = nullptr;
        std::vector<std::uint8_t> residualBytes;
        if (!ResolveDecodedNumericArrayBlockBytes(
                residualParams,
                layer.refineCompressor,
                static_cast<std::uint32_t>(layer.refinedElementCount),
                layer.residualBytesCodec,
                layer.componentLayouts,
                std::span<const std::uint8_t>(bytes.data() + cursor, residualByteLength),
                residualBytes,
                error)) {
            return false;
        }
        std::size_t refinedElementCount = 0u;
        std::size_t expectedResidualBytes = 0u;
        if (!validation::CheckedCastSizeT(
                layer.refinedElementCount,
                refinedElementCount,
                "region residual refined element count",
                error) ||
            !validation::CheckedMulSizeT(
                refinedElementCount,
                tupleBytes,
                expectedResidualBytes,
                "region residual expected bytes",
                error)) {
            return false;
        }
        if (residualBytes.size() != expectedResidualBytes) {
            return validation::AssignError(error, "region residual decoded bytes do not match refined range");
        }
        if (params.dataType == DataType::Float32) {
            AddResidualBytesInPlace<float>(
                decodedBytes,
                std::span<const std::uint8_t>(residualBytes.data(), residualBytes.size()),
                componentCount,
                params.valueSize,
                std::span<const NumericArrayRegionRunLayoutParams>(layer.runs.data(), layer.runs.size()));
        } else {
            AddResidualBytesInPlace<double>(
                decodedBytes,
                std::span<const std::uint8_t>(residualBytes.data(), residualBytes.size()),
                componentCount,
                params.valueSize,
                std::span<const NumericArrayRegionRunLayoutParams>(layer.runs.data(), layer.runs.size()));
        }
        if (!validation::CheckedAddSizeT(
                cursor,
                residualByteLength,
                cursor,
                "layered residual payload cursor",
                error)) {
            return false;
        }
    }
    if (cursor != bytes.size()) {
        decodedBytes.clear();
        return validation::AssignError(error, "layered residual block has trailing payload bytes");
    }
    return true;
}

} // namespace numericarray
} // namespace datacodec

#endif
