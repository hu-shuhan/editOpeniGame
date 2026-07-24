#ifndef DATACODEC_CODEC_TOPOLOGY_TOPOLOGYDECODE_H
#define DATACODEC_CODEC_TOPOLOGY_TOPOLOGYDECODE_H

#include "DataCodec/API/Adapter/IDecodeTopologyBlockObserver.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedTopologyCache.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Codec/Topology/Connectivity/ConnectivityTopologyDecode.h"
#include "DataCodec/Codec/Topology/Connectivity/ConnectivityTopologyDecodeInputReader.h"
#include "DataCodec/Codec/Topology/Polyhedron/PolyhedronTopologyStreamDecode.h"
#include "DataCodec/Codec/Topology/TopologyDecodeCacheSink.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <condition_variable>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

using TopologyDecodeResult = CodecStatus;

struct TopologyDecodeTimingEvent {
    std::string name;
    double elapsedMs{0.0};
    std::string scope;
};

using TopologyDecodeTimingCallback = std::function<void(const TopologyDecodeTimingEvent&)>;

struct TopologyDecodeData {
    const CodecStorageParams& storageParams;
};

struct TopologyDecodeCache {
    const CacheResources& cacheResources;
    bytestore::ByteStoreSession& byteStoreSession;
    DecodedTopologyCache& topology;
    std::uint64_t topologyMemoryInputLimitBytes{0u};
    std::uint64_t topologyMemoryCacheLimitBytes{0u};
    DecodeStorageMode topologyInputStorageMode{DecodeStorageMode::Managed};
    DecodeStorageMode topologyCacheStorageMode{DecodeStorageMode::Managed};
};

struct TopologyDecodeContext {
    TopologyDecodeTimingCallback timingCallback;
    std::shared_ptr<IDecodeTopologyBlockObserver> topologyBlockObserver;
};

struct TopologyDecodeSchedule {
    IParallelTaskRunner* parallelTaskRunner{nullptr};
    std::size_t workerCount{1u};
};

struct TopologyDecodeRuntime {
    TopologyDecodeData data;
    TopologyDecodeCache cache;
    TopologyDecodeContext context;
    TopologyDecodeSchedule schedule;
};

namespace detail {

inline TopologyDecodeResult MakeTopologyDecodeSuccess() {
    return CodecStatus::Ok();
}

inline TopologyDecodeResult MakeTopologyDecodeFailure(
    const CodecErrorCode code,
    std::string message) {
    return CodecStatus::Failure(code, std::move(message));
}

inline TopologyDecodeResult ValidateTopologyConsumed(
    const std::uint64_t position,
    const std::uint64_t expected,
    std::string* error) {
    if (!validation::ValidateExactConsumed(position, expected, "topology field", error)) {
        return MakeTopologyDecodeFailure(
            CodecErrorCode::DecodeFailure,
            error != nullptr ? *error : "topology field bytes were not consumed exactly");
    }
    return MakeTopologyDecodeSuccess();
}

inline void RecordTopologyDecodeTiming(
    const TopologyDecodeTimingCallback& timingCallback,
    std::string name,
    const callback::PhaseTimePoint startTime,
    std::string scope = {}) {
    callback::InvokeTimingEvent(timingCallback, TopologyDecodeTimingEvent{
        std::move(name),
        callback::ElapsedMilliseconds(startTime),
        std::move(scope),
    });
}

inline void ForwardConnectivityTopologyDecodeTiming(
    const TopologyDecodeTimingCallback& timingCallback,
    const topocodec::ConnectivityTopologyDecodeTimingEvent& event) {
    if (!timingCallback) {
        return;
    }
    timingCallback(TopologyDecodeTimingEvent{
        "TopoDecodeCoreStage." + event.name,
        event.elapsedMs,
        event.scope,
    });
}

inline topocodec::ConnectivityTopologyEncodedMetadata MakeConnectivityTopologyEncodedMetadata(
    const TopologyConnectivityBlockLayoutParams& block) noexcept {
    topocodec::ConnectivityTopologyEncodedMetadata metadata;
    metadata.connectivityByteCount = block.connectivityByteCount;
    metadata.cellSizeByteCount = block.cellSizeByteCount;
    metadata.cellPolynomialOrderByteCount = block.cellPolynomialOrderByteCount;
    metadata.cellTypeByteCount = block.cellTypeByteCount;
    return metadata;
}

class TopologyBlockCacheSink final : public topocodec::IConnectivityTopologyDecodeSink {
public:
    TopologyBlockCacheSink(
        topology::CacheTopologyDecodeSink& output,
        std::mutex& outputWriteMutex,
        std::shared_ptr<IDecodeTopologyBlockObserver> observer,
        const std::size_t blockIndex,
        const std::size_t cellOffset,
        const std::size_t connectivityOffset,
        const std::size_t cellCount,
        const std::size_t connectivityCount,
        const int fixedCellSize,
        const bool hasOffsets,
        const bool hasCellTypes,
        const bool hasCellPolynomialOrders)
        : m_output(output),
          m_outputWriteMutex(outputWriteMutex),
          m_observer(std::move(observer)),
          m_cellOffset(cellOffset),
          m_connectivityOffset(connectivityOffset),
          m_cellCount(cellCount),
          m_connectivityCount(connectivityCount),
          m_hasOffsets(hasOffsets),
          m_hasCellTypes(hasCellTypes),
          m_hasCellPolynomialOrders(hasCellPolynomialOrders) {
        if (m_observer != nullptr) {
            m_observedBlock.blockIndex = blockIndex;
            m_observedBlock.cellOffset = cellOffset;
            m_observedBlock.fixedCellSize = fixedCellSize;
            m_observedBlock.connectivity.resize(connectivityCount);
            if (hasOffsets) { m_observedBlock.offsets.resize(cellCount + 1u); }
            if (hasCellTypes) { m_observedBlock.cellTypes.resize(cellCount); }
        }
    }

    bool BeginConnectivityTopology(
        const std::size_t cellCount,
        const std::size_t connectivityCount,
        const bool hasOffsets,
        const bool hasCellTypes,
        const bool hasCellPolynomialOrders,
        std::string* error) override {
        if (cellCount != m_cellCount ||
            connectivityCount != m_connectivityCount ||
            hasOffsets != m_hasOffsets ||
            hasCellTypes != m_hasCellTypes ||
            hasCellPolynomialOrders != m_hasCellPolynomialOrders) {
            return validation::AssignError(error, "topology block output layout does not match decoder layout");
        }
        return true;
    }

    bool WriteConnectivityRange(
        const std::size_t offset,
        const std::span<const IndexType> values,
        std::string* error) override {
        std::size_t outputOffset = 0u;
        if (!ValidateRange(offset, values.size(), m_connectivityCount, "topology block connectivity", error) ||
            !TryAddOffset(m_connectivityOffset, offset, outputOffset, "topology block connectivity", error)) {
            return false;
        }
        if (m_observer != nullptr) {
            std::copy(
                values.begin(),
                values.end(),
                m_observedBlock.connectivity.begin() + static_cast<std::ptrdiff_t>(offset));
        }
        std::lock_guard<std::mutex> lock(m_outputWriteMutex);
        return m_output.WriteConnectivityRange(outputOffset, values, error);
    }

    bool WriteOffsetsRange(
        const std::size_t offset,
        const std::span<const IndexType> values,
        std::string* error) override {
        if (!m_hasOffsets ||
            !ValidateRange(offset, values.size(), m_cellCount + 1u, "topology block offsets", error)) {
            return false;
        }
        if (m_observer != nullptr) {
            std::copy(
                values.begin(),
                values.end(),
                m_observedBlock.offsets.begin() + static_cast<std::ptrdiff_t>(offset));
        }
        auto outputValues = values;
        std::size_t outputLocalOffset = offset;
        if (m_cellOffset != 0u && outputLocalOffset == 0u && !outputValues.empty()) {
            outputValues = outputValues.subspan(1u);
            ++outputLocalOffset;
        }
        if (outputValues.empty()) {
            return true;
        }
        std::size_t outputOffset = 0u;
        if (!TryAddOffset(m_cellOffset, outputLocalOffset, outputOffset, "topology block offsets", error)) {
            return false;
        }
        if (m_connectivityOffset == 0u) {
            std::lock_guard<std::mutex> lock(m_outputWriteMutex);
            return m_output.WriteOffsetsRange(outputOffset, outputValues, error);
        }
        if (m_connectivityOffset > static_cast<std::size_t>(std::numeric_limits<IndexType>::max())) {
            return validation::AssignError(error, "topology block connectivity offset exceeds index capacity");
        }
        const auto connectivityBase = static_cast<IndexType>(m_connectivityOffset);
        m_offsetScratch.assign(outputValues.begin(), outputValues.end());
        for (auto& value : m_offsetScratch) {
            if (value > std::numeric_limits<IndexType>::max() - connectivityBase) {
                return validation::AssignError(error, "topology block offset exceeds index capacity");
            }
            value += connectivityBase;
        }
        std::lock_guard<std::mutex> lock(m_outputWriteMutex);
        return m_output.WriteOffsetsRange(outputOffset, m_offsetScratch, error);
    }

    bool WriteCellTypesRange(
        const std::size_t offset,
        const std::span<const IndexType> values,
        std::string* error) override {
        std::size_t outputOffset = 0u;
        if (!m_hasCellTypes ||
            !ValidateRange(offset, values.size(), m_cellCount, "topology block cell types", error) ||
            !TryAddOffset(m_cellOffset, offset, outputOffset, "topology block cell types", error)) {
            return false;
        }
        if (m_observer != nullptr) {
            std::copy(
                values.begin(),
                values.end(),
                m_observedBlock.cellTypes.begin() + static_cast<std::ptrdiff_t>(offset));
        }
        std::lock_guard<std::mutex> lock(m_outputWriteMutex);
        return m_output.WriteCellTypesRange(outputOffset, values, error);
    }

    bool WriteCellPolynomialOrdersRange(
        const std::size_t offset,
        const std::span<const std::uint16_t> values,
        std::string* error) override {
        std::size_t outputOffset = 0u;
        if (!m_hasCellPolynomialOrders ||
            !ValidateRange(offset, values.size(), m_cellCount, "topology block cell polynomial orders", error) ||
            !TryAddOffset(m_cellOffset, offset, outputOffset, "topology block cell polynomial orders", error)) {
            return false;
        }
        std::lock_guard<std::mutex> lock(m_outputWriteMutex);
        return m_output.WriteCellPolynomialOrdersRange(outputOffset, values, error);
    }

    bool EndConnectivityTopology(std::string* error) override {
        if (m_observer != nullptr) {
            return m_observer->ObserveConnectivityBlock(
                std::move(m_observedBlock),
                error);
        }
        return true;
    }

private:
    static bool ValidateRange(
        const std::size_t offset,
        const std::size_t valueCount,
        const std::size_t capacity,
        const char* label,
        std::string* error) {
        if (offset > capacity || valueCount > capacity - offset) {
            return validation::AssignError(error, std::string(label) + " range is invalid");
        }
        return true;
    }

    static bool TryAddOffset(
        const std::size_t base,
        const std::size_t localOffset,
        std::size_t& outputOffset,
        const char* label,
        std::string* error) {
        if (base > std::numeric_limits<std::size_t>::max() - localOffset) {
            return validation::AssignError(error, std::string(label) + " output offset exceeds local size capacity");
        }
        outputOffset = base + localOffset;
        return true;
    }

    topology::CacheTopologyDecodeSink& m_output;
    std::mutex& m_outputWriteMutex;
    std::shared_ptr<IDecodeTopologyBlockObserver> m_observer;
    DecodedConnectivityTopologyBlock m_observedBlock;
    std::size_t m_cellOffset{0u};
    std::size_t m_connectivityOffset{0u};
    std::size_t m_cellCount{0u};
    std::size_t m_connectivityCount{0u};
    bool m_hasOffsets{false};
    bool m_hasCellTypes{false};
    bool m_hasCellPolynomialOrders{false};
    std::vector<IndexType> m_offsetScratch;
};

template<typename TStream>
inline bool DecodeConnectivityTopologyBlocksToCache(
    const TopoStorageParams& topo,
    TopologyDecodeRuntime& decodeRuntime,
    TStream& stream,
    std::string* error = nullptr) {
    const auto& cacheResources = decodeRuntime.cache.cacheResources;
    auto& byteStoreSession = decodeRuntime.cache.byteStoreSession;
    auto& topologyCache = decodeRuntime.cache.topology;
    const auto& timingCallback = decodeRuntime.context.timingCallback;
    const auto topologyBlockObserver = decodeRuntime.context.topologyBlockObserver;
    const auto& blocks = topo.connectivityLayout.blockLayouts;
    if (blocks.empty()) {
        return validation::AssignError(error, "connectivity topology block layout is empty");
    }
    if (decodeRuntime.cache.topologyInputStorageMode == DecodeStorageMode::Memory &&
        (decodeRuntime.cache.topologyMemoryInputLimitBytes == 0u ||
         topo.binaryCount > decodeRuntime.cache.topologyMemoryInputLimitBytes)) {
        return validation::AssignError(error, "connectivity topology block input exceeds configured memory limit");
    }

    struct BlockDecodeState {
        std::mutex mutex;
        std::condition_variable available;
        std::size_t inFlight{0u};
        bool failed{false};
        std::string error;
    } blockState;

    double inputLoadMs = 0.0;
    bool allMemory = true;
    std::size_t cellCount = 0u;
    std::size_t connectivityCount = 0u;
    if (!TryParamSizeToSizeT(topo.cellCount, cellCount) ||
        !TryParamSizeToSizeT(topo.cellBufferSize, connectivityCount)) {
        return validation::AssignError(error, "topology counts exceed local size capacity");
    }
    topology::CacheTopologyDecodeSink outputSink(
        topologyCache,
        byteStoreSession,
        decodeRuntime.cache.topologyMemoryCacheLimitBytes,
        decodeRuntime.cache.topologyCacheStorageMode);
    const auto hasOffsets = topo.fixedCellSize <= 0;
    const auto hasOrders = topo.connectivityLayout.cellPolynomialOrderByteCount != 0u;
    if (!outputSink.BeginConnectivityTopology(
            cellCount,
            connectivityCount,
            hasOffsets,
            topo.hasCellTypes != 0u,
            hasOrders,
            error)) {
        return false;
    }
    std::size_t pointCount = 0u;
    if (!TryParamSizeToSizeT(
            decodeRuntime.data.storageParams.geomParams.elementCount,
            pointCount)) {
        return validation::AssignError(error, "topology point count exceeds local size capacity");
    }
    if (topologyBlockObserver != nullptr) {
        try {
            if (!topologyBlockObserver->BeginConnectivityTopology(
                    ConnectivityTopologyDecodeInfo{
                    .blockCount = blocks.size(),
                    .pointCount = pointCount,
                    .cellCount = cellCount,
                    .fixedCellSize = static_cast<int>(topo.fixedCellSize),
                    .hasOffsets = hasOffsets,
                    .hasCellTypes = topo.hasCellTypes != 0u,
                },
                    error)) {
                return false;
            }
        } catch (const std::exception& exception) {
            return validation::AssignError(
                error,
                std::string("topology observer initialization failed: ") + exception.what());
        } catch (...) {
            return validation::AssignError(error, "topology observer initialization failed");
        }
    }
    bool observerFinalized = false;
    std::string observerEndError;
    const auto finalizeObserver = [&]() noexcept {
        if (topologyBlockObserver == nullptr || observerFinalized) {
            return true;
        }
        observerFinalized = true;
        try {
            return topologyBlockObserver->EndConnectivityTopology(&observerEndError);
        } catch (const std::exception& exception) {
            observerEndError = std::string("topology observer finalization raised an exception: ") + exception.what();
        } catch (...) {
            observerEndError = "topology observer finalization raised an unknown exception";
        }
        return false;
    };

    const auto workerCount = ResolveNestedParallelTaskCount(
        blocks.size(),
        decodeRuntime.schedule.parallelTaskRunner,
        decodeRuntime.schedule.workerCount);
    const bool parallelBlocks = workerCount > 1u && decodeRuntime.schedule.parallelTaskRunner != nullptr;
    const std::size_t maxInFlightBlocks = std::max<std::size_t>(workerCount, 1u);
    std::mutex outputWriteMutex;
    std::mutex timingMutex;
    topocodec::ConnectivityTopologyDecodeTimingCallback connectivityTiming;
    if (timingCallback) {
        connectivityTiming = [&timingCallback, &timingMutex](const topocodec::ConnectivityTopologyDecodeTimingEvent& event) {
            std::lock_guard<std::mutex> lock(timingMutex);
            ForwardConnectivityTopologyDecodeTiming(timingCallback, event);
        };
    }
    const auto decodeBlock = [&](const std::size_t blockIndex,
                                 const TopologyConnectivityBlockLayoutParams& layout,
                                 const std::shared_ptr<topocodec::ConnectivityTopologyDecodeInputReader>& encodedBlock,
                                 const std::size_t localCellCount,
                                 const std::size_t localConnectivityCount,
                                 const std::size_t cellOffset,
                                 const std::size_t connectivityOffset) {
        std::string blockError;
        try {
            TopologyBlockCacheSink blockSink(
                outputSink,
                outputWriteMutex,
                topologyBlockObserver,
                blockIndex,
                cellOffset,
                connectivityOffset,
                localCellCount,
                localConnectivityCount,
                static_cast<int>(topo.fixedCellSize),
                hasOffsets,
                topo.hasCellTypes != 0u,
                hasOrders);
            const auto decoded = topocodec::DecodeConnectivityTopologyToSink(
                *encodedBlock,
                pointCount,
                localCellCount,
                localConnectivityCount,
                static_cast<int>(topo.fixedCellSize),
                topo.hasCellTypes != 0u,
                blockSink,
                &blockError,
                connectivityTiming);
            if (!decoded && blockError.empty()) {
                blockError = "topology block decoder returned failure";
            }
        } catch (const std::exception& exception) {
            blockError = std::string("topology block decoder raised an exception: ") + exception.what();
        } catch (...) {
            blockError = "topology block decoder raised an unknown exception";
        }

        {
            std::lock_guard<std::mutex> lock(blockState.mutex);
            if (!blockError.empty() && !blockState.failed) {
                blockState.failed = true;
                blockState.error = "topology block " + std::to_string(blockIndex) + " decode failed: " + blockError;
            }
            --blockState.inFlight;
        }
        blockState.available.notify_all();
    };
    std::unique_ptr<IParallelTaskGroup> taskGroup;
    if (parallelBlocks) {
        try {
            taskGroup = decodeRuntime.schedule.parallelTaskRunner->CreateGroup();
        } catch (const std::exception& exception) {
            const auto message = std::string("topology block task group creation failed: ") + exception.what();
            (void)finalizeObserver();
            return validation::AssignError(error, message);
        } catch (...) {
            (void)finalizeObserver();
            return validation::AssignError(error, "topology block task group creation failed");
        }
        if (taskGroup == nullptr) {
            (void)finalizeObserver();
            return validation::AssignError(error, "topology block task group is unavailable");
        }
    }
    const auto blockDecodeStart = callback::StartTiming(timingCallback);
    for (std::size_t blockIndex = 0u; blockIndex < blocks.size(); ++blockIndex) {
        const auto& layout = blocks[blockIndex];
        std::size_t localCellCount = 0u;
        std::size_t localConnectivityCount = 0u;
        std::size_t cellOffset = 0u;
        std::size_t connectivityOffset = 0u;
        if (!TryParamSizeToSizeT(layout.cellCount, localCellCount) ||
            !TryParamSizeToSizeT(layout.connectivityCount, localConnectivityCount) ||
            !TryParamSizeToSizeT(layout.cellOffset, cellOffset) ||
            !TryParamSizeToSizeT(layout.connectivityOffset, connectivityOffset)) {
            {
                std::lock_guard<std::mutex> lock(blockState.mutex);
                blockState.failed = true;
                blockState.error = "topology block layout exceeds local size capacity";
            }
            blockState.available.notify_all();
            break;
        }

        {
            std::unique_lock<std::mutex> lock(blockState.mutex);
            blockState.available.wait(lock, [&]() {
                return blockState.failed || blockState.inFlight < maxInFlightBlocks;
            });
            if (blockState.failed) {
                break;
            }
            ++blockState.inFlight;
        }

        auto encodedBlock = std::make_shared<topocodec::ConnectivityTopologyDecodeInputReader>();
        const auto inputLoadStart = callback::StartTiming(timingCallback);
        if (!encodedBlock->LoadFrom(
                stream,
                MakeConnectivityTopologyEncodedMetadata(layout),
                cacheResources,
                byteStoreSession,
                decodeRuntime.cache.topologyMemoryInputLimitBytes,
                decodeRuntime.cache.topologyInputStorageMode,
                error)) {
            {
                std::lock_guard<std::mutex> lock(blockState.mutex);
                --blockState.inFlight;
                blockState.failed = true;
                blockState.error = error != nullptr && !error->empty()
                    ? *error
                    : "failed to load topology block input";
            }
            blockState.available.notify_all();
            break;
        }
        if (timingCallback) {
            std::lock_guard<std::mutex> lock(timingMutex);
            inputLoadMs += callback::ElapsedMilliseconds(inputLoadStart);
        }
        allMemory = allMemory &&
            encodedBlock->StoreMode() == topocodec::ConnectivityTopologyDecodeInputReader::ByteStoreMode::Memory;

        const auto* blockLayout = &layout;
        if (taskGroup != nullptr) {
            try {
                taskGroup->Submit([&, blockIndex, blockLayout, encodedBlock, localCellCount, localConnectivityCount, cellOffset, connectivityOffset]() {
                    decodeBlock(
                        blockIndex,
                        *blockLayout,
                        encodedBlock,
                        localCellCount,
                        localConnectivityCount,
                        cellOffset,
                        connectivityOffset);
                });
            } catch (const std::exception& exception) {
                {
                    std::lock_guard<std::mutex> lock(blockState.mutex);
                    if (!blockState.failed) {
                        blockState.failed = true;
                        blockState.error = std::string("topology block task submission failed: ") + exception.what();
                    }
                    --blockState.inFlight;
                }
                blockState.available.notify_all();
                break;
            } catch (...) {
                {
                    std::lock_guard<std::mutex> lock(blockState.mutex);
                    if (!blockState.failed) {
                        blockState.failed = true;
                        blockState.error = "topology block task submission failed";
                    }
                    --blockState.inFlight;
                }
                blockState.available.notify_all();
                break;
            }
        } else {
            decodeBlock(
                blockIndex,
                *blockLayout,
                encodedBlock,
                localCellCount,
                localConnectivityCount,
                cellOffset,
                connectivityOffset);
        }
    }
    try {
        if (taskGroup != nullptr) {
            taskGroup->Wait();
        }
    } catch (const std::exception& exception) {
        std::lock_guard<std::mutex> lock(blockState.mutex);
        if (!blockState.failed) {
            blockState.failed = true;
            blockState.error = std::string("topology block task group wait failed: ") + exception.what();
        }
    } catch (...) {
        std::lock_guard<std::mutex> lock(blockState.mutex);
        if (!blockState.failed) {
            blockState.failed = true;
            blockState.error = "topology block task group wait failed";
        }
    }
    const auto observerEnded = finalizeObserver();
    {
        std::lock_guard<std::mutex> lock(blockState.mutex);
        if (blockState.failed) {
            return validation::AssignError(error, blockState.error);
        }
    }
    if (!observerEnded) {
        return validation::AssignError(error, observerEndError);
    }
    if (timingCallback) {
        timingCallback(TopologyDecodeTimingEvent{
            "TopoDecodeCoreStage.connectivity.block_input_load",
            inputLoadMs,
            "blocks=" + std::to_string(blocks.size()) +
                ";bytes=" + std::to_string(topo.binaryCount),
        });
        timingCallback(TopologyDecodeTimingEvent{
            "TopoDecodeCoreStage.connectivity.block_decode",
            callback::ElapsedMilliseconds(blockDecodeStart),
            "blocks=" + std::to_string(blocks.size()) +
                ";workers=" + std::to_string(workerCount),
        });
    }
    if (!outputSink.EndConnectivityTopology(error)) {
        return false;
    }
    topologyCache.SetInputByteStoreMode(
        allMemory
            ? DecodedTopologyCache::ByteStoreMode::Memory
            : DecodedTopologyCache::ByteStoreMode::Managed);
    return true;
}

template<typename TStream>
inline bool DecodeConnectivityTopologyStreamToCache(
    const TopoStorageParams& topo,
    TopologyDecodeRuntime& decodeRuntime,
    TStream& stream,
    std::string* error = nullptr) {
    return DecodeConnectivityTopologyBlocksToCache(topo, decodeRuntime, stream, error);
}


} // namespace detail

inline bool DecodeStructuredTopologyToCache(
    const CodecStorageParams& storageParams,
    DecodedTopologyCache& topology) {
    if (!storageParams.topoParams.isStructured) {
        return false;
    }
    topology.InitializeStructured(storageParams.structuredMeshParams.axisSize);
    return true;
}

template<typename TStream>
inline TopologyDecodeResult DecodeTopologyFieldToCache(
    TopologyDecodeRuntime& decodeRuntime,
    TStream& stream) {
    const auto& storageParams = decodeRuntime.data.storageParams;
    const auto& cacheResources = decodeRuntime.cache.cacheResources;
    auto& byteStoreSession = decodeRuntime.cache.byteStoreSession;
    auto& topology = decodeRuntime.cache.topology;
    const auto& timingCallback = decodeRuntime.context.timingCallback;
    const auto begin = stream.Position();
    std::uint64_t end = 0u;
    std::string error;
    const auto rangeStart = callback::StartTiming(timingCallback);
    if (!validation::CheckedAddU64(
            begin,
            storageParams.topoParams.binaryCount,
            end,
            "topology field range",
            &error)) {
        return detail::MakeTopologyDecodeFailure(
            CodecErrorCode::DecodeFailure,
            error);
    }
    detail::RecordTopologyDecodeTiming(
        timingCallback,
        "TopoDecodeCoreStage.range",
        rangeStart,
        "begin=" + std::to_string(begin) +
            ";bytes=" + std::to_string(storageParams.topoParams.binaryCount));

    if (DecodeStructuredTopologyToCache(storageParams, topology)) {
        return detail::ValidateTopologyConsumed(stream.Position(), end, &error);
    }

    if (storageParams.topoParams.isPolyhedron != 0u) {
        const auto polyhedronStart = callback::StartTiming(timingCallback);
        if (!polyhedron::DecodePolyhedronTopologyStreamsToCache(
                cacheResources,
                byteStoreSession,
                decodeRuntime.cache.topologyCacheStorageMode,
                decodeRuntime.cache.topologyMemoryCacheLimitBytes,
                topology,
                storageParams.topoParams,
                stream,
                &error)) {
            return detail::MakeTopologyDecodeFailure(
                CodecErrorCode::InvalidTopology,
                "failed to decode polyhedron topology streams: " + error);
        }
        detail::RecordTopologyDecodeTiming(
            timingCallback,
            "TopoDecodeCoreStage.polyhedron.decode",
            polyhedronStart,
            "cells=" + std::to_string(storageParams.topoParams.cellCount) +
                ";binaryBytes=" + std::to_string(storageParams.topoParams.binaryCount));
        return detail::ValidateTopologyConsumed(stream.Position(), end, &error);
    }
    if (!detail::DecodeConnectivityTopologyStreamToCache(
            storageParams.topoParams,
            decodeRuntime,
            stream,
            &error)) {
        return detail::MakeTopologyDecodeFailure(
            CodecErrorCode::PipelineFailure,
            "failed to decode topology stream: " + error);
    }
    return detail::ValidateTopologyConsumed(stream.Position(), end, &error);
}

} // namespace datacodec

#endif
