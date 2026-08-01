#ifndef DATACODEC_CODEC_ATTRIBUTES_ATTRIBUTEDECODE_H
#define DATACODEC_CODEC_ATTRIBUTES_ATTRIBUTEDECODE_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedAttributeCacheSet.h"
#include "DataCodec/Runtime/Cache/DecodeCache/ReferenceCacheNumericArraySource.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Codec/Reference/DecodedReference.h"
#include "DataCodec/Codec/Reference/NumericArrayReferenceBytes.h"
#include "DataCodec/Codec/Reference/ReferenceCodec.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockReader.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <limits>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace datacodec {
namespace decodeimpl {

namespace detail {

inline bool ResolveDecodedBlockBytes(
    const AttrStorageParams& meta,
    const ParsedNumericArrayBlock& block,
    std::vector<std::uint8_t>& decodedBytes,
    std::string* error = nullptr) {
    numericarray::NumericArrayBlockParams params;
    if (!numericarray::MakeNumericArrayBlockParamsFromMeta(meta, params, error)) {
        return false;
    }
    if (block.header.mode == NumericArrayBlockMode::LayeredResidual) {
        return numericarray::ResolveDecodedLayeredResidualNumericArrayBlockBytes(
            params,
            block.header.elementCount,
            block.backgroundCompressor,
            block.backgroundEncodedByteLength,
            block.componentLayouts,
            block.regionLayers,
            block.bytes,
            decodedBytes,
            error);
    }
    return numericarray::ResolveDecodedNumericArrayBlockBytes(
        params,
        block.backgroundCompressor,
        block.header.elementCount,
        block.header.bytesCodec,
        block.componentLayouts,
        block.bytes,
        decodedBytes,
        error);
}

inline bool TryFindReferenceAttrIndex(
    const CodecStorageParams& storageParams,
    const AttrStorageParams& targetMeta,
    std::size_t& referenceIndex) {
    for (std::size_t attrIndex = 0; attrIndex < storageParams.attrParams.size(); ++attrIndex) {
        const auto& candidate = storageParams.attrParams[attrIndex];
        if (candidate.name == targetMeta.name &&
            candidate.attachmentType == targetMeta.attachmentType &&
            candidate.dataType == targetMeta.dataType &&
            candidate.dimension == targetMeta.dimension) {
            referenceIndex = attrIndex;
            return true;
        }
    }
    referenceIndex = 0;
    return false;
}

inline bool UsesNonReferenceCodec(const ParsedNumericArrayBlock& block) {
    return block.header.codecId == NumericArrayReferenceCodecId::NonReference ||
        block.header.mode == NumericArrayBlockMode::NonReference;
}

template<typename TReferenceDecoder>
inline bool EnsureTemporalReferencesDecoded(
    DecodedAttributeReference* attributeKeyFrameReference,
    const AttrStorageParams& targetMeta,
    TReferenceDecoder& referenceDecoder,
    std::string* error = nullptr);

inline ParsedNumericArrayBlock MakeParsedBlockView(const numericarray::NumericArrayBlockPayload& block) {
    ParsedNumericArrayBlock parsed;
    parsed.header = block.header;
    parsed.backgroundCompressor = block.backgroundCompressor;
    parsed.backgroundEncodedByteLength = block.backgroundEncodedByteLength;
    parsed.componentLayouts = block.componentLayouts;
    parsed.regionLayers = block.regionLayers;
    parsed.alpha = block.alpha;
    parsed.beta = block.beta;
    parsed.bytes = block.bytes.Span();
    return parsed;
}

struct AttributePayloadRange {
    std::size_t attrIndex{0u};
    std::uint64_t offset{0u};
    std::uint64_t byteCount{0u};
};

class AttributePayloadRangeStream final {
public:
    AttributePayloadRangeStream(
        std::span<const std::uint8_t> bytes,
        const std::uint64_t begin,
        const std::uint64_t byteCount)
        : m_bytes(bytes),
          m_backingByteSize(static_cast<std::uint64_t>(bytes.size())),
          m_begin(begin),
          m_byteCount(byteCount) {}

    AttributePayloadRangeStream(
        const bytestore::IByteSource& source,
        const std::uint64_t begin,
        const std::uint64_t byteCount)
        : m_source(&source),
          m_backingByteSize(source.ByteSizeHint()),
          m_begin(begin),
          m_byteCount(byteCount) {}

    [[nodiscard]] std::uint64_t Position() const noexcept { return m_position; }

    bool ReadBytes(void* target, const std::size_t byteCount, std::string* error = nullptr) {
        if (byteCount == 0u) {
            return true;
        }
        if (target == nullptr) {
            return validation::AssignError(error, "attribute range stream target is null");
        }
        if (m_position > m_byteCount || byteCount > m_byteCount - m_position) {
            return validation::AssignError(error, "attribute range stream read exceeds payload range");
        }
        std::uint64_t absoluteOffset = 0u;
        if (!validation::CheckedAddU64(
                m_begin,
                m_position,
                absoluteOffset,
                "attribute range stream absolute offset",
                error)) {
            return false;
        }
        if (absoluteOffset > m_backingByteSize ||
            byteCount > m_backingByteSize - absoluteOffset) {
            return validation::AssignError(error, "attribute range stream read exceeds backing bytes");
        }
        if (m_source != nullptr) {
            if (!m_source->Read(
                    absoluteOffset,
                    std::span<std::uint8_t>(static_cast<std::uint8_t*>(target), byteCount),
                    error)) {
                return false;
            }
        } else {
            std::memcpy(
                target,
                m_bytes.data() + static_cast<std::size_t>(absoluteOffset),
                byteCount);
        }
        if (!validation::CheckedAddU64(
                m_position,
                static_cast<std::uint64_t>(byteCount),
                m_position,
                "attribute range stream position",
                error)) {
            return false;
        }
        return true;
    }

    bool ReadVector(std::vector<std::uint8_t>& output, const std::size_t byteCount, std::string* error = nullptr) {
        output.assign(byteCount, 0u);
        return byteCount == 0u || ReadBytes(output.data(), output.size(), error);
    }

    template<typename T>
    bool ReadScalar(T& value, std::string* error = nullptr) {
        value = {};
        return ReadBytes(&value, sizeof(T), error);
    }

    bool Skip(const std::uint64_t byteCount, std::string* error = nullptr) {
        if (m_position > m_byteCount || byteCount > m_byteCount - m_position) {
            return validation::AssignError(error, "attribute range stream skip exceeds payload range");
        }
        if (!validation::CheckedAddU64(
                m_position,
                byteCount,
                m_position,
                "attribute range stream position",
                error)) {
            return false;
        }
        return true;
    }

private:
    std::span<const std::uint8_t> m_bytes;
    const bytestore::IByteSource* m_source{nullptr};
    std::uint64_t m_backingByteSize{0u};
    std::uint64_t m_begin{0u};
    std::uint64_t m_byteCount{0u};
    std::uint64_t m_position{0u};
};

inline std::uint64_t AttributePayloadBackingByteSize(
    const std::span<const std::uint8_t> bytes) noexcept {
    return static_cast<std::uint64_t>(bytes.size());
}

inline std::uint64_t AttributePayloadBackingByteSize(
    const bytestore::IByteSource& source) noexcept {
    return source.ByteSizeHint();
}

inline bool ResolveAttributePayloadRanges(
    const CodecStorageParams& storageParams,
    std::vector<AttributePayloadRange>& payloadRanges,
    std::string* error = nullptr) {
    payloadRanges.clear();
    std::vector<std::size_t> payloadOrder;
    if (!ResolveAttributePayloadOrder(storageParams, payloadOrder, error)) {
        return false;
    }
    payloadRanges.reserve(payloadOrder.size());
    std::uint64_t offset = 0u;
    for (const auto attrIndex : payloadOrder) {
        const auto payloadBytes = static_cast<std::uint64_t>(storageParams.attrParams[attrIndex].binaryCount);
        if (!validation::CanAddU64(offset, payloadBytes)) {
            validation::AssignError(error, "attribute payload ranges exceed stream address space");
            return false;
        }
        payloadRanges.push_back(AttributePayloadRange{
            attrIndex,
            offset,
            payloadBytes,
        });
        if (!validation::CheckedAddU64(
                offset,
                payloadBytes,
                offset,
                "attribute payload ranges",
                error)) {
            return false;
        }
    }
    return true;
}

inline bool BuildDecodedAttributeReferenceRangeBytes(
    const DecodedAttributeCacheSet& cacheSet,
    const std::size_t referenceAttrIndex,
    const AttrStorageParams& referenceMeta,
    const AttrStorageParams& targetMeta,
    const std::size_t targetElementOffset,
    const std::size_t targetElementCount,
    ScratchByteBufferPool& scratchBytePool,
    ScratchByteBuffer& outputBytes,
    std::string* error = nullptr) {
    outputBytes.Release();
    numericarray::NumericArraySource referenceSource;
    if (!BuildDecodedAttributeCacheNumericArraySource(cacheSet, referenceAttrIndex, referenceMeta, referenceSource, error)) {
        return false;
    }
    numericarray::NumericArrayReader referenceReader;
    if (!numericarray::BuildNumericArrayReader(referenceSource, referenceReader, error)) {
        return false;
    }
    if (!numericarrayreference::BuildNumericArrayReferenceRangeBytes(
            referenceReader,
            referenceMeta,
            targetMeta,
            scratchBytePool,
            targetElementOffset,
            targetElementCount,
            outputBytes,
            error)) {
        return false;
    }
    return true;
}

inline bool BuildDecodedAttributePredictorReferenceBlockBytes(
    const DecodedAttributeCacheSet& cacheSet,
    const std::size_t referenceAttrIndex,
    const AttrStorageParams& referenceMeta,
    const AttrStorageParams& targetMeta,
    const std::size_t elementOffset,
    const std::size_t elementCount,
    const std::int32_t predictorOffset,
    ScratchByteBufferPool& scratchBytePool,
    ScratchByteBuffer& outputBytes,
    std::string* error = nullptr) {
    outputBytes.Release();
    numericarray::NumericArraySource referenceSource;
    if (!BuildDecodedAttributeCacheNumericArraySource(cacheSet, referenceAttrIndex, referenceMeta, referenceSource, error)) {
        return false;
    }
    numericarray::NumericArrayReader referenceReader;
    if (!numericarray::BuildNumericArrayReader(referenceSource, referenceReader, error)) {
        return false;
    }
    if (!numericarrayreference::BuildNumericArrayPredictorReferenceBlockBytes(
        referenceReader,
        referenceMeta,
        targetMeta,
        scratchBytePool,
        elementOffset,
        elementCount,
        predictorOffset,
        outputBytes,
        error)) {
        return false;
    }
    return true;
}

inline bool WriteDecodedAttributeBlock(
    DecodedAttributeCacheSet& attributes,
    const std::size_t attrIndex,
    const AttrStorageParams& meta,
    const NumericArrayBlockHeader& header,
    const std::span<const std::uint8_t> decodedBlockBytes,
    std::string* error = nullptr) {
    std::size_t localValueSize = 0u;
    std::size_t localElementCount = 0u;
    if (!TryParamSizeToSizeT(NumericArrayValueSize(meta), localValueSize) ||
        !TryParamSizeToSizeT(meta.elementCount, localElementCount)) {
        return validation::AssignError(error, "attribute metadata exceeds this platform size limit");
    }
    const auto componentCount = static_cast<std::size_t>(std::max(meta.dimension, 0));
    std::size_t tupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            localValueSize,
            tupleBytes,
            "attribute block tuple bytes",
            error)) {
        return false;
    }
    std::size_t expectedBytes = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(header.elementCount),
            tupleBytes,
            expectedBytes,
            "attribute block decoded bytes",
            error)) {
        return false;
    }
    if (decodedBlockBytes.size() != expectedBytes) {
        return validation::AssignError(error, "attribute block decoded size does not match range");
    }
    std::size_t byteOffset = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(header.elementOffset),
            tupleBytes,
            byteOffset,
            "attribute block decoded byte offset",
            error)) {
        return false;
    }
    std::size_t fieldBytes = 0u;
    if (!validation::CheckedMulSizeT(
            localElementCount,
            tupleBytes,
            fieldBytes,
            "attribute decoded field bytes",
            error)) {
        return false;
    }
    if (byteOffset > fieldBytes ||
        decodedBlockBytes.size() > fieldBytes - byteOffset) {
        return validation::AssignError(error, "attribute block decoded range exceeds field size");
    }

    return attributes.WriteAttributeRange(
        attrIndex,
        static_cast<std::size_t>(header.elementOffset),
        static_cast<std::size_t>(header.elementCount),
        decodedBlockBytes.data(),
        decodedBlockBytes.size(),
        error);
}

struct AttributeReferenceRangeObservation {
    ParamSize worksetBytes{0u};
    ParamSize resampledWorksetBytes{0u};
    ParamSize predictorRangeStagingBytes{0u};
    ParamSize predictorShiftedCopyBytes{0u};
    bool temporal{false};
    bool intra{false};
    bool resampled{false};
    bool predictorZeroOffset{false};
    bool predictorContinuousShift{false};
    bool predictorBoundaryClamp{false};
};

inline ParamSize SaturatingReferenceMetricMultiply(
    const ParamSize left,
    const ParamSize right) noexcept {
    if (left != 0u && right > std::numeric_limits<ParamSize>::max() / left) {
        return std::numeric_limits<ParamSize>::max();
    }
    return left * right;
}

inline void ObserveAttributeReferenceRange(
    const NumericArrayReferenceKind referenceKind,
    const NumericArrayReferenceCodecId codecId,
    const NumericArrayStorageParams& referenceMeta,
    const NumericArrayStorageParams& targetMeta,
    const ParsedNumericArrayBlock& block,
    const ScratchByteBuffer& referenceBytes,
    AttributeReferenceRangeObservation* observation) {
    if (observation == nullptr) {
        return;
    }
    *observation = {};
    observation->temporal = referenceKind == NumericArrayReferenceKind::TemporalKeyFrame;
    observation->intra = referenceKind == NumericArrayReferenceKind::IntraArray;
    observation->worksetBytes = static_cast<ParamSize>(referenceBytes.Span().size());
    observation->resampled = referenceMeta.elementCount != targetMeta.elementCount;
    if (observation->resampled) {
        observation->resampledWorksetBytes = observation->worksetBytes;
    }
    if (codecId != NumericArrayReferenceCodecId::Predictor) {
        return;
    }
    if (block.header.elementCount == 0u) {
        return;
    }

    std::size_t targetElementCount = 0u;
    std::size_t targetValueSize = 0u;
    if (!TryParamSizeToSizeT(targetMeta.elementCount, targetElementCount) ||
        !TryParamSizeToSizeT(NumericArrayValueSize(targetMeta), targetValueSize) ||
        targetElementCount == 0u) {
        return;
    }
    const auto firstReferenceIndex = numericarrayreference::ClampNumericArrayShiftedReferenceIndex(
        static_cast<std::size_t>(block.header.elementOffset),
        block.header.predictorOffset,
        targetElementCount);
    const auto lastReferenceIndex = numericarrayreference::ClampNumericArrayShiftedReferenceIndex(
        static_cast<std::size_t>(block.header.elementOffset) +
            static_cast<std::size_t>(block.header.elementCount) - 1u,
        block.header.predictorOffset,
        targetElementCount);
    const auto stagedElementCount = lastReferenceIndex - firstReferenceIndex + 1u;
    const auto tupleBytes = SaturatingReferenceMetricMultiply(
        static_cast<ParamSize>(std::max(targetMeta.dimension, 0)),
        static_cast<ParamSize>(targetValueSize));
    observation->predictorRangeStagingBytes = SaturatingReferenceMetricMultiply(
        static_cast<ParamSize>(stagedElementCount),
        tupleBytes);
    if (block.header.predictorOffset == 0) {
        observation->predictorZeroOffset = true;
        return;
    }
    if (observation->predictorRangeStagingBytes == observation->worksetBytes) {
        observation->predictorContinuousShift = true;
        return;
    }
    observation->predictorBoundaryClamp = true;
    observation->predictorShiftedCopyBytes = observation->worksetBytes;
}

inline bool ResolveReferenceBytesForBlock(
    const CodecStorageParams& storageParams,
    const DecodedAttributeCacheSet& decodedAttrs,
    const ParsedNumericArrayBlock& block,
    const AttrStorageParams& meta,
    const DecodedAttributeReference* attrKeyFrameLeafReference,
    ScratchByteBufferPool& scratchBytePool,
    const ScratchByteBuffer*& referenceBytes,
    ScratchByteBuffer& intraReferenceBytes,
    ScratchByteBuffer& resampledReferenceBytes,
    ScratchByteBuffer& shiftedPredictorBlockBytes,
    std::size_t& referenceElementOffset,
    AttributeReferenceRangeObservation* rangeObservation = nullptr,
    std::string* error = nullptr) {
    referenceBytes = nullptr;
    referenceElementOffset = static_cast<std::size_t>(block.header.elementOffset);
    if (numericarray::IsIntegerNumericArrayDataType(meta.dataType) &&
        block.header.codecId != NumericArrayReferenceCodecId::Wavelet) {
        return validation::AssignError(error, "integer attribute reference requires wavelet codec");
    }
    if (block.header.referenceKind == NumericArrayReferenceKind::IntraArray) {
        const auto referenceAttrIndex = static_cast<std::size_t>(block.header.localParentFieldIndex);
        if (referenceAttrIndex >= storageParams.attrParams.size()) {
            return validation::AssignError(error, "intra-field reference index is out of range");
        }
        const auto& referenceMeta = storageParams.attrParams[referenceAttrIndex];
        if (!BuildDecodedAttributeReferenceRangeBytes(
                decodedAttrs,
                referenceAttrIndex,
                referenceMeta,
                meta,
                static_cast<std::size_t>(block.header.elementOffset),
                static_cast<std::size_t>(block.header.elementCount),
                scratchBytePool,
                intraReferenceBytes,
                error)) {
            if (error != nullptr && error->empty()) {
                return validation::AssignError(error, "failed to resolve intra-field reference range");
            }
            return false;
        }
        referenceBytes = &intraReferenceBytes;
        referenceElementOffset = 0u;
        ObserveAttributeReferenceRange(
            block.header.referenceKind,
            block.header.codecId,
            referenceMeta,
            meta,
            block,
            *referenceBytes,
            rangeObservation);
        return true;
    }

    if (block.header.referenceKind == NumericArrayReferenceKind::TemporalKeyFrame) {
        if (attrKeyFrameLeafReference == nullptr ||
            attrKeyFrameLeafReference->store == nullptr) {
            return validation::AssignError(error, "key frame reference store is not ready");
        }

        std::size_t referenceAttrIndex = 0u;
        if (!TryFindReferenceAttrIndex(
                attrKeyFrameLeafReference->reference.storageParams,
                meta,
                referenceAttrIndex)) {
            return validation::AssignError(error, "failed to resolve reference attr from key frame");
        }
        auto& store = *attrKeyFrameLeafReference->store;
        if (!store.Complete(referenceAttrIndex)) {
            return validation::AssignError(error, "key frame reference field is not ready");
        }
        const auto& referenceMeta =
            attrKeyFrameLeafReference->reference.storageParams.attrParams[referenceAttrIndex];
        if (block.header.codecId != NumericArrayReferenceCodecId::Predictor) {
            if (!BuildDecodedAttributeReferenceRangeBytes(
                    store,
                    referenceAttrIndex,
                    referenceMeta,
                    meta,
                    static_cast<std::size_t>(block.header.elementOffset),
                    static_cast<std::size_t>(block.header.elementCount),
                    scratchBytePool,
                    resampledReferenceBytes,
                    error)) {
                return false;
            }
            referenceBytes = &resampledReferenceBytes;
            referenceElementOffset = 0u;
            ObserveAttributeReferenceRange(
                block.header.referenceKind,
                block.header.codecId,
                referenceMeta,
                meta,
                block,
                *referenceBytes,
                rangeObservation);
            return true;
        }

        if (!BuildDecodedAttributePredictorReferenceBlockBytes(
                store,
                referenceAttrIndex,
                referenceMeta,
                meta,
                static_cast<std::size_t>(block.header.elementOffset),
                static_cast<std::size_t>(block.header.elementCount),
                block.header.predictorOffset,
                scratchBytePool,
                shiftedPredictorBlockBytes,
                error)) {
            return false;
        }
        referenceBytes = &shiftedPredictorBlockBytes;
        referenceElementOffset = 0u;
        ObserveAttributeReferenceRange(
            block.header.referenceKind,
            block.header.codecId,
            referenceMeta,
            meta,
            block,
            *referenceBytes,
            rangeObservation);
        return true;
    }

    return validation::AssignError(error, "delta block is missing a valid reference kind");
}

struct AttributeDecodeTimingDetail {
    std::size_t attrIndex{0u};
    std::string name;
    AttrAttachment attachmentType{AttrAttachment::Point};
    DataType dataType{DataType::Float32};
    ParamSize elementCount{0u};
    ParamSize binaryCount{0u};
    ParamSize rawValueBytes{0u};
    ParamSize encodedBlockBytes{0u};
    ParamSize backgroundEncodedBytes{0u};
    std::int32_t dimension{0};
    ParamSize valueSize{0u};
    std::size_t blockCount{0u};
    std::size_t nonReferenceBlocks{0u};
    std::size_t referenceBlocks{0u};
    std::size_t intraReferenceBlocks{0u};
    std::size_t temporalReferenceBlocks{0u};
    std::size_t affineReferenceBlocks{0u};
    std::size_t waveletReferenceBlocks{0u};
    std::size_t predictorReferenceBlocks{0u};
    std::size_t layeredResidualBlocks{0u};
    std::size_t regionLayerCount{0u};
    std::size_t componentLayoutCount{0u};
    ParamSize estimatedDecodeCost{0u};
    ParamSize criticalPathCost{0u};
    double readyOffsetMs{0.0};
    double startOffsetMs{0.0};
    double finishOffsetMs{0.0};
    double readyWaitMs{0.0};
    std::size_t workerIndex{kInvalidParallelWorkerIndex};
    double elapsedMs{0.0};
    double payloadBlockReadMs{0.0};
    double ordinaryDecodeMs{0.0};
    double referenceResolveMs{0.0};
    double temporalKeyReferenceEnsureMs{0.0};
    double referenceRangeResolveMs{0.0};
    double referenceDecodeMs{0.0};
    double affineReferenceDecodeMs{0.0};
    double waveletReferenceDecodeMs{0.0};
    double predictorReferenceDecodeMs{0.0};
    double cacheWriteMs{0.0};
    ParamSize ordinaryDecodedBytes{0u};
    ParamSize referenceDecodedBytes{0u};
    ParamSize referenceBytesRead{0u};
    ParamSize intraReferenceWorksetBytes{0u};
    ParamSize temporalReferenceWorksetBytes{0u};
    ParamSize resampledReferenceWorksetBytes{0u};
    ParamSize predictorRangeStagingBytes{0u};
    ParamSize predictorShiftedCopyBytes{0u};
    ParamSize waveletLowBlobBytes{0u};
    ParamSize waveletHighBlobBytes{0u};
    ParamSize waveletPeakTemporaryDoubleBytes{0u};
    ParamSize cacheWriteBytes{0u};
    std::size_t temporalKeyFieldCacheHits{0u};
    std::size_t temporalKeyFieldCacheMisses{0u};
    std::size_t predictorZeroOffsetBlocks{0u};
    std::size_t predictorContinuousShiftBlocks{0u};
    std::size_t predictorBoundaryClampBlocks{0u};
};

struct AttributeDecodeWorkBreakdown {
    double payloadBlockReadMs{0.0};
    double ordinaryDecodeMs{0.0};
    double referenceResolveMs{0.0};
    double temporalKeyReferenceEnsureMs{0.0};
    double referenceRangeResolveMs{0.0};
    double referenceDecodeMs{0.0};
    double affineReferenceDecodeMs{0.0};
    double waveletReferenceDecodeMs{0.0};
    double predictorReferenceDecodeMs{0.0};
    double cacheWriteMs{0.0};
    ParamSize ordinaryDecodedBytes{0u};
    ParamSize referenceDecodedBytes{0u};
    ParamSize referenceBytesRead{0u};
    ParamSize intraReferenceWorksetBytes{0u};
    ParamSize temporalReferenceWorksetBytes{0u};
    ParamSize resampledReferenceWorksetBytes{0u};
    ParamSize predictorRangeStagingBytes{0u};
    ParamSize predictorShiftedCopyBytes{0u};
    ParamSize waveletLowBlobBytes{0u};
    ParamSize waveletHighBlobBytes{0u};
    ParamSize waveletPeakTemporaryDoubleBytes{0u};
    ParamSize cacheWriteBytes{0u};
    std::size_t temporalKeyFieldCacheHits{0u};
    std::size_t temporalKeyFieldCacheMisses{0u};
    std::size_t predictorZeroOffsetBlocks{0u};
    std::size_t predictorContinuousShiftBlocks{0u};
    std::size_t predictorBoundaryClampBlocks{0u};
};

inline ParamSize SaturatingParamSizeMultiply(const ParamSize left, const ParamSize right) noexcept {
    if (left != 0u && right > std::numeric_limits<ParamSize>::max() / left) {
        return std::numeric_limits<ParamSize>::max();
    }
    return left * right;
}

inline ParamSize SaturatingParamSizeAdd(const ParamSize left, const ParamSize right) noexcept {
    if (right > std::numeric_limits<ParamSize>::max() - left) {
        return std::numeric_limits<ParamSize>::max();
    }
    return left + right;
}

inline void AccumulateAttributeReferenceRangeObservation(
    AttributeDecodeWorkBreakdown& workBreakdown,
    const AttributeReferenceRangeObservation& observation) noexcept {
    if (observation.intra) {
        workBreakdown.intraReferenceWorksetBytes = SaturatingParamSizeAdd(
            workBreakdown.intraReferenceWorksetBytes,
            observation.worksetBytes);
    }
    if (observation.temporal) {
        workBreakdown.temporalReferenceWorksetBytes = SaturatingParamSizeAdd(
            workBreakdown.temporalReferenceWorksetBytes,
            observation.worksetBytes);
    }
    workBreakdown.resampledReferenceWorksetBytes = SaturatingParamSizeAdd(
        workBreakdown.resampledReferenceWorksetBytes,
        observation.resampledWorksetBytes);
    workBreakdown.predictorRangeStagingBytes = SaturatingParamSizeAdd(
        workBreakdown.predictorRangeStagingBytes,
        observation.predictorRangeStagingBytes);
    workBreakdown.predictorShiftedCopyBytes = SaturatingParamSizeAdd(
        workBreakdown.predictorShiftedCopyBytes,
        observation.predictorShiftedCopyBytes);
    workBreakdown.predictorZeroOffsetBlocks += observation.predictorZeroOffset ? 1u : 0u;
    workBreakdown.predictorContinuousShiftBlocks += observation.predictorContinuousShift ? 1u : 0u;
    workBreakdown.predictorBoundaryClampBlocks += observation.predictorBoundaryClamp ? 1u : 0u;
}

inline bool IsTemporalReferenceFieldCached(
    const DecodedAttributeReference* attributeKeyFrameReference,
    const AttrStorageParams& targetMeta) {
    if (attributeKeyFrameReference == nullptr ||
        attributeKeyFrameReference->store == nullptr) {
        return false;
    }
    std::size_t referenceAttrIndex = 0u;
    return TryFindReferenceAttrIndex(
               attributeKeyFrameReference->reference.storageParams,
               targetMeta,
               referenceAttrIndex) &&
        attributeKeyFrameReference->store->Complete(referenceAttrIndex);
}

inline ParamSize AttributeDecodeRuntimeCostForScheduling(const AttrStorageParams& meta) noexcept {
    const auto rawBytes = NumericArrayRawValueBytes(meta);
    auto cost = EstimateAttributeDecodeCost(meta);
    cost = SaturatingParamSizeAdd(cost, rawBytes);
    if (NumericArrayValueSize(meta) >= 8u || meta.dataType == DataType::Float64) {
        cost = SaturatingParamSizeAdd(cost, rawBytes);
    } else {
        cost = SaturatingParamSizeAdd(cost, rawBytes / 2u);
    }
    if (meta.dimension > 1) {
        cost = SaturatingParamSizeAdd(cost, rawBytes / 2u);
    }
    if (meta.elementCount >= 1024u * 1024u) {
        cost = SaturatingParamSizeAdd(
            cost,
            SaturatingParamSizeMultiply(
                meta.elementCount,
                std::max<ParamSize>(NumericArrayValueSize(meta), 1u)));
    }
    return cost;
}

inline AttributeDecodeTimingDetail BuildAttributeDecodeTimingDetail(
    const std::size_t attrIndex,
    const AttrStorageParams& meta,
    const double elapsedMs,
    const double readyOffsetMs,
    const double startOffsetMs,
    const double finishOffsetMs,
    const double readyWaitMs,
    const std::size_t workerIndex,
    const AttributeDecodeWorkBreakdown& workBreakdown = {}) {
    AttributeDecodeTimingDetail detail;
    detail.attrIndex = attrIndex;
    detail.name = meta.name;
    detail.attachmentType = meta.attachmentType;
    detail.dataType = meta.dataType;
    detail.elementCount = meta.elementCount;
    detail.binaryCount = meta.binaryCount;
    detail.dimension = meta.dimension;
    detail.valueSize = NumericArrayValueSize(meta);
    detail.estimatedDecodeCost = AttributeDecodeRuntimeCostForScheduling(meta);
    detail.criticalPathCost = std::max(meta.decodeScheduleHint.criticalPathCost, detail.estimatedDecodeCost);
    detail.readyOffsetMs = readyOffsetMs;
    detail.startOffsetMs = startOffsetMs;
    detail.finishOffsetMs = finishOffsetMs;
    detail.readyWaitMs = readyWaitMs;
    detail.workerIndex = workerIndex;
    detail.elapsedMs = elapsedMs;
    detail.payloadBlockReadMs = workBreakdown.payloadBlockReadMs;
    detail.ordinaryDecodeMs = workBreakdown.ordinaryDecodeMs;
    detail.referenceResolveMs = workBreakdown.referenceResolveMs;
    detail.temporalKeyReferenceEnsureMs = workBreakdown.temporalKeyReferenceEnsureMs;
    detail.referenceRangeResolveMs = workBreakdown.referenceRangeResolveMs;
    detail.referenceDecodeMs = workBreakdown.referenceDecodeMs;
    detail.affineReferenceDecodeMs = workBreakdown.affineReferenceDecodeMs;
    detail.waveletReferenceDecodeMs = workBreakdown.waveletReferenceDecodeMs;
    detail.predictorReferenceDecodeMs = workBreakdown.predictorReferenceDecodeMs;
    detail.cacheWriteMs = workBreakdown.cacheWriteMs;
    detail.ordinaryDecodedBytes = workBreakdown.ordinaryDecodedBytes;
    detail.referenceDecodedBytes = workBreakdown.referenceDecodedBytes;
    detail.referenceBytesRead = workBreakdown.referenceBytesRead;
    detail.intraReferenceWorksetBytes = workBreakdown.intraReferenceWorksetBytes;
    detail.temporalReferenceWorksetBytes = workBreakdown.temporalReferenceWorksetBytes;
    detail.resampledReferenceWorksetBytes = workBreakdown.resampledReferenceWorksetBytes;
    detail.predictorRangeStagingBytes = workBreakdown.predictorRangeStagingBytes;
    detail.predictorShiftedCopyBytes = workBreakdown.predictorShiftedCopyBytes;
    detail.waveletLowBlobBytes = workBreakdown.waveletLowBlobBytes;
    detail.waveletHighBlobBytes = workBreakdown.waveletHighBlobBytes;
    detail.waveletPeakTemporaryDoubleBytes = workBreakdown.waveletPeakTemporaryDoubleBytes;
    detail.cacheWriteBytes = workBreakdown.cacheWriteBytes;
    detail.temporalKeyFieldCacheHits = workBreakdown.temporalKeyFieldCacheHits;
    detail.temporalKeyFieldCacheMisses = workBreakdown.temporalKeyFieldCacheMisses;
    detail.predictorZeroOffsetBlocks = workBreakdown.predictorZeroOffsetBlocks;
    detail.predictorContinuousShiftBlocks = workBreakdown.predictorContinuousShiftBlocks;
    detail.predictorBoundaryClampBlocks = workBreakdown.predictorBoundaryClampBlocks;
    const auto tupleWidth = static_cast<ParamSize>(std::max(meta.dimension, 0));
    detail.rawValueBytes = SaturatingParamSizeMultiply(
        SaturatingParamSizeMultiply(meta.elementCount, tupleWidth),
        NumericArrayValueSize(meta));

    detail.blockCount = meta.blockLayouts.size();
    for (const auto& layout : meta.blockLayouts) {
        detail.encodedBlockBytes = SaturatingParamSizeAdd(detail.encodedBlockBytes, layout.encodedByteLength);
        detail.backgroundEncodedBytes = SaturatingParamSizeAdd(
            detail.backgroundEncodedBytes,
            layout.backgroundEncodedByteLength);
        detail.regionLayerCount += layout.regionLayers.size();
        detail.componentLayoutCount += layout.componentLayouts.size();
        if (layout.referenceKind == NumericArrayReferenceKind::None) {
            ++detail.nonReferenceBlocks;
        } else {
            ++detail.referenceBlocks;
        }
        if (layout.referenceKind == NumericArrayReferenceKind::IntraArray) {
            ++detail.intraReferenceBlocks;
        } else if (layout.referenceKind == NumericArrayReferenceKind::TemporalKeyFrame) {
            ++detail.temporalReferenceBlocks;
        }

        if (layout.mode == NumericArrayBlockMode::AffineReference) {
            ++detail.affineReferenceBlocks;
        } else if (layout.mode == NumericArrayBlockMode::WaveletReference) {
            ++detail.waveletReferenceBlocks;
        } else if (layout.mode == NumericArrayBlockMode::PredictorReference) {
            ++detail.predictorReferenceBlocks;
        } else if (layout.mode == NumericArrayBlockMode::LayeredResidual) {
            ++detail.layeredResidualBlocks;
        }
    }
    return detail;
}

using AttributeDecodeTimingCallback = std::function<void(const AttributeDecodeTimingDetail&)>;

struct AttributeDecodeData {
    const CodecStorageParams& storageParams;
    DecodedAttributeReference* attributeKeyFrameReference{nullptr};
};

struct AttributeDecodeSchedule {
    std::size_t workerCount{1u};
    IParallelTaskRunner* parallelTaskRunner{nullptr};
};

struct AttributeDecodeCache {
    const CacheResources& cacheResources;
    bytestore::ByteStoreSession& byteStoreSession;
    DecodedAttributeCacheSet& attributes;
    std::uint64_t attributeMemoryCacheLimitBytes{0u};
    DecodeStorageMode attributeCacheStorageMode{DecodeStorageMode::Managed};
};

struct AttributeDecodeContext {
    AttributeDecodeTimingCallback timingCallback;
};

struct AttributePayloadDecodeRuntime {
    AttributeDecodeData data;
    AttributeDecodeSchedule schedule;
    AttributeDecodeCache cache;
    AttributeDecodeContext context;
};

struct AttributeStreamDecodeRuntime {
    AttributeDecodeData data;
    AttributeDecodeCache cache;
};

inline ParamSize AttributeDecodeEstimatedCostForScheduling(const AttrStorageParams& meta) noexcept {
    const auto storedCost = meta.decodeScheduleHint.estimatedDecodeCost != 0u
        ? meta.decodeScheduleHint.estimatedDecodeCost
        : EstimateAttributeDecodeCost(meta);
    return std::max(storedCost, AttributeDecodeRuntimeCostForScheduling(meta));
}

inline ParamSize AttributeDecodeCriticalCostForScheduling(const AttrStorageParams& meta) noexcept {
    return std::max(
        meta.decodeScheduleHint.criticalPathCost,
        AttributeDecodeEstimatedCostForScheduling(meta));
}

inline ParamSize AttributeDecodeReadyWaitBoostCost(const double readyWaitMs) noexcept {
    constexpr double kAgingStartMs = 2000.0;
    constexpr double kAgingCostPerMs = 16.0 * 1024.0;
    constexpr double kAgingCapMs = 3000.0;
    if (readyWaitMs <= kAgingStartMs) {
        return 0u;
    }
    const auto effectiveWaitMs = std::min(readyWaitMs - kAgingStartMs, kAgingCapMs);
    return static_cast<ParamSize>(effectiveWaitMs * kAgingCostPerMs);
}

inline bool AttributeDecodeSchedulePriorityLess(
    const CodecStorageParams& storageParams,
    const std::size_t leftIndex,
    const std::size_t rightIndex) noexcept {
    const auto& left = storageParams.attrParams[leftIndex];
    const auto& right = storageParams.attrParams[rightIndex];
    const auto leftCriticalCost = AttributeDecodeCriticalCostForScheduling(left);
    const auto rightCriticalCost = AttributeDecodeCriticalCostForScheduling(right);
    if (leftCriticalCost != rightCriticalCost) {
        return leftCriticalCost < rightCriticalCost;
    }
    const auto leftOwnCost = AttributeDecodeEstimatedCostForScheduling(left);
    const auto rightOwnCost = AttributeDecodeEstimatedCostForScheduling(right);
    if (leftOwnCost != rightOwnCost) {
        return leftOwnCost < rightOwnCost;
    }
    if (left.binaryCount != right.binaryCount) {
        return left.binaryCount < right.binaryCount;
    }
    const auto leftRawBytes = NumericArrayRawValueBytes(left);
    const auto rightRawBytes = NumericArrayRawValueBytes(right);
    if (leftRawBytes != rightRawBytes) {
        return leftRawBytes < rightRawBytes;
    }
    return leftIndex > rightIndex;
}

inline bool AttributeDecodeDynamicSchedulePriorityLess(
    const CodecStorageParams& storageParams,
    const std::size_t leftIndex,
    const std::size_t rightIndex,
    const double leftReadyWaitMs,
    const double rightReadyWaitMs) noexcept {
    const auto& left = storageParams.attrParams[leftIndex];
    const auto& right = storageParams.attrParams[rightIndex];
    const auto leftScore = SaturatingParamSizeAdd(
        AttributeDecodeCriticalCostForScheduling(left),
        AttributeDecodeReadyWaitBoostCost(leftReadyWaitMs));
    const auto rightScore = SaturatingParamSizeAdd(
        AttributeDecodeCriticalCostForScheduling(right),
        AttributeDecodeReadyWaitBoostCost(rightReadyWaitMs));
    if (leftScore != rightScore) {
        return leftScore < rightScore;
    }
    return AttributeDecodeSchedulePriorityLess(storageParams, leftIndex, rightIndex);
}

inline bool ResolveAttributeIntraParents(
    const CodecStorageParams& storageParams,
    const std::size_t attrIndex,
    std::vector<std::size_t>& parents,
    std::string* error = nullptr) {
    parents.clear();
    if (attrIndex >= storageParams.attrParams.size()) {
        return validation::AssignError(error, "attribute dependency index is out of range");
    }
    const auto& meta = storageParams.attrParams[attrIndex];
    for (const auto& layout : meta.blockLayouts) {
        if (layout.referenceKind != NumericArrayReferenceKind::IntraArray) {
            continue;
        }
        const auto parentIndex = static_cast<std::size_t>(layout.localParentFieldIndex);
        if (parentIndex >= storageParams.attrParams.size() || parentIndex == attrIndex) {
            return validation::AssignError(error, "attribute intra-field parent index is invalid");
        }
        if (std::find(parents.begin(), parents.end(), parentIndex) == parents.end()) {
            parents.push_back(parentIndex);
        }
    }
    return true;
}

template<typename TPayloadBacking, typename TReferenceDecoder>
inline bool DecodeSingleAttributeRangeToCache(
    const CodecStorageParams& storageParams,
    const CacheResources& runtime,
    DecodedAttributeCacheSet& attributes,
    DecodedAttributeReference* attributeKeyFrameReference,
    const TPayloadBacking& payloadBacking,
    const AttributePayloadRange& payloadRange,
    TReferenceDecoder& referenceDecoder,
    AttributeDecodeWorkBreakdown* workBreakdown = nullptr,
    std::string* error = nullptr) {
    if (payloadRange.attrIndex >= storageParams.attrParams.size()) {
        return validation::AssignError(error, "attribute payload range index is out of range");
    }
    const auto& meta = storageParams.attrParams[payloadRange.attrIndex];
    if (payloadRange.byteCount != static_cast<std::uint64_t>(meta.binaryCount)) {
        return validation::AssignError(error, "attribute payload range size does not match metadata");
    }
    const auto backingByteSize = AttributePayloadBackingByteSize(payloadBacking);
    if (payloadRange.offset > backingByteSize ||
        payloadRange.byteCount > backingByteSize - payloadRange.offset) {
        return validation::AssignError(error, "attribute payload range exceeds backing bytes");
    }
    if (!attributes.BeginAttribute(payloadRange.attrIndex, meta, error)) {
        return false;
    }

    if (payloadRange.byteCount == 0u) {
        if (meta.elementCount != 0u) {
            return validation::AssignError(error, "attribute payload is empty for a non-empty field");
        }
        return attributes.EndAttribute(payloadRange.attrIndex, error);
    }

    if (meta.blockLayouts.empty()) {
        return validation::AssignError(error, "attribute params are missing block layout metadata");
    }

    AttributePayloadRangeStream stream(payloadBacking, payloadRange.offset, payloadRange.byteCount);
    std::uint64_t decodedElements = 0u;
    for (const auto& blockLayout : meta.blockLayouts) {
        numericarray::NumericArrayBlockPayload ownedBlock;
        const auto blockReadStart = callback::StartTiming(workBreakdown != nullptr);
        if (!numericarray::ReadNumericArrayBlockPayload(
                stream,
                static_cast<std::size_t>(std::max(meta.dimension, 0)),
                blockLayout,
                runtime,
                ownedBlock,
                error)) {
            return false;
        }
        if (workBreakdown != nullptr) {
            workBreakdown->payloadBlockReadMs += callback::ElapsedMilliseconds(blockReadStart);
        }
        if (stream.Position() > payloadRange.byteCount) {
            return validation::AssignError(error, "attribute decode consumed bytes beyond payload");
        }
        const auto block = MakeParsedBlockView(ownedBlock);
        if (block.header.elementCount == 0u ||
            block.header.elementOffset != decodedElements ||
            decodedElements > meta.elementCount ||
            block.header.elementCount > meta.elementCount - decodedElements) {
            return validation::AssignError(error, "attribute block range is not contiguous");
        }

        std::vector<std::uint8_t> decodedBlockBytes;
        if (UsesNonReferenceCodec(block)) {
            const auto decodeStart = callback::StartTiming(workBreakdown != nullptr);
            if (!ResolveDecodedBlockBytes(meta, block, decodedBlockBytes, error)) {
                return false;
            }
            if (workBreakdown != nullptr) {
                workBreakdown->ordinaryDecodeMs += callback::ElapsedMilliseconds(decodeStart);
                workBreakdown->ordinaryDecodedBytes = SaturatingParamSizeAdd(
                    workBreakdown->ordinaryDecodedBytes,
                    static_cast<ParamSize>(decodedBlockBytes.size()));
            }
        } else {
            const ScratchByteBuffer* referenceBytes = nullptr;
            ScratchByteBuffer intraReferenceBytes;
            ScratchByteBuffer resampledReferenceBytes;
            ScratchByteBuffer shiftedPredictorBlockBytes;
            std::size_t referenceElementOffset = 0u;
            const auto referenceResolveStart = callback::StartTiming(workBreakdown != nullptr);
            if (block.header.referenceKind == NumericArrayReferenceKind::TemporalKeyFrame) {
                const auto temporalKeyReferenceEnsureStart = callback::StartTiming(workBreakdown != nullptr);
                const auto temporalKeyFieldCached =
                    IsTemporalReferenceFieldCached(attributeKeyFrameReference, meta);
                if (workBreakdown != nullptr) {
                    if (temporalKeyFieldCached) {
                        ++workBreakdown->temporalKeyFieldCacheHits;
                    } else {
                        ++workBreakdown->temporalKeyFieldCacheMisses;
                    }
                }
                if (!EnsureTemporalReferencesDecoded(
                        attributeKeyFrameReference,
                        meta,
                        referenceDecoder,
                        error)) {
                    return false;
                }
                if (workBreakdown != nullptr) {
                    workBreakdown->temporalKeyReferenceEnsureMs +=
                        callback::ElapsedMilliseconds(temporalKeyReferenceEnsureStart);
                }
            }
            if (block.header.referenceKind == NumericArrayReferenceKind::IntraArray) {
                const auto parentIndex = static_cast<std::size_t>(block.header.localParentFieldIndex);
                if (parentIndex >= storageParams.attrParams.size() || !attributes.Complete(parentIndex)) {
                    return validation::AssignError(error, "attribute intra-field parent is not decoded");
                }
            }
            AttributeReferenceRangeObservation rangeObservation;
            const auto referenceRangeResolveStart = callback::StartTiming(workBreakdown != nullptr);
            if (!ResolveReferenceBytesForBlock(
                    storageParams,
                    attributes,
                    block,
                    meta,
                    attributeKeyFrameReference,
                    runtime.scratchBytePool,
                    referenceBytes,
                    intraReferenceBytes,
                    resampledReferenceBytes,
                    shiftedPredictorBlockBytes,
                    referenceElementOffset,
                    &rangeObservation,
                    error)) {
                return false;
            }
            if (workBreakdown != nullptr) {
                workBreakdown->referenceResolveMs += callback::ElapsedMilliseconds(referenceResolveStart);
                workBreakdown->referenceRangeResolveMs +=
                    callback::ElapsedMilliseconds(referenceRangeResolveStart);
                workBreakdown->referenceBytesRead = SaturatingParamSizeAdd(
                    workBreakdown->referenceBytesRead,
                    static_cast<ParamSize>(referenceBytes->Span().size()));
                AccumulateAttributeReferenceRangeObservation(*workBreakdown, rangeObservation);
            }

            const auto* codec = ResolveNumericArrayReferenceCodec(block.header.codecId);
            if (codec == nullptr) {
                return validation::AssignError(error, "unsupported attribute reference codec");
            }
            const auto referenceDecodeStart = callback::StartTiming(workBreakdown != nullptr);
            NumericArrayReferenceCodecDecodeTelemetry codecTelemetry;
            if (!codec->DecodeBlock(
                    NumericArrayReferenceCodecDecodeInput{
                        .meta = meta,
                        .block = block,
                        .referenceBytes = referenceBytes->Span(),
                        .referenceElementOffset = referenceElementOffset,
                        .telemetry = workBreakdown != nullptr ? &codecTelemetry : nullptr,
                    },
                    decodedBlockBytes,
                    error)) {
                return false;
            }
            if (workBreakdown != nullptr) {
                const auto referenceDecodeMs = callback::ElapsedMilliseconds(referenceDecodeStart);
                workBreakdown->referenceDecodeMs += referenceDecodeMs;
                switch (block.header.codecId) {
                    case NumericArrayReferenceCodecId::Affine:
                        workBreakdown->affineReferenceDecodeMs += referenceDecodeMs;
                        break;
                    case NumericArrayReferenceCodecId::Wavelet:
                        workBreakdown->waveletReferenceDecodeMs += referenceDecodeMs;
                        break;
                    case NumericArrayReferenceCodecId::Predictor:
                        workBreakdown->predictorReferenceDecodeMs += referenceDecodeMs;
                        break;
                    case NumericArrayReferenceCodecId::NonReference:
                    default:
                        break;
                }
                workBreakdown->referenceDecodedBytes = SaturatingParamSizeAdd(
                    workBreakdown->referenceDecodedBytes,
                    static_cast<ParamSize>(decodedBlockBytes.size()));
                workBreakdown->waveletLowBlobBytes = SaturatingParamSizeAdd(
                    workBreakdown->waveletLowBlobBytes,
                    codecTelemetry.waveletLowBlobBytes);
                workBreakdown->waveletHighBlobBytes = SaturatingParamSizeAdd(
                    workBreakdown->waveletHighBlobBytes,
                    codecTelemetry.waveletHighBlobBytes);
                workBreakdown->waveletPeakTemporaryDoubleBytes = std::max(
                    workBreakdown->waveletPeakTemporaryDoubleBytes,
                    codecTelemetry.waveletPeakTemporaryDoubleBytes);
            }
        }

        const auto writeStart = callback::StartTiming(workBreakdown != nullptr);
        if (!WriteDecodedAttributeBlock(
                attributes,
                payloadRange.attrIndex,
                meta,
                block.header,
                std::span<const std::uint8_t>(decodedBlockBytes.data(), decodedBlockBytes.size()),
                error)) {
            return false;
        }
        if (workBreakdown != nullptr) {
            workBreakdown->cacheWriteMs += callback::ElapsedMilliseconds(writeStart);
            workBreakdown->cacheWriteBytes = SaturatingParamSizeAdd(
                workBreakdown->cacheWriteBytes,
                static_cast<ParamSize>(decodedBlockBytes.size()));
        }
        if (!validation::CheckedAddU64(
                decodedElements,
                block.header.elementCount,
                decodedElements,
                "attribute decoded element count",
                error)) {
            return false;
        }
    }
    if (decodedElements != meta.elementCount) {
        return validation::AssignError(error, "attribute decoded blocks did not cover the full field");
    }
    if (stream.Position() != payloadRange.byteCount) {
        return validation::AssignError(error, "attribute decode consumed an unexpected payload size");
    }
    return attributes.EndAttribute(payloadRange.attrIndex, error);
}

template<typename TPayloadBacking, typename TReferenceDecoder>
inline bool DecodeAttributePayloadRangesToCache(
    AttributePayloadDecodeRuntime& decodeRuntime,
    const TPayloadBacking& payloadBacking,
    const std::span<const std::size_t> targetAttrIndices,
    TReferenceDecoder& referenceDecoder,
    std::string* error = nullptr) {
    const auto& storageParams = decodeRuntime.data.storageParams;
    auto* attributeKeyFrameReference = decodeRuntime.data.attributeKeyFrameReference;
    const auto& cacheResources = decodeRuntime.cache.cacheResources;
    auto& byteStoreSession = decodeRuntime.cache.byteStoreSession;
    auto& attributes = decodeRuntime.cache.attributes;
    const auto& timingCallback = decodeRuntime.context.timingCallback;
    if (!attributes.IsInitialized() &&
        !attributes.Initialize(
            storageParams,
            byteStoreSession,
            decodeRuntime.cache.attributeMemoryCacheLimitBytes,
            decodeRuntime.cache.attributeCacheStorageMode,
            error)) {
        return false;
    }
    std::vector<AttributePayloadRange> payloadRanges;
    if (!ResolveAttributePayloadRanges(storageParams, payloadRanges, error)) {
        return false;
    }
    if (payloadRanges.empty() || targetAttrIndices.empty()) {
        return true;
    }

    const auto attrCount = storageParams.attrParams.size();
    std::vector<AttributePayloadRange> rangeByAttr(attrCount);
    std::vector<std::uint8_t> hasRange(attrCount, 0u);
    for (const auto& range : payloadRanges) {
        if (range.attrIndex >= attrCount) {
            return validation::AssignError(error, "attribute payload range index is out of range");
        }
        rangeByAttr[range.attrIndex] = range;
        hasRange[range.attrIndex] = 1u;
    }

    std::vector<std::vector<std::size_t>> parentsByAttr(attrCount);
    for (std::size_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
        if (hasRange[attrIndex] == 0u) {
            return validation::AssignError(error, "attribute payload range is missing");
        }
        if (!ResolveAttributeIntraParents(storageParams, attrIndex, parentsByAttr[attrIndex], error)) {
            return false;
        }
    }

    std::vector<std::uint8_t> required(attrCount, 0u);
    std::vector<std::uint8_t> visiting(attrCount, 0u);
    std::function<bool(std::size_t)> requireAttribute;
    requireAttribute = [&](const std::size_t attrIndex) {
        if (attrIndex >= attrCount) {
            return validation::AssignError(error, "attribute target index is out of range");
        }
        if (required[attrIndex] != 0u) {
            return true;
        }
        if (visiting[attrIndex] != 0u) {
            return validation::AssignError(error, "attribute dependency graph contains a cycle");
        }
        visiting[attrIndex] = 1u;
        for (const auto parentIndex : parentsByAttr[attrIndex]) {
            if (!requireAttribute(parentIndex)) {
                return false;
            }
        }
        visiting[attrIndex] = 0u;
        required[attrIndex] = 1u;
        return true;
    };
    for (const auto attrIndex : targetAttrIndices) {
        if (!requireAttribute(attrIndex)) {
            return false;
        }
    }

    std::vector<std::vector<std::size_t>> dependents(attrCount);
    std::vector<std::size_t> remainingDependencies(attrCount, 0u);
    std::size_t workCount = 0u;
    for (std::size_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
        if (required[attrIndex] == 0u || attributes.Complete(attrIndex)) {
            continue;
        }
        ++workCount;
        for (const auto parentIndex : parentsByAttr[attrIndex]) {
            if (!attributes.Complete(parentIndex)) {
                ++remainingDependencies[attrIndex];
                dependents[parentIndex].push_back(attrIndex);
            }
        }
    }
    if (workCount == 0u) {
        return true;
    }

    const auto resolvedWorkerCount = ResolveParallelTaskCount(
        workCount,
        decodeRuntime.schedule.parallelTaskRunner,
        decodeRuntime.schedule.workerCount);
    InlineParallelTaskRunner inlineRunner;
    IParallelTaskRunner* runner = resolvedWorkerCount > 1u
        ? decodeRuntime.schedule.parallelTaskRunner
        : &inlineRunner;
    auto taskGroup = runner->CreateGroup();
    if (taskGroup == nullptr) {
        return validation::AssignError(error, "attribute decode task group is unavailable");
    }

    const auto scheduleOriginTime = callback::Now();
    std::vector<callback::PhaseTimePoint> readyTimes(attrCount, scheduleOriginTime);
    const auto millisecondsSinceOrigin = [&](const callback::PhaseTimePoint timePoint) {
        return callback::ElapsedMilliseconds(scheduleOriginTime, timePoint);
    };
    const auto millisecondsBetween = [](
        const callback::PhaseTimePoint begin,
        const callback::PhaseTimePoint end) {
        return callback::ElapsedMilliseconds(begin, end);
    };

    std::vector<std::uint8_t> scheduled(attrCount, 0u);
    std::vector<std::size_t> readyQueue;
    readyQueue.reserve(attrCount);
    std::size_t completedCount = 0u;
    std::size_t inFlightCount = 0u;
    std::atomic<std::uint8_t> failed{0u};
    std::mutex schedulerMutex;
    std::mutex errorMutex;
    std::string firstError;
    std::function<void(std::size_t)> submitAttribute;
    const bool collectTiming = static_cast<bool>(timingCallback);

    const auto pushReadyLocked = [&](const std::size_t attrIndex) {
        if (scheduled[attrIndex] != 0u) {
            return;
        }
        scheduled[attrIndex] = 1u;
        readyTimes[attrIndex] = callback::Now();
        readyQueue.push_back(attrIndex);
    };
    const auto popBestReadyLocked = [&]() -> std::size_t {
        const auto now = callback::Now();
        auto best = std::max_element(
            readyQueue.begin(),
            readyQueue.end(),
            [&](const std::size_t left, const std::size_t right) {
                return AttributeDecodeDynamicSchedulePriorityLess(
                    storageParams,
                    left,
                    right,
                    millisecondsBetween(readyTimes[left], now),
                    millisecondsBetween(readyTimes[right], now));
            });
        const auto attrIndex = *best;
        readyQueue.erase(best);
        return attrIndex;
    };
    const auto collectReadyWorkLocked = [&]() {
        std::vector<std::size_t> readyAttrs;
        while (failed.load(std::memory_order_acquire) == 0u &&
               inFlightCount < resolvedWorkerCount &&
               !readyQueue.empty()) {
            readyAttrs.push_back(popBestReadyLocked());
            ++inFlightCount;
        }
        return readyAttrs;
    };
    submitAttribute = [&](const std::size_t attrIndex) {
        taskGroup->Submit([&, attrIndex]() {
            if (failed.load(std::memory_order_acquire) != 0u) {
                return;
            }
            const auto& meta = storageParams.attrParams[attrIndex];
            const auto startTime = callback::StartTiming(collectTiming);
            const auto workerIndex = collectTiming ? CurrentParallelWorkerIndex() : 0u;
            AttributeDecodeWorkBreakdown workBreakdown;
            std::string localError;
            if (!DecodeSingleAttributeRangeToCache(
                    storageParams,
                    cacheResources,
                    attributes,
                    attributeKeyFrameReference,
                    payloadBacking,
                    rangeByAttr[attrIndex],
                    referenceDecoder,
                    collectTiming ? &workBreakdown : nullptr,
                    &localError)) {
                failed.store(1u, std::memory_order_release);
                std::lock_guard<std::mutex> lock(errorMutex);
                if (firstError.empty()) {
                    firstError = meta.name.empty()
                        ? localError
                        : meta.name + ": " + localError;
                }
                return;
            }
            if (timingCallback) {
                const auto finishTime = callback::Now();
                const auto readyTime = readyTimes[attrIndex];
                timingCallback(BuildAttributeDecodeTimingDetail(
                    attrIndex,
                    meta,
                    millisecondsBetween(startTime, finishTime),
                    millisecondsSinceOrigin(readyTime),
                    millisecondsSinceOrigin(startTime),
                    millisecondsSinceOrigin(finishTime),
                    millisecondsBetween(readyTime, startTime),
                    workerIndex,
                    workBreakdown));
            }

            std::vector<std::size_t> newlyReady;
            {
                std::lock_guard<std::mutex> lock(schedulerMutex);
                if (inFlightCount > 0u) {
                    --inFlightCount;
                }
                ++completedCount;
                for (const auto dependentIndex : dependents[attrIndex]) {
                    if (remainingDependencies[dependentIndex] > 0u) {
                        --remainingDependencies[dependentIndex];
                    }
                    if (remainingDependencies[dependentIndex] == 0u &&
                        scheduled[dependentIndex] == 0u) {
                        pushReadyLocked(dependentIndex);
                    }
                }
                newlyReady = collectReadyWorkLocked();
            }
            for (const auto readyIndex : newlyReady) {
                submitAttribute(readyIndex);
            }
        });
    };

    std::vector<std::size_t> initialReadyAttrs;
    {
        std::lock_guard<std::mutex> lock(schedulerMutex);
        for (std::size_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
            if (required[attrIndex] != 0u &&
                !attributes.Complete(attrIndex) &&
                remainingDependencies[attrIndex] == 0u) {
                pushReadyLocked(attrIndex);
            }
        }
        initialReadyAttrs = collectReadyWorkLocked();
    }
    if (initialReadyAttrs.empty()) {
        return validation::AssignError(error, "attribute decode scheduler detected a dependency cycle");
    }
    for (const auto attrIndex : initialReadyAttrs) {
        submitAttribute(attrIndex);
    }
    taskGroup->Wait();
    if (failed.load(std::memory_order_acquire) != 0u) {
        std::lock_guard<std::mutex> lock(errorMutex);
        return validation::AssignError(
            error,
            firstError.empty() ? "attribute decode failed" : firstError);
    }
    if (completedCount != workCount) {
        return validation::AssignError(error, "attribute decode scheduler did not complete all requested fields");
    }
    for (const auto attrIndex : targetAttrIndices) {
        if (attrIndex >= attrCount || !attributes.Complete(attrIndex)) {
            return validation::AssignError(error, "requested attribute was not decoded");
        }
    }
    return true;
}

template<typename TStream, typename TReferenceDecoder>
inline bool DecodeAttributeToCache(
    AttributeStreamDecodeRuntime& decodeRuntime,
    TStream& stream,
    const std::span<const std::size_t> targetAttrIndices,
    TReferenceDecoder& referenceDecoder,
    std::string* error = nullptr) {
    const auto& storageParams = decodeRuntime.data.storageParams;
    auto* attributeKeyFrameReference = decodeRuntime.data.attributeKeyFrameReference;
    const auto& cacheResources = decodeRuntime.cache.cacheResources;
    auto& byteStoreSession = decodeRuntime.cache.byteStoreSession;
    auto& attributes = decodeRuntime.cache.attributes;
    if (!attributes.IsInitialized() &&
        !attributes.Initialize(storageParams, byteStoreSession, error)) {
        return false;
    }
    std::uint64_t scratchPeakBytes = 0u;
    const auto updateScratchPeak = [&scratchPeakBytes](const auto&... buffers) {
        std::uint64_t currentBytes = 0u;
        ((currentBytes = validation::SaturatingAddU64(
            currentBytes,
            static_cast<std::uint64_t>(buffers.capacity()))), ...);
        scratchPeakBytes = std::max(scratchPeakBytes, currentBytes);
    };

    std::vector<std::size_t> payloadOrder;
    if (!ResolveAttributePayloadOrder(storageParams, payloadOrder, error)) {
        return false;
    }
    const auto attrCount = storageParams.attrParams.size();
    std::vector<std::vector<std::size_t>> parentsByAttr(attrCount);
    for (std::size_t attrIndex = 0u; attrIndex < attrCount; ++attrIndex) {
        if (!ResolveAttributeIntraParents(storageParams, attrIndex, parentsByAttr[attrIndex], error)) {
            return false;
        }
    }
    std::vector<std::uint8_t> required(attrCount, 0u);
    std::vector<std::uint8_t> visiting(attrCount, 0u);
    std::function<bool(std::size_t)> requireAttribute;
    requireAttribute = [&](const std::size_t attrIndex) {
        if (attrIndex >= attrCount) {
            return validation::AssignError(error, "attribute target index is out of range");
        }
        if (required[attrIndex] != 0u) {
            return true;
        }
        if (visiting[attrIndex] != 0u) {
            return validation::AssignError(error, "attribute dependency graph contains a cycle");
        }
        visiting[attrIndex] = 1u;
        for (const auto parentIndex : parentsByAttr[attrIndex]) {
            if (!requireAttribute(parentIndex)) {
                return false;
            }
        }
        visiting[attrIndex] = 0u;
        required[attrIndex] = 1u;
        return true;
    };
    for (const auto attrIndex : targetAttrIndices) {
        if (!requireAttribute(attrIndex)) {
            return false;
        }
    }

    std::vector<std::uint8_t> seen(attrCount, 0u);
    for (const auto attrIndex : payloadOrder) {
        const auto& meta = storageParams.attrParams[attrIndex];
        const auto payloadBytes = static_cast<std::uint64_t>(meta.binaryCount);
        const auto begin = stream.Position();
        if (!validation::CanAddU64(begin, payloadBytes)) {
            validation::AssignError(error, "attribute payload range exceeds stream address space");
            return false;
        }
        const auto end = begin + payloadBytes;
        if (required[attrIndex] == 0u || attributes.Complete(attrIndex)) {
            if (!stream.Skip(payloadBytes, error)) {
                return false;
            }
            seen[attrIndex] = attributes.Complete(attrIndex) ? 1u : 0u;
            continue;
        }
        if (!attributes.BeginAttribute(attrIndex, meta, error)) {
            return false;
        }

        if (payloadBytes == 0u) {
            if (meta.elementCount != 0u) {
                return validation::AssignError(error, "attribute payload is empty for a non-empty field");
            }
            if (!attributes.EndAttribute(attrIndex, error)) {
                return false;
            }
            seen[attrIndex] = 1u;
            continue;
        }

        if (meta.blockLayouts.empty()) {
            return validation::AssignError(error, "attribute params are missing block layout metadata");
        }
        std::uint64_t decodedElements = 0u;
        for (const auto& blockLayout : meta.blockLayouts) {
            numericarray::NumericArrayBlockPayload ownedBlock;
            if (!numericarray::ReadNumericArrayBlockPayload(
                    stream,
                    static_cast<std::size_t>(std::max(meta.dimension, 0)),
                    blockLayout,
                    cacheResources,
                    ownedBlock,
                    error)) {
                return false;
            }
            if (stream.Position() > end) {
                return validation::AssignError(error, "attribute decode consumed bytes beyond payload");
            }
            const auto block = MakeParsedBlockView(ownedBlock);
            if (block.header.elementCount == 0u ||
                block.header.elementOffset != decodedElements ||
                decodedElements > meta.elementCount ||
                block.header.elementCount > meta.elementCount - decodedElements) {
                return validation::AssignError(error, "attribute block range is not contiguous");
            }

            std::vector<std::uint8_t> decodedBlockBytes;
            if (UsesNonReferenceCodec(block)) {
                if (!ResolveDecodedBlockBytes(meta, block, decodedBlockBytes, error)) {
                    return false;
                }
                updateScratchPeak(decodedBlockBytes);
            } else {
                const ScratchByteBuffer* referenceBytes = nullptr;
                ScratchByteBuffer intraReferenceBytes;
                ScratchByteBuffer resampledReferenceBytes;
                ScratchByteBuffer shiftedPredictorBlockBytes;
                std::size_t referenceElementOffset = 0u;
                if (block.header.referenceKind == NumericArrayReferenceKind::TemporalKeyFrame &&
                    !EnsureTemporalReferencesDecoded(
                        attributeKeyFrameReference,
                        meta,
                        referenceDecoder,
                        error)) {
                    return false;
                }
                if (block.header.referenceKind == NumericArrayReferenceKind::IntraArray) {
                    const auto parentIndex = static_cast<std::size_t>(block.header.localParentFieldIndex);
                    if (parentIndex >= seen.size() || seen[parentIndex] == 0u) {
                        return validation::AssignError(
                            error,
                            "attribute intra-field parent is not decoded in topology order");
                    }
                }
                if (!ResolveReferenceBytesForBlock(
                        storageParams,
                        attributes,
                        block,
                        meta,
                        attributeKeyFrameReference,
                        cacheResources.scratchBytePool,
                        referenceBytes,
                        intraReferenceBytes,
                        resampledReferenceBytes,
                        shiftedPredictorBlockBytes,
                        referenceElementOffset,
                        nullptr,
                        error)) {
                    return false;
                }

                const auto* codec = ResolveNumericArrayReferenceCodec(block.header.codecId);
                if (codec == nullptr) {
                    return validation::AssignError(error, "unsupported attribute reference codec");
                }
                if (!codec->DecodeBlock(
                        NumericArrayReferenceCodecDecodeInput{
                            .meta = meta,
                            .block = block,
                            .referenceBytes = referenceBytes->Span(),
                            .referenceElementOffset = referenceElementOffset,
                        },
                        decodedBlockBytes,
                        error)) {
                    return false;
                }
                updateScratchPeak(
                    intraReferenceBytes.Bytes(),
                    resampledReferenceBytes.Bytes(),
                    shiftedPredictorBlockBytes.Bytes(),
                    decodedBlockBytes);
            }

            if (!WriteDecodedAttributeBlock(
                    attributes,
                    attrIndex,
                    meta,
                    block.header,
                    std::span<const std::uint8_t>(decodedBlockBytes.data(), decodedBlockBytes.size()),
                    error)) {
                return false;
            }
            if (!validation::CheckedAddU64(
                    decodedElements,
                    block.header.elementCount,
                    decodedElements,
                    "attribute decoded element count",
                    error)) {
                return false;
            }
        }
        if (decodedElements != meta.elementCount) {
            return validation::AssignError(error, "attribute decoded blocks did not cover the full field");
        }

        if (!attributes.EndAttribute(attrIndex, error)) {
            return false;
        }
        if (stream.Position() != end) {
            return validation::AssignError(error, "attribute decode consumed an unexpected payload size");
        }
        seen[attrIndex] = 1u;
    }

    for (const auto attrIndex : targetAttrIndices) {
        if (attrIndex >= attrCount || !attributes.Complete(attrIndex)) {
            return validation::AssignError(error, "requested attribute reference was not decoded");
        }
    }
    return true;
}

template<typename TReferenceDecoder>
inline bool EnsureTemporalReferencesDecoded(
    DecodedAttributeReference* attributeKeyFrameReference,
    const AttrStorageParams& targetMeta,
    TReferenceDecoder& referenceDecoder,
    std::string* error) {
    if (attributeKeyFrameReference != nullptr &&
        !referenceDecoder(*attributeKeyFrameReference, targetMeta, "key frame", error)) {
        if (error != nullptr && !error->empty()) {
            return validation::AssignError(error, "failed to decode key-frame reference: " + *error);
        }
        return validation::AssignError(error, "failed to decode key-frame reference");
    }
    return true;
}

} // namespace detail

} // namespace decodeimpl
} // namespace datacodec

#endif
