#ifndef DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYREMAP_H
#define DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYREMAP_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Codec/Remap/CellRemapBuilder.h"
#include "DataCodec/Codec/Remap/Common/MortonRemapBuilder.h"
#include "DataCodec/Codec/Remap/RemapProvider.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace datacodec::polyhedron {

inline bool TryTranslateCellLocalPointId(
    const IndexType pointId,
    const std::size_t pointCount,
    const IRemapProvider* pointRemapInverse,
    IndexType& translatedPointId,
    std::string* error = nullptr) {
    const auto pointIndex = static_cast<std::size_t>(pointId);
    if (pointIndex >= pointCount) {
        return SetCodecError(
            error,
            CodecErrorCode::InvalidTopology,
            "polyhedron topology point id is out of range");
    }

    translatedPointId = pointId;
    if (pointRemapInverse == nullptr) {
        return true;
    }
    if (pointRemapInverse->Size() != pointCount) {
        return SetCodecError(
            error,
            CodecErrorCode::InvalidRemap,
            "polyhedron point inverse order does not cover the point domain");
    }
    if (!pointRemapInverse->IsIdentity()) {
        std::string readError;
        if (!ReadRemapValue(pointRemapInverse, pointIndex, translatedPointId, &readError)) {
            return SetCodecError(
                error,
                CodecErrorCode::InvalidRemap,
                "polyhedron topology failed to read point inverse order: " + readError);
        }
    }
    if (static_cast<std::size_t>(translatedPointId) >= pointCount) {
        return SetCodecError(
            error,
            CodecErrorCode::InvalidRemap,
            "polyhedron ordered point id is out of range");
    }
    return true;
}

inline bool TryResolveOrderedCellIndex(
    const IRemapProvider* cellOrderProvider,
    const std::size_t cellCount,
    const std::size_t newCell,
    std::size_t& oldCell,
    std::string* error = nullptr) {
    if (newCell >= cellCount) {
        return SetCodecError(
            error,
            CodecErrorCode::InvalidRemap,
            "polyhedron cell order input index is out of range");
    }

    IndexType resolvedCell = static_cast<IndexType>(newCell);
    if (cellOrderProvider != nullptr) {
        if (cellOrderProvider->Size() != cellCount) {
            return SetCodecError(
                error,
                CodecErrorCode::InvalidRemap,
                "polyhedron cell order provider does not cover the cell domain");
        }
        if (!cellOrderProvider->IsIdentity()) {
            std::string readError;
            if (!ReadRemapValue(cellOrderProvider, newCell, resolvedCell, &readError)) {
                return SetCodecError(
                    error,
                    CodecErrorCode::InvalidRemap,
                    "polyhedron topology failed to read cell order: " + readError);
            }
        }
    }

    oldCell = static_cast<std::size_t>(resolvedCell);
    if (oldCell >= cellCount) {
        return SetCodecError(
            error,
            CodecErrorCode::InvalidRemap,
            "polyhedron cell order output index is out of range");
    }
    return true;
}

inline bool ValidatePolyhedronOffsetRange(
    const IndexType* offsets,
    const std::size_t offsetCount,
    const std::size_t bufferSize,
    const char* name,
    std::size_t& finalOffset,
    std::string* error = nullptr) {
    finalOffset = 0u;
    if (offsets == nullptr || offsetCount == 0u) {
        return SetCodecError(
            error,
            CodecErrorCode::MissingInput,
            std::string(name) + " is missing");
    }
    if (offsets[0] != 0u) {
        return SetCodecError(
            error,
            CodecErrorCode::InvalidTopology,
            std::string(name) + " must start at zero");
    }
    for (std::size_t index = 1u; index < offsetCount; ++index) {
        if (offsets[index] < offsets[index - 1u]) {
            return SetCodecError(
                error,
                CodecErrorCode::InvalidTopology,
                std::string(name) + " is not monotonic");
        }
    }
    finalOffset = static_cast<std::size_t>(offsets[offsetCount - 1u]);
    if (finalOffset != bufferSize) {
        return SetCodecError(
            error,
            CodecErrorCode::InvalidTopology,
            std::string(name) + " does not match its id buffer");
    }
    return true;
}

inline bool ValidatePolyhedronOffsetRange(
    const IndexType* offsets,
    const std::size_t offsetCount,
    const std::size_t bufferSize,
    const char* name,
    std::string* error = nullptr) {
    std::size_t finalOffset = 0u;
    return ValidatePolyhedronOffsetRange(
        offsets,
        offsetCount,
        bufferSize,
        name,
        finalOffset,
        error);
}

struct PolyhedronCellRangeSource {
    const IndexType* cellFaceIds{nullptr};
    const IndexType* cellFaceOffsets{nullptr};
    const IndexType* faceVertexIds{nullptr};
    const IndexType* faceVertexOffsets{nullptr};
    std::size_t cellCount{0u};
    std::size_t faceCount{0u};
    std::size_t pointCount{0u};
    std::size_t cellFaceIdCount{0u};
    std::size_t faceVertexIdCount{0u};

    bool Initialize(const IEncodeAdapter& adapter, std::string* error = nullptr) {
        if (!adapter.IsPolyhedronMesh()) {
            return SetCodecError(
                error,
                CodecErrorCode::InvalidTopology,
                "polyhedron cell range source requires a polyhedron mesh");
        }
        cellFaceIds = adapter.GetCellFaceBufferPtr();
        cellFaceOffsets = adapter.GetCellFaceOffsetPtr();
        faceVertexIds = adapter.GetFaceIdBufferPtr();
        faceVertexOffsets = adapter.GetFaceIdOffsetPtr();
        cellCount = adapter.GetNumberOfCells();
        faceCount = adapter.GetNumberOfFaces();
        pointCount = adapter.GetNumberOfPoints();
        cellFaceIdCount = adapter.GetCellFaceBufferSize();
        faceVertexIdCount = adapter.GetFaceIdBufferSize();
        if (cellCount == 0u) {
            return true;
        }
        if (cellCount > 1u && pointCount == 0u) {
            return SetCodecError(
                error,
                CodecErrorCode::InvalidTopology,
                "polyhedron cells require a non-empty point domain");
        }
        if (cellFaceIds == nullptr || cellFaceOffsets == nullptr ||
            faceVertexIds == nullptr || faceVertexOffsets == nullptr) {
            return SetCodecError(
                error,
                CodecErrorCode::MissingInput,
                "polyhedron cell range source is missing adapter face table");
        }
        return ValidatePolyhedronOffsetRange(
                   cellFaceOffsets,
                   cellCount + 1u,
                   cellFaceIdCount,
                   "polyhedron cell-face offsets",
                   error) &&
            ValidatePolyhedronOffsetRange(
                faceVertexOffsets,
                faceCount + 1u,
                faceVertexIdCount,
                "polyhedron face-vertex offsets",
                error);
    }

    bool ResolveRange(
        const std::size_t cellIndex,
        const IRemapProvider* pointInverse,
        IndexType& minPointId,
        IndexType& maxPointId,
        bool& hasPoint,
        std::string* error = nullptr) const {
        minPointId = std::numeric_limits<IndexType>::max();
        maxPointId = 0u;
        hasPoint = false;
        if (cellIndex >= cellCount) {
            return SetCodecError(
                error,
                CodecErrorCode::InvalidRemap,
                "polyhedron cell range source cell index is out of range");
        }
        const auto faceBegin = static_cast<std::size_t>(cellFaceOffsets[cellIndex]);
        const auto faceEnd = static_cast<std::size_t>(cellFaceOffsets[cellIndex + 1u]);
        if (faceEnd <= faceBegin || faceEnd > cellFaceIdCount) {
            return SetCodecError(
                error,
                CodecErrorCode::InvalidTopology,
                "polyhedron cell range source has an invalid cell-face range");
        }
        for (std::size_t faceCursor = faceBegin; faceCursor < faceEnd; ++faceCursor) {
            const auto faceId = static_cast<std::size_t>(cellFaceIds[faceCursor]);
            if (faceId >= faceCount) {
                return SetCodecError(
                    error,
                    CodecErrorCode::InvalidTopology,
                    "polyhedron cell range source found an out-of-range face id");
            }
            const auto vertexBegin = static_cast<std::size_t>(faceVertexOffsets[faceId]);
            const auto vertexEnd = static_cast<std::size_t>(faceVertexOffsets[faceId + 1u]);
            if (vertexEnd > faceVertexIdCount || vertexEnd - vertexBegin < 3u) {
                return SetCodecError(
                    error,
                    CodecErrorCode::InvalidTopology,
                    "polyhedron cell range source has an invalid face-vertex range");
            }
            for (std::size_t vertexCursor = vertexBegin; vertexCursor < vertexEnd; ++vertexCursor) {
                IndexType orderedPointId = 0u;
                if (!TryTranslateCellLocalPointId(
                        faceVertexIds[vertexCursor],
                        pointCount,
                        pointInverse,
                        orderedPointId,
                        error)) {
                    return false;
                }
                minPointId = hasPoint ? std::min(minPointId, orderedPointId) : orderedPointId;
                maxPointId = hasPoint ? std::max(maxPointId, orderedPointId) : orderedPointId;
                hasPoint = true;
            }
        }
        return hasPoint || SetCodecError(
            error,
            CodecErrorCode::InvalidTopology,
            "polyhedron cell has no point ids");
    }
};

inline bool BuildPolyhedronCellRangeMortonRemapProviderFromSource(
    const PolyhedronCellRangeSource& source,
    const cellremap::BuildOptions& options,
    std::shared_ptr<IRemapProvider>& orderProvider,
    std::string* error = nullptr) {
    orderProvider.reset();
    if (options.pointInverse != nullptr &&
        options.pointInverse->Size() != source.pointCount) {
        return SetCodecError(
            error,
            CodecErrorCode::InvalidRemap,
            "polyhedron point inverse order does not cover the point domain");
    }
    if (source.cellCount <= 1u) {
        orderProvider = std::make_shared<IdentityRemapProvider>(source.cellCount);
        callback::InvokeProgress(options.progressCallback, 1.0);
        return true;
    }

    mortonremap::MortonRemapOptions remapOptions;
    remapOptions.resourcePrefix = "cell_remap.polyhedron.range_morton";
    remapOptions.progressCallback = options.progressCallback;
    remapOptions.resourceCallback = options.resourceCallback;
    remapOptions.providerFactory = options.providerFactory;
    remapOptions.byteStoreSession = options.byteStoreSession;
    remapOptions.scratchBudget = options.scratchBudget;
    remapOptions.leafBudgetBytes = options.mortonLeafBudgetBytes;
    remapOptions.runBufferBytes = options.mortonRunBufferBytes;
    remapOptions.buildInverse = false;
    remapOptions.useMemoryScratchStore = options.useMemoryScratchStore;

    mortonremap::MortonRemapResult result;
    try {
        const auto keyGetter = [&](const std::size_t cellIndex) {
            IndexType minPointId = 0u;
            IndexType maxPointId = 0u;
            bool hasPoint = false;
            std::string localError;
            if (!source.ResolveRange(
                    cellIndex,
                    options.pointInverse,
                    minPointId,
                    maxPointId,
                    hasPoint,
                    &localError)) {
                throw std::runtime_error(localError);
            }
            return cellremap::BuildIntervalMortonKey(
                minPointId,
                maxPointId,
                source.pointCount);
        };
        if (!mortonremap::BuildMortonRemapProvider(
                source.cellCount,
                keyGetter,
                result,
                remapOptions,
                error)) {
            return false;
        }
    } catch (const std::exception& exception) {
        return validation::AssignError(error, exception.what());
    }
    if (result.orderProvider == nullptr) {
        return validation::AssignError(
            error,
            "polyhedron range Morton remap did not produce an order provider");
    }
    orderProvider = std::move(result.orderProvider);
    return true;
}

inline bool BuildPolyhedronCellRangeMortonRemapProvider(
    const IEncodeAdapter& adapter,
    const cellremap::BuildOptions& options,
    std::shared_ptr<IRemapProvider>& orderProvider,
    std::string* error = nullptr) {
    PolyhedronCellRangeSource source;
    if (!source.Initialize(adapter, error)) {
        orderProvider.reset();
        return false;
    }
    return BuildPolyhedronCellRangeMortonRemapProviderFromSource(
        source,
        options,
        orderProvider,
        error);
}

} // namespace datacodec::polyhedron

#endif
