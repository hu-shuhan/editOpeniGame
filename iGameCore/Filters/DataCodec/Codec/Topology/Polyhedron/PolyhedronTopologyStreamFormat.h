#ifndef DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYSTREAMFORMAT_H
#define DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYSTREAMFORMAT_H

#include "DataCodec/Common/DataCodecTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
namespace datacodec::polyhedron {

enum class PolyhedronTopologyStreamKind : std::uint8_t {
    UniqueVertexCounts = 0,
    CellUniqueVertexIds = 1,
    CellFaceCounts = 2,
    FaceVertexCounts = 3,
    LocalFaceVertexIds = 4,
};

enum class PolyhedronTopologyStreamCodec : std::uint16_t {
    // 保留 v1 wire tag，0 曾用于已移除的 Raw 编码
    Varint = 1,
    SegmentedBitpack = 2,
};

inline const char* PolyhedronTopologyStreamKindName(const PolyhedronTopologyStreamKind kind) {
    switch (kind) {
        case PolyhedronTopologyStreamKind::UniqueVertexCounts:
            return "UniqueVertexCounts";
        case PolyhedronTopologyStreamKind::CellUniqueVertexIds:
            return "CellUniqueVertexIds";
        case PolyhedronTopologyStreamKind::CellFaceCounts:
            return "CellFaceCounts";
        case PolyhedronTopologyStreamKind::FaceVertexCounts:
            return "FaceVertexCounts";
        case PolyhedronTopologyStreamKind::LocalFaceVertexIds:
            return "LocalFaceVertexIds";
    }
    return "Unknown";
}

struct PolyhedronTopologyStreamHeader {
    std::uint16_t flags{0};
    std::uint32_t streamCount{0};
    std::uint64_t cellCount{0};
    std::uint64_t faceCount{0};
    std::uint64_t uniqueVertexIdCount{0};
    std::uint64_t localFaceVertexIdCount{0};
};

struct PolyhedronTopologyStreamSchedule {
    PolyhedronTopologyStreamKind kind{PolyhedronTopologyStreamKind::UniqueVertexCounts};
    PolyhedronTopologyStreamCodec codec{PolyhedronTopologyStreamCodec::Varint};
    std::uint16_t flags{0};
    std::uint16_t reserved{0};
    std::uint32_t meshIndexPadding{0};
    std::uint64_t elementCount{0};
    std::uint64_t auxiliaryStreamByteSize{0};
    std::uint64_t streamByteSize{0};
};

inline constexpr std::uint16_t kPolyhedronTopologyStreamFlagHasAuxBytes = 1u << 0;

inline constexpr std::array<PolyhedronTopologyStreamKind, 5> kStatefulPolyhedronTopologyStreamOrder{
    PolyhedronTopologyStreamKind::UniqueVertexCounts,
    PolyhedronTopologyStreamKind::CellFaceCounts,
    PolyhedronTopologyStreamKind::FaceVertexCounts,
    PolyhedronTopologyStreamKind::CellUniqueVertexIds,
    PolyhedronTopologyStreamKind::LocalFaceVertexIds,
};

inline std::size_t StatefulPolyhedronTopologyStreamIndex(const PolyhedronTopologyStreamKind kind) noexcept {
    for (std::size_t index = 0; index < kStatefulPolyhedronTopologyStreamOrder.size(); ++index) {
        if (kStatefulPolyhedronTopologyStreamOrder[index] == kind) {
            return index;
        }
    }
    return kStatefulPolyhedronTopologyStreamOrder.size();
}

} // namespace datacodec::polyhedron

#endif
