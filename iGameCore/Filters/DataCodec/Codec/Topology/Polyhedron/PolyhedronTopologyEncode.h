#ifndef DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYENCODE_H
#define DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYENCODE_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Runtime/Cache/TransferCache/Common/TopologyTransferCache.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Codec/Remap/RemapOrderSource.h"
#include "DataCodec/Codec/Topology/Common/TopologyWorkBudget.h"
#include "DataCodec/Codec/Topology/Polyhedron/PolyhedronTopologyStreamEncoder.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace datacodec::polyhedron {

inline constexpr std::size_t kSmallCellLinearLookupThreshold = 24u;

struct PolyhedronCellWorkWorkspace {
    std::vector<IndexType> localIndexTable;
    std::vector<IndexType> overflowPointIds;
    std::vector<IndexType> overflowLocalIds;

    void Prepare(const std::size_t pointCount, const std::size_t overflowReserve) {
        if (localIndexTable.size() < pointCount) {
            localIndexTable.resize(pointCount, std::numeric_limits<IndexType>::max());
        }
        overflowPointIds.clear();
        overflowLocalIds.clear();
        overflowPointIds.reserve(overflowReserve);
        overflowLocalIds.reserve(overflowReserve);
    }
};

struct PolyhedronCellWorkBuffer {
    std::vector<IndexType> uniqueVertexIds;
    std::vector<IndexType> faceVertexCounts;
    std::vector<IndexType> localFaceVertexIds;

    void Clear() noexcept {
        uniqueVertexIds.clear();
        faceVertexCounts.clear();
        localFaceVertexIds.clear();
    }
};

struct PolyhedronTopologyData {
    const IEncodeAdapter& adapter;
    const RemapOrderSource& pointInverseOrderSource;
    const RemapOrderSource& cellOrderSource;
};

struct PolyhedronTopologySchedule {
    const EncodeResourceBudgetControlParams& resourceBudget;
};

struct PolyhedronTopologyCache {
    bytestore::ByteStoreSession& byteStoreSession;
};

struct PolyhedronTopologyContext {
    std::function<void(double)> progressCallback;
    std::function<void(std::string_view, double)> phaseTimingCallback;
    std::function<void(const char*, std::uint64_t, std::string)> memoryCheckpoint;
};

struct PolyhedronTopologyEncodeInput {
    PolyhedronTopologyData data;
    PolyhedronTopologySchedule schedule;
    PolyhedronTopologyCache cache;
    PolyhedronTopologyContext context;
};

struct PolyhedronTopologyEncodeResult {
    TopoStorageParams topo;
    std::shared_ptr<bytestore::IByteSource> transferCache;
};

inline void InvokePolyhedronTopologyProgress(
    const PolyhedronTopologyEncodeInput& input,
    const double normalized) {
    callback::InvokeProgress(input.context.progressCallback, normalized);
}

inline void AppendPolyhedronWorkPointLinear(
    PolyhedronCellWorkBuffer& workBuffer,
    const IndexType pointId) {
    for (std::size_t localIndex = 0; localIndex < workBuffer.uniqueVertexIds.size(); ++localIndex) {
        if (workBuffer.uniqueVertexIds[localIndex] == pointId) {
            workBuffer.localFaceVertexIds.push_back(static_cast<IndexType>(localIndex));
            return;
        }
    }

    const auto localId = static_cast<IndexType>(workBuffer.uniqueVertexIds.size());
    workBuffer.uniqueVertexIds.push_back(pointId);
    workBuffer.localFaceVertexIds.push_back(localId);
}

inline void AppendPolyhedronWorkPointHash(
    PolyhedronCellWorkBuffer& workBuffer,
    PolyhedronCellWorkWorkspace& workspace,
    const IndexType pointId) {
    if (static_cast<std::size_t>(pointId) < workspace.localIndexTable.size()) {
        const auto storedLocalId = workspace.localIndexTable[pointId];
        if (storedLocalId != std::numeric_limits<IndexType>::max() &&
            static_cast<std::size_t>(storedLocalId) < workBuffer.uniqueVertexIds.size() &&
            workBuffer.uniqueVertexIds[storedLocalId] == pointId) {
            workBuffer.localFaceVertexIds.push_back(storedLocalId);
            return;
        }

        const auto localId = static_cast<IndexType>(workBuffer.uniqueVertexIds.size());
        workspace.localIndexTable[pointId] = localId;
        workBuffer.uniqueVertexIds.push_back(pointId);
        workBuffer.localFaceVertexIds.push_back(localId);
        return;
    }

    for (std::size_t index = 0; index < workspace.overflowPointIds.size(); ++index) {
        if (workspace.overflowPointIds[index] == pointId) {
            workBuffer.localFaceVertexIds.push_back(workspace.overflowLocalIds[index]);
            return;
        }
    }

    const auto localId = static_cast<IndexType>(workBuffer.uniqueVertexIds.size());
    workspace.overflowPointIds.push_back(pointId);
    workspace.overflowLocalIds.push_back(localId);
    workBuffer.uniqueVertexIds.push_back(pointId);
    workBuffer.localFaceVertexIds.push_back(localId);
}

// [DC防护:领域] polyhedron 编码前校验局部拓扑偏移、引用范围和面单元关系
inline bool ValidateAdapterPolyhedronTopology(
    const IEncodeAdapter& adapter,
    const IndexType* cellFaceIds,
    const IndexType* cellFaceOffsets,
    const IndexType* faceVertexIds,
    const IndexType* faceVertexOffsets,
    const IRemapProvider* pointRemapInverse,
    std::string* error = nullptr) {
    const auto cellCount = adapter.GetNumberOfCells();
    const auto faceCount = adapter.GetNumberOfFaces();
    const auto pointCount = adapter.GetNumberOfPoints();
    const auto cellFaceIdCount = adapter.GetCellFaceBufferSize();
    const auto faceVertexIdCount = adapter.GetFaceIdBufferSize();
    if (cellCount == 0u) {
        return true;
    }
    if (faceCount == 0u || pointCount == 0u) {
        return validation::AssignError(error, "polyhedron adapter topology has cells without faces or points");
    }
    if ((cellFaceIdCount != 0u && cellFaceIds == nullptr) ||
        (faceVertexIdCount != 0u && faceVertexIds == nullptr)) {
        return validation::AssignError(error, "polyhedron adapter topology id buffer is missing");
    }

    std::size_t finalCellFaceOffset = 0u;
    std::size_t finalFaceVertexOffset = 0u;
    if (!ValidatePolyhedronOffsetRange(
            cellFaceOffsets,
            cellCount + 1u,
            cellFaceIdCount,
            "polyhedron adapter cell-face offsets",
            finalCellFaceOffset,
            error) ||
        !ValidatePolyhedronOffsetRange(
            faceVertexOffsets,
            faceCount + 1u,
            faceVertexIdCount,
            "polyhedron adapter face-vertex offsets",
            finalFaceVertexOffset,
            error)) {
        return false;
    }
    (void)finalCellFaceOffset;
    (void)finalFaceVertexOffset;

    std::vector<std::uint8_t> visitedFaces(faceCount, 0u);
    for (std::size_t cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
        const auto faceBegin = static_cast<std::size_t>(cellFaceOffsets[cellIndex]);
        const auto faceEnd = static_cast<std::size_t>(cellFaceOffsets[cellIndex + 1u]);
        if (faceEnd <= faceBegin) {
            return validation::AssignError(error, "polyhedron adapter topology contains a cell without faces");
        }

        for (std::size_t faceCursor = faceBegin; faceCursor < faceEnd; ++faceCursor) {
            const auto faceId = static_cast<std::size_t>(cellFaceIds[faceCursor]);
            if (faceId >= faceCount) {
                return validation::AssignError(
                    error,
                    "polyhedron adapter topology contains an out-of-range face id");
            }
            if (visitedFaces[faceId] != 0u) {
                continue;
            }

            const auto vertexBegin = static_cast<std::size_t>(faceVertexOffsets[faceId]);
            const auto vertexEnd = static_cast<std::size_t>(faceVertexOffsets[faceId + 1u]);
            if (vertexEnd < vertexBegin || vertexEnd - vertexBegin < 3u) {
                return validation::AssignError(
                    error,
                    "polyhedron adapter topology contains a face with fewer than three vertices");
            }
            for (std::size_t vertexCursor = vertexBegin; vertexCursor < vertexEnd; ++vertexCursor) {
                IndexType translatedPointId = 0u;
                if (!TryTranslateCellLocalPointId(
                        faceVertexIds[vertexCursor],
                        pointCount,
                        pointRemapInverse,
                        translatedPointId,
                        error)) {
                    return false;
                }
            }
            visitedFaces[faceId] = 1u;
        }
    }
    for (const auto visited : visitedFaces) {
        if (visited == 0u) {
            return validation::AssignError(error, "polyhedron adapter topology contains an unreferenced face");
        }
    }
    return true;
}

inline bool BuildAdapterPolyhedronCellWorkBuffer(
    PolyhedronCellWorkBuffer& workBuffer,
    const std::span<const IndexType> faceIds,
    const IndexType* faceVertexIds,
    const IndexType* faceVertexOffsets,
    const std::size_t faceCount,
    const std::size_t pointCount,
    const IRemapProvider* pointRemapInverse,
    PolyhedronCellWorkWorkspace& workspace,
    std::string* error = nullptr) {
    workBuffer.Clear();

    std::size_t totalFaceVertexCount = 0u;
    for (const auto rawFaceId : faceIds) {
        if (rawFaceId < 0 || static_cast<std::size_t>(rawFaceId) >= faceCount) {
            return validation::AssignError(
                error,
                "polyhedron adapter topology contains an out-of-range face id");
        }
        const auto faceId = static_cast<std::size_t>(rawFaceId);
        const auto start = static_cast<std::size_t>(faceVertexOffsets[faceId]);
        const auto end = static_cast<std::size_t>(faceVertexOffsets[faceId + 1u]);
        if (end < start || end - start < 3u) {
            return validation::AssignError(
                error,
                "polyhedron adapter topology contains an invalid face-vertex range");
        }
        if (!validation::CheckedAddSizeT(
                totalFaceVertexCount,
                end - start,
                totalFaceVertexCount,
                "polyhedron adapter total face vertex count",
                error)) {
            return false;
        }
    }

    workBuffer.faceVertexCounts.reserve(faceIds.size());
    workBuffer.localFaceVertexIds.reserve(totalFaceVertexCount);
    workBuffer.uniqueVertexIds.reserve(std::min<std::size_t>(totalFaceVertexCount, pointCount));
    if (totalFaceVertexCount > kSmallCellLinearLookupThreshold) {
        workspace.Prepare(
            pointRemapInverse == nullptr ? pointCount : pointRemapInverse->Size(),
            totalFaceVertexCount);
    }

    for (const auto rawFaceId : faceIds) {
        const auto faceId = static_cast<std::size_t>(rawFaceId);
        const auto faceLocalBegin = workBuffer.localFaceVertexIds.size();
        const auto start = static_cast<std::size_t>(faceVertexOffsets[faceId]);
        const auto end = static_cast<std::size_t>(faceVertexOffsets[faceId + 1u]);
        for (std::size_t cursor = start; cursor < end; ++cursor) {
            IndexType pointId = 0u;
            if (!TryTranslateCellLocalPointId(
                    faceVertexIds[cursor],
                    pointCount,
                    pointRemapInverse,
                    pointId,
                    error)) {
                return false;
            }
            if (totalFaceVertexCount <= kSmallCellLinearLookupThreshold) {
                AppendPolyhedronWorkPointLinear(workBuffer, pointId);
            } else {
                AppendPolyhedronWorkPointHash(workBuffer, workspace, pointId);
            }
        }
        workBuffer.faceVertexCounts.push_back(
            static_cast<IndexType>(workBuffer.localFaceVertexIds.size() - faceLocalBegin));
    }
    return true;
}

inline bool CompletePolyhedronTopologyStreamSpooler(
    topology::PolyhedronTopologyStreamSpooler& streams,
    const PolyhedronTopologyStreamStats& stats,
    std::string* error = nullptr) {
    return streams.CompleteStream(
            PolyhedronTopologyStreamKind::UniqueVertexCounts,
            PolyhedronTopologyStreamCodec::Varint,
            stats.cellCount,
            0u,
            error) &&
        streams.CompleteStream(
            PolyhedronTopologyStreamKind::CellFaceCounts,
            PolyhedronTopologyStreamCodec::Varint,
            stats.cellCount,
            0u,
            error) &&
        streams.CompleteStream(
            PolyhedronTopologyStreamKind::FaceVertexCounts,
            PolyhedronTopologyStreamCodec::Varint,
            stats.faceCount,
            0u,
            error) &&
        streams.CompleteStream(
            PolyhedronTopologyStreamKind::CellUniqueVertexIds,
            PolyhedronTopologyStreamCodec::Varint,
            stats.uniqueVertexIdCount,
            0u,
            error) &&
        streams.CompleteStream(
            PolyhedronTopologyStreamKind::LocalFaceVertexIds,
            PolyhedronTopologyStreamCodec::SegmentedBitpack,
            stats.localFaceVertexIdCount,
            0u,
            error);
}

inline bool EncodeAdapterPolyhedronStreamsToTransferCache(
    const IEncodeAdapter& adapter,
    const IRemapProvider* cellOrderProvider,
    const IRemapProvider* pointInverse,
    topology::PolyhedronTopologyStreamSpooler& streams,
    PolyhedronTopologyStreamStats& stats,
    const std::function<void(double)>& progressCallback,
    const std::function<void(std::string_view, double)>& phaseTimingCallback,
    std::string* error = nullptr) {
    stats = {};
    if (!adapter.IsPolyhedronMesh()) {
        return validation::AssignError(error, "adapter is not a polyhedron mesh");
    }

    const auto cellCount = adapter.GetNumberOfCells();
    const auto* cellFaceIds = adapter.GetCellFaceBufferPtr();
    const auto* cellFaceOffsets = adapter.GetCellFaceOffsetPtr();
    const auto* faceVertexIds = adapter.GetFaceIdBufferPtr();
    const auto* faceVertexOffsets = adapter.GetFaceIdOffsetPtr();
    const auto faceCount = adapter.GetNumberOfFaces();
    if (cellCount == 0) {
        return true;
    }
    if (cellFaceIds == nullptr || cellFaceOffsets == nullptr || faceVertexIds == nullptr || faceVertexOffsets == nullptr) {
        return validation::AssignError(error, "polyhedron stream encoder is missing adapter face table");
    }

    const auto pointCount = adapter.GetNumberOfPoints();
    auto phaseStart = callback::StartPhase(phaseTimingCallback);
    if (!ValidateAdapterPolyhedronTopology(
            adapter,
            cellFaceIds,
            cellFaceOffsets,
            faceVertexIds,
            faceVertexOffsets,
            pointInverse,
            error)) {
        return false;
    }
    phaseStart = callback::MarkPhase(phaseTimingCallback, "validate_adapter", phaseStart);
    PolyhedronCellWorkWorkspace workspace;
    PolyhedronCellWorkBuffer workBuffer;
    PolyhedronTopologyStreamEncoder encoder(streams);
    const auto progressStep = std::max<std::size_t>(cellCount / 64u, 1u);
    phaseStart = callback::StartPhase(phaseTimingCallback);

    for (std::size_t newCell = 0; newCell < cellCount; ++newCell) {
        std::size_t oldCell = 0u;
        if (!TryResolveOrderedCellIndex(
                cellOrderProvider,
                cellCount,
                newCell,
                oldCell,
                error)) {
            return false;
        }
        const auto begin = static_cast<std::size_t>(cellFaceOffsets[oldCell]);
        const auto end = static_cast<std::size_t>(cellFaceOffsets[oldCell + 1u]);
        if (!BuildAdapterPolyhedronCellWorkBuffer(
                workBuffer,
                std::span<const IndexType>(cellFaceIds + begin, end - begin),
                faceVertexIds,
                faceVertexOffsets,
                faceCount,
                pointCount,
                pointInverse,
                workspace,
                error)) {
            return false;
        }
        if (!encoder.AppendCell(
                PolyhedronTopologyStreamCellView{
                    std::span<const IndexType>(workBuffer.uniqueVertexIds.data(), workBuffer.uniqueVertexIds.size()),
                    std::span<const IndexType>(workBuffer.faceVertexCounts.data(), workBuffer.faceVertexCounts.size()),
                    std::span<const IndexType>(workBuffer.localFaceVertexIds.data(), workBuffer.localFaceVertexIds.size())},
                error)) {
            return false;
        }
        if (newCell + 1u == cellCount || (newCell + 1u) % progressStep == 0u) {
            callback::InvokeProgress(
                progressCallback,
                static_cast<double>(newCell + 1u) / static_cast<double>(cellCount));
        }
    }
    phaseStart = callback::MarkPhase(phaseTimingCallback, "adapter_cell_loop", phaseStart);
    if (!encoder.Finish(error)) {
        return false;
    }
    phaseStart = callback::MarkPhase(phaseTimingCallback, "adapter_encoder_finish", phaseStart);
    stats = encoder.Stats();
    const bool completed = CompletePolyhedronTopologyStreamSpooler(streams, stats, error);
    if (completed) {
        callback::MarkPhase(phaseTimingCallback, "adapter_complete_spooler", phaseStart);
    }
    return completed;
}

inline void UpdatePolyhedronStorageParamsFromStreamStats(
    TopoStorageParams& topo,
    const PolyhedronTopologyStreamStats& stats) {
    topo.cellBufferSize = stats.localFaceVertexIdCount;
    topo.polyhedronVertexCount = stats.uniqueVertexIdCount;
    topo.polyhedronFaceVertexCount = stats.faceCount;
}

inline bool EncodePolyhedronTopologyToTransferCache(
    const PolyhedronTopologyEncodeInput& input,
    PolyhedronTopologyEncodeResult& result,
    std::string* error = nullptr) {
    result = {};

    auto phaseStart = callback::StartPhase(input.context.phaseTimingCallback);
    auto& topo = result.topo;
    topo.cellCount = input.data.adapter.GetNumberOfCells();
    topo.isPolyhedron = true;
    phaseStart = callback::MarkPhase(input.context.phaseTimingCallback, "input_counts", phaseStart);

    if (!input.data.adapter.IsPolyhedronMesh()) {
        return validation::AssignError(error, "adapter is not a polyhedron mesh");
    }
    phaseStart = callback::MarkPhase(input.context.phaseTimingCallback, "is_polyhedron_mesh", phaseStart);

    if (topo.cellCount == 0) {
        result.transferCache = std::make_shared<bytestore::VectorByteSource>(std::vector<std::uint8_t>{});
        return true;
    }

    const auto topologyPath =
        topology::MakePolyhedronAdapterTopologyEncodePath(input.data.adapter);
    phaseStart = callback::MarkPhase(input.context.phaseTimingCallback, "make_topology_path", phaseStart);
    const auto topologyWorkBytes = topology::TopologyWorkBudget::Estimate(topologyPath);
    if (input.context.memoryCheckpoint) {
        input.context.memoryCheckpoint(
            "topology.work_budget.requested",
            topologyWorkBytes,
            "polyhedron");
    }
    phaseStart = callback::MarkPhase(input.context.phaseTimingCallback, "estimate_budget", phaseStart);

    auto polyhedronStreamSpooler = topology::MakePolyhedronTopologyStreamSpooler(
        input.cache.byteStoreSession,
        input.schedule.resourceBudget.TopologyEncodeTransferCacheStorageMode() ==
            EncodeStorageMode::Memory,
        error);
    if (polyhedronStreamSpooler == nullptr) {
        return false;
    }
    phaseStart = callback::MarkPhase(input.context.phaseTimingCallback, "make_spooler", phaseStart);
    PolyhedronTopologyStreamStats streamStats;
    const auto publishStreamProgress = [&](const double normalized) {
        InvokePolyhedronTopologyProgress(input, 0.05 + normalized * 0.82);
    };
    const bool encodedStreams = EncodeAdapterPolyhedronStreamsToTransferCache(
        input.data.adapter,
        input.data.cellOrderSource.Provider(),
        input.data.pointInverseOrderSource.Provider(),
        *polyhedronStreamSpooler,
        streamStats,
        publishStreamProgress,
        input.context.phaseTimingCallback,
        error);
    phaseStart = callback::MarkPhase(
        input.context.phaseTimingCallback,
        "encode_adapter_streams",
        phaseStart);
    if (!encodedStreams) {
        return false;
    }
    if (streamStats.cellCount != topo.cellCount) {
        return validation::AssignError(error, "encoded incomplete polyhedron topology stream transfer set");
    }

    UpdatePolyhedronStorageParamsFromStreamStats(
        topo,
        streamStats);
    InvokePolyhedronTopologyProgress(input, 0.90);
    phaseStart = callback::MarkPhase(input.context.phaseTimingCallback, "update_storage_params", phaseStart);

    if (!topology::ExportPolyhedronTopologyStreamLayouts(
            *polyhedronStreamSpooler,
            topo.polyhedronStreamLayouts,
            error)) {
        return false;
    }
    phaseStart = callback::MarkPhase(input.context.phaseTimingCallback, "export_stream_layouts", phaseStart);
    result.transferCache = topology::BuildPolyhedronTopologyStreamTransferCache(
        *polyhedronStreamSpooler,
        error);
    if (result.transferCache == nullptr) {
        return false;
    }
    phaseStart = callback::MarkPhase(input.context.phaseTimingCallback, "build_transfer_cache", phaseStart);
    topo.binaryCount = static_cast<std::size_t>(result.transferCache->ByteSizeHint());
    InvokePolyhedronTopologyProgress(input, 0.98);
    return true;
}

} // namespace datacodec::polyhedron

#endif
