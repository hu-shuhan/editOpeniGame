#ifndef DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYSTREAMENCODER_H
#define DATACODEC_CODEC_TOPOLOGY_POLYHEDRON_POLYHEDRONTOPOLOGYSTREAMENCODER_H

#include "DataCodec/Codec/SubCodec/SegmentedBitpackCodec.h"
#include "DataCodec/Codec/Topology/Polyhedron/PolyhedronTopologyStreamFormat.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace datacodec::polyhedron {

class IPolyhedronTopologyStreamWriter {
public:
    virtual ~IPolyhedronTopologyStreamWriter() = default;
    virtual bool WriteStreamBytes(
        PolyhedronTopologyStreamKind kind,
        std::span<const std::uint8_t> bytes,
        std::string* error) = 0;
};

struct PolyhedronTopologyStreamCellView {
    std::span<const IndexType> uniqueVertexIds;
    std::span<const IndexType> faceVertexCounts;
    std::span<const IndexType> localFaceVertexIds;
};

struct PolyhedronTopologyStreamStats {
    std::uint64_t cellCount{0};
    std::uint64_t faceCount{0};
    std::uint64_t uniqueVertexIdCount{0};
    std::uint64_t localFaceVertexIdCount{0};
};

class PolyhedronTopologyStreamEncoder {
public:
    explicit PolyhedronTopologyStreamEncoder(
        IPolyhedronTopologyStreamWriter& sink,
        const std::size_t flushThresholdBytes = 8u * 1024u * 1024u)
        : m_sink(sink),
          m_flushThresholdBytes(std::max<std::size_t>(flushThresholdBytes, 1024u)) {}

    bool AppendCell(const PolyhedronTopologyStreamCellView& cell, std::string* error = nullptr) {
        std::size_t localIndexCount = 0u;
        for (const auto faceVertexCount : cell.faceVertexCounts) {
            if (faceVertexCount < 0) {
                return validation::AssignError(
                    error,
                    "polyhedron topology stream cell has a negative face vertex count");
            }
            if (!validation::CheckedAddSizeT(
                    localIndexCount,
                    static_cast<std::size_t>(faceVertexCount),
                    localIndexCount,
                    "polyhedron topology stream cell local id count",
                    error)) {
                return false;
            }
        }
        if (localIndexCount != cell.localFaceVertexIds.size()) {
            return validation::AssignError(
                error,
                "polyhedron topology stream cell local id count does not match face vertex counts");
        }

        const auto uniqueCount = cell.uniqueVertexIds.size();
        if (!AppendVarint(
                PolyhedronTopologyStreamKind::UniqueVertexCounts,
                static_cast<std::uint64_t>(uniqueCount),
                error) ||
            !AppendVarint(
                PolyhedronTopologyStreamKind::CellFaceCounts,
                static_cast<std::uint64_t>(cell.faceVertexCounts.size()),
                error)) {
            return false;
        }
        for (const auto faceVertexCount : cell.faceVertexCounts) {
            if (!AppendVarint(
                    PolyhedronTopologyStreamKind::FaceVertexCounts,
                    static_cast<std::uint64_t>(faceVertexCount),
                    error)) {
                return false;
            }
        }
        for (const auto pointId : cell.uniqueVertexIds) {
            if (pointId < 0) {
                return validation::AssignError(error, "polyhedron topology stream cell has a negative point id");
            }
            if (!AppendVarint(
                    PolyhedronTopologyStreamKind::CellUniqueVertexIds,
                    static_cast<std::uint64_t>(pointId),
                    error)) {
                return false;
            }
        }
        if (!AppendLocalFaceVertexIds(cell, error)) {
            return false;
        }

        if (!validation::CheckedAddU64(
                m_stats.cellCount,
                1u,
                m_stats.cellCount,
                "polyhedron topology stream cell count",
                error) ||
            !validation::CheckedAddU64(
                m_stats.faceCount,
                static_cast<std::uint64_t>(cell.faceVertexCounts.size()),
                m_stats.faceCount,
                "polyhedron topology stream face count",
                error) ||
            !validation::CheckedAddU64(
                m_stats.uniqueVertexIdCount,
                static_cast<std::uint64_t>(cell.uniqueVertexIds.size()),
                m_stats.uniqueVertexIdCount,
                "polyhedron topology stream unique vertex id count",
                error) ||
            !validation::CheckedAddU64(
                m_stats.localFaceVertexIdCount,
                static_cast<std::uint64_t>(cell.localFaceVertexIds.size()),
                m_stats.localFaceVertexIdCount,
                "polyhedron topology stream local face vertex id count",
                error)) {
            return false;
        }
        return true;
    }

    bool Finish(std::string* error = nullptr) {
        if (m_localBitCursor != 0u) {
            m_buffers[StreamIndex(PolyhedronTopologyStreamKind::LocalFaceVertexIds)].push_back(m_localByte);
            m_localByte = 0u;
            m_localBitCursor = 0u;
        }
        for (std::size_t index = 0; index < m_buffers.size(); ++index) {
            if (!Flush(static_cast<PolyhedronTopologyStreamKind>(index), error)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] const PolyhedronTopologyStreamStats& Stats() const noexcept {
        return m_stats;
    }

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept {
        std::uint64_t bytes = 0u;
        for (const auto& buffer : m_buffers) {
            bytes = validation::SaturatingAddU64(bytes, static_cast<std::uint64_t>(buffer.capacity()));
        }
        return bytes;
    }

private:
    static std::size_t StreamIndex(const PolyhedronTopologyStreamKind kind) noexcept {
        return static_cast<std::size_t>(kind);
    }

    static void AppendVarUInt64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
        do {
            auto byte = static_cast<std::uint8_t>(value & 0x7fu);
            value >>= 7u;
            if (value != 0u) {
                byte |= 0x80u;
            }
            bytes.push_back(byte);
        } while (value != 0u);
    }

    bool AppendVarint(
        const PolyhedronTopologyStreamKind kind,
        const std::uint64_t value,
        std::string* error) {
        auto& buffer = m_buffers[StreamIndex(kind)];
        AppendVarUInt64(buffer, value);
        if (buffer.size() >= m_flushThresholdBytes) {
            return Flush(kind, error);
        }
        return true;
    }

    bool AppendLocalFaceVertexIds(
        const PolyhedronTopologyStreamCellView& cell,
        std::string* error) {
        const auto bitWidth = codec::RequiredBitWidthForExclusiveUpperBound(
            static_cast<IndexType>(cell.uniqueVertexIds.size()));
        for (const auto localId : cell.localFaceVertexIds) {
            if (localId < 0 ||
                static_cast<std::size_t>(localId) >= cell.uniqueVertexIds.size()) {
                return validation::AssignError(error, "polyhedron local face vertex id exceeds unique vertex count");
            }
            if (bitWidth == 0u) {
                continue;
            }
            for (std::uint8_t bit = 0; bit < bitWidth; ++bit) {
                if (((localId >> bit) & 1u) != 0u) {
                    m_localByte |= static_cast<std::uint8_t>(1u << m_localBitCursor);
                }
                ++m_localBitCursor;
                if (m_localBitCursor == 8u) {
                    auto& buffer = m_buffers[StreamIndex(PolyhedronTopologyStreamKind::LocalFaceVertexIds)];
                    buffer.push_back(m_localByte);
                    m_localByte = 0u;
                    m_localBitCursor = 0u;
                    if (buffer.size() >= m_flushThresholdBytes &&
                        !Flush(PolyhedronTopologyStreamKind::LocalFaceVertexIds, error)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool Flush(const PolyhedronTopologyStreamKind kind, std::string* error) {
        auto& buffer = m_buffers[StreamIndex(kind)];
        if (buffer.empty()) {
            return true;
        }
        if (!m_sink.WriteStreamBytes(
                kind,
                std::span<const std::uint8_t>(buffer.data(), buffer.size()),
                error)) {
            return false;
        }
        buffer.clear();
        return true;
    }

    IPolyhedronTopologyStreamWriter& m_sink;
    std::size_t m_flushThresholdBytes{8u * 1024u * 1024u};
    std::array<std::vector<std::uint8_t>, 5> m_buffers{};
    PolyhedronTopologyStreamStats m_stats;
    std::uint8_t m_localByte{0};
    std::uint8_t m_localBitCursor{0};
};

} // namespace datacodec::polyhedron

#endif
