#ifndef DATACODEC_CODEC_REMAP_CELLREMAPBUILDER_H
#define DATACODEC_CODEC_REMAP_CELLREMAPBUILDER_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Codec/Remap/Common/MortonRemapBuilder.h"
#include "DataCodec/Codec/Remap/RemapProvider.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Common/Views/TopologyViews.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace datacodec::cellremap {

struct BuildOptions {
    const IRemapProvider* pointInverse{nullptr};
    WritableRemapProviderFactory providerFactory{};
    bytestore::ByteStoreSession* byteStoreSession{nullptr};
    resource::ActiveByteBudget* scratchBudget{nullptr};
    std::size_t mortonLeafBudgetBytes{mortonremap::kMortonDefaultLeafBudgetBytes};
    std::size_t mortonRunBufferBytes{mortonremap::kMortonRunBufferBytes};
    bool useMemoryScratchStore{false};
    std::function<void(double)> progressCallback{};
    std::function<void(std::string_view, std::uint64_t)> resourceCallback{};
};

inline bool BuildCellTopologyView(
    const IEncodeAdapter& adapter,
    TopologyView& topology,
    std::string* error = nullptr) {
    topology = {};
    if (!adapter.BuildTopologyView(topology)) {
        return validation::AssignError(error, "encode adapter failed to build the cell topology view");
    }
    return true;
}

[[nodiscard]] inline bool CellTopologyIsFixed(const TopologyView& topology) noexcept {
    return topology.fixedCellSizeEnabled && topology.fixedCellSize > 0;
}

[[nodiscard]] inline const IndexType* CellTopologyConnectivityData(
    const TopologyView& topology) noexcept {
    return static_cast<const IndexType*>(topology.connectivity.data);
}

[[nodiscard]] inline const IndexType* CellTopologyOffsetsData(
    const TopologyView& topology) noexcept {
    return static_cast<const IndexType*>(topology.offsets.data);
}

inline bool ValidateCellTopologyView(
    const TopologyView& topology,
    const IRemapProvider* pointInverse,
    std::string* error = nullptr) {
    if (topology.cellCount > 1u && topology.pointCount == 0u) {
        return validation::AssignError(error, "cell remap topology has cells without a point domain");
    }
    if (topology.connectivity.indexType != IndexValueType::UInt32 ||
        !topology.connectivity.IsValid() ||
        (topology.connectivity.count != 0u && !topology.connectivity.IsCompact())) {
        return validation::AssignError(error, "cell remap requires compact uint32 connectivity");
    }
    if (topology.cellCount == 0u) {
        if (topology.connectivity.count != 0u) {
            return validation::AssignError(error, "empty cell topology contains connectivity values");
        }
    } else if (CellTopologyIsFixed(topology)) {
        const auto cellSize = static_cast<std::size_t>(topology.fixedCellSize);
        if (!validation::CanMulSizeT(topology.cellCount, cellSize) ||
            topology.connectivity.count != topology.cellCount * cellSize) {
            return validation::AssignError(error, "fixed cell topology connectivity size is invalid");
        }
    } else {
        if (topology.offsets.indexType != IndexValueType::UInt32 ||
            !topology.offsets.IsValid() ||
            !topology.offsets.IsCompact() ||
            topology.offsets.count != topology.cellCount + 1u) {
            return validation::AssignError(error, "cell topology offsets are incomplete");
        }
        const auto* offsets = CellTopologyOffsetsData(topology);
        if (offsets[0] != 0u) {
            return validation::AssignError(error, "cell topology offsets must start at zero");
        }
        for (std::size_t index = 0u; index < topology.cellCount; ++index) {
            if (offsets[index] > offsets[index + 1u]) {
                return validation::AssignError(error, "cell topology offsets are not monotonic");
            }
        }
        if (static_cast<std::size_t>(offsets[topology.cellCount]) !=
            topology.connectivity.count) {
            return validation::AssignError(error, "cell topology offsets do not end at connectivity size");
        }
    }

    const auto* connectivity = CellTopologyConnectivityData(topology);
    for (std::size_t index = 0u; index < topology.connectivity.count; ++index) {
        if (static_cast<std::size_t>(connectivity[index]) >= topology.pointCount) {
            return validation::AssignError(error, "cell topology point index is out of range");
        }
    }

    if (pointInverse != nullptr) {
        if (pointInverse->Size() != topology.pointCount) {
            return validation::AssignError(error, "point inverse remap does not cover the point domain");
        }
        for (std::size_t pointIndex = 0u; pointIndex < topology.pointCount; ++pointIndex) {
            IndexType mappedPoint = 0u;
            if (!ReadRemapValue(pointInverse, pointIndex, mappedPoint, error)) {
                return false;
            }
            if (static_cast<std::size_t>(mappedPoint) >= topology.pointCount) {
                return validation::AssignError(error, "point inverse remap value is out of range");
            }
        }
    }
    return true;
}

[[nodiscard]] inline const IndexType* CellTopologyCellData(
    const TopologyView& topology,
    const std::size_t cellIndex) noexcept {
    const auto* connectivity = CellTopologyConnectivityData(topology);
    if (CellTopologyIsFixed(topology)) {
        return connectivity + cellIndex * static_cast<std::size_t>(topology.fixedCellSize);
    }
    return connectivity + CellTopologyOffsetsData(topology)[cellIndex];
}

[[nodiscard]] inline std::size_t CellTopologyCellSize(
    const TopologyView& topology,
    const std::size_t cellIndex) noexcept {
    if (CellTopologyIsFixed(topology)) {
        return static_cast<std::size_t>(topology.fixedCellSize);
    }
    const auto* offsets = CellTopologyOffsetsData(topology);
    return static_cast<std::size_t>(offsets[cellIndex + 1u] - offsets[cellIndex]);
}

[[nodiscard]] inline IndexType ResolvePointOrder(
    const IndexType pointId,
    const IRemapProvider* pointInverse) {
    if (pointInverse == nullptr || pointInverse->IsIdentity()) {
        return pointId;
    }
    IndexType value = 0u;
    std::string error;
    if (!ReadRemapValue(pointInverse, static_cast<std::size_t>(pointId), value, &error)) {
        throw std::runtime_error("DataCodec cell remap failed to read point inverse order: " + error);
    }
    return value;
}

[[nodiscard]] inline std::uint32_t MortonPart1By1(std::uint32_t value) noexcept {
    value &= mortonremap::kMortonBucketMask16;
    value = (value ^ (value << 8u)) & 0x00ff00ffu;
    value = (value ^ (value << 4u)) & 0x0f0f0f0fu;
    value = (value ^ (value << 2u)) & 0x33333333u;
    value = (value ^ (value << 1u)) & 0x55555555u;
    return value;
}

[[nodiscard]] inline std::uint32_t QuantizeMortonCoordinate(
    const IndexType pointId,
    const std::size_t pointCount) noexcept {
    if (pointCount <= 1u) {
        return 0u;
    }
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(pointId) * mortonremap::kMortonBucketMask16) /
        static_cast<std::uint64_t>(pointCount - 1u));
}

[[nodiscard]] inline std::uint32_t BuildIntervalMortonKey(
    const IndexType minPointId,
    const IndexType maxPointId,
    const std::size_t pointCount) noexcept {
    const auto quantizedMin = QuantizeMortonCoordinate(minPointId, pointCount);
    const auto quantizedMax = QuantizeMortonCoordinate(maxPointId, pointCount);
    return MortonPart1By1(quantizedMin) | (MortonPart1By1(quantizedMax) << 1u);
}

[[nodiscard]] inline std::uint32_t ComputeIntervalMortonKey(
    const TopologyView& topology,
    const IRemapProvider* pointInverse,
    const std::size_t cellIndex) {
    const auto* cellData = CellTopologyCellData(topology, cellIndex);
    const auto cellSize = CellTopologyCellSize(topology, cellIndex);
    auto minPointId = std::numeric_limits<IndexType>::max();
    auto maxPointId = IndexType{0u};
    for (std::size_t localIndex = 0u; localIndex < cellSize; ++localIndex) {
        const auto orderedPointId = ResolvePointOrder(cellData[localIndex], pointInverse);
        minPointId = std::min(minPointId, orderedPointId);
        maxPointId = std::max(maxPointId, orderedPointId);
    }
    if (cellSize == 0u) {
        minPointId = 0u;
    }
    return BuildIntervalMortonKey(minPointId, maxPointId, topology.pointCount);
}

inline bool BuildMortonRemapProvider(
    const TopologyView& topology,
    const BuildOptions& options,
    std::shared_ptr<IRemapProvider>& orderProvider,
    std::string* error = nullptr) {
    orderProvider.reset();
    if (!ValidateCellTopologyView(topology, options.pointInverse, error)) {
        return false;
    }
    if (topology.cellCount <= 1u) {
        orderProvider = std::make_shared<IdentityRemapProvider>(topology.cellCount);
        callback::InvokeProgress(options.progressCallback, 1.0);
        return true;
    }

    mortonremap::MortonRemapOptions remapOptions;
    remapOptions.resourcePrefix = "cell_remap.morton";
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
            return ComputeIntervalMortonKey(topology, options.pointInverse, cellIndex);
        };
        if (!mortonremap::BuildMortonRemapProvider(
                topology.cellCount,
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
        return validation::AssignError(error, "cell Morton remap did not produce an order provider");
    }
    orderProvider = std::move(result.orderProvider);
    return true;
}

} // namespace datacodec::cellremap

#endif
