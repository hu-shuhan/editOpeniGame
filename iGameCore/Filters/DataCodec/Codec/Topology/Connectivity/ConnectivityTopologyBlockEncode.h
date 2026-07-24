#ifndef DATACODEC_CODEC_TOPOLOGY_CONNECTIVITY_CONNECTIVITYTOPOLOGYBLOCKENCODE_H
#define DATACODEC_CODEC_TOPOLOGY_CONNECTIVITY_CONNECTIVITYTOPOLOGYBLOCKENCODE_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/API/Params/CodecStorageParams.h"
#include "DataCodec/Codec/Remap/RemapOrderSource.h"
#include "DataCodec/Codec/Topology/Common/TopologyWorkBudget.h"
#include "DataCodec/Codec/Topology/Connectivity/ConnectivityTopologyCodec.h"
#include "DataCodec/Codec/Topology/Connectivity/ConnectivityTopologyTypes.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Runtime/Cache/TransferCache/Common/TopologyTransferCache.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Storage/ByteStore/SegmentedBinaryObject.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace datacodec::topocodec {

struct OrderedTopologyCellSource {
    const IEncodeAdapter* adapter{nullptr};
    const IndexType* indices{nullptr};
    const IndexType* offsets{nullptr};
    const IndexType* cellTypes{nullptr};
    const std::uint16_t* cellPolynomialOrders{nullptr};
    NumericArrayView cellTypesView;
    NumericArrayView cellPolynomialOrdersView;
    std::size_t pointCount{0u};
    std::size_t cellCount{0u};
    std::size_t connectivityCount{0u};
    int fixedCellSize{0};
    const IRemapProvider* inversePointRemap{nullptr};
    std::span<const IndexType> inversePointRemapValues;
    const IRemapProvider* cellOrderProvider{nullptr};
    std::span<const IndexType> cellOrderValues;
    bool hasCellTypes{false};
    bool hasCellPolynomialOrders{false};

    [[nodiscard]] std::size_t CellCount() const noexcept { return cellCount; }
    [[nodiscard]] std::size_t ConnectivityCount() const noexcept { return connectivityCount; }
    [[nodiscard]] bool HasCellTypes() const noexcept { return hasCellTypes; }
    [[nodiscard]] bool HasCellPolynomialOrders() const noexcept { return hasCellPolynomialOrders; }

    [[nodiscard]] std::size_t OldCellIndex(const std::size_t newCellIndex) const {
        if (newCellIndex >= cellCount) {
            throw std::runtime_error("DataCodec topology cell index is out of range");
        }
        IndexType oldCellIndex = static_cast<IndexType>(newCellIndex);
        if (!cellOrderValues.empty()) {
            oldCellIndex = cellOrderValues[newCellIndex];
        } else if (cellOrderProvider != nullptr && !cellOrderProvider->IsIdentity()) {
            std::string error;
            if (!ReadRemapValue(cellOrderProvider, newCellIndex, oldCellIndex, &error)) {
                throw std::runtime_error("DataCodec topology failed to read cell order: " + error);
            }
        }
        if (static_cast<std::size_t>(oldCellIndex) >= cellCount) {
            throw std::runtime_error("DataCodec topology cell order is out of range");
        }
        return static_cast<std::size_t>(oldCellIndex);
    }

    [[nodiscard]] IndexType CellTypeFromOld(const std::size_t oldCellIndex) const {
        if (cellTypes != nullptr) {
            return cellTypes[oldCellIndex];
        }
        if (cellTypesView.tupleCount == cellCount && cellTypesView.IsValid()) {
            IndexType value = 0u;
            std::string error;
            if (!ReadNumericArrayTupleBytes(cellTypesView, oldCellIndex, 1u, &value, &error)) {
                throw std::runtime_error("DataCodec topology failed to read cell type: " + error);
            }
            return value;
        }
        IndexType value = 0u;
        if (adapter != nullptr && adapter->ReadCellType(oldCellIndex, value)) {
            return value;
        }
        throw std::runtime_error("DataCodec topology failed to read cell type");
    }

    [[nodiscard]] std::uint16_t CellPolynomialOrderFromOld(const std::size_t oldCellIndex) const {
        if (cellPolynomialOrders != nullptr) {
            return cellPolynomialOrders[oldCellIndex];
        }
        if (cellPolynomialOrdersView.tupleCount == cellCount && cellPolynomialOrdersView.IsValid()) {
            std::uint16_t value = 0u;
            std::string error;
            if (!ReadNumericArrayTupleBytes(
                    cellPolynomialOrdersView,
                    oldCellIndex,
                    1u,
                    &value,
                    &error)) {
                throw std::runtime_error(
                    "DataCodec topology failed to read cell polynomial order: " + error);
            }
            return value;
        }
        std::uint16_t value = 0u;
        if (adapter != nullptr && adapter->ReadCellPolynomialOrder(oldCellIndex, value)) {
            return value;
        }
        throw std::runtime_error("DataCodec topology failed to read cell polynomial order");
    }

    void ReadCell(
        const std::size_t newCellIndex,
        std::vector<IndexType>& output,
        IndexType* cellType,
        std::uint16_t* cellPolynomialOrder) const {
        const auto oldCellIndex = OldCellIndex(newCellIndex);
        std::size_t begin = 0u;
        std::size_t end = 0u;
        if (fixedCellSize > 0) {
            const auto cellSize = static_cast<std::size_t>(fixedCellSize);
            if (!validation::CheckedMulSizeT(
                    oldCellIndex,
                    cellSize,
                    begin,
                    "topology fixed cell offset",
                    nullptr) ||
                !validation::CheckedAddSizeT(
                    begin,
                    cellSize,
                    end,
                    "topology fixed cell end",
                    nullptr)) {
                throw std::runtime_error("DataCodec topology fixed cell range exceeds local capacity");
            }
        } else {
            if (offsets == nullptr) {
                throw std::runtime_error("DataCodec topology offset stream is missing");
            }
            begin = static_cast<std::size_t>(offsets[oldCellIndex]);
            end = static_cast<std::size_t>(offsets[oldCellIndex + 1u]);
            if (end < begin) {
                throw std::runtime_error("DataCodec topology offsets are not monotonic");
            }
        }
        if (end > connectivityCount) {
            throw std::runtime_error("DataCodec topology cell range exceeds connectivity storage");
        }

        output.resize(end - begin);
        for (std::size_t local = 0u; local < output.size(); ++local) {
            const auto sourcePoint = indices[begin + local];
            if (static_cast<std::size_t>(sourcePoint) >= pointCount) {
                throw std::runtime_error("DataCodec topology point index is out of range");
            }
            IndexType mappedPoint = sourcePoint;
            if (!inversePointRemapValues.empty()) {
                mappedPoint = inversePointRemapValues[static_cast<std::size_t>(sourcePoint)];
            } else if (inversePointRemap != nullptr && !inversePointRemap->IsIdentity()) {
                std::string error;
                if (!ReadRemapValue(
                        inversePointRemap,
                        static_cast<std::size_t>(sourcePoint),
                        mappedPoint,
                        &error)) {
                    throw std::runtime_error("DataCodec topology failed to read point order: " + error);
                }
            }
            if (static_cast<std::size_t>(mappedPoint) >= pointCount) {
                throw std::runtime_error("DataCodec topology mapped point index is out of range");
            }
            output[local] = mappedPoint;
        }
        if (cellType != nullptr) {
            *cellType = CellTypeFromOld(oldCellIndex);
        }
        if (cellPolynomialOrder != nullptr) {
            *cellPolynomialOrder = CellPolynomialOrderFromOld(oldCellIndex);
        }
    }
};

struct OrderedTopologyCellRangeSource {
    const OrderedTopologyCellSource* source{nullptr};
    std::size_t firstCell{0u};
    std::size_t cellCount{0u};

    [[nodiscard]] std::size_t CellCount() const noexcept { return cellCount; }
    [[nodiscard]] bool HasCellTypes() const noexcept { return source->HasCellTypes(); }
    [[nodiscard]] bool HasCellPolynomialOrders() const noexcept {
        return source->HasCellPolynomialOrders();
    }
};

struct TopologyEncodeData {
    const IEncodeAdapter& adapter;
    const RemapOrderSource& pointInverseOrderSource;
    const RemapOrderSource& cellOrderSource;
};

struct TopologyEncodeExecutionParams {
    const ResourceBudgetControlParams& resourceBudget;
    std::uint32_t cellElementCount{262144u};
    IParallelTaskRunner* parallelTaskRunner{nullptr};
    std::size_t workerCount{1u};
};

struct TopologyEncodeRuntime {
    bytestore::ByteStoreSession& byteStoreSession;
};

struct TopologyEncodeContext {
    std::function<void(double)> progressCallback;
    std::function<void(const char*, std::uint64_t, std::string)> memoryCheckpoint;
};

struct TopologyEncodeInput {
    TopologyEncodeData data;
    TopologyEncodeExecutionParams execution;
    TopologyEncodeRuntime runtime;
    TopologyEncodeContext context;
};

struct TopologyEncodeResult {
    TopoStorageParams topo;
    std::array<int, 3> structuredAxisSize{0, 0, 0};
    std::shared_ptr<bytestore::IByteSource> transferCache;
};

inline void InvokeConnectivityProgress(
    const TopologyEncodeInput& input,
    const double normalized) {
    callback::InvokeProgress(input.context.progressCallback, normalized);
}

inline void RecordTopologyPathBudgetEstimate(
    const TopologyEncodeInput& input,
    const topology::TopologyEncodePath& path,
    const char* scope) {
    if (input.context.memoryCheckpoint) {
        input.context.memoryCheckpoint(
            "topology.work_budget.requested",
            topology::TopologyWorkBudget::Estimate(path, &input.execution.resourceBudget),
            scope);
    }
}

inline bool BuildOrderedTopologyCellRanges(
    const OrderedTopologyCellSource& source,
    const std::size_t cellElementCount,
    std::vector<OrderedTopologyCellRangeSource>& ranges,
    std::vector<TopologyConnectivityBlockLayoutParams>& layouts,
    std::string* error = nullptr) {
    if (cellElementCount == 0u) {
        return validation::AssignError(error, "topology cell block size is zero");
    }
    ranges.clear();
    layouts.clear();
    const auto blockCount = source.CellCount() == 0u
        ? 0u
        : 1u + (source.CellCount() - 1u) / cellElementCount;
    ranges.reserve(blockCount);
    layouts.reserve(blockCount);
    for (std::size_t firstCell = 0u; firstCell < source.CellCount(); firstCell += cellElementCount) {
        const auto currentCellCount = std::min(cellElementCount, source.CellCount() - firstCell);
        ranges.push_back(OrderedTopologyCellRangeSource{
            .source = &source,
            .firstCell = firstCell,
            .cellCount = currentCellCount,
        });
        layouts.push_back(TopologyConnectivityBlockLayoutParams{
            .cellOffset = firstCell,
            .cellCount = currentCellCount,
        });
    }
    return true;
}

struct TopologyBlockEncodeArtifact {
    ConnectivityTopologyEncodedMetadata metadata;
    std::size_t connectivityCount{0u};
    std::shared_ptr<bytestore::IByteSource> payload;
};

inline bool WriteEncodedStream(
    IConnectivityTopologyEncodedStreamSink& sink,
    const ConnectivityTopologyStreamKind kind,
    const std::span<const std::uint8_t> bytes,
    std::string* error = nullptr) {
    return sink.BeginStream(kind, error) &&
        (bytes.empty() || sink.WriteStreamBytes(kind, bytes, error)) &&
        sink.EndStream(kind, error);
}

inline bool EncodeTopologyBlock(
    const OrderedTopologyCellRangeSource& range,
    const std::size_t pointCount,
    const int fixedCellSize,
    const TopologyEncodeInput& input,
    TopologyBlockEncodeArtifact& artifact,
    std::string* error = nullptr) {
    std::vector<IndexType> connectivity;
    if (fixedCellSize > 0) {
        std::size_t reserveCount = 0u;
        if (!validation::CheckedMulSizeT(
                range.cellCount,
                static_cast<std::size_t>(fixedCellSize),
                reserveCount,
                "topology block connectivity reservation",
                error)) {
            return false;
        }
        connectivity.reserve(reserveCount);
    } else if (range.source->CellCount() != 0u) {
        const auto averageCellSize = std::max<std::size_t>(
            range.source->ConnectivityCount() / range.source->CellCount(),
            1u);
        connectivity.reserve(validation::SaturatingMulSizeT(range.cellCount, averageCellSize));
    }

    std::vector<IndexType> cellSizes;
    std::vector<IndexType> cellTypes;
    std::vector<std::uint16_t> cellPolynomialOrders;
    if (fixedCellSize <= 0) {
        cellSizes.reserve(range.cellCount);
    }
    if (range.HasCellTypes()) {
        cellTypes.reserve(range.cellCount);
    }
    if (range.HasCellPolynomialOrders()) {
        cellPolynomialOrders.reserve(range.cellCount);
    }

    std::vector<IndexType> cell;
    for (std::size_t localCell = 0u; localCell < range.cellCount; ++localCell) {
        IndexType cellType = 0u;
        std::uint16_t cellPolynomialOrder = 0u;
        range.source->ReadCell(
            range.firstCell + localCell,
            cell,
            range.HasCellTypes() ? &cellType : nullptr,
            range.HasCellPolynomialOrders() ? &cellPolynomialOrder : nullptr);
        if (cell.size() > static_cast<std::size_t>(std::numeric_limits<IndexType>::max())) {
            return validation::AssignError(error, "topology cell size exceeds index capacity");
        }
        connectivity.insert(connectivity.end(), cell.begin(), cell.end());
        if (fixedCellSize <= 0) {
            cellSizes.push_back(static_cast<IndexType>(cell.size()));
        }
        if (range.HasCellTypes()) {
            cellTypes.push_back(cellType);
        }
        if (range.HasCellPolynomialOrders()) {
            cellPolynomialOrders.push_back(cellPolynomialOrder);
        }
    }

    std::vector<std::uint8_t> connectivityBytes;
    std::vector<std::uint8_t> cellSizeBytes;
    std::vector<std::uint8_t> cellPolynomialOrderBytes;
    std::vector<std::uint8_t> cellTypeBytes;
    if (!blockcodec::EncodeConnectivity(
            connectivity,
            cellSizes,
            range.cellCount,
            fixedCellSize,
            pointCount,
            connectivityBytes,
            error,
            true) ||
        !blockcodec::EncodeUnsignedSequence<IndexType>(cellSizes, cellSizeBytes, error) ||
        !blockcodec::EncodeUnsignedSequence<std::uint16_t>(
            cellPolynomialOrders,
            cellPolynomialOrderBytes,
            error) ||
        !blockcodec::EncodeUnsignedSequence<IndexType>(cellTypes, cellTypeBytes, error)) {
        return false;
    }

    auto spooler = topology::MakeConnectivityTopologyStreamSpooler(
        input.runtime.byteStoreSession,
        input.execution.resourceBudget.TopologyEncodeTransferCacheStorageMode() == EncodeStorageMode::Memory,
        error);
    if (!spooler.HasAllWriters() ||
        !WriteEncodedStream(
            spooler,
            ConnectivityTopologyStreamKind::Connectivity,
            connectivityBytes,
            error) ||
        !WriteEncodedStream(spooler, ConnectivityTopologyStreamKind::CellSize, cellSizeBytes, error) ||
        !WriteEncodedStream(
            spooler,
            ConnectivityTopologyStreamKind::CellPolynomialOrder,
            cellPolynomialOrderBytes,
            error) ||
        !WriteEncodedStream(spooler, ConnectivityTopologyStreamKind::CellType, cellTypeBytes, error)) {
        return false;
    }

    artifact.metadata.connectivityByteCount = connectivityBytes.size();
    artifact.metadata.cellSizeByteCount = cellSizeBytes.size();
    artifact.metadata.cellPolynomialOrderByteCount = cellPolynomialOrderBytes.size();
    artifact.metadata.cellTypeByteCount = cellTypeBytes.size();
    artifact.connectivityCount = connectivity.size();
    artifact.payload = topology::BuildTopologyTransferCache(spooler, error);
    return artifact.payload != nullptr;
}

inline bool EncodeTopologyToTransferCache(
    const TopologyEncodeInput& input,
    TopologyEncodeResult& result,
    std::string* error = nullptr) {
    result = {};
    TopologyInputDescriptor descriptor;
    if (!input.data.adapter.DescribeTopology(descriptor, error)) {
        return false;
    }
    auto& topo = result.topo;
    topo.cellCount = descriptor.cellCount;
    topo.isStructured = false;
    topo.isPolyhedron = false;
    topo.hasCellTypes = false;
    topo.fixedCellSize = 0;

    int axisSize[3]{0, 0, 0};
    if (descriptor.structured) {
        if (!input.data.adapter.GetStructuredAxisSize(axisSize)) {
            return validation::AssignError(error, "structured topology descriptor is missing axis size");
        }
        topo.isStructured = true;
        result.structuredAxisSize = {axisSize[0], axisSize[1], axisSize[2]};
        RecordTopologyPathBudgetEstimate(
            input,
            topology::MakeStructuredTopologyEncodePath(descriptor),
            "structured");
        result.transferCache = std::make_shared<bytestore::VectorByteSource>(std::vector<std::uint8_t>{});
        return true;
    }
    if (descriptor.cellCount == 0u) {
        RecordTopologyPathBudgetEstimate(
            input,
            topology::MakeEmptyTopologyEncodePath(descriptor),
            "empty");
        result.transferCache = std::make_shared<bytestore::VectorByteSource>(std::vector<std::uint8_t>{});
        return true;
    }
    if (descriptor.polyhedron) {
        return validation::AssignError(error, "polyhedron topology requires the polyhedron topology encoder");
    }

    const auto cellCount = descriptor.cellCount;
    const auto pointCount = descriptor.pointCount;
    TopologyView topologyView;
    const auto hasTopologyView = input.data.adapter.BuildTopologyView(topologyView);
    const auto* indices =
        hasTopologyView &&
            topologyView.connectivity.indexType == IndexValueType::UInt32 &&
            topologyView.connectivity.IsCompact()
        ? static_cast<const IndexType*>(topologyView.connectivity.data)
        : input.data.adapter.GetCellIdBufferPtr();
    const auto* offsets =
        hasTopologyView &&
            !topologyView.fixedCellSizeEnabled &&
            topologyView.offsets.indexType == IndexValueType::UInt32 &&
            topologyView.offsets.IsCompact()
        ? static_cast<const IndexType*>(topologyView.offsets.data)
        : input.data.adapter.GetCellIdOffsetPtr();
    const auto* cellTypes =
        hasTopologyView &&
            topologyView.cellTypes.scalarType == ScalarType::UInt32 &&
            topologyView.cellTypes.IsCompact()
        ? static_cast<const IndexType*>(topologyView.cellTypes.data)
        : input.data.adapter.GetCellTypesPtr();
    const auto* cellPolynomialOrders =
        hasTopologyView &&
            topologyView.cellPolynomialOrders.scalarType == ScalarType::UInt16 &&
            topologyView.cellPolynomialOrders.IsCompact()
        ? static_cast<const std::uint16_t*>(topologyView.cellPolynomialOrders.data)
        : input.data.adapter.GetCellPolynomialOrdersPtr();
    const auto hasCellTypeView = hasTopologyView &&
        topologyView.cellTypes.tupleCount == cellCount &&
        topologyView.cellTypes.IsValid();
    const auto hasCellPolynomialOrderView = hasTopologyView &&
        topologyView.cellPolynomialOrders.tupleCount == cellCount &&
        topologyView.cellPolynomialOrders.IsValid();
    const auto hasCellTypes = TopologyValueSourceProvidesStream(descriptor.cellTypes);
    const auto hasCellPolynomialOrders = TopologyValueSourceProvidesStream(descriptor.cellPolynomialOrders);
    const auto usesTopologyConnectivity =
        hasTopologyView &&
        topologyView.connectivity.indexType == IndexValueType::UInt32 &&
        topologyView.connectivity.IsCompact();
    const auto connectivityCount = usesTopologyConnectivity
        ? topologyView.connectivity.count
        : descriptor.connectivityCount;
    const auto isFixedCellSize = descriptor.cellSize == TopologyCellSizeSource::FixedCellSize;
    if (indices == nullptr || (!isFixedCellSize && offsets == nullptr)) {
        return validation::AssignError(error, "missing compact connectivity or offset stream");
    }
    if (pointCount > static_cast<std::size_t>(std::numeric_limits<IndexType>::max()) + 1u) {
        return validation::AssignError(error, "topology point count exceeds index capacity");
    }

    const auto topologyPath = topology::MakeConnectivityTopologyEncodePath(descriptor, connectivityCount);
    RecordTopologyPathBudgetEstimate(input, topologyPath, "connectivity");
    topo.fixedCellSize = isFixedCellSize
        ? static_cast<int>(std::max(descriptor.fixedCellSize, 0))
        : 0;
    topo.cellBufferSize = connectivityCount;
    topo.hasCellTypes = hasCellTypes;
    topo.cellTypeCount = hasCellTypes ? cellCount : 0u;

    if (topo.fixedCellSize > 0) {
        std::size_t expectedConnectivityCount = 0u;
        if (!validation::CheckedMulSizeT(
                cellCount,
                static_cast<std::size_t>(topo.fixedCellSize),
                expectedConnectivityCount,
                "topology fixed connectivity count",
                error) ||
            expectedConnectivityCount != connectivityCount) {
            return validation::AssignError(
                error,
                "topology fixed cell size does not match connectivity count");
        }
    }

    OrderedTopologyCellSource orderedCells;
    orderedCells.adapter = &input.data.adapter;
    orderedCells.indices = indices;
    orderedCells.offsets = offsets;
    orderedCells.cellTypes = cellTypes;
    orderedCells.cellPolynomialOrders = cellPolynomialOrders;
    orderedCells.cellTypesView = hasCellTypeView ? topologyView.cellTypes : NumericArrayView{};
    orderedCells.cellPolynomialOrdersView = hasCellPolynomialOrderView
        ? topologyView.cellPolynomialOrders
        : NumericArrayView{};
    orderedCells.pointCount = pointCount;
    orderedCells.cellCount = cellCount;
    orderedCells.connectivityCount = connectivityCount;
    orderedCells.fixedCellSize = topo.fixedCellSize;
    orderedCells.inversePointRemap = input.data.pointInverseOrderSource.Provider();
    orderedCells.cellOrderProvider = input.data.cellOrderSource.Provider();
    orderedCells.hasCellTypes = hasCellTypes;
    orderedCells.hasCellPolynomialOrders = hasCellPolynomialOrders;

    std::vector<IndexType> inversePointRemapValues;
    if (orderedCells.inversePointRemap != nullptr && !orderedCells.inversePointRemap->IsIdentity()) {
        if (orderedCells.inversePointRemap->Size() != pointCount ||
            !orderedCells.inversePointRemap->ReadRange(
                0u,
                pointCount,
                inversePointRemapValues,
                error)) {
            return error != nullptr && !error->empty()
                ? false
                : validation::AssignError(error, "topology inverse point remap is invalid");
        }
        orderedCells.inversePointRemapValues = inversePointRemapValues;
    }
    std::vector<IndexType> cellOrderValues;
    if (orderedCells.cellOrderProvider != nullptr && !orderedCells.cellOrderProvider->IsIdentity()) {
        if (orderedCells.cellOrderProvider->Size() != cellCount ||
            !orderedCells.cellOrderProvider->ReadRange(0u, cellCount, cellOrderValues, error)) {
            return error != nullptr && !error->empty()
                ? false
                : validation::AssignError(error, "topology cell order is invalid");
        }
        orderedCells.cellOrderValues = cellOrderValues;
    }
    InvokeConnectivityProgress(input, 0.30);

    std::vector<OrderedTopologyCellRangeSource> blockRanges;
    std::vector<TopologyConnectivityBlockLayoutParams> blockLayouts;
    if (!BuildOrderedTopologyCellRanges(
            orderedCells,
            std::max<std::size_t>(input.execution.cellElementCount, 1u),
            blockRanges,
            blockLayouts,
            error)) {
        return false;
    }
    InvokeConnectivityProgress(input, 0.35);

    std::vector<TopologyBlockEncodeArtifact> artifacts(blockRanges.size());
    std::atomic<bool> failed{false};
    std::mutex errorMutex;
    std::string blockError;
    const auto encodeBlocks = [&](const std::size_t beginBlock, const std::size_t endBlock) {
        for (std::size_t blockIndex = beginBlock;
             blockIndex < endBlock && !failed.load(std::memory_order_acquire);
             ++blockIndex) {
            std::string localError;
            try {
                if (!EncodeTopologyBlock(
                        blockRanges[blockIndex],
                        pointCount,
                        topo.fixedCellSize,
                        input,
                        artifacts[blockIndex],
                        &localError)) {
                    if (localError.empty()) {
                        localError = "topology block encoder returned failure";
                    }
                }
            } catch (const std::exception& exception) {
                localError = std::string("topology block encoder raised an exception: ") + exception.what();
            } catch (...) {
                localError = "topology block encoder raised an unknown exception";
            }
            if (!localError.empty()) {
                failed.store(true, std::memory_order_release);
                std::lock_guard<std::mutex> lock(errorMutex);
                if (blockError.empty()) {
                    blockError = "topology block " + std::to_string(blockIndex) +
                        " encode failed: " + localError;
                }
            }
        }
    };
    ParallelForChunksAllowNested(
        0u,
        blockRanges.size(),
        encodeBlocks,
        input.execution.parallelTaskRunner,
        input.execution.workerCount);
    if (failed.load(std::memory_order_acquire)) {
        return validation::AssignError(error, blockError);
    }
    InvokeConnectivityProgress(input, 0.85);

    auto transferCache = std::make_shared<bytestore::SegmentedBinaryObject>(
        std::vector<bytestore::SegmentedBinaryObject::Segment>{},
        bytestore::ByteSourceConsumptionMode::OneShot);
    auto& aggregate = topo.connectivityLayout;
    std::size_t connectivityOffset = 0u;
    for (std::size_t blockIndex = 0u; blockIndex < artifacts.size(); ++blockIndex) {
        auto& layout = blockLayouts[blockIndex];
        const auto& artifact = artifacts[blockIndex];
        layout.connectivityOffset = connectivityOffset;
        layout.connectivityCount = artifact.connectivityCount;
        layout.connectivityByteCount = artifact.metadata.connectivityByteCount;
        layout.cellSizeByteCount = artifact.metadata.cellSizeByteCount;
        layout.cellPolynomialOrderByteCount = artifact.metadata.cellPolynomialOrderByteCount;
        layout.cellTypeByteCount = artifact.metadata.cellTypeByteCount;
        if (!validation::CheckedAddSizeT(
                connectivityOffset,
                artifact.connectivityCount,
                connectivityOffset,
                "topology block connectivity offset",
                error)) {
            return false;
        }
        aggregate.connectivityByteCount += layout.connectivityByteCount;
        aggregate.cellSizeByteCount += layout.cellSizeByteCount;
        aggregate.cellPolynomialOrderByteCount += layout.cellPolynomialOrderByteCount;
        aggregate.cellTypeByteCount += layout.cellTypeByteCount;
        if (!transferCache->AddSegment(std::move(artifacts[blockIndex].payload), error)) {
            return false;
        }
    }
    if (connectivityOffset != connectivityCount) {
        return validation::AssignError(
            error,
            "topology block connectivity total does not match the source");
    }
    aggregate.blockLayouts = std::move(blockLayouts);
    result.transferCache = std::move(transferCache);
    topo.binaryCount = result.transferCache->ByteSizeHint();
    InvokeConnectivityProgress(input, 0.98);
    return true;
}

} // namespace datacodec::topocodec

#endif
