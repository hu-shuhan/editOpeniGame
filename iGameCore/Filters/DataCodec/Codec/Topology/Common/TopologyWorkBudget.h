#ifndef DATACODEC_CODEC_TOPOLOGY_COMMON_TOPOLOGYWORKBUDGET_H
#define DATACODEC_CODEC_TOPOLOGY_COMMON_TOPOLOGYWORKBUDGET_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Storage/ByteIO/Window/WindowRuntimeParams.h"
#include "DataCodec/Codec/Topology/Common/TopologyEncodePath.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstdint>
namespace datacodec::topology {

class TopologyWorkBudget final {
public:
    [[nodiscard]] static std::uint64_t EstimateConnectivity(
        const TopologyInputDescriptor& descriptor,
        const std::size_t connectivityCount) noexcept {
        return Estimate(MakeConnectivityTopologyEncodePath(descriptor, connectivityCount));
    }

    [[nodiscard]] static std::uint64_t Estimate(
        const TopologyEncodePath& path) noexcept {
        switch (path.family) {
            case TopologyEncodeFamily::Structured:
            case TopologyEncodeFamily::Empty:
                return 1u;
            case TopologyEncodeFamily::Connectivity:
                return EstimateConnectivityPath(path);
            case TopologyEncodeFamily::Polyhedron:
                return EstimatePolyhedronPath(path);
        }
        return 1u;
    }

    [[nodiscard]] static std::uint64_t EstimatePolyhedronAdapter(
        const IEncodeAdapter& adapter) noexcept {
        return Estimate(MakePolyhedronAdapterTopologyEncodePath(adapter));
    }

private:
    [[nodiscard]] static std::uint64_t EstimateConnectivityPath(
        const TopologyEncodePath& path) noexcept {
        const auto& descriptor = path.descriptor;
        const auto cellCount = static_cast<std::uint64_t>(descriptor.cellCount);
        const auto indexBytes = validation::SaturatingMulU64(
            static_cast<std::uint64_t>(path.connectivityCount),
            static_cast<std::uint64_t>(sizeof(IndexType)));
        const auto offsetBytes = descriptor.cellSize == TopologyCellSizeSource::FixedCellSize
            ? 0u
            : validation::SaturatingMulU64(
                  validation::SaturatingAddU64(cellCount, 1u),
                  static_cast<std::uint64_t>(sizeof(IndexType)));
        const auto cellTypeBytes = TopologyValueSourceProvidesStream(descriptor.cellTypes)
            ? validation::SaturatingMulU64(cellCount, static_cast<std::uint64_t>(sizeof(IndexType)))
            : 0u;
        const auto polynomialOrderBytes = TopologyValueSourceProvidesStream(descriptor.cellPolynomialOrders)
            ? validation::SaturatingMulU64(cellCount, static_cast<std::uint64_t>(sizeof(std::uint16_t)))
            : 0u;
        const auto logicalBytes = validation::SaturatingAddU64(
            validation::SaturatingAddU64(indexBytes, offsetBytes),
            validation::SaturatingAddU64(cellTypeBytes, polynomialOrderBytes));
        const auto streamBuffers = validation::SaturatingMulU64(
            static_cast<std::uint64_t>(path.streamCount),
            StreamBufferBytes());
        const auto scratchHeadroom = std::max<std::uint64_t>(
            ScratchFloorBytes(),
            validation::SaturatingMulU64(logicalBytes, ScratchPercent()) / 100u);
        return std::max<std::uint64_t>(
            1u,
            validation::SaturatingAddU64(
                validation::SaturatingAddU64(logicalBytes, scratchHeadroom),
                streamBuffers));
    }

    [[nodiscard]] static std::uint64_t EstimatePolyhedronPath(
        const TopologyEncodePath& path) noexcept {
        const auto cellCount = static_cast<std::uint64_t>(path.cellCount);
        const auto faceCount = static_cast<std::uint64_t>(path.faceCount);
        const auto pointCount = static_cast<std::uint64_t>(path.pointCount);
        const auto logicalBytes = EstimatePolyhedronAdapterLogicalBytes(
            path,
            cellCount,
            faceCount);
        const auto lookupBytes = validation::SaturatingMulU64(
            pointCount,
            static_cast<std::uint64_t>(sizeof(IndexType)));
        const auto streamBuffers = validation::SaturatingMulU64(
            static_cast<std::uint64_t>(path.streamCount),
            StreamBufferBytes());
        const auto scratchHeadroom = std::max<std::uint64_t>(
            ScratchFloorBytes(),
            validation::SaturatingAddU64(
                validation::SaturatingMulU64(logicalBytes, ScratchPercent()) / 100u,
                lookupBytes));
        return std::max<std::uint64_t>(
            1u,
            validation::SaturatingAddU64(
                validation::SaturatingAddU64(logicalBytes, scratchHeadroom),
                validation::SaturatingAddU64(faceCount, streamBuffers)));
    }

    [[nodiscard]] static std::uint64_t EstimatePolyhedronAdapterLogicalBytes(
        const TopologyEncodePath& path,
        const std::uint64_t cellCount,
        const std::uint64_t faceCount) noexcept {
        return validation::SaturatingAddU64(
            validation::SaturatingAddU64(
                validation::SaturatingMulU64(
                    static_cast<std::uint64_t>(path.cellFaceIdCount),
                    static_cast<std::uint64_t>(sizeof(IndexType))),
                validation::SaturatingMulU64(
                    static_cast<std::uint64_t>(path.faceVertexIdCount),
                    static_cast<std::uint64_t>(sizeof(IndexType)))),
            validation::SaturatingAddU64(
                validation::SaturatingMulU64(
                    validation::SaturatingAddU64(cellCount, 1u),
                    static_cast<std::uint64_t>(sizeof(IndexType))),
                validation::SaturatingMulU64(
                    validation::SaturatingAddU64(faceCount, 1u),
                    static_cast<std::uint64_t>(sizeof(IndexType)))));
    }

    [[nodiscard]] static std::uint64_t ScratchFloorBytes() noexcept {
        return 32u * kBytesPerMiB;
    }

    [[nodiscard]] static std::uint64_t ScratchPercent() noexcept {
        return 50u;
    }

    [[nodiscard]] static std::uint64_t StreamBufferBytes() noexcept {
        return 1u * kBytesPerMiB;
    }
};

} // namespace datacodec::topology

#endif
