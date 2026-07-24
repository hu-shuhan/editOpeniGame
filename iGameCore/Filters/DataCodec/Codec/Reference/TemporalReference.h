#ifndef DATACODEC_CODEC_REFERENCE_TEMPORALREFERENCE_H
#define DATACODEC_CODEC_REFERENCE_TEMPORALREFERENCE_H

#include "DataCodec/Storage/ByteIO/ScratchByteBuffer.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockEncode.h"
#include "DataCodec/Codec/NumericArray/NumericArrayReader.h"
#include "DataCodec/Codec/Reference/ReferenceCodec.h"
#include "DataCodec/Codec/SubCodec/ZstdCodec.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <vector>
namespace datacodec {

// ============================================================
// 帧间 temporal reference 是否启用
// ============================================================

inline bool IsTemporalReferenceEnabled(const AttrReferenceControlParams& params) noexcept {
    return params.enabled &&
        params.temporalField.codec != TemporalFieldReferenceCodec::Disabled;
}

inline NumericArrayReferenceCodecId ResolveTemporalCodecId(
    const TemporalFieldReferenceCodec codec) noexcept {
    switch (codec) {
        case TemporalFieldReferenceCodec::Wavelet:
            return NumericArrayReferenceCodecId::Wavelet;
        case TemporalFieldReferenceCodec::Predictor:
            return NumericArrayReferenceCodecId::Predictor;
        case TemporalFieldReferenceCodec::Disabled:
        default:
            return NumericArrayReferenceCodecId::NonReference;
    }
}

// ============================================================
// Predictor offset 搜索
// ============================================================

struct TemporalPredictorOffsetSearchResult {
    std::int32_t offset{0};
    double squaredError{std::numeric_limits<double>::infinity()};
    std::size_t estimatedBytes{std::numeric_limits<std::size_t>::max()};
};

inline bool EstimateCompressedBytes(
    const std::span<const std::uint8_t> bytes,
    ScratchByteBufferPool& scratchBytePool,
    std::size_t& compressedBytes,
    std::string* error = nullptr) {
    compressedBytes = 0u;
    if (bytes.empty()) {
        return true;
    }
    auto buf = scratchBytePool.Acquire(0u);
    auto& compressed = buf.Bytes();
    if (!codec::ZstdCodec::Compress(
            std::span<const std::uint8_t>(bytes.data(), bytes.size()),
            1,
            compressed,
            error)) {
        return false;
    }
    compressedBytes = compressed.size();
    return true;
}

inline bool UsesEstimatedBytesTemporalPredictorSearch(
    const TemporalPredictorSearchStrategy strategy) {
    return strategy == TemporalPredictorSearchStrategy::ExhaustiveEstimatedBytes ||
        strategy == TemporalPredictorSearchStrategy::CoarseToFineEstimatedBytes;
}

inline bool UsesCoarseToFineTemporalPredictorSearch(
    const TemporalPredictorSearchStrategy strategy) {
    return strategy == TemporalPredictorSearchStrategy::CoarseToFineL2 ||
        strategy == TemporalPredictorSearchStrategy::CoarseToFineEstimatedBytes;
}

inline std::int32_t ResolveTemporalPredictorCoarseStep(const std::int32_t radius) {
    return std::max<std::int32_t>(1, radius / 4);
}

inline void AppendUniqueOffset(std::vector<std::int32_t>& offsets, const std::int32_t offset) {
    if (std::find(offsets.begin(), offsets.end(), offset) == offsets.end()) {
        offsets.push_back(offset);
    }
}

inline std::vector<std::int32_t> BuildExhaustiveOffsets(const std::int32_t radius) {
    std::vector<std::int32_t> offsets;
    offsets.reserve(static_cast<std::size_t>(radius * 2 + 1));
    for (std::int32_t o = -radius; o <= radius; ++o) { offsets.push_back(o); }
    return offsets;
}

inline std::vector<std::int32_t> BuildCoarseOffsets(const std::int32_t radius) {
    std::vector<std::int32_t> offsets;
    const auto step = ResolveTemporalPredictorCoarseStep(radius);
    for (std::int32_t o = -radius; o <= radius; o += step) { AppendUniqueOffset(offsets, o); }
    AppendUniqueOffset(offsets, 0);
    AppendUniqueOffset(offsets, radius);
    return offsets;
}

inline std::vector<std::int32_t> BuildRefinedOffsets(
    const std::int32_t radius,
    const std::int32_t coarseBest) {
    const auto step = ResolveTemporalPredictorCoarseStep(radius);
    if (step <= 1) { return BuildExhaustiveOffsets(radius); }
    std::vector<std::int32_t> offsets;
    const auto begin = std::max(-radius, coarseBest - step + 1);
    const auto end = std::min(radius, coarseBest + step - 1);
    for (std::int32_t o = begin; o <= end; ++o) { AppendUniqueOffset(offsets, o); }
    AppendUniqueOffset(offsets, coarseBest);
    return offsets;
}

inline bool IsBetterTemporalPredictorOffset(
    const TemporalPredictorSearchStrategy strategy,
    const TemporalPredictorOffsetSearchResult& candidate,
    const TemporalPredictorOffsetSearchResult& best) {
    if (UsesEstimatedBytesTemporalPredictorSearch(strategy)) {
        if (candidate.estimatedBytes != best.estimatedBytes) {
            return candidate.estimatedBytes < best.estimatedBytes;
        }
    }
    if (candidate.squaredError != best.squaredError) {
        return candidate.squaredError < best.squaredError;
    }
    return std::abs(candidate.offset) < std::abs(best.offset);
}

template<typename TValue>
inline bool EvaluateTemporalPredictorOffsetForBlock(
    const NumericArrayStorageParams& meta,
    const CompressorConfig& defaultCompressor,
    const std::span<const std::uint8_t> currentBytes,
    const std::span<const std::uint8_t> predictorBytes,
    ScratchByteBufferPool& scratchBytePool,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    const NumericArrayReferenceKind referenceKind,
    const std::uint16_t localParentFieldIndex,
    const TemporalPredictorSearchStrategy strategy,
    const std::int32_t predictorOffset,
    TemporalPredictorOffsetSearchResult& result,
    std::string* error = nullptr) {
    result = {};
    result.offset = predictorOffset;
    if (currentBytes.size() != predictorBytes.size() || componentCount == 0u) {
        return validation::AssignError(error, "temporal predictor input layout is invalid");
    }
    if (!validation::CanMulSizeT(componentCount, sizeof(TValue))) {
        return validation::AssignError(error, "temporal predictor tuple size exceeds addressable range");
    }
    const auto tupleBytes = componentCount * sizeof(TValue);
    if (tupleBytes == 0u ||
        !validation::CanMulSizeT(static_cast<std::size_t>(elementCount), tupleBytes)) {
        return validation::AssignError(error, "temporal predictor block size exceeds addressable range");
    }
    const auto expectedBytes = static_cast<std::size_t>(elementCount) * tupleBytes;
    if (currentBytes.size() != expectedBytes) {
        return validation::AssignError(error, "temporal predictor block byte size does not match metadata");
    }

    ScratchByteBuffer deltaBytes;
    TValue* deltaValues = nullptr;
    if (UsesEstimatedBytesTemporalPredictorSearch(strategy)) {
        deltaBytes = scratchBytePool.Acquire(expectedBytes);
        deltaValues = reinterpret_cast<TValue*>(deltaBytes.Bytes().data());
    }

    const auto valueCount = static_cast<std::size_t>(elementCount) * componentCount;
    const auto* currentValues = reinterpret_cast<const TValue*>(currentBytes.data());
    const auto* predictorValues = reinterpret_cast<const TValue*>(predictorBytes.data());
    result.squaredError = 0.0;
    for (std::size_t valueIndex = 0; valueIndex < valueCount; ++valueIndex) {
        const auto delta = static_cast<TValue>(
            static_cast<double>(currentValues[valueIndex]) -
            static_cast<double>(predictorValues[valueIndex]));
        const auto deltaValue = static_cast<double>(delta);
        result.squaredError += deltaValue * deltaValue;
        if (deltaValues != nullptr) {
            deltaValues[valueIndex] = delta;
        }
    }

    if (UsesEstimatedBytesTemporalPredictorSearch(strategy)) {
        // 估算压缩后大小
        NumericArrayBlockHeader header{
            .mode = NumericArrayBlockMode::PredictorReference,
            .referenceKind = referenceKind,
            .codecId = NumericArrayReferenceCodecId::Predictor,
            .localParentFieldIndex = localParentFieldIndex,
            .elementOffset = elementOffset,
            .elementCount = elementCount,
            .encodedByteLength = 0u,
            .bytesCodec = NumericArrayBytesCodec::RawBytes,
            .predictorOffset = predictorOffset,
        };
        auto buf = scratchBytePool.Acquire(0u);
        auto& encoded = buf.Bytes();
        numericarray::NumericArrayBlockParams params;
        if (!numericarray::MakeNumericArrayBlockParamsFromMeta(meta, params, error)) {
            return false;
        }
        if (!numericarray::ResolveEncodedNumericArrayBlockBytes(
                params,
                defaultCompressor,
                elementCount,
                std::span<const std::uint8_t>(
                    deltaBytes.Bytes().data(),
                    deltaBytes.Bytes().size()),
                encoded,
                header.bytesCodec,
                error,
                &scratchBytePool)) {
            return false;
        }
        std::size_t compressedBytes = 0u;
        if (!EstimateCompressedBytes(
                std::span<const std::uint8_t>(encoded.data(), encoded.size()),
                scratchBytePool,
                compressedBytes,
                error)) {
            return false;
        }
        result.estimatedBytes = std::min(encoded.size(), compressedBytes);
    }
    return true;
}

// ============================================================
// Shifted predictor block 构建（帧间 predictor 的核心操作）
// ============================================================

inline std::size_t ClampShiftedReferenceIndex(
    const std::size_t elementIndex,
    const std::int32_t predictorOffset,
    const std::size_t elementCount) {
    if (elementCount == 0u) { return 0u; }
    const auto shifted = static_cast<std::int64_t>(elementIndex) + static_cast<std::int64_t>(predictorOffset);
    return static_cast<std::size_t>(std::clamp<std::int64_t>(
        shifted, 0, static_cast<std::int64_t>(elementCount - 1u)));
}

} // namespace datacodec

#endif
