#ifndef DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYEMIT_H
#define DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYEMIT_H

#include "DataCodec/API/Adapter/IDecodeAdapter.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedIndexCache.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Codec/Topology/Polyhedron/PolyhedronTopologyStreamFormat.h"
#include "DataCodec/Common/Views/TopologyViews.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace datacodec::polyhedron {
struct PolyhedronCellBatchScratch {
    std::vector<IndexType> cellVertexOffsets;
    std::vector<IndexType> cellFaceOffsets;
    std::vector<IndexType> faceVertexOffsets;
    std::vector<IndexType> cellUniqueVertexIds;
    std::vector<IndexType> localFaceVertexIds;
};

inline std::uint64_t PolyhedronCellBatchScratchBytes(const PolyhedronCellBatchScratch& scratch) noexcept {
    std::uint64_t bytes = 0u;
    bytes = validation::SaturatingAddU64(bytes, VectorCapacityBytes(scratch.cellVertexOffsets));
    bytes = validation::SaturatingAddU64(bytes, VectorCapacityBytes(scratch.cellFaceOffsets));
    bytes = validation::SaturatingAddU64(bytes, VectorCapacityBytes(scratch.faceVertexOffsets));
    bytes = validation::SaturatingAddU64(bytes, VectorCapacityBytes(scratch.cellUniqueVertexIds));
    bytes = validation::SaturatingAddU64(bytes, VectorCapacityBytes(scratch.localFaceVertexIds));
    return bytes;
}

inline bool EmitPolyhedronCacheToAdapter(
    const CacheResources& runtime,
    IDecodeAdapter& adapter,
    const PolyhedronTopologyStreamHeader& header,
    const DecodedIndexCache& uniqueVertexCountStore,
    const DecodedIndexCache& cellFaceCountStore,
    const DecodedIndexCache& faceVertexCountStore,
    const DecodedIndexCache& uniqueIdStore,
    const DecodedIndexCache& localIdStore,
    std::uint64_t& peakBatchBytes,
    std::uint64_t& batchCount,
    std::string* error = nullptr) {
    if (!adapter.SupportsPolyhedronTopology()) {
        return validation::AssignError(error, "decode adapter does not support polyhedron topology");
    }

    const auto polyhedronBatchBytes = std::max<std::uint64_t>(
        static_cast<std::uint64_t>(runtime.accessWindowBytes),
        sizeof(IndexType) * 5u);
    PolyhedronCellBatchScratch scratch;
    std::vector<IndexType> oneUniqueCount;
    std::vector<IndexType> oneCellFaceCount;
    std::vector<IndexType> nextFaceVertexCounts;
    std::vector<IndexType> uniqueVertexCounts;
    std::vector<IndexType> cellFaceCounts;
    std::vector<IndexType> faceVertexCounts;
    std::size_t cellCursor = 0u;
    std::size_t faceCursor = 0u;
    std::size_t uniqueCursor = 0u;
    std::size_t localCursor = 0u;
    const auto totalCells = static_cast<std::size_t>(header.cellCount);
    const auto totalFaces = static_cast<std::size_t>(header.faceCount);
    peakBatchBytes = 0u;
    batchCount = 0u;
    oneUniqueCount.resize(1u);
    oneCellFaceCount.resize(1u);

    while (cellCursor < totalCells) {
        const auto batchFirstCell = cellCursor;
        auto scanCell = cellCursor;
        auto scanFace = faceCursor;
        std::size_t batchUniqueCount = 0u;
        std::size_t batchLocalCount = 0u;
        std::uint64_t batchBytes = sizeof(IndexType) * 5u;
        uniqueVertexCounts.clear();
        cellFaceCounts.clear();
        faceVertexCounts.clear();

        while (scanCell < totalCells) {
            if (!uniqueVertexCountStore.ReadRangeInto(scanCell, std::span<IndexType>(oneUniqueCount.data(), oneUniqueCount.size()), error) ||
                !cellFaceCountStore.ReadRangeInto(scanCell, std::span<IndexType>(oneCellFaceCount.data(), oneCellFaceCount.size()), error)) {
                return false;
            }
            if (oneUniqueCount[0] < 0 || oneCellFaceCount[0] < 0) {
                return validation::AssignError(error, "stateful polyhedron count stream contains negative values");
            }
            const auto uniqueCount = static_cast<std::size_t>(oneUniqueCount[0]);
            const auto cellFaceCount = static_cast<std::size_t>(oneCellFaceCount[0]);
            if (scanFace > totalFaces || cellFaceCount > totalFaces - scanFace) {
                return validation::AssignError(
                    error,
                    "stateful polyhedron cell face counts exceed face vertex count stream");
            }
            nextFaceVertexCounts.assign(cellFaceCount, 0);
            if (!faceVertexCountStore.ReadRangeInto(
                    scanFace,
                    std::span<IndexType>(nextFaceVertexCounts.data(), nextFaceVertexCounts.size()),
                    error)) {
                return false;
            }
            std::size_t nextLocalCount = 0u;
            for (const auto faceVertexCount : nextFaceVertexCounts) {
                if (faceVertexCount < 0) {
                    return validation::AssignError(
                        error,
                        "stateful polyhedron face vertex count stream contains negative values");
                }
                if (!validation::CheckedAddSizeT(
                        nextLocalCount,
                        static_cast<std::size_t>(faceVertexCount),
                        nextLocalCount,
                        "stateful polyhedron next local id count",
                        error)) {
                    return false;
                }
            }
            std::size_t nextCellValues = 0u;
            if (!validation::CheckedAddSizeT(
                    uniqueCount,
                    cellFaceCount,
                    nextCellValues,
                    "stateful polyhedron next cell value count",
                    error) ||
                !validation::CheckedAddSizeT(
                    nextCellValues,
                    nextLocalCount,
                    nextCellValues,
                    "stateful polyhedron next cell value count",
                    error) ||
                !validation::CheckedAddSizeT(
                    nextCellValues,
                    2u,
                    nextCellValues,
                    "stateful polyhedron next cell value count",
                    error)) {
                return false;
            }
            std::uint64_t nextCellBytes = 0u;
            if (!validation::CheckedMulU64(
                    static_cast<std::uint64_t>(nextCellValues),
                    static_cast<std::uint64_t>(sizeof(IndexType)),
                    nextCellBytes,
                    "stateful polyhedron next cell bytes",
                    error)) {
                return false;
            }
            if (scanCell != cellCursor &&
                (batchBytes > polyhedronBatchBytes || nextCellBytes > polyhedronBatchBytes - batchBytes)) {
                break;
            }
            uniqueVertexCounts.push_back(oneUniqueCount[0]);
            cellFaceCounts.push_back(oneCellFaceCount[0]);
            faceVertexCounts.insert(
                faceVertexCounts.end(),
                nextFaceVertexCounts.begin(),
                nextFaceVertexCounts.end());
            if (!validation::CheckedAddSizeT(
                    batchUniqueCount,
                    uniqueCount,
                    batchUniqueCount,
                    "stateful polyhedron batch unique id count",
                    error) ||
                !validation::CheckedAddSizeT(
                    batchLocalCount,
                    nextLocalCount,
                    batchLocalCount,
                    "stateful polyhedron batch local id count",
                    error) ||
                !validation::CheckedAddU64(
                    batchBytes,
                    nextCellBytes,
                    batchBytes,
                    "stateful polyhedron batch bytes",
                    error) ||
                !validation::CheckedAddSizeT(
                    scanFace,
                    cellFaceCount,
                    scanFace,
                    "stateful polyhedron face cursor",
                    error)) {
                return false;
            }
            ++scanCell;
        }

        scratch.cellVertexOffsets.clear();
        scratch.cellFaceOffsets.clear();
        scratch.faceVertexOffsets.clear();
        scratch.localFaceVertexIds.clear();
        scratch.cellVertexOffsets.reserve(scanCell - cellCursor + 1u);
        scratch.cellFaceOffsets.reserve(scanCell - cellCursor + 1u);
        scratch.faceVertexOffsets.reserve(scanFace - faceCursor + 1u);
        scratch.cellVertexOffsets.push_back(0u);
        scratch.cellFaceOffsets.push_back(0u);
        scratch.faceVertexOffsets.push_back(0u);

        scratch.cellUniqueVertexIds.assign(batchUniqueCount, 0);
        scratch.localFaceVertexIds.assign(batchLocalCount, 0);
        if (!uniqueIdStore.ReadRangeInto(
                uniqueCursor,
                std::span<IndexType>(scratch.cellUniqueVertexIds.data(), scratch.cellUniqueVertexIds.size()),
                error) ||
            !localIdStore.ReadRangeInto(
                localCursor,
                std::span<IndexType>(scratch.localFaceVertexIds.data(), scratch.localFaceVertexIds.size()),
                error)) {
            return false;
        }

        std::size_t batchFaceCursor = 0u;
        auto buildUnique = uniqueCursor;
        for (std::size_t cell = 0u; cell < uniqueVertexCounts.size(); ++cell) {
            const auto uniqueCount = static_cast<std::size_t>(uniqueVertexCounts[cell]);
            if (!validation::CheckedAddSizeT(
                    buildUnique,
                    uniqueCount,
                    buildUnique,
                    "stateful polyhedron unique cursor",
                    error)) {
                return false;
            }
            scratch.cellVertexOffsets.push_back(static_cast<IndexType>(buildUnique - uniqueCursor));
            const auto cellFaceCount = static_cast<std::size_t>(cellFaceCounts[cell]);
            for (std::size_t localFace = 0u; localFace < cellFaceCount; ++localFace) {
                const auto faceVertexCount = static_cast<std::size_t>(faceVertexCounts[batchFaceCursor++]);
                std::size_t nextFaceOffset = 0u;
                if (!validation::CheckedAddSizeT(
                        static_cast<std::size_t>(scratch.faceVertexOffsets.back()),
                        faceVertexCount,
                        nextFaceOffset,
                        "stateful polyhedron face vertex offset",
                        error)) {
                    return false;
                }
                scratch.faceVertexOffsets.push_back(
                    static_cast<IndexType>(nextFaceOffset));
            }
            scratch.cellFaceOffsets.push_back(static_cast<IndexType>(batchFaceCursor));
        }

        PolyhedronTopologyView batch;
        batch.cellVertexOffsets = scratch.cellVertexOffsets.data();
        batch.cellVertexOffsetCount = scratch.cellVertexOffsets.size();
        batch.cellUniqueVertexIds = scratch.cellUniqueVertexIds.data();
        batch.cellUniqueVertexIdCount = scratch.cellUniqueVertexIds.size();
        batch.cellFaceOffsets = scratch.cellFaceOffsets.data();
        batch.cellFaceOffsetCount = scratch.cellFaceOffsets.size();
        batch.faceVertexOffsets = scratch.faceVertexOffsets.data();
        batch.faceVertexOffsetCount = scratch.faceVertexOffsets.size();
        batch.localFaceVertexIds = scratch.localFaceVertexIds.data();
        batch.localFaceVertexIdCount = scratch.localFaceVertexIds.size();
        if (!adapter.WritePolyhedronCellBatch(batchFirstCell, batch, error)) {
            return false;
        }
        ++batchCount;
        peakBatchBytes = std::max<std::uint64_t>(peakBatchBytes, PolyhedronCellBatchScratchBytes(scratch));

        cellCursor = scanCell;
        faceCursor = scanFace;
        if (!validation::CheckedAddSizeT(
                uniqueCursor,
                batchUniqueCount,
                uniqueCursor,
                "stateful polyhedron unique cursor",
                error) ||
            !validation::CheckedAddSizeT(
                localCursor,
                batchLocalCount,
                localCursor,
                "stateful polyhedron local id cursor",
                error)) {
            return false;
        }
    }

    if (faceCursor != totalFaces ||
        uniqueCursor != static_cast<std::size_t>(header.uniqueVertexIdCount) ||
        localCursor != static_cast<std::size_t>(header.localFaceVertexIdCount)) {
        return validation::AssignError(error, "stateful polyhedron stream assembly did not consume every stream");
    }
    return true;
}


} // namespace datacodec::polyhedron

#endif
