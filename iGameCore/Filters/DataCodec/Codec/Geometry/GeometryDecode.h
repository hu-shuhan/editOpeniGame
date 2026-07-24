#ifndef DATACODEC_CODEC_GEOMETRY_GEOMETRYDECODE_H
#define DATACODEC_CODEC_GEOMETRY_GEOMETRYDECODE_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedGeometryCache.h"
#include "DataCodec/Runtime/Cache/DecodeCache/ReferenceCacheNumericArraySource.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockReader.h"
#include "DataCodec/Codec/Reference/DecodedReference.h"
#include "DataCodec/Codec/Reference/NumericArrayReferenceBytes.h"
#include "DataCodec/Codec/Reference/ReferenceCodec.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

using GeometryDecodeResult = CodecStatus;

struct GeometryDecodeData {
    const GeometryStorageParams& meta;
    const DecodedGeometryReference* keyFrameReference{nullptr};
    std::uint64_t payloadBytes{0u};
};

struct GeometryDecodeCache {
    const CacheResources& cacheResources;
    bytestore::ByteStoreSession& byteStoreSession;
    DecodedGeometryCache& geometry;
    DecodedGeometryReferenceCache* referenceCache{nullptr};
    DecodeStorageMode geometryCacheStorageMode{DecodeStorageMode::Managed};
    std::uint64_t geometryMemoryCacheLimitBytes{0u};
    DecodeStorageMode geometryReferenceCacheStorageMode{DecodeStorageMode::Managed};
    std::uint64_t geometryMemoryReferenceLimitBytes{0u};
};

struct GeometryDecodeRuntime {
    GeometryDecodeData data;
    GeometryDecodeCache cache;
};

namespace detail {

inline GeometryDecodeResult MakeGeometryDecodeSuccess() {
    return CodecStatus::Ok();
}

inline GeometryDecodeResult MakeGeometryDecodeFailure(
    const CodecErrorCode code,
    std::string message) {
    return CodecStatus::Failure(code, std::move(message));
}

template<typename TValue>
inline bool WriteConvertedPointRange(
    const CacheResources& cacheResources,
    DecodedGeometryCache& geometry,
    const NumericArrayBlockHeader& header,
    const std::span<const std::uint8_t> decodedBytes,
    const std::size_t dimension,
    const std::uint64_t byteOffset,
    std::uint64_t& scratchPeakBytes,
    std::string* error = nullptr) {
    const auto* source = reinterpret_cast<const TValue*>(decodedBytes.data());
    const auto decodedValueCount = decodedBytes.size() / sizeof(TValue);
    std::size_t convertedByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            decodedValueCount,
            sizeof(float),
            convertedByteCount,
            "converted geometry scratch bytes",
            error)) {
        return false;
    }
    auto converted = cacheResources.scratchBytePool.Acquire(convertedByteCount);
    auto* output = reinterpret_cast<float*>(converted.Bytes().data());
    for (std::size_t index = 0; index < decodedValueCount; ++index) {
        output[index] = static_cast<float>(source[index]);
    }
    scratchPeakBytes = std::max<std::uint64_t>(
        scratchPeakBytes,
        validation::SaturatingAddU64(
            static_cast<std::uint64_t>(decodedBytes.size()),
            static_cast<std::uint64_t>(converted.Bytes().capacity())));
    std::size_t localElementCount = 0u;
    std::size_t expectedValues = 0u;
    if (!validation::CheckedCastSizeT(header.elementCount, localElementCount, "geometry block element count", error) ||
        !validation::CheckedMulSizeT(
            localElementCount,
            dimension,
            expectedValues,
            "converted geometry block value count",
            error)) {
        return false;
    }
    if (decodedValueCount != expectedValues) {
        return validation::AssignError(error, "converted geometry block value count does not match range");
    }
    return geometry.bytes != nullptr &&
        geometry.bytes->WriteBytesAt(byteOffset, converted.Span(), error);
}

inline bool WriteRawGeometryReferenceBlock(
    DecodedGeometryReferenceCache* referenceCache,
    const GeometryStorageParams& meta,
    const NumericArrayBlockHeader& header,
    const std::span<const std::uint8_t> decodedBytes,
    std::string* error = nullptr) {
    if (referenceCache == nullptr) {
        return true;
    }
    std::size_t localValueSize = 0u;
    if (!TryParamSizeToSizeT(meta.valueSize, localValueSize)) {
        return validation::AssignError(error, "geometry metadata exceeds this platform size limit");
    }
    std::size_t tupleBytes = 0u;
    std::size_t expectedBytes = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(std::max(meta.dimension, 0)),
            localValueSize,
            tupleBytes,
            "decoded geometry reference tuple bytes",
            error) ||
        !validation::CheckedMulSizeT(
            static_cast<std::size_t>(header.elementCount),
            tupleBytes,
            expectedBytes,
            "decoded geometry reference block bytes",
            error)) {
        return false;
    }
    if (decodedBytes.size() != expectedBytes) {
        return validation::AssignError(error, "decoded geometry reference block size does not match range");
    }
    return referenceCache->WriteRange(
        header.elementOffset,
        header.elementCount,
        decodedBytes.data(),
        decodedBytes.size(),
        error);
}

inline bool WriteConvertedGeometryBlock(
    const CacheResources& cacheResources,
    DecodedGeometryCache& geometry,
    DecodedGeometryReferenceCache* referenceCache,
    const GeometryStorageParams& meta,
    const NumericArrayBlockHeader& header,
    const std::span<const std::uint8_t> decodedBytes,
    const std::size_t dimension,
    const std::size_t expectedSourceBytes,
    const std::size_t outputTupleBytes,
    std::size_t& writtenBytes,
    std::uint64_t& scratchPeakBytes,
    std::string* error = nullptr) {
    std::size_t localValueSize = 0u;
    if (!TryParamSizeToSizeT(meta.valueSize, localValueSize)) {
        return validation::AssignError(error, "geometry metadata exceeds this platform size limit");
    }
    std::size_t sourceTupleBytes = 0u;
    std::size_t localElementOffset = 0u;
    std::size_t localElementCount = 0u;
    std::size_t sourceByteOffset = 0u;
    if (!validation::CheckedMulSizeT(
            dimension,
            localValueSize,
            sourceTupleBytes,
            "decoded geometry source tuple bytes",
            error) ||
        !validation::CheckedCastSizeT(header.elementOffset, localElementOffset, "geometry block element offset", error) ||
        !validation::CheckedCastSizeT(header.elementCount, localElementCount, "geometry block element count", error) ||
        !validation::CheckedMulSizeT(
            localElementOffset,
            sourceTupleBytes,
            sourceByteOffset,
            "decoded geometry source byte offset",
            error)) {
        return false;
    }
    if (sourceByteOffset > expectedSourceBytes || decodedBytes.size() > expectedSourceBytes - sourceByteOffset) {
        return validation::AssignError(error, "geometry block is outside output range");
    }
    if (!WriteRawGeometryReferenceBlock(referenceCache, meta, header, decodedBytes, error)) {
        return false;
    }

    std::uint64_t outputByteOffset = 0u;
    std::size_t expectedOutputBytes = 0u;
    if (!validation::CheckedMulU64(
            header.elementOffset,
            static_cast<std::uint64_t>(outputTupleBytes),
            outputByteOffset,
            "decoded geometry output byte offset",
            error) ||
        !validation::CheckedMulSizeT(
            localElementCount,
            outputTupleBytes,
            expectedOutputBytes,
            "decoded geometry output block bytes",
            error)) {
        return false;
    }
    switch (meta.dataType) {
        case DataType::Float32:
            scratchPeakBytes = std::max<std::uint64_t>(
                scratchPeakBytes,
                static_cast<std::uint64_t>(decodedBytes.size()));
            if (decodedBytes.size() != expectedOutputBytes ||
                geometry.bytes == nullptr ||
                !geometry.bytes->WriteBytesAt(outputByteOffset, decodedBytes, error)) {
                return false;
            }
            break;
        case DataType::Float64:
            if (!WriteConvertedPointRange<double>(cacheResources, geometry, header, decodedBytes, dimension, outputByteOffset, scratchPeakBytes, error)) {
                return false;
            }
            break;
        case DataType::Int8:
            if (!WriteConvertedPointRange<std::int8_t>(cacheResources, geometry, header, decodedBytes, dimension, outputByteOffset, scratchPeakBytes, error)) {
                return false;
            }
            break;
        case DataType::UInt8:
            if (!WriteConvertedPointRange<std::uint8_t>(cacheResources, geometry, header, decodedBytes, dimension, outputByteOffset, scratchPeakBytes, error)) {
                return false;
            }
            break;
        case DataType::Int16:
            if (!WriteConvertedPointRange<std::int16_t>(cacheResources, geometry, header, decodedBytes, dimension, outputByteOffset, scratchPeakBytes, error)) {
                return false;
            }
            break;
        case DataType::UInt16:
            if (!WriteConvertedPointRange<std::uint16_t>(cacheResources, geometry, header, decodedBytes, dimension, outputByteOffset, scratchPeakBytes, error)) {
                return false;
            }
            break;
        case DataType::Int32:
            if (!WriteConvertedPointRange<std::int32_t>(cacheResources, geometry, header, decodedBytes, dimension, outputByteOffset, scratchPeakBytes, error)) {
                return false;
            }
            break;
        case DataType::UInt32:
            if (!WriteConvertedPointRange<std::uint32_t>(cacheResources, geometry, header, decodedBytes, dimension, outputByteOffset, scratchPeakBytes, error)) {
                return false;
            }
            break;
        case DataType::Int64:
            if (!WriteConvertedPointRange<std::int64_t>(cacheResources, geometry, header, decodedBytes, dimension, outputByteOffset, scratchPeakBytes, error)) {
                return false;
            }
            break;
        case DataType::UInt64:
            if (!WriteConvertedPointRange<std::uint64_t>(cacheResources, geometry, header, decodedBytes, dimension, outputByteOffset, scratchPeakBytes, error)) {
                return false;
            }
            break;
    }
    return validation::CheckedAddSizeT(
        writtenBytes,
        expectedOutputBytes,
        writtenBytes,
        "decoded geometry written bytes",
        error);
}

inline bool ResolveGeometryReferenceBytesForBlock(
    const GeometryStorageParams& meta,
    const DecodedGeometryReference* keyFrameReference,
    const ParsedNumericArrayBlock& block,
    ScratchByteBufferPool& scratchBytePool,
    ScratchByteBuffer& referenceBytes,
    std::string* error = nullptr) {
    if (block.header.referenceKind != NumericArrayReferenceKind::TemporalKeyFrame) {
        return validation::AssignError(error, "geometry reference block requires temporal key-frame reference");
    }
    if (keyFrameReference == nullptr ||
        keyFrameReference->store == nullptr ||
        !keyFrameReference->store->IsComplete()) {
        return validation::AssignError(error, "geometry key frame reference store is not ready");
    }

    GeometryStorageParams referenceMeta;
    numericarray::NumericArraySource referenceSource;
    if (!BuildGeometryReferenceCacheNumericArraySource(
            keyFrameReference->store,
            meta,
            referenceMeta,
            referenceSource,
            error)) {
        return false;
    }
    numericarray::NumericArrayReader referenceReader;
    if (!numericarray::BuildNumericArrayReader(referenceSource, referenceReader, error)) {
        return false;
    }
    std::size_t localElementOffset = 0u;
    std::size_t localElementCount = 0u;
    if (!validation::CheckedCastSizeT(
            block.header.elementOffset,
            localElementOffset,
            "geometry reference block element offset",
            error) ||
        !validation::CheckedCastSizeT(
            block.header.elementCount,
            localElementCount,
            "geometry reference block element count",
            error)) {
        return false;
    }
    if (block.header.codecId == NumericArrayReferenceCodecId::Predictor) {
        return numericarrayreference::BuildNumericArrayPredictorReferenceBlockBytes(
            referenceReader,
            referenceMeta,
            meta,
            scratchBytePool,
            localElementOffset,
            localElementCount,
            block.header.predictorOffset,
            referenceBytes,
            error);
    }
    return numericarrayreference::BuildNumericArrayReferenceRangeBytes(
        referenceReader,
        referenceMeta,
        meta,
        scratchBytePool,
        localElementOffset,
        localElementCount,
        referenceBytes,
        error);
}

inline GeometryDecodeResult PrepareGeometryCache(
    bytestore::ByteStoreSession& byteStoreSession,
    const GeometryStorageParams& meta,
    DecodedGeometryCache& geometry,
    DecodedGeometryReferenceCache* referenceCache,
    const DecodeStorageMode geometryCacheStorageMode,
    const std::uint64_t geometryMemoryCacheLimitBytes,
    const DecodeStorageMode geometryReferenceCacheStorageMode,
    const std::uint64_t geometryMemoryReferenceLimitBytes,
    const std::size_t count,
    const std::size_t dimension) {
    std::string error;
    if (!geometry.Initialize(
            count,
            dimension,
            byteStoreSession,
            geometryCacheStorageMode,
            geometryMemoryCacheLimitBytes,
            &error)) {
        return MakeGeometryDecodeFailure(
            CodecErrorCode::PipelineFailure,
            "failed to allocate geometry cache: " + error);
    }
    if (referenceCache != nullptr &&
        !referenceCache->IsInitialized() &&
        !referenceCache->Initialize(
            meta,
            byteStoreSession,
            geometryReferenceCacheStorageMode,
            geometryMemoryReferenceLimitBytes,
            &error)) {
        return MakeGeometryDecodeFailure(
            CodecErrorCode::PipelineFailure,
            "failed to prepare current geometry reference cache: " + error);
    }
    if (referenceCache != nullptr &&
        !referenceCache->BeginGeometry(meta, &error)) {
        return MakeGeometryDecodeFailure(
            CodecErrorCode::PipelineFailure,
            "failed to prepare current geometry reference cache: " + error);
    }
    return MakeGeometryDecodeSuccess();
}

inline GeometryDecodeResult FinishGeometryCache(
    DecodedGeometryCache& geometry,
    DecodedGeometryReferenceCache* referenceCache,
    const std::size_t writtenBytes,
    const std::size_t expectedOutputBytes,
    std::string incompleteMessage) {
    std::string error;
    if (referenceCache != nullptr &&
        !referenceCache->EndGeometry(&error)) {
        return MakeGeometryDecodeFailure(
            CodecErrorCode::PipelineFailure,
            "failed to finish current geometry reference cache: " + error);
    }
    geometry.complete = true;
    if (writtenBytes != expectedOutputBytes) {
        return MakeGeometryDecodeFailure(
            CodecErrorCode::PipelineFailure,
            std::move(incompleteMessage));
    }
    return MakeGeometryDecodeSuccess();
}

} // namespace detail

template<typename TStream>
inline GeometryDecodeResult DecodeNumericArrayGeometryField(
    GeometryDecodeRuntime& decodeRuntime,
    TStream& stream) {
    const auto& meta = decodeRuntime.data.meta;
    const auto payloadBytes = decodeRuntime.data.payloadBytes;
    const auto& cacheResources = decodeRuntime.cache.cacheResources;
    auto& byteStoreSession = decodeRuntime.cache.byteStoreSession;
    auto& geometry = decodeRuntime.cache.geometry;
    auto* referenceCache = decodeRuntime.cache.referenceCache;
    std::size_t count = 0u;
    std::size_t localValueSize = 0u;
    if (!TryParamSizeToSizeT(meta.elementCount, count) ||
        !TryParamSizeToSizeT(meta.valueSize, localValueSize)) {
        return detail::MakeGeometryDecodeFailure(
            CodecErrorCode::UnsupportedPlatform,
            "geometry metadata exceeds this platform size limit");
    }
    const auto dimension = static_cast<std::size_t>(std::max(0, meta.dimension));
    const auto sourceTupleBytes = dimension * localValueSize;
    const auto outputTupleBytes = dimension * sizeof(float);
    const auto expectedSourceBytes = count * sourceTupleBytes;
    const auto expectedOutputBytes = count * outputTupleBytes;
    std::size_t writtenBytes = 0u;
    std::uint64_t scratchPeakBytes = 0u;
    const auto begin = stream.Position();
    if (!validation::CanAddU64(begin, payloadBytes)) {
        return detail::MakeGeometryDecodeFailure(
            CodecErrorCode::InvalidInput,
            "geometry payload range exceeds stream address space");
    }
    const auto end = begin + payloadBytes;

    if (auto prepared = detail::PrepareGeometryCache(
            byteStoreSession,
            meta,
            geometry,
            referenceCache,
            decodeRuntime.cache.geometryCacheStorageMode,
            decodeRuntime.cache.geometryMemoryCacheLimitBytes,
            decodeRuntime.cache.geometryReferenceCacheStorageMode,
            decodeRuntime.cache.geometryMemoryReferenceLimitBytes,
            count,
            dimension); !prepared) {
        return prepared;
    }

    const auto writeDecodedBlock = [&](const NumericArrayBlockHeader& header,
                                       const std::span<const std::uint8_t> decodedBytes,
                                       std::string* blockError) -> bool {
        return detail::WriteConvertedGeometryBlock(
            cacheResources,
            geometry,
            referenceCache,
            meta,
            header,
            decodedBytes,
            dimension,
            expectedSourceBytes,
            outputTupleBytes,
            writtenBytes,
            scratchPeakBytes,
            blockError);
    };

    std::string error;
    auto blockParams = numericarray::MakeNumericArrayBlockParams(
        numericarray::MakeNumericArrayLayout(meta.dataType, localValueSize, count, dimension));
    if (!numericarray::DecodeNumericArrayBlocks(
            stream,
            blockParams,
            meta.blockLayouts,
            cacheResources,
            writeDecodedBlock,
            &error)) {
        return detail::MakeGeometryDecodeFailure(
            CodecErrorCode::PipelineFailure,
            "failed to decode numeric-array geometry blocks: " + error);
    }
    if (stream.Position() != end) {
        return detail::MakeGeometryDecodeFailure(
            CodecErrorCode::PipelineFailure,
            "geometry decode consumed an unexpected payload size");
    }

    return detail::FinishGeometryCache(
        geometry,
        referenceCache,
        writtenBytes,
        expectedOutputBytes,
        "geometry blocks did not cover the full output range");
}

template<typename TStream>
inline GeometryDecodeResult DecodeReferenceGeometryField(
    GeometryDecodeRuntime& decodeRuntime,
    TStream& stream) {
    const auto& meta = decodeRuntime.data.meta;
    const auto* keyFrameReference = decodeRuntime.data.keyFrameReference;
    const auto payloadBytes = decodeRuntime.data.payloadBytes;
    const auto& cacheResources = decodeRuntime.cache.cacheResources;
    auto& byteStoreSession = decodeRuntime.cache.byteStoreSession;
    auto& geometry = decodeRuntime.cache.geometry;
    auto* referenceCache = decodeRuntime.cache.referenceCache;
    std::size_t count = 0u;
    std::size_t localValueSize = 0u;
    if (!TryParamSizeToSizeT(meta.elementCount, count) ||
        !TryParamSizeToSizeT(meta.valueSize, localValueSize)) {
        return detail::MakeGeometryDecodeFailure(
            CodecErrorCode::UnsupportedPlatform,
            "geometry metadata exceeds this platform size limit");
    }
    const auto dimension = static_cast<std::size_t>(std::max(0, meta.dimension));
    const auto sourceTupleBytes = dimension * localValueSize;
    const auto outputTupleBytes = dimension * sizeof(float);
    const auto expectedSourceBytes = count * sourceTupleBytes;
    const auto expectedOutputBytes = count * outputTupleBytes;
    std::size_t writtenBytes = 0u;
    std::uint64_t scratchPeakBytes = 0u;
    const auto begin = stream.Position();
    if (!validation::CanAddU64(begin, payloadBytes)) {
        return detail::MakeGeometryDecodeFailure(
            CodecErrorCode::InvalidInput,
            "geometry payload range exceeds stream address space");
    }
    const auto end = begin + payloadBytes;

    if (auto prepared = detail::PrepareGeometryCache(
            byteStoreSession,
            meta,
            geometry,
            referenceCache,
            decodeRuntime.cache.geometryCacheStorageMode,
            decodeRuntime.cache.geometryMemoryCacheLimitBytes,
            decodeRuntime.cache.geometryReferenceCacheStorageMode,
            decodeRuntime.cache.geometryMemoryReferenceLimitBytes,
            count,
            dimension); !prepared) {
        return prepared;
    }

    if (count != 0u && meta.blockLayouts.empty()) {
        return detail::MakeGeometryDecodeFailure(
            CodecErrorCode::InvalidInput,
            "geometry reference params are missing spatial block layouts");
    }
    std::string error;
    auto blockParams = numericarray::MakeNumericArrayBlockParams(
        numericarray::MakeNumericArrayLayout(meta.dataType, localValueSize, count, dimension));
    std::uint64_t consumedElements = 0u;
    for (const auto& layout : meta.blockLayouts) {
        numericarray::NumericArrayBlockPayload ownedBlock;
        if (!numericarray::ReadNumericArrayBlockPayload(
                stream,
                dimension,
                layout,
                cacheResources,
                ownedBlock,
                &error)) {
            return detail::MakeGeometryDecodeFailure(
                CodecErrorCode::PipelineFailure,
                "failed to read geometry spatial block: " + error);
        }
        if (ownedBlock.header.elementCount == 0u ||
            ownedBlock.header.elementOffset != consumedElements ||
            ownedBlock.header.elementCount > count - consumedElements) {
            return detail::MakeGeometryDecodeFailure(
                CodecErrorCode::InvalidInput,
                "geometry spatial block ranges are not contiguous");
        }

        std::vector<std::uint8_t> decodedBlockBytes;
        if (ownedBlock.header.codecId == NumericArrayReferenceCodecId::NonReference) {
            if (!numericarray::ResolveDecodedNumericArrayBlockBytes(
                    blockParams,
                    ownedBlock,
                    decodedBlockBytes,
                    &error)) {
                return detail::MakeGeometryDecodeFailure(
                    CodecErrorCode::DecodeFailure,
                    "failed to decode ordinary geometry spatial block: " + error);
            }
        } else {
            ParsedNumericArrayBlock block;
            block.header = ownedBlock.header;
            block.backgroundCompressor = ownedBlock.backgroundCompressor;
            block.backgroundEncodedByteLength = ownedBlock.backgroundEncodedByteLength;
            block.componentLayouts = ownedBlock.componentLayouts;
            block.regionLayers = ownedBlock.regionLayers;
            block.alpha = ownedBlock.alpha;
            block.beta = ownedBlock.beta;
            block.bytes = ownedBlock.bytes.Span();

            ScratchByteBuffer referenceBytes;
            if (!detail::ResolveGeometryReferenceBytesForBlock(
                    meta,
                    keyFrameReference,
                    block,
                    cacheResources.scratchBytePool,
                    referenceBytes,
                    &error)) {
                return detail::MakeGeometryDecodeFailure(
                    CodecErrorCode::DecodeFailure,
                    "failed to resolve geometry spatial block reference: " + error);
            }
            const auto* codec = ResolveNumericArrayReferenceCodec(block.header.codecId);
            if (codec == nullptr) {
                return detail::MakeGeometryDecodeFailure(
                    CodecErrorCode::InvalidInput,
                    "unsupported geometry spatial block reference codec");
            }
            if (!codec->DecodeBlock(
                    NumericArrayReferenceCodecDecodeInput{
                        .meta = meta,
                        .block = block,
                        .referenceBytes = referenceBytes.Span(),
                        .referenceElementOffset = 0u,
                    },
                    decodedBlockBytes,
                    &error)) {
                return detail::MakeGeometryDecodeFailure(
                    CodecErrorCode::DecodeFailure,
                    "failed to decode geometry reference spatial block: " + error);
            }
        }
        if (!detail::WriteConvertedGeometryBlock(
                cacheResources,
                geometry,
                referenceCache,
                meta,
                ownedBlock.header,
                std::span<const std::uint8_t>(decodedBlockBytes.data(), decodedBlockBytes.size()),
                dimension,
                expectedSourceBytes,
                outputTupleBytes,
                writtenBytes,
                scratchPeakBytes,
                &error)) {
            return detail::MakeGeometryDecodeFailure(
                CodecErrorCode::DecodeFailure,
                "failed to write decoded geometry spatial block: " + error);
        }
        consumedElements += ownedBlock.header.elementCount;
    }
    if (consumedElements != count) {
        return detail::MakeGeometryDecodeFailure(
            CodecErrorCode::InvalidInput,
            "geometry spatial block layouts do not cover the full field");
    }
    if (stream.Position() != end) {
        return detail::MakeGeometryDecodeFailure(
            CodecErrorCode::PipelineFailure,
            "geometry decode consumed an unexpected payload size");
    }

    return detail::FinishGeometryCache(
        geometry,
        referenceCache,
        writtenBytes,
        expectedOutputBytes,
        "reference geometry did not cover the full output range");
}

} // namespace datacodec

#endif
