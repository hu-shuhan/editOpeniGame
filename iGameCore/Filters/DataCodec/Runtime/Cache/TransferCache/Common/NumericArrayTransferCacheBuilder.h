#ifndef DATACODEC_RUNTIME_CACHE_TRANSFERCACHE_COMMON_NUMERICARRAYTRANSFERCACHEBUILDER_H
#define DATACODEC_RUNTIME_CACHE_TRANSFERCACHE_COMMON_NUMERICARRAYTRANSFERCACHEBUILDER_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockEncode.h"
#include "DataCodec/Codec/NumericArray/NumericArrayReader.h"
#include "DataCodec/Codec/NumericArray/SpatialBlockLayout.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {
namespace encodeimpl {

struct NumericArrayTransferCacheStats {
    std::size_t encodeBlockCount{0};
    std::uint64_t logicalRawBytes{0};
    std::uint64_t maxEncodeBlockRawBytes{0};
    std::uint64_t maxEncodeBlockResidentBytes{0};
    std::uint64_t maxComponentBufferBytes{0};
    std::uint64_t maxEncodedBlockBytes{0};
    std::uint64_t maxBlockResidentBytes{0};
    std::uint64_t fragmentBytes{0};
};

struct NumericArrayTransferCacheResult {
    std::shared_ptr<bytestore::IByteSource> transferCache;
    NumericArrayTransferCacheStats stats;
    std::vector<NumericArrayBlockLayoutParams> blockLayouts;
};

struct NumericArrayTransferCacheRuntime {
    ScratchByteQuotaAcquire acquireScratchQuota;
    std::function<std::shared_ptr<void>()> acquireFloatingPointEncodeLane;
    std::function<void(std::chrono::nanoseconds)> recordFloatingPointEncodeDuration;
    bool useMemoryTransferCache{false};
};

inline bool FinalizeNumericArrayTransferCacheResult(
    NumericArrayTransferCacheResult& result,
    std::string* error) {
    if (result.transferCache == nullptr) {
        return validation::AssignError(error, "numeric array transfer cache is null");
    }
    result.stats.fragmentBytes = result.transferCache->ByteSizeHint();
    return true;
}

inline bool BuildNumericArrayTransferCache(
    const numericarray::NumericArrayBlockParams& params,
    const numericarray::NumericArrayReader& reader,
    ScratchByteBufferPool& scratchBytePool,
    NumericArrayTransferCacheResult& result,
    bytestore::ByteStoreSession& byteStoreSession,
    std::string* error = nullptr,
    const std::string& storeLabel = "numeric_array_transfer",
    const std::size_t accessWindowBytes = 0u,
    const std::uint32_t codecBlockElementCount = 0u,
    const NumericArrayTransferCacheRuntime* runtime = nullptr) {
    result = {};

    if (!numericarray::ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }
    const auto* regionControl = params.regionControl;
    if (regionControl == nullptr) {
        return validation::AssignError(error, "numeric array transfer cache requires region precision control");
    }
    if (!numericarray::ValidateRegionControlForEncode(*regionControl, error)) {
        return false;
    }
    if (!regionControl->regions.empty()) {
        if (params.regionRuns == nullptr || params.regionRuns->empty()) {
            return validation::AssignError(error, "region residual encoding requires region runs");
        }
        if (!numericarray::ValidateRegionRunsForEncode(
                std::span<const RegionRun>(params.regionRuns->data(), params.regionRuns->size()),
                static_cast<ParamSize>(reader.source.layout.elementCount),
                regionControl->regions.size(),
                error)) {
            return false;
        }
    }

    const auto componentCount = params.componentCount;
    std::size_t checkedTupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            params.valueSize,
            checkedTupleBytes,
            "numeric array tuple bytes",
            error)) {
        return false;
    }

    result.stats.encodeBlockCount = 0u;
    result.stats.logicalRawBytes = validation::SaturatingMulU64(
        static_cast<std::uint64_t>(reader.source.layout.elementCount),
        static_cast<std::uint64_t>(checkedTupleBytes));
    auto bodyTransferCache = bytestore::CreateAppendableByteStore(
        byteStoreSession,
        storeLabel,
        runtime != nullptr && runtime->useMemoryTransferCache,
        error);
    if (bodyTransferCache == nullptr) {
        if (error != nullptr && error->empty()) {
            validation::AssignError(error, "failed to create numeric array transfer cache");
        }
        return false;
    }
    if (reader.source.layout.elementCount != 0u) {
        const auto maxBlockElementCount =
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
        if (reader.source.layout.elementCount > maxBlockElementCount) {
            return validation::AssignError(error, "numeric array element count exceeds uint32 field capacity");
        }
        bytestore::AppendableByteStoreWriter transferWriter(bodyTransferCache);
        const auto tupleBytes = checkedTupleBytes;
        if (tupleBytes == 0u) {
            return validation::AssignError(error, "numeric array transfer cache requires a non-empty tuple layout");
        }
        (void)accessWindowBytes;
        const auto elementsPerBlock = codecBlockElementCount == 0u
            ? reader.source.layout.elementCount
            : static_cast<std::size_t>(codecBlockElementCount);
        std::vector<numericarray::SpatialBlockRange> spatialBlocks;
        if (!numericarray::BuildSpatialBlockLayout(
                reader.source.layout.elementCount,
                static_cast<std::uint32_t>(elementsPerBlock),
                spatialBlocks,
                error)) {
            return false;
        }
        for (const auto& spatialBlock : spatialBlocks) {
            const auto elementOffset = static_cast<std::size_t>(spatialBlock.elementOffset);
            const auto localElementCount = static_cast<std::size_t>(spatialBlock.elementCount);
            const auto blockElementOffset = spatialBlock.elementOffset;
            const auto blockElementCount = spatialBlock.elementCount;

            ScratchByteBuffer rawBlockBytes;
            if (!reader.ReadElements(
                    elementOffset,
                    localElementCount,
                    scratchBytePool,
                    runtime != nullptr ? runtime->acquireScratchQuota : ScratchByteQuotaAcquire{},
                    rawBlockBytes,
                    error)) {
                return false;
            }

            auto componentBundleScratchBuffer = scratchBytePool.Acquire(0u);
            auto& componentBundleBytes = componentBundleScratchBuffer.Bytes();
            NumericArrayBytesCodec blockBytesCodec{NumericArrayBytesCodec::NumericArrayCodec};
            NumericArrayBlockLayoutParams layout;
            NumericArrayBlockHeader header;
            std::shared_ptr<void> encodeLane;
            const bool recordFloatingPointEncode =
                runtime != nullptr &&
                !numericarray::IsIntegerNumericArrayDataType(params.dataType);
            if (recordFloatingPointEncode && runtime->acquireFloatingPointEncodeLane) {
                encodeLane = runtime->acquireFloatingPointEncodeLane();
            }
            const auto encodeStartTime = callback::StartTiming(
                recordFloatingPointEncode && runtime->recordFloatingPointEncodeDuration);
            bool encodedBlockOk = false;
            if (regionControl->regions.empty()) {
                const auto& defaultCompressor = regionControl->defaultPrecision.compressor;
                std::vector<NumericArrayComponentLayoutParams> componentLayouts;
                encodedBlockOk = numericarray::ResolveEncodedNumericArrayBlockBytes(
                        params,
                        defaultCompressor,
                        blockElementCount,
                        rawBlockBytes.Span(),
                        componentBundleBytes,
                        blockBytesCodec,
                        error,
                        &scratchBytePool,
                        &componentLayouts);
                if (!encodedBlockOk) {
                    return false;
                }
                if (componentBundleBytes.size() >
                    static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
                    return validation::AssignError(error, "numeric array block bundle exceeds current block format");
                }
                header = NumericArrayBlockHeader{
                    .mode = NumericArrayBlockMode::NonReference,
                    .referenceKind = NumericArrayReferenceKind::None,
                    .codecId = NumericArrayReferenceCodecId::NonReference,
                    .localParentFieldIndex = 0xFFFFu,
                    .elementOffset = blockElementOffset,
                    .elementCount = blockElementCount,
                    .encodedByteLength = static_cast<std::uint32_t>(componentBundleBytes.size()),
                    .bytesCodec = blockBytesCodec,
                };
                layout = MakeNumericArrayBlockLayoutParams(header, {}, {});
                layout.componentLayouts = std::move(componentLayouts);
                layout.backgroundCompressor = defaultCompressor;
            } else {
                encodedBlockOk = numericarray::ResolveEncodedLayeredResidualNumericArrayBlockBytes(
                        params,
                        *regionControl,
                        blockElementOffset,
                        blockElementCount,
                        rawBlockBytes.Span(),
                        std::span<const RegionRun>(params.regionRuns->data(), params.regionRuns->size()),
                        componentBundleBytes,
                        blockBytesCodec,
                        layout,
                        error,
                        &scratchBytePool);
                if (!encodedBlockOk) {
                    return false;
                }
                if (!MakeNumericArrayBlockHeader(layout, header, error)) {
                    return false;
                }
            }
            if (recordFloatingPointEncode && runtime->recordFloatingPointEncodeDuration) {
                runtime->recordFloatingPointEncodeDuration(callback::ElapsedNanoseconds(encodeStartTime));
            }
            result.blockLayouts.push_back(std::move(layout));
            if (!WriteNumericArrayBlock(
                    transferWriter,
                    header,
                    {},
                    {},
                    std::span<const std::uint8_t>(componentBundleBytes.data(), componentBundleBytes.size()),
                    error)) {
                return false;
            }

            const auto rawBlockResidentBytes = VectorCapacityBytes(rawBlockBytes.Bytes());
            const auto bundleResidentBytes = VectorCapacityBytes(componentBundleBytes);
            const auto componentRawBytes = static_cast<std::uint64_t>(blockElementCount) * params.valueSize;
            result.stats.maxEncodeBlockRawBytes = std::max<std::uint64_t>(
                result.stats.maxEncodeBlockRawBytes,
                rawBlockBytes.Bytes().size());
            result.stats.maxEncodeBlockResidentBytes = std::max<std::uint64_t>(
                result.stats.maxEncodeBlockResidentBytes,
                rawBlockResidentBytes);
            result.stats.maxComponentBufferBytes = std::max<std::uint64_t>(
                result.stats.maxComponentBufferBytes,
                componentRawBytes);
            result.stats.maxBlockResidentBytes = std::max<std::uint64_t>(
                result.stats.maxBlockResidentBytes,
                rawBlockResidentBytes + bundleResidentBytes);
            result.stats.maxEncodedBlockBytes = std::max<std::uint64_t>(
                result.stats.maxEncodedBlockBytes,
                componentBundleBytes.size());
            ++result.stats.encodeBlockCount;
        }
    }
    if (!bodyTransferCache->Seal(error)) {
        return false;
    }
    result.transferCache = std::move(bodyTransferCache);
    return FinalizeNumericArrayTransferCacheResult(result, error);
}

} // namespace encodeimpl
} // namespace datacodec

#endif
