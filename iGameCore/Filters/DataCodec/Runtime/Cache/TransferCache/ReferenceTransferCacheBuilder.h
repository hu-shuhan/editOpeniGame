#ifndef DATACODEC_RUNTIME_CACHE_TRANSFERCACHE_REFERENCETRANSFERCACHEBUILDER_H
#define DATACODEC_RUNTIME_CACHE_TRANSFERCACHE_REFERENCETRANSFERCACHEBUILDER_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Storage/ByteIO/Window/WindowBudget.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockFormat.h"
#include "DataCodec/Codec/NumericArray/SpatialBlockLayout.h"
#include "DataCodec/Codec/Reference/NumericArrayReferenceBytes.h"
#include "DataCodec/Codec/Reference/ReferenceCodec.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace datacodec {
namespace numericarrayreference {

struct NumericArrayReferenceSourceData {
    NumericArrayReferenceCandidate candidate;
    NumericArrayStorageParams meta;
    numericarray::NumericArraySource source;

    [[nodiscard]] bool HasReference() const noexcept {
        return candidate.HasReference();
    }
};

using NumericArrayReferenceStagingStoreFactory = std::function<std::shared_ptr<bytestore::IByteStore>(
    const std::string&,
    std::uint64_t,
    std::string*)>;

struct NumericArrayReferenceTransferControl {
    double affineBlockRSquared{0.95};
    TemporalPredictorControlParams predictor;
    ReferenceSelectionMode selectionMode{ReferenceSelectionMode::Auto};
    ReferenceAutoSelectionStrategy autoSelectionStrategy{
        ReferenceAutoSelectionStrategy::Exact};
    std::uint32_t spatialBlockElementCount{262144u};
    ScratchByteQuotaAcquire acquireScratchQuota;
    std::function<bool(
        const NumericArrayStorageParams&,
        std::span<const std::uint8_t>,
        const numericarray::NumericArrayReader&,
        const NumericArrayStorageParams&,
        ScratchByteBufferPool&,
        std::uint32_t,
        std::uint32_t,
        NumericArrayReferenceKind,
        std::uint16_t,
        std::int32_t&,
        std::string*)> selectPredictorOffset;
    NumericArrayReferenceStagingStoreFactory createStagingStore;
    bool useMemoryStaging{false};
    bool useMemoryTransferCache{false};
};

struct StagedNumericArrayReferenceBytes {
    std::shared_ptr<bytestore::IByteStore> store;
    std::span<const std::uint8_t> bytes;
};

struct OrdinaryNumericArrayEncodedBlock {
    NumericArrayBlockHeader header;
    NumericArrayBlockLayoutParams layout;
    std::vector<std::uint8_t> bytes;
};

inline constexpr std::size_t kReferenceProbeElementCount = 4096u;

inline std::uint64_t EstimateNumericArrayBlockStoredBytes(
    const NumericArrayBlockLayoutParams& layout) noexcept {
    auto bytes = static_cast<std::uint64_t>(layout.encodedByteLength);
    bytes = validation::SaturatingAddU64(
        bytes,
        validation::SaturatingMulU64(
            static_cast<std::uint64_t>(layout.alpha.size() + layout.beta.size()),
            sizeof(double)));
    bytes = validation::SaturatingAddU64(
        bytes,
        validation::SaturatingMulU64(
            static_cast<std::uint64_t>(layout.componentLayouts.size()),
            sizeof(NumericArrayComponentLayoutParams)));
    for (const auto& option : layout.backgroundCompressor.options) {
        bytes = validation::SaturatingAddU64(
            bytes,
            static_cast<std::uint64_t>(option.first.size() + sizeof(double)));
    }
    return bytes;
}

inline bool BuildUniformNumericArrayTupleSample(
    const std::span<const std::uint8_t> sourceBytes,
    const std::size_t elementCount,
    const std::size_t tupleBytes,
    const std::size_t sampleElementCount,
    ScratchByteBufferPool& scratchBytePool,
    const ScratchByteQuotaAcquire& acquireScratchQuota,
    ScratchByteBuffer& sample,
    std::string* error = nullptr) {
    sample.Release();
    std::size_t expectedSourceBytes = 0u;
    std::size_t sampleByteCount = 0u;
    if (sampleElementCount == 0u || sampleElementCount > elementCount || tupleBytes == 0u ||
        !validation::CheckedMulSizeT(
            elementCount,
            tupleBytes,
            expectedSourceBytes,
            "reference probe source bytes",
            error) ||
        !validation::CheckedMulSizeT(
            sampleElementCount,
            tupleBytes,
            sampleByteCount,
            "reference probe sample bytes",
            error)) {
        if (error != nullptr && error->empty()) {
            validation::AssignError(error, "reference probe shape is invalid");
        }
        return false;
    }
    if (sourceBytes.size() != expectedSourceBytes) {
        return validation::AssignError(error, "reference probe source byte size is invalid");
    }
    const auto requestedBytes = static_cast<std::uint64_t>(sampleByteCount);
    sample = scratchBytePool.Acquire(
        sampleByteCount,
        acquireScratchQuota ? acquireScratchQuota(requestedBytes) : ScratchByteQuotaLease{});
    auto& sampleBytes = sample.Bytes();
    for (std::size_t sampleIndex = 0u; sampleIndex < sampleElementCount; ++sampleIndex) {
        const auto sourceIndex = sampleElementCount == 1u
            ? std::size_t{0u}
            : static_cast<std::size_t>(
                (static_cast<std::uint64_t>(sampleIndex) *
                    static_cast<std::uint64_t>(elementCount - 1u)) /
                static_cast<std::uint64_t>(sampleElementCount - 1u));
        std::memcpy(
            sampleBytes.data() + sampleIndex * tupleBytes,
            sourceBytes.data() + sourceIndex * tupleBytes,
            tupleBytes);
    }
    return true;
}

inline bool BuildOrdinaryNumericArrayEncodedBlock(
    const NumericArrayStorageParams& meta,
    const CompressorConfig& defaultCompressor,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const std::span<const std::uint8_t> currentBytes,
    ScratchByteBufferPool& scratchBytePool,
    OrdinaryNumericArrayEncodedBlock& block,
    std::string* error = nullptr) {
    block = {};
    numericarray::NumericArrayBlockParams params;
    if (!numericarray::MakeNumericArrayBlockParamsFromMeta(meta, params, error)) {
        return false;
    }
    NumericArrayBytesCodec bytesCodec{NumericArrayBytesCodec::NumericArrayCodec};
    std::vector<NumericArrayComponentLayoutParams> componentLayouts;
    if (!numericarray::ResolveEncodedNumericArrayBlockBytes(
            params,
            defaultCompressor,
            elementCount,
            currentBytes,
            block.bytes,
            bytesCodec,
            error,
            &scratchBytePool,
            &componentLayouts)) {
        return false;
    }
    if (block.bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return validation::AssignError(error, "ordinary numeric array block exceeds current block format");
    }
    block.header = NumericArrayBlockHeader{
        .mode = NumericArrayBlockMode::NonReference,
        .referenceKind = NumericArrayReferenceKind::None,
        .codecId = NumericArrayReferenceCodecId::NonReference,
        .localParentFieldIndex = 0xFFFFu,
        .elementOffset = elementOffset,
        .elementCount = elementCount,
        .encodedByteLength = static_cast<std::uint32_t>(block.bytes.size()),
        .bytesCodec = bytesCodec,
    };
    block.layout = MakeNumericArrayBlockLayoutParams(block.header, {}, {});
    block.layout.backgroundCompressor = defaultCompressor;
    block.layout.componentLayouts = std::move(componentLayouts);
    return true;
}

inline bool ValidateIntegerReferenceCodecAndLayout(
    const NumericArrayStorageParams& meta,
    const NumericArrayStorageParams& referenceMeta,
    const NumericArrayReferenceCodecId codecId,
    std::string* error = nullptr) {
    if (!numericarray::IsIntegerNumericArrayDataType(meta.dataType)) {
        return true;
    }
    if (codecId != NumericArrayReferenceCodecId::Wavelet) {
        return validation::AssignError(error, "integer numeric array reference requires wavelet codec");
    }
    if (referenceMeta.dataType != meta.dataType ||
        referenceMeta.valueSize != meta.valueSize ||
        referenceMeta.dimension != meta.dimension ||
        referenceMeta.elementCount != meta.elementCount) {
        return validation::AssignError(
            error,
            "integer wavelet reference requires matching data type, value size, dimension, and element count");
    }
    return true;
}

inline bool PrepareStagedNumericArrayBytes(
    std::shared_ptr<bytestore::IByteStore> store,
    const std::uint64_t expectedBytes,
    StagedNumericArrayReferenceBytes& staged,
    std::string* error = nullptr) {
    staged = {};
    if (store == nullptr) {
        return validation::AssignError(error, "numeric array reference staging store is null");
    }
    if (store->ByteSizeHint() != expectedBytes) {
        return validation::AssignError(error, "numeric array reference staging byte size mismatch");
    }
    std::span<const std::uint8_t> bytes;
    const auto contiguousStatus = store->PrepareContiguousBytes(bytes, error);
    if (contiguousStatus == ContiguousViewStatus::Error) {
        return false;
    }
    if (contiguousStatus == ContiguousViewStatus::Unavailable) {
        return validation::AssignError(
            error,
            "numeric array reference staging store does not provide contiguous bytes");
    }
    if (bytes.size() != expectedBytes) {
        return validation::AssignError(error, "numeric array reference contiguous staging byte size mismatch");
    }
    staged.store = std::move(store);
    staged.bytes = bytes;
    return true;
}

inline bool StageNumericArrayReaderRange(
    const numericarray::NumericArrayReader& reader,
    const std::uint64_t elementOffset,
    const std::uint64_t elementCount,
    ScratchByteBufferPool& scratchBytePool,
    const ScratchByteQuotaAcquire& acquireScratchQuota,
    bytestore::ByteStoreSession& byteStoreSession,
    const NumericArrayReferenceStagingStoreFactory& createStagingStore,
    const bool useMemoryStaging,
    const std::string& label,
    const std::size_t accessWindowBytes,
    StagedNumericArrayReferenceBytes& staged,
    std::string* error = nullptr) {
    staged = {};
    const auto tupleBytes = reader.ElementBytes();
    if (tupleBytes == 0u) {
        return validation::AssignError(error, "numeric array reference staging requires a non-empty tuple layout");
    }
    if (!validation::CanMulU64(elementCount, tupleBytes)) {
        validation::AssignError(error, "numeric array reference staging byte size exceeds addressable range");
        return false;
    }
    const auto expectedBytes = elementCount * static_cast<std::uint64_t>(tupleBytes);
    auto store = createStagingStore
        ? createStagingStore(label, expectedBytes, error)
        : bytestore::CreateByteStore(byteStoreSession, label, useMemoryStaging, error);
    if (store == nullptr) {
        if (error != nullptr && error->empty()) {
            validation::AssignError(error, "failed to create numeric array reference staging store");
        }
        return false;
    }
    const auto resolvedWindowBytes = std::max<std::size_t>(accessWindowBytes, tupleBytes);
    const auto elementsPerWindow = std::max<std::uint64_t>(
        1u,
        static_cast<std::uint64_t>(resolvedWindowBytes / tupleBytes));
    std::uint64_t localOffset = 0u;
    while (localOffset < elementCount) {
        const auto localCount = std::min<std::uint64_t>(
            elementsPerWindow,
            elementCount - localOffset);
        ScratchByteBuffer windowBytes;
        if (!reader.ReadElements(
                elementOffset + localOffset,
                localCount,
                scratchBytePool,
                acquireScratchQuota,
                windowBytes,
                error)) {
            return false;
        }
        if (!store->Append(windowBytes.Span(), error)) {
            return false;
        }
        localOffset += localCount;
    }
    return PrepareStagedNumericArrayBytes(std::move(store), expectedBytes, staged, error);
}

inline bool StageNumericArrayPredictorReferenceBlock(
    const numericarray::NumericArrayReader& referenceReader,
    const NumericArrayStorageParams& referenceMeta,
    const NumericArrayStorageParams& targetMeta,
    const std::uint64_t elementOffset,
    const std::uint64_t elementCount,
    const std::int32_t predictorOffset,
    ScratchByteBufferPool& scratchBytePool,
    const ScratchByteQuotaAcquire& acquireScratchQuota,
    bytestore::ByteStoreSession& byteStoreSession,
    const NumericArrayReferenceStagingStoreFactory& createStagingStore,
    const bool useMemoryStaging,
    const std::string& label,
    const std::size_t accessWindowBytes,
    StagedNumericArrayReferenceBytes& staged,
    std::string* error = nullptr) {
    staged = {};
    const auto tupleBytes = static_cast<std::size_t>(std::max(targetMeta.dimension, 0)) * targetMeta.valueSize;
    if (tupleBytes == 0u) {
        return validation::AssignError(error, "numeric array predictor staging requires a non-empty tuple layout");
    }
    if (!validation::CanMulU64(elementCount, tupleBytes)) {
        validation::AssignError(error, "numeric array predictor staging byte size exceeds addressable range");
        return false;
    }
    const auto expectedBytes = elementCount * static_cast<std::uint64_t>(tupleBytes);
    auto store = createStagingStore
        ? createStagingStore(label, expectedBytes, error)
        : bytestore::CreateByteStore(byteStoreSession, label, useMemoryStaging, error);
    if (store == nullptr) {
        if (error != nullptr && error->empty()) {
            validation::AssignError(error, "failed to create numeric array predictor staging store");
        }
        return false;
    }
    const auto resolvedWindowBytes = std::max<std::size_t>(accessWindowBytes, tupleBytes);
    const auto elementsPerWindow = std::max<std::uint64_t>(
        1u,
        static_cast<std::uint64_t>(resolvedWindowBytes / tupleBytes));
    std::uint64_t localOffset = 0u;
    while (localOffset < elementCount) {
        const auto localCount = std::min<std::uint64_t>(
            elementsPerWindow,
            elementCount - localOffset);
        ScratchByteBuffer windowBytes;
        if (!BuildNumericArrayPredictorReferenceBlockBytes(
                referenceReader,
                referenceMeta,
                targetMeta,
                scratchBytePool,
                acquireScratchQuota,
                static_cast<std::size_t>(elementOffset + localOffset),
                static_cast<std::size_t>(localCount),
                predictorOffset,
                windowBytes,
                error)) {
            return false;
        }
        if (!store->Append(windowBytes.Span(), error)) {
            return false;
        }
        localOffset += localCount;
    }
    return PrepareStagedNumericArrayBytes(std::move(store), expectedBytes, staged, error);
}

inline bool BuildNumericArrayReferenceTransferCache(
    const NumericArrayStorageParams& meta,
    const CompressorConfig& defaultCompressor,
    const numericarray::NumericArraySource& currentSource,
    const NumericArrayReferenceSourceData& referenceData,
    const NumericArrayReferenceCodecId codecId,
    const NumericArrayReferenceTransferControl& control,
    ScratchByteBufferPool& scratchBytePool,
    window::WindowBudget& windowBudget,
    const std::size_t windowBytes,
    std::shared_ptr<bytestore::IByteSource>& transferCache,
    bytestore::ByteStoreSession& byteStoreSession,
    std::vector<NumericArrayBlockLayoutParams>* blockLayouts = nullptr,
    std::string* error = nullptr,
    const std::string& storeLabel = "numeric_array_reference_transfer") {
    transferCache.reset();
    if (blockLayouts != nullptr) {
        blockLayouts->clear();
    }
    numericarray::NumericArrayReader currentReader;
    numericarray::NumericArrayReader referenceReader;
    if (!numericarray::BuildNumericArrayReader(currentSource, currentReader, error) ||
        !numericarray::BuildNumericArrayReader(referenceData.source, referenceReader, error)) {
        return false;
    }
    if (!ValidateIntegerReferenceCodecAndLayout(meta, referenceData.meta, codecId, error)) {
        return false;
    }
    if (referenceData.meta.elementCount != meta.elementCount) {
        return validation::AssignError(
            error,
            "reference numeric array does not share the current spatial block domain");
    }

    auto bodyTransferCache = bytestore::CreateAppendableByteStore(
        byteStoreSession,
        storeLabel,
        control.useMemoryTransferCache,
        error);
    if (bodyTransferCache == nullptr) {
        if (error != nullptr && error->empty()) {
            validation::AssignError(error, "failed to create numeric array reference transfer cache");
        }
        return false;
    }

    if (currentReader.source.layout.elementCount != 0u) {
        const auto maxBlockElementCount =
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
        if (currentReader.source.layout.elementCount > maxBlockElementCount) {
            return validation::AssignError(error, "numeric array element count exceeds uint32 field capacity");
        }
        std::size_t localMetaElementCount = 0u;
        std::size_t localMetaValueSize = 0u;
        if (!TryParamSizeToSizeT(meta.elementCount, localMetaElementCount) ||
            !TryParamSizeToSizeT(meta.valueSize, localMetaValueSize)) {
            return validation::AssignError(error, "current numeric array metadata exceeds this platform size limit");
        }
        if (currentReader.source.layout.elementCount != localMetaElementCount) {
            return validation::AssignError(error, "current numeric array element count does not match metadata");
        }
        const auto tupleBytes = currentReader.ElementBytes();
        const auto metaTupleBytes = static_cast<std::size_t>(std::max(meta.dimension, 0)) * localMetaValueSize;
        if (tupleBytes == 0u || tupleBytes != metaTupleBytes) {
            return validation::AssignError(error, "current numeric array tuple size does not match metadata");
        }

        const auto* codec = ResolveNumericArrayReferenceCodec(codecId);
        if (codec == nullptr) {
            return validation::AssignError(error, "unsupported numeric array reference codec id");
        }
        const auto referenceKind = ToNumericArrayReferenceKind(referenceData.candidate.scope);
        const auto localParentFieldIndex = referenceData.candidate.localParentFieldIndex;
        bytestore::AppendableByteStoreWriter transferWriter(bodyTransferCache);

        const auto elementCount = currentReader.source.layout.elementCount;
        std::vector<numericarray::SpatialBlockRange> spatialBlocks;
        if (!numericarray::BuildSpatialBlockLayout(
                elementCount,
                control.spatialBlockElementCount,
                spatialBlocks,
                error)) {
            return false;
        }
        if (!referenceData.meta.blockLayouts.empty()) {
            if (referenceData.meta.blockLayouts.size() != spatialBlocks.size()) {
                return validation::AssignError(
                    error,
                    "reference numeric array spatial block count does not match the current layout");
            }
            for (std::size_t blockIndex = 0u; blockIndex < spatialBlocks.size(); ++blockIndex) {
                const auto& expected = spatialBlocks[blockIndex];
                const auto& referenceLayout = referenceData.meta.blockLayouts[blockIndex];
                if (referenceLayout.elementOffset != expected.elementOffset ||
                    referenceLayout.elementCount != expected.elementCount) {
                    return validation::AssignError(
                        error,
                        "reference numeric array spatial block range does not match the current layout");
                }
            }
        }
        for (const auto& spatialBlock : spatialBlocks) {
            const auto elementOffset = static_cast<std::size_t>(spatialBlock.elementOffset);
            const auto localElementCount = static_cast<std::size_t>(spatialBlock.elementCount);
            const auto blockElementOffset = spatialBlock.elementOffset;
            const auto blockElementCount = spatialBlock.elementCount;
            std::size_t currentBlockLocalBytes = 0u;
            if (!validation::CheckedMulSizeT(
                    localElementCount,
                    tupleBytes,
                    currentBlockLocalBytes,
                    "numeric array reference spatial block",
                    error)) {
                return false;
            }
            auto windowLease = windowBudget.Acquire(
                validation::SaturatingMulU64(
                    static_cast<std::uint64_t>(currentBlockLocalBytes),
                    2u));
            (void)windowLease;

            StagedNumericArrayReferenceBytes currentStaged;
            if (!StageNumericArrayReaderRange(
                    currentReader,
                    elementOffset,
                    localElementCount,
                    scratchBytePool,
                    control.acquireScratchQuota,
                    byteStoreSession,
                    control.createStagingStore,
                    control.useMemoryStaging,
                    storeLabel + "_current_" + std::to_string(elementOffset),
                    windowBytes,
                    currentStaged,
                    error) ||
                !ValidateNumericArrayRawByteSpan(
                    meta,
                    currentStaged.bytes,
                    localElementCount,
                    "current numeric array spatial block",
                    error)) {
                return false;
            }

            std::string referenceError;
            std::int32_t predictorOffset = 0;
            StagedNumericArrayReferenceBytes referenceStaged;
            const auto referenceLabel = storeLabel + "_reference_" + std::to_string(elementOffset);
            if (codecId == NumericArrayReferenceCodecId::Predictor) {
                if (control.selectPredictorOffset &&
                    !control.selectPredictorOffset(
                        meta,
                        currentStaged.bytes,
                        referenceReader,
                        referenceData.meta,
                        scratchBytePool,
                        blockElementOffset,
                        blockElementCount,
                        referenceKind,
                        localParentFieldIndex,
                        predictorOffset,
                        &referenceError)) {
                    return validation::AssignError(
                        error,
                        "reference predictor selection failed: " + referenceError);
                }
                if (!StageNumericArrayPredictorReferenceBlock(
                        referenceReader,
                        referenceData.meta,
                        meta,
                        blockElementOffset,
                        blockElementCount,
                        predictorOffset,
                        scratchBytePool,
                        control.acquireScratchQuota,
                        byteStoreSession,
                        control.createStagingStore,
                        control.useMemoryStaging,
                        referenceLabel,
                        windowBytes,
                        referenceStaged,
                        &referenceError)) {
                    return validation::AssignError(
                        error,
                        "reference predictor staging failed: " + referenceError);
                }
            } else if (!StageNumericArrayReaderRange(
                    referenceReader,
                    elementOffset,
                    localElementCount,
                    scratchBytePool,
                    control.acquireScratchQuota,
                    byteStoreSession,
                    control.createStagingStore,
                    control.useMemoryStaging,
                    referenceLabel,
                    windowBytes,
                    referenceStaged,
                    &referenceError)) {
                return validation::AssignError(
                    error,
                    "reference spatial block staging failed: " + referenceError);
            }
            if (!ValidateNumericArrayRawByteSpan(
                    meta,
                    referenceStaged.bytes,
                    localElementCount,
                    "reference numeric array spatial block",
                    &referenceError)) {
                return validation::AssignError(
                    error,
                    "reference spatial block validation failed: " + referenceError);
            }

            const NumericArrayReferenceCodecEncodeInput fullReferenceInput{
                .meta = meta,
                .defaultCompressor = defaultCompressor,
                .scratchBytePool = scratchBytePool,
                .acquireScratchQuota = control.acquireScratchQuota,
                .control = NumericArrayReferenceCodecControl{
                    .affineBlockRSquared = control.affineBlockRSquared,
                },
                .currentBytes = currentStaged.bytes,
                .referenceBytes = referenceStaged.bytes,
                .elementOffset = blockElementOffset,
                .elementCount = blockElementCount,
                .componentCount = static_cast<std::size_t>(std::max(meta.dimension, 0)),
                .referenceKind = referenceKind,
                .localParentFieldIndex = localParentFieldIndex,
                .predictorOffset = predictorOffset,
            };

            NumericArrayReferencePreparedBlock preparedReference;
            const auto prepareResult = codec->PrepareBlock(
                fullReferenceInput,
                preparedReference,
                &referenceError);
            if (prepareResult.IsFailed()) {
                return validation::AssignError(
                    error,
                    "reference spatial block preparation failed: " + referenceError);
            }

            const auto writeOrdinaryBlock = [&](OrdinaryNumericArrayEncodedBlock& block) {
                if (!WriteNumericArrayBlock(
                        transferWriter,
                        block.header,
                        {},
                        {},
                        std::span<const std::uint8_t>(block.bytes.data(), block.bytes.size()),
                        error)) {
                    return false;
                }
                if (blockLayouts != nullptr) {
                    blockLayouts->push_back(std::move(block.layout));
                }
                return true;
            };
            const auto writeReferenceBlock = [&](
                NumericArrayReferenceEncodedBlock& block,
                NumericArrayBlockLayoutParams& layout) {
                if (!WriteNumericArrayReferenceEncodedBlock(transferWriter, block, error)) {
                    return false;
                }
                if (blockLayouts != nullptr) {
                    blockLayouts->push_back(std::move(layout));
                }
                return true;
            };
            const auto buildFullOrdinary = [&](OrdinaryNumericArrayEncodedBlock& block) {
                return BuildOrdinaryNumericArrayEncodedBlock(
                    meta,
                    defaultCompressor,
                    blockElementOffset,
                    blockElementCount,
                    currentStaged.bytes,
                    scratchBytePool,
                    block,
                    error);
            };
            const auto buildFullReference = [&](NumericArrayReferenceEncodedBlock& block) {
                referenceError.clear();
                const auto result = codec->EncodePreparedBlock(
                    fullReferenceInput,
                    preparedReference,
                    block,
                    &referenceError);
                if (result.IsFailed()) {
                    return validation::AssignError(
                        error,
                        "reference spatial block encoding failed: " + referenceError);
                }
                if (result.IsRejected()) {
                    return validation::AssignError(
                        error,
                        "prepared reference spatial block was unexpectedly rejected");
                }
                return true;
            };

            if (prepareResult.IsRejected()) {
                if (control.selectionMode == ReferenceSelectionMode::Forced) {
                    return validation::AssignError(
                        error,
                        std::string("forced reference spatial block was rejected: ") +
                            NumericArrayReferenceRejectReasonName(prepareResult.rejectReason));
                }
                OrdinaryNumericArrayEncodedBlock ordinaryBlock;
                if (!buildFullOrdinary(ordinaryBlock) || !writeOrdinaryBlock(ordinaryBlock)) {
                    return false;
                }
                continue;
            }

            if (control.selectionMode == ReferenceSelectionMode::Forced) {
                NumericArrayReferenceEncodedBlock referenceBlock;
                if (!buildFullReference(referenceBlock)) {
                    return false;
                }
                auto referenceLayout = MakeNumericArrayReferenceBlockLayout(referenceBlock);
                if (!writeReferenceBlock(referenceBlock, referenceLayout)) {
                    return false;
                }
                continue;
            }

            bool useReference = false;
            const bool useBoundedProbe =
                control.autoSelectionStrategy == ReferenceAutoSelectionStrategy::BoundedProbe &&
                referenceKind == NumericArrayReferenceKind::IntraArray &&
                localElementCount > kReferenceProbeElementCount;

            OrdinaryNumericArrayEncodedBlock ordinaryBlock;
            NumericArrayReferenceEncodedBlock referenceBlock;
            NumericArrayBlockLayoutParams referenceLayout;
            if (!useBoundedProbe) {
                if (!buildFullOrdinary(ordinaryBlock) || !buildFullReference(referenceBlock)) {
                    return false;
                }
                referenceLayout = MakeNumericArrayReferenceBlockLayout(referenceBlock);
                useReference = EstimateNumericArrayBlockStoredBytes(referenceLayout) <
                    EstimateNumericArrayBlockStoredBytes(ordinaryBlock.layout);
            } else {
                ScratchByteBuffer currentSample;
                ScratchByteBuffer referenceSample;
                if (!BuildUniformNumericArrayTupleSample(
                        currentStaged.bytes,
                        localElementCount,
                        tupleBytes,
                        kReferenceProbeElementCount,
                        scratchBytePool,
                        control.acquireScratchQuota,
                        currentSample,
                        error) ||
                    !BuildUniformNumericArrayTupleSample(
                        referenceStaged.bytes,
                        localElementCount,
                        tupleBytes,
                        kReferenceProbeElementCount,
                        scratchBytePool,
                        control.acquireScratchQuota,
                        referenceSample,
                        error)) {
                    return false;
                }

                OrdinaryNumericArrayEncodedBlock ordinaryProbe;
                if (!BuildOrdinaryNumericArrayEncodedBlock(
                        meta,
                        defaultCompressor,
                        blockElementOffset,
                        static_cast<std::uint32_t>(kReferenceProbeElementCount),
                        currentSample.Span(),
                        scratchBytePool,
                        ordinaryProbe,
                        error)) {
                    return false;
                }
                const NumericArrayReferenceCodecEncodeInput probeInput{
                    .meta = meta,
                    .defaultCompressor = defaultCompressor,
                    .scratchBytePool = scratchBytePool,
                    .acquireScratchQuota = control.acquireScratchQuota,
                    .control = NumericArrayReferenceCodecControl{
                        .affineBlockRSquared = control.affineBlockRSquared,
                    },
                    .currentBytes = currentSample.Span(),
                    .referenceBytes = referenceSample.Span(),
                    .elementOffset = blockElementOffset,
                    .elementCount = static_cast<std::uint32_t>(kReferenceProbeElementCount),
                    .componentCount = static_cast<std::size_t>(std::max(meta.dimension, 0)),
                    .referenceKind = referenceKind,
                    .localParentFieldIndex = localParentFieldIndex,
                    .predictorOffset = predictorOffset,
                };
                NumericArrayReferencePreparedBlock probePreparedReference;
                referenceError.clear();
                const auto probePrepareResult = codec->PrepareBlock(
                    probeInput,
                    probePreparedReference,
                    &referenceError);
                if (probePrepareResult.IsFailed()) {
                    return validation::AssignError(
                        error,
                        "reference probe preparation failed: " + referenceError);
                }
                NumericArrayReferenceEncodedBlock referenceProbe;
                if (probePrepareResult.IsEncoded()) {
                    referenceError.clear();
                    const auto probeResult = codec->EncodePreparedBlock(
                        probeInput,
                        probePreparedReference,
                        referenceProbe,
                        &referenceError);
                    if (probeResult.IsFailed()) {
                        return validation::AssignError(
                            error,
                            "reference probe encoding failed: " + referenceError);
                    }
                    if (probeResult.IsRejected()) {
                        return validation::AssignError(
                            error,
                            "prepared reference probe was unexpectedly rejected");
                    }
                    const auto probeLayout = MakeNumericArrayReferenceBlockLayout(referenceProbe);
                    useReference = EstimateNumericArrayBlockStoredBytes(probeLayout) <
                        EstimateNumericArrayBlockStoredBytes(ordinaryProbe.layout);
                }

                if (useReference) {
                    if (!buildFullReference(referenceBlock)) {
                        return false;
                    }
                    referenceLayout = MakeNumericArrayReferenceBlockLayout(referenceBlock);
                } else if (!buildFullOrdinary(ordinaryBlock)) {
                    return false;
                }
            }

            if (useReference) {
                if (!writeReferenceBlock(referenceBlock, referenceLayout)) {
                    return false;
                }
            } else if (!writeOrdinaryBlock(ordinaryBlock)) {
                return false;
            }
        }
    }

    if (!bodyTransferCache->Seal(error)) {
        return false;
    }
    transferCache = std::move(bodyTransferCache);
    return true;
}

} // namespace numericarrayreference
} // namespace datacodec

#endif
