#ifndef DATACODEC_CODEC_TOPOLOGY_COMMON_TOPOLOGYENCODEPATH_H
#define DATACODEC_CODEC_TOPOLOGY_COMMON_TOPOLOGYENCODEPATH_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"

#include <cstddef>
#include <cstdint>
namespace datacodec::topology {

enum class TopologyEncodeFamily : std::uint8_t {
    Empty = 0,
    Structured = 1,
    Connectivity = 2,
    Polyhedron = 3,
};

struct TopologyEncodePath {
    TopologyEncodeFamily family{TopologyEncodeFamily::Empty};
    TopologyInputDescriptor descriptor;
    std::size_t connectivityCount{0};
    std::size_t pointCount{0};
    std::size_t cellCount{0};
    std::size_t faceCount{0};
    std::size_t cellFaceIdCount{0};
    std::size_t faceVertexIdCount{0};
    std::uint32_t streamCount{0};
};

[[nodiscard]] inline TopologyEncodePath MakeEmptyTopologyEncodePath(
    const TopologyInputDescriptor& descriptor) noexcept {
    TopologyEncodePath path;
    path.family = TopologyEncodeFamily::Empty;
    path.descriptor = descriptor;
    path.pointCount = descriptor.pointCount;
    path.cellCount = descriptor.cellCount;
    return path;
}

[[nodiscard]] inline TopologyEncodePath MakeStructuredTopologyEncodePath(
    const TopologyInputDescriptor& descriptor) noexcept {
    TopologyEncodePath path = MakeEmptyTopologyEncodePath(descriptor);
    path.family = TopologyEncodeFamily::Structured;
    return path;
}

[[nodiscard]] inline TopologyEncodePath MakeConnectivityTopologyEncodePath(
    const TopologyInputDescriptor& descriptor,
    const std::size_t connectivityCount) noexcept {
    TopologyEncodePath path;
    path.family = TopologyEncodeFamily::Connectivity;
    path.descriptor = descriptor;
    path.connectivityCount = connectivityCount;
    path.pointCount = descriptor.pointCount;
    path.cellCount = descriptor.cellCount;
    path.streamCount = 4u;
    return path;
}

[[nodiscard]] inline TopologyEncodePath MakePolyhedronAdapterTopologyEncodePath(
    const IEncodeAdapter& adapter) noexcept {
    TopologyEncodePath path;
    path.family = TopologyEncodeFamily::Polyhedron;
    path.pointCount = adapter.GetNumberOfPoints();
    path.cellCount = adapter.GetNumberOfCells();
    path.faceCount = adapter.GetNumberOfFaces();
    path.cellFaceIdCount = adapter.GetCellFaceBufferSize();
    path.faceVertexIdCount = adapter.GetFaceIdBufferSize();
    path.streamCount = 5u;
    return path;
}

} // namespace datacodec::topology

#endif
