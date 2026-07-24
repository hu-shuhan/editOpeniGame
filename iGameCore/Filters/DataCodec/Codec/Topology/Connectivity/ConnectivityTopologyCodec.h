#ifndef DATACODEC_CODEC_TOPOLOGY_CONNECTIVITY_CONNECTIVITYTOPOLOGYCODEC_H
#define DATACODEC_CODEC_TOPOLOGY_CONNECTIVITY_CONNECTIVITYTOPOLOGYCODEC_H

#include "DataCodec/Codec/SubCodec/CanonicalHuffmanCodec.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace datacodec::topocodec::blockcodec {

inline constexpr std::array<std::uint8_t, 4u> kTopologyGrammarMagic{{'U', 'I', 'T', 'P'}};
inline constexpr std::uint8_t kTopologyGrammarVersion = 1u;
inline constexpr std::uint8_t kTopologyGrammarIndexWidth = sizeof(IndexType);
inline constexpr std::size_t kGeneralVertexFifoLimit = 12u;
inline constexpr std::size_t kSeedVertexFifoLimit = 14u;
inline constexpr std::size_t kSeedEdgeFifoLimit = 15u;
inline constexpr std::uint16_t kSeedAuxEventMask = 0x100u;

enum class SymbolStreamCodec : std::uint8_t {
    NibblePacked = 0u,
    CanonicalHuffman = 1u,
    RawBytes = 2u,
};

struct TopologyEncodeStatsSnapshot {
    std::uint64_t blockCount{0u};
    std::uint64_t cellCount{0u};
    std::uint64_t connectivityCount{0u};
    std::uint64_t seedEdgeHitCount{0u};
    std::uint64_t seedEdgeMissCount{0u};
    std::uint64_t mainStreamBytes{0u};
    std::uint64_t seedLeadStreamBytes{0u};
    std::uint64_t seedOrderBytes{0u};
    std::uint64_t residualBytes{0u};
    std::uint64_t seedAuxSymbolCount{0u};
    std::uint64_t seedAuxStreamBytes{0u};
    std::uint64_t payloadBytes{0u};
};

struct TopologyEncodeStatsAccumulator {
    std::atomic<std::uint64_t> blockCount{0u};
    std::atomic<std::uint64_t> cellCount{0u};
    std::atomic<std::uint64_t> connectivityCount{0u};
    std::atomic<std::uint64_t> seedEdgeHitCount{0u};
    std::atomic<std::uint64_t> seedEdgeMissCount{0u};
    std::atomic<std::uint64_t> mainStreamBytes{0u};
    std::atomic<std::uint64_t> seedLeadStreamBytes{0u};
    std::atomic<std::uint64_t> seedOrderBytes{0u};
    std::atomic<std::uint64_t> residualBytes{0u};
    std::atomic<std::uint64_t> seedAuxSymbolCount{0u};
    std::atomic<std::uint64_t> seedAuxStreamBytes{0u};
    std::atomic<std::uint64_t> payloadBytes{0u};
};

inline TopologyEncodeStatsAccumulator g_topologyEncodeStats;

inline void ResetTopologyEncodeStats() noexcept {
    g_topologyEncodeStats.blockCount.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.cellCount.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.connectivityCount.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.seedEdgeHitCount.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.seedEdgeMissCount.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.mainStreamBytes.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.seedLeadStreamBytes.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.seedOrderBytes.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.residualBytes.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.seedAuxSymbolCount.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.seedAuxStreamBytes.store(0u, std::memory_order_relaxed);
    g_topologyEncodeStats.payloadBytes.store(0u, std::memory_order_relaxed);
}

[[nodiscard]] inline TopologyEncodeStatsSnapshot SnapshotTopologyEncodeStats() noexcept {
    TopologyEncodeStatsSnapshot snapshot;
    snapshot.blockCount = g_topologyEncodeStats.blockCount.load(std::memory_order_relaxed);
    snapshot.cellCount = g_topologyEncodeStats.cellCount.load(std::memory_order_relaxed);
    snapshot.connectivityCount =
        g_topologyEncodeStats.connectivityCount.load(std::memory_order_relaxed);
    snapshot.seedEdgeHitCount =
        g_topologyEncodeStats.seedEdgeHitCount.load(std::memory_order_relaxed);
    snapshot.seedEdgeMissCount =
        g_topologyEncodeStats.seedEdgeMissCount.load(std::memory_order_relaxed);
    snapshot.mainStreamBytes =
        g_topologyEncodeStats.mainStreamBytes.load(std::memory_order_relaxed);
    snapshot.seedLeadStreamBytes =
        g_topologyEncodeStats.seedLeadStreamBytes.load(std::memory_order_relaxed);
    snapshot.seedOrderBytes =
        g_topologyEncodeStats.seedOrderBytes.load(std::memory_order_relaxed);
    snapshot.residualBytes =
        g_topologyEncodeStats.residualBytes.load(std::memory_order_relaxed);
    snapshot.seedAuxSymbolCount =
        g_topologyEncodeStats.seedAuxSymbolCount.load(std::memory_order_relaxed);
    snapshot.seedAuxStreamBytes =
        g_topologyEncodeStats.seedAuxStreamBytes.load(std::memory_order_relaxed);
    snapshot.payloadBytes =
        g_topologyEncodeStats.payloadBytes.load(std::memory_order_relaxed);
    return snapshot;
}

inline void WriteVarUInt(std::vector<std::uint8_t>& output, std::uint64_t value) {
    while (value >= 0x80u) {
        output.push_back(static_cast<std::uint8_t>(value) | 0x80u);
        value >>= 7u;
    }
    output.push_back(static_cast<std::uint8_t>(value));
}

inline bool ReadVarUInt(
    const std::span<const std::uint8_t> input,
    std::size_t& offset,
    std::uint64_t& value,
    std::string* error = nullptr) {
    value = 0u;
    for (std::uint32_t shift = 0u; shift < 64u; shift += 7u) {
        if (offset >= input.size()) {
            return validation::AssignError(error, "topology integer stream ended inside a varint");
        }
        const auto byte = input[offset++];
        const auto payload = static_cast<std::uint64_t>(byte & 0x7fu);
        if (shift == 63u && payload > 1u) {
            return validation::AssignError(error, "topology integer stream varint exceeds 64 bits");
        }
        value |= payload << shift;
        if ((byte & 0x80u) == 0u) {
            return true;
        }
    }
    return validation::AssignError(error, "topology integer stream varint is invalid");
}

template<typename TValue>
bool EncodeUnsignedSequence(
    const std::span<const TValue> values,
    std::vector<std::uint8_t>& output,
    std::string* error = nullptr) {
    static_assert(std::is_unsigned_v<TValue>);
    output.clear();
    if (values.empty()) {
        return true;
    }
    output.reserve(values.size() + 2u);
    output.push_back(0xd7u);
    output.push_back(1u);

    std::size_t index = 0u;
    while (index < values.size()) {
        std::size_t runLength = 1u;
        while (runLength < values.size() - index && values[index + runLength] == values[index]) {
            ++runLength;
        }
        if (runLength >= 3u) {
            WriteVarUInt(output, (static_cast<std::uint64_t>(runLength - 1u) << 1u) | 1u);
            WriteVarUInt(output, static_cast<std::uint64_t>(values[index]));
            index += runLength;
            continue;
        }

        const auto literalBegin = index;
        index += runLength;
        while (index < values.size()) {
            runLength = 1u;
            while (runLength < values.size() - index && values[index + runLength] == values[index]) {
                ++runLength;
            }
            if (runLength >= 3u) {
                break;
            }
            index += runLength;
        }
        const auto literalCount = index - literalBegin;
        if (literalCount == 0u || literalCount > (std::numeric_limits<std::uint64_t>::max() >> 1u)) {
            return validation::AssignError(error, "topology integer literal run is invalid");
        }
        WriteVarUInt(output, static_cast<std::uint64_t>(literalCount - 1u) << 1u);
        for (std::size_t literal = literalBegin; literal < index; ++literal) {
            WriteVarUInt(output, static_cast<std::uint64_t>(values[literal]));
        }
    }
    return true;
}

template<typename TValue>
bool DecodeUnsignedSequence(
    const std::span<const std::uint8_t> input,
    const std::size_t valueCount,
    std::vector<TValue>& output,
    std::string* error = nullptr) {
    static_assert(std::is_unsigned_v<TValue>);
    output.clear();
    if (valueCount == 0u) {
        return input.empty() || validation::AssignError(
            error,
            "topology integer stream has bytes for an empty sequence");
    }
    if (input.size() < 2u || input[0] != 0xd7u || input[1] != 1u) {
        return validation::AssignError(error, "topology integer stream header is invalid");
    }

    output.reserve(valueCount);
    std::size_t offset = 2u;
    constexpr auto maxValue = static_cast<std::uint64_t>(std::numeric_limits<TValue>::max());
    while (output.size() < valueCount) {
        std::uint64_t token = 0u;
        if (!ReadVarUInt(input, offset, token, error)) {
            return false;
        }
        const auto encodedCount = (token >> 1u) + 1u;
        if (encodedCount > static_cast<std::uint64_t>(valueCount - output.size())) {
            return validation::AssignError(error, "topology integer stream run exceeds the expected value count");
        }
        const auto count = static_cast<std::size_t>(encodedCount);
        if ((token & 1u) != 0u) {
            std::uint64_t value = 0u;
            if (!ReadVarUInt(input, offset, value, error) || value > maxValue) {
                return value > maxValue
                    ? validation::AssignError(error, "topology integer stream value exceeds its destination type")
                    : false;
            }
            output.insert(output.end(), count, static_cast<TValue>(value));
            continue;
        }
        for (std::size_t valueIndex = 0u; valueIndex < count; ++valueIndex) {
            std::uint64_t value = 0u;
            if (!ReadVarUInt(input, offset, value, error) || value > maxValue) {
                return value > maxValue
                    ? validation::AssignError(error, "topology integer stream value exceeds its destination type")
                    : false;
            }
            output.push_back(static_cast<TValue>(value));
        }
    }
    return offset == input.size() || validation::AssignError(
        error,
        "topology integer stream has trailing bytes");
}

struct EdgeKey {
    IndexType first{0u};
    IndexType second{0u};
};

struct TopologyState {
    std::uint64_t nextVertex{0u};
    IndexType deltaReference{0u};
    std::array<IndexType, kSeedVertexFifoLimit> vertexFifo{};
    std::size_t vertexFifoSize{0u};
    std::array<EdgeKey, kSeedEdgeFifoLimit> edgeFifo{};
    std::size_t edgeFifoSize{0u};
};

inline EdgeKey MakeCanonicalEdge(const IndexType first, const IndexType second) noexcept {
    return first <= second ? EdgeKey{first, second} : EdgeKey{second, first};
}

inline bool SameEdge(const EdgeKey lhs, const EdgeKey rhs) noexcept {
    return lhs.first == rhs.first && lhs.second == rhs.second;
}

inline std::size_t FindVertexFifoIndex(
    const TopologyState& state,
    const IndexType value) noexcept {
    for (std::size_t index = 0u; index < state.vertexFifoSize; ++index) {
        if (state.vertexFifo[index] == value) {
            return index;
        }
    }
    return state.vertexFifoSize;
}

inline void PushVertexFifoAt(
    TopologyState& state,
    const IndexType value,
    const std::size_t existing) noexcept {
    if (existing < state.vertexFifoSize) {
        if (existing != 0u) {
            std::memmove(
                state.vertexFifo.data() + 1u,
                state.vertexFifo.data(),
                existing * sizeof(IndexType));
        }
        state.vertexFifo[0] = value;
        return;
    }
    const auto newSize = (std::min)(state.vertexFifoSize + 1u, kSeedVertexFifoLimit);
    if (newSize > 1u) {
        std::memmove(
            state.vertexFifo.data() + 1u,
            state.vertexFifo.data(),
            (newSize - 1u) * sizeof(IndexType));
    }
    state.vertexFifo[0] = value;
    state.vertexFifoSize = newSize;
}

inline void PushVertexFifo(TopologyState& state, const IndexType value) noexcept {
    PushVertexFifoAt(state, value, FindVertexFifoIndex(state, value));
}

inline void CommitVertex(
    TopologyState& state,
    const IndexType value,
    const bool updateDeltaReference) noexcept {
    if (updateDeltaReference) {
        state.deltaReference = value;
    }
    PushVertexFifo(state, value);
}

inline void CommitVertexAt(
    TopologyState& state,
    const IndexType value,
    const bool updateDeltaReference,
    const std::size_t existing) noexcept {
    if (updateDeltaReference) {
        state.deltaReference = value;
    }
    PushVertexFifoAt(state, value, existing);
}

inline bool ReadVertexFifo(
    const TopologyState& state,
    const std::uint8_t rank,
    const std::size_t limit,
    IndexType& value,
    std::string* error = nullptr) {
    if (rank == 0u || rank > limit || rank > state.vertexFifoSize) {
        return validation::AssignError(error, "topology vertex fifo rank is invalid");
    }
    value = state.vertexFifo[static_cast<std::size_t>(rank - 1u)];
    return true;
}

inline void PushEdgeFifo(
    TopologyState& state,
    const IndexType first,
    const IndexType second) noexcept {
    const auto edge = MakeCanonicalEdge(first, second);
    std::size_t existing = state.edgeFifoSize;
    for (std::size_t index = 0u; index < state.edgeFifoSize; ++index) {
        if (SameEdge(state.edgeFifo[index], edge)) {
            existing = index;
            break;
        }
    }
    if (existing < state.edgeFifoSize) {
        if (existing != 0u) {
            std::memmove(
                state.edgeFifo.data() + 1u,
                state.edgeFifo.data(),
                existing * sizeof(EdgeKey));
        }
        state.edgeFifo[0] = edge;
        return;
    }
    const auto newSize = (std::min)(state.edgeFifoSize + 1u, kSeedEdgeFifoLimit);
    if (newSize > 1u) {
        std::memmove(
            state.edgeFifo.data() + 1u,
            state.edgeFifo.data(),
            (newSize - 1u) * sizeof(EdgeKey));
    }
    state.edgeFifo[0] = edge;
    state.edgeFifoSize = newSize;
}

inline void PushEdgeFifoAbsent(TopologyState& state, const EdgeKey edge) noexcept {
    const auto newSize = (std::min)(state.edgeFifoSize + 1u, kSeedEdgeFifoLimit);
    if (newSize > 1u) {
        std::memmove(
            state.edgeFifo.data() + 1u,
            state.edgeFifo.data(),
            (newSize - 1u) * sizeof(EdgeKey));
    }
    state.edgeFifo[0] = edge;
    state.edgeFifoSize = newSize;
}

inline int FindEdgeFifoPosition(
    const TopologyState& state,
    const EdgeKey edge) noexcept {
    for (std::size_t index = 0u; index < state.edgeFifoSize; ++index) {
        if (SameEdge(state.edgeFifo[index], edge)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

inline bool ReadEdgeFifo(
    const TopologyState& state,
    const std::uint8_t position,
    EdgeKey& edge,
    std::string* error = nullptr) {
    if (position >= state.edgeFifoSize) {
        return validation::AssignError(error, "topology seed edge fifo position is invalid");
    }
    edge = state.edgeFifo[position];
    return true;
}

inline std::uint64_t ZigZagEncode(const std::int64_t value) noexcept {
    return (static_cast<std::uint64_t>(value) << 1u) ^
        static_cast<std::uint64_t>(value >> 63u);
}

inline std::int64_t ZigZagDecode(const std::uint64_t value) noexcept {
    return static_cast<std::int64_t>(value >> 1u) ^
        -static_cast<std::int64_t>(value & 1u);
}

inline void WriteSignedDelta(
    std::vector<std::uint8_t>& output,
    const IndexType value,
    const IndexType reference) {
    const auto delta = static_cast<std::int64_t>(value) - static_cast<std::int64_t>(reference);
    WriteVarUInt(output, ZigZagEncode(delta));
}

inline bool ReadSignedDelta(
    const std::span<const std::uint8_t> input,
    std::size_t& offset,
    const IndexType reference,
    IndexType& value,
    std::string* error = nullptr) {
    std::uint64_t encoded = 0u;
    if (!ReadVarUInt(input, offset, encoded, error)) {
        return false;
    }
    const auto decoded = static_cast<std::int64_t>(reference) + ZigZagDecode(encoded);
    if (decoded < 0 || decoded > static_cast<std::int64_t>(std::numeric_limits<IndexType>::max())) {
        return validation::AssignError(error, "topology residual exceeds index capacity");
    }
    value = static_cast<IndexType>(decoded);
    return true;
}

inline void PushBit(
    std::vector<std::uint8_t>& output,
    const std::size_t bitIndex,
    const bool value) {
    if ((bitIndex & 7u) == 0u) {
        output.push_back(0u);
    }
    if (value) {
        output.back() |= static_cast<std::uint8_t>(1u << (bitIndex & 7u));
    }
}

inline bool ReadBit(
    const std::span<const std::uint8_t> input,
    const std::size_t bitIndex,
    bool& value) noexcept {
    if (bitIndex >= input.size() * 8u) {
        return false;
    }
    value = ((input[bitIndex >> 3u] >> (bitIndex & 7u)) & 1u) != 0u;
    return true;
}

class MsbBitWriter {
public:
    explicit MsbBitWriter(std::vector<std::uint8_t>& output) : m_output(output) {}

    void Write(const std::uint32_t code, const std::uint8_t bitCount) {
        for (std::uint8_t remaining = bitCount; remaining > 0u; --remaining) {
            const auto bit = static_cast<std::uint8_t>((code >> (remaining - 1u)) & 1u);
            m_current |= static_cast<std::uint8_t>(bit << (7u - m_used));
            ++m_used;
            if (m_used == 8u) {
                m_output.push_back(m_current);
                m_current = 0u;
                m_used = 0u;
            }
        }
    }

    void Finish() {
        if (m_used != 0u) {
            m_output.push_back(m_current);
            m_current = 0u;
            m_used = 0u;
        }
    }

private:
    std::vector<std::uint8_t>& m_output;
    std::uint8_t m_current{0u};
    std::uint8_t m_used{0u};
};

template <std::size_t AlphabetSize = 16u>
inline bool EncodeSymbolStream(
    const std::span<const std::uint8_t> symbols,
    std::vector<std::uint8_t>& output,
    std::string* error = nullptr) {
    static_assert(AlphabetSize > 0u && AlphabetSize <= 256u);
    output.clear();
    if (symbols.empty()) {
        return true;
    }
    std::array<std::uint64_t, AlphabetSize> counts{};
    for (const auto symbol : symbols) {
        if (static_cast<std::size_t>(symbol) >= counts.size()) {
            return validation::AssignError(error, "topology symbol exceeds the alphabet");
        }
        ++counts[symbol];
    }
    std::array<std::uint8_t, AlphabetSize> codeLengths{};
    if (!codec::BuildHuffmanCodeLengths(counts, codeLengths)) {
        return validation::AssignError(error, "failed to build topology huffman model");
    }
    std::uint64_t bitCount = 0u;
    for (std::size_t symbol = 0u; symbol < counts.size(); ++symbol) {
        bitCount += counts[symbol] * codeLengths[symbol];
    }
    const auto baselineBytes = 1u + (AlphabetSize <= 16u
        ? (symbols.size() + 1u) / 2u
        : symbols.size());
    const auto huffmanBytes = 1u + codeLengths.size() + static_cast<std::size_t>((bitCount + 7u) / 8u);
    if (huffmanBytes >= baselineBytes) {
        output.reserve(baselineBytes);
        if constexpr (AlphabetSize <= 16u) {
            output.push_back(static_cast<std::uint8_t>(SymbolStreamCodec::NibblePacked));
            for (std::size_t index = 0u; index < symbols.size(); index += 2u) {
                auto packed = static_cast<std::uint8_t>(symbols[index] << 4u);
                if (index + 1u < symbols.size()) {
                    packed |= symbols[index + 1u];
                }
                output.push_back(packed);
            }
        } else {
            output.push_back(static_cast<std::uint8_t>(SymbolStreamCodec::RawBytes));
            output.insert(output.end(), symbols.begin(), symbols.end());
        }
        return true;
    }

    output.reserve(huffmanBytes);
    output.push_back(static_cast<std::uint8_t>(SymbolStreamCodec::CanonicalHuffman));
    output.insert(output.end(), codeLengths.begin(), codeLengths.end());
    const auto codes = codec::BuildCanonicalHuffmanCodes(codeLengths);
    MsbBitWriter writer(output);
    for (const auto symbol : symbols) {
        writer.Write(codes[symbol], codeLengths[symbol]);
    }
    writer.Finish();
    return true;
}

template <std::size_t AlphabetSize = 16u>
class SymbolStreamReader {
private:
    struct LookupEntry {
        std::uint8_t symbol{0u};
        std::uint8_t bitCount{0u};
    };

    static constexpr std::uint8_t kLookupBitCount = 10u;
    static constexpr std::size_t kLookupTableSize = 1u << kLookupBitCount;

public:
    bool Reset(
        const std::span<const std::uint8_t> input,
        const std::size_t symbolCount,
        std::string* error = nullptr) {
        m_input = input;
        m_symbolCount = symbolCount;
        m_symbolIndex = 0u;
        m_bitIndex = 0u;
        m_payloadOffset = 0u;
        m_payloadByteIndex = 0u;
        m_bitBuffer = 0u;
        m_bufferedBitCount = 0u;
        m_codeLengths = {};
        m_firstCodes = {};
        m_firstSymbolOffsets = {};
        m_lengthCounts = {};
        m_symbolsByLength = {};
        m_lookupTable = {};
        if (symbolCount == 0u) {
            return input.empty() || validation::AssignError(
                error,
                "empty topology symbol stream contains bytes");
        }
        if (input.empty()) {
            return validation::AssignError(error, "topology symbol stream is empty");
        }
        const auto codecValue = input[0];
        if (codecValue > static_cast<std::uint8_t>(SymbolStreamCodec::RawBytes)) {
            return validation::AssignError(error, "topology symbol stream codec is invalid");
        }
        m_codec = static_cast<SymbolStreamCodec>(codecValue);
        if (m_codec == SymbolStreamCodec::NibblePacked) {
            if constexpr (AlphabetSize > 16u) {
                return validation::AssignError(error, "topology nibble stream alphabet is invalid");
            }
            m_payloadOffset = 1u;
            const auto expected = 1u + (symbolCount + 1u) / 2u;
            return input.size() == expected || validation::AssignError(
                error,
                "topology nibble stream length is invalid");
        }
        if (m_codec == SymbolStreamCodec::RawBytes) {
            m_payloadOffset = 1u;
            return input.size() == 1u + symbolCount || validation::AssignError(
                error,
                "topology raw symbol stream length is invalid");
        }
        if (input.size() < 1u + AlphabetSize) {
            return validation::AssignError(error, "topology huffman stream header is truncated");
        }
        std::copy_n(input.begin() + 1u, m_codeLengths.size(), m_codeLengths.begin());
        for (const auto length : m_codeLengths) {
            if (length > 32u) {
                return validation::AssignError(error, "topology huffman code length exceeds 32 bits");
            }
        }
        std::uint32_t code = 0u;
        std::uint8_t previousLength = 0u;
        std::size_t symbolOffset = 0u;
        for (std::uint8_t length = 1u; length <= 32u; ++length) {
            code <<= static_cast<std::uint8_t>(length - previousLength);
            previousLength = length;
            m_firstCodes[length] = code;
            m_firstSymbolOffsets[length] = symbolOffset;
            for (std::size_t candidate = 0u; candidate < m_codeLengths.size(); ++candidate) {
                if (m_codeLengths[candidate] != length) {
                    continue;
                }
                m_symbolsByLength[symbolOffset++] = static_cast<std::uint8_t>(candidate);
                ++m_lengthCounts[length];
                ++code;
            }
        }
        m_payloadOffset = 1u + AlphabetSize;
        return BuildLookupTable(error);
    }

    bool Read(std::uint8_t& symbol, std::string* error = nullptr) {
        if (m_symbolIndex >= m_symbolCount) {
            return validation::AssignError(error, "topology symbol stream was over-read");
        }
        if (m_codec == SymbolStreamCodec::RawBytes) {
            symbol = m_input[m_payloadOffset + m_symbolIndex];
            ++m_symbolIndex;
            return true;
        }
        if (m_codec == SymbolStreamCodec::NibblePacked) {
            const auto packed = m_input[m_payloadOffset + m_symbolIndex / 2u];
            symbol = (m_symbolIndex & 1u) == 0u
                ? static_cast<std::uint8_t>(packed >> 4u)
                : static_cast<std::uint8_t>(packed & 0x0fu);
            ++m_symbolIndex;
            return true;
        }

        RefillLookupBits();
        if (m_bufferedBitCount != 0u) {
            const auto availableBits = (std::min)(m_bufferedBitCount, kLookupBitCount);
            const auto lookupIndex = static_cast<std::size_t>(
                m_bitBuffer >> (32u - kLookupBitCount));
            const auto entry = m_lookupTable[lookupIndex];
            if (entry.bitCount != 0u && entry.bitCount <= availableBits) {
                symbol = entry.symbol;
                ConsumeBufferedBits(entry.bitCount);
                ++m_symbolIndex;
                return true;
            }
        }
        return ReadHuffmanSymbolSlow(symbol, error);
    }

    bool Finish(std::string* error = nullptr) const {
        if (m_symbolIndex != m_symbolCount) {
            return validation::AssignError(error, "topology symbol stream ended before all symbols were read");
        }
        if (m_codec == SymbolStreamCodec::NibblePacked ||
            m_codec == SymbolStreamCodec::RawBytes) {
            return true;
        }
        const auto payloadBytes = m_input.size() - m_payloadOffset;
        return (m_bitIndex + 7u) / 8u == payloadBytes || validation::AssignError(
            error,
            "topology huffman stream contains trailing bytes");
    }

private:
    bool BuildLookupTable(std::string* error) {
        for (std::uint8_t length = 1u; length <= kLookupBitCount; ++length) {
            const auto count = m_lengthCounts[length];
            const auto firstCode = m_firstCodes[length];
            const auto symbolOffset = m_firstSymbolOffsets[length];
            const auto codeLimit = static_cast<std::uint32_t>(1u << length);
            for (std::size_t rank = 0u; rank < count; ++rank) {
                const auto code = firstCode + static_cast<std::uint32_t>(rank);
                const auto symbolIndex = symbolOffset + rank;
                if (code >= codeLimit || symbolIndex >= m_symbolsByLength.size()) {
                    return validation::AssignError(error, "topology huffman lookup model is invalid");
                }
                const auto tableBegin = static_cast<std::size_t>(
                    code << (kLookupBitCount - length));
                const auto tableCount = static_cast<std::size_t>(
                    1u << (kLookupBitCount - length));
                const LookupEntry entry{
                    m_symbolsByLength[symbolIndex],
                    length,
                };
                for (std::size_t index = tableBegin; index < tableBegin + tableCount; ++index) {
                    if (m_lookupTable[index].bitCount != 0u) {
                        return validation::AssignError(error, "topology huffman lookup prefixes overlap");
                    }
                    m_lookupTable[index] = entry;
                }
            }
        }
        return true;
    }

    void RefillLookupBits() noexcept {
        const auto payloadByteCount = m_input.size() - m_payloadOffset;
        while (m_bufferedBitCount < kLookupBitCount &&
               m_payloadByteIndex < payloadByteCount) {
            const auto shift = static_cast<std::uint8_t>(
                32u - m_bufferedBitCount - 8u);
            m_bitBuffer |= static_cast<std::uint32_t>(
                m_input[m_payloadOffset + m_payloadByteIndex]) << shift;
            ++m_payloadByteIndex;
            m_bufferedBitCount = static_cast<std::uint8_t>(m_bufferedBitCount + 8u);
        }
    }

    void ConsumeBufferedBits(const std::uint8_t bitCount) noexcept {
        m_bitBuffer <<= bitCount;
        m_bufferedBitCount = static_cast<std::uint8_t>(m_bufferedBitCount - bitCount);
        m_bitIndex += bitCount;
    }

    bool ReadHuffmanBit(std::uint8_t& bit, std::string* error) {
        if (m_bufferedBitCount == 0u) {
            RefillLookupBits();
        }
        if (m_bufferedBitCount == 0u) {
            return validation::AssignError(error, "topology huffman stream ended early");
        }
        bit = static_cast<std::uint8_t>(m_bitBuffer >> 31u);
        ConsumeBufferedBits(1u);
        return true;
    }

    bool ReadHuffmanSymbolSlow(
        std::uint8_t& symbol,
        std::string* error) {
        std::uint32_t codeValue = 0u;
        for (std::uint8_t length = 1u; length <= 32u; ++length) {
            std::uint8_t bit = 0u;
            if (!ReadHuffmanBit(bit, error)) {
                return false;
            }
            codeValue = (codeValue << 1u) | bit;
            const auto firstCode = m_firstCodes[length];
            const auto lengthCount = m_lengthCounts[length];
            if (codeValue >= firstCode &&
                static_cast<std::size_t>(codeValue - firstCode) < lengthCount) {
                const auto symbolIndex =
                    m_firstSymbolOffsets[length] + static_cast<std::size_t>(codeValue - firstCode);
                symbol = m_symbolsByLength[symbolIndex];
                ++m_symbolIndex;
                return true;
            }
        }
        return validation::AssignError(error, "topology huffman code is invalid");
    }

    std::span<const std::uint8_t> m_input;
    SymbolStreamCodec m_codec{SymbolStreamCodec::NibblePacked};
    std::array<std::uint8_t, AlphabetSize> m_codeLengths{};
    std::array<std::uint32_t, 33u> m_firstCodes{};
    std::array<std::size_t, 33u> m_firstSymbolOffsets{};
    std::array<std::size_t, 33u> m_lengthCounts{};
    std::array<std::uint8_t, AlphabetSize> m_symbolsByLength{};
    std::array<LookupEntry, kLookupTableSize> m_lookupTable{};
    std::size_t m_payloadOffset{0u};
    std::size_t m_payloadByteIndex{0u};
    std::size_t m_symbolCount{0u};
    std::size_t m_symbolIndex{0u};
    std::size_t m_bitIndex{0u};
    std::uint32_t m_bitBuffer{0u};
    std::uint8_t m_bufferedBitCount{0u};
};

inline bool BuildCellOffsets(
    const std::size_t cellCount,
    const int fixedCellSize,
    const std::span<const IndexType> cellSizes,
    const std::size_t connectivityCount,
    std::vector<std::size_t>& offsets,
    std::string* error = nullptr) {
    offsets.assign(cellCount + 1u, 0u);
    if (fixedCellSize > 0) {
        const auto width = static_cast<std::size_t>(fixedCellSize);
        if (cellCount != 0u && width > std::numeric_limits<std::size_t>::max() / cellCount) {
            return validation::AssignError(error, "topology fixed cell layout overflows local size capacity");
        }
        for (std::size_t cell = 0u; cell < cellCount; ++cell) {
            offsets[cell + 1u] = offsets[cell] + width;
        }
    } else {
        if (cellSizes.size() != cellCount) {
            return validation::AssignError(error, "topology cell-size count does not match cell count");
        }
        for (std::size_t cell = 0u; cell < cellCount; ++cell) {
            const auto width = static_cast<std::size_t>(cellSizes[cell]);
            if (width > std::numeric_limits<std::size_t>::max() - offsets[cell]) {
                return validation::AssignError(error, "topology variable cell layout overflows local size capacity");
            }
            offsets[cell + 1u] = offsets[cell] + width;
        }
    }
    return offsets.back() == connectivityCount || validation::AssignError(
        error,
        "topology connectivity count does not match cell layout");
}

inline IndexType SelectInitialNextVertex(const std::span<const IndexType> connectivity) noexcept {
    if (connectivity.empty()) {
        return 0u;
    }
    const auto sampleCount = (std::min)(connectivity.size(), static_cast<std::size_t>(4096u));
    return *std::min_element(connectivity.begin(), connectivity.begin() + sampleCount);
}

inline void BuildSeedLookupTable(
    const std::array<std::uint64_t, 256u>& frequencies,
    std::array<std::uint8_t, kSeedEdgeFifoLimit>& table,
    std::size_t& entryCount) noexcept {
    table = {};
    entryCount = 0u;
    std::array<bool, 256u> selected{};
    while (entryCount < table.size()) {
        int best = -1;
        for (std::size_t value = 0u; value < frequencies.size(); ++value) {
            if (selected[value] || frequencies[value] == 0u) {
                continue;
            }
            if (best < 0 ||
                frequencies[value] > frequencies[static_cast<std::size_t>(best)] ||
                (frequencies[value] == frequencies[static_cast<std::size_t>(best)] &&
                 value < static_cast<std::size_t>(best))) {
                best = static_cast<int>(value);
            }
        }
        if (best < 0) {
            break;
        }
        selected[static_cast<std::size_t>(best)] = true;
        table[entryCount++] = static_cast<std::uint8_t>(best);
    }
}

inline bool EncodeConnectivityGrammar(
    const std::span<const IndexType> connectivity,
    const std::span<const IndexType> cellSizes,
    const std::size_t cellCount,
    const int fixedCellSize,
    const std::size_t pointCount,
    std::vector<std::uint8_t>& output,
    std::string* error = nullptr,
    const bool inputAlreadyValidated = false) {
    output.clear();
    if (!connectivity.empty() && pointCount == 0u) {
        return validation::AssignError(error, "topology connectivity references an empty point set");
    }
    std::vector<std::size_t> cellOffsets;
    if (!BuildCellOffsets(
            cellCount,
            fixedCellSize,
            cellSizes,
            connectivity.size(),
            cellOffsets,
            error)) {
        return false;
    }

    const auto initialNextVertex = SelectInitialNextVertex(connectivity);
    TopologyState state;
    state.nextVertex = initialNextVertex;
    state.deltaReference = initialNextVertex;
    std::vector<std::uint16_t> mainEvents;
    std::vector<std::uint8_t> seedLeadSymbols;
    std::vector<std::uint8_t> seedOrderBytes;
    std::vector<std::uint8_t> residualBytes;
    std::array<std::uint64_t, 256u> seedAuxFrequencies{};
    std::size_t seedOrderBitCount = 0u;
    std::uint64_t seedEdgeHitCount = 0u;
    std::uint64_t seedEdgeMissCount = 0u;
    mainEvents.reserve(connectivity.size());

    const auto encodeGeneralVertex = [&](const IndexType value,
                                         const IndexType* previousInCell,
                                         const IndexType* firstInCell) -> std::uint8_t {
        if (static_cast<std::uint64_t>(value) == state.nextVertex) {
            ++state.nextVertex;
            CommitVertex(state, value, true);
            return 0u;
        }
        const auto fifoIndex = FindVertexFifoIndex(state, value);
        if (fifoIndex < state.vertexFifoSize && fifoIndex < kGeneralVertexFifoLimit) {
            CommitVertexAt(state, value, false, fifoIndex);
            return static_cast<std::uint8_t>(fifoIndex + 1u);
        }
        const auto reference = previousInCell != nullptr ? *previousInCell : state.deltaReference;
        if (reference > 0u && value == reference - 1u) {
            CommitVertexAt(state, value, true, fifoIndex);
            return 13u;
        }
        if (reference < std::numeric_limits<IndexType>::max() && value == reference + 1u) {
            CommitVertexAt(state, value, true, fifoIndex);
            return 14u;
        }
        const auto residualReference = firstInCell != nullptr
            ? *firstInCell
            : state.deltaReference;
        WriteSignedDelta(residualBytes, value, residualReference);
        CommitVertexAt(state, value, true, fifoIndex);
        return 15u;
    };
    const auto encodeSeedLeadVertex = [&](const IndexType value) -> std::uint8_t {
        if (static_cast<std::uint64_t>(value) == state.nextVertex) {
            ++state.nextVertex;
            CommitVertex(state, value, true);
            return 0u;
        }
        const auto fifoIndex = FindVertexFifoIndex(state, value);
        if (fifoIndex < state.vertexFifoSize && fifoIndex < kGeneralVertexFifoLimit) {
            CommitVertexAt(state, value, false, fifoIndex);
            return static_cast<std::uint8_t>(fifoIndex + 1u);
        }
        const auto reference = state.deltaReference;
        if (reference > 0u && value == reference - 1u) {
            CommitVertexAt(state, value, true, fifoIndex);
            return 13u;
        }
        if (reference < std::numeric_limits<IndexType>::max() && value == reference + 1u) {
            CommitVertexAt(state, value, true, fifoIndex);
            return 14u;
        }
        WriteSignedDelta(residualBytes, value, reference);
        CommitVertexAt(state, value, true, fifoIndex);
        return 15u;
    };
    const auto encodeSeedTailVertex = [&](const IndexType value,
                                          const IndexType* firstInCell) -> std::uint8_t {
        if (static_cast<std::uint64_t>(value) == state.nextVertex) {
            ++state.nextVertex;
            CommitVertex(state, value, true);
            return 0u;
        }
        const auto fifoIndex = FindVertexFifoIndex(state, value);
        if (fifoIndex < state.vertexFifoSize) {
            CommitVertexAt(state, value, false, fifoIndex);
            return static_cast<std::uint8_t>(fifoIndex + 1u);
        }
        const auto residualReference = firstInCell != nullptr
            ? *firstInCell
            : state.deltaReference;
        WriteSignedDelta(residualBytes, value, residualReference);
        CommitVertexAt(state, value, true, fifoIndex);
        return 15u;
    };

    for (std::size_t cellIndex = 0u; cellIndex < cellCount; ++cellIndex) {
        const auto cell = connectivity.subspan(
            cellOffsets[cellIndex],
            cellOffsets[cellIndex + 1u] - cellOffsets[cellIndex]);
        if (!inputAlreadyValidated) {
            for (const auto point : cell) {
                if (static_cast<std::size_t>(point) >= pointCount) {
                    return validation::AssignError(error, "topology point index is out of range");
                }
            }
        }
        if (cell.empty()) {
            continue;
        }
        if (cell.size() == 1u) {
            mainEvents.push_back(encodeGeneralVertex(cell[0], nullptr, nullptr));
            continue;
        }
        if (cell.size() == 2u) {
            mainEvents.push_back(encodeGeneralVertex(cell[0], nullptr, nullptr));
            mainEvents.push_back(encodeGeneralVertex(cell[1], &cell[0], &cell[0]));
            PushEdgeFifo(state, cell[0], cell[1]);
            continue;
        }

        const auto seedEdge = MakeCanonicalEdge(cell[0], cell[1]);
        const auto edgePosition = FindEdgeFifoPosition(state, seedEdge);
        if (edgePosition >= 0) {
            ++seedEdgeHitCount;
            mainEvents.push_back(static_cast<std::uint16_t>(edgePosition));
            PushBit(seedOrderBytes, seedOrderBitCount++, cell[0] > cell[1]);
            mainEvents.push_back(encodeGeneralVertex(cell[2], &cell[1], &cell[0]));
            PushEdgeFifo(state, cell[1], cell[2]);
        } else {
            ++seedEdgeMissCount;
            const auto v0 = encodeSeedLeadVertex(cell[0]);
            const auto v1 = encodeSeedTailVertex(cell[1], &cell[0]);
            const auto v2 = encodeSeedTailVertex(cell[2], &cell[0]);
            const auto auxCode = static_cast<std::uint8_t>((v1 << 4u) | v2);
            mainEvents.push_back(15u);
            seedLeadSymbols.push_back(v0);
            mainEvents.push_back(static_cast<std::uint16_t>(kSeedAuxEventMask | auxCode));
            ++seedAuxFrequencies[auxCode];
            PushEdgeFifoAbsent(state, seedEdge);
            PushEdgeFifo(state, cell[1], cell[2]);
        }
        for (std::size_t local = 3u; local < cell.size(); ++local) {
            mainEvents.push_back(encodeGeneralVertex(
                cell[local],
                &cell[local - 1u],
                &cell[0]));
            PushEdgeFifo(state, cell[local - 1u], cell[local]);
        }
        PushEdgeFifo(state, cell.back(), cell.front());
    }

    std::array<std::uint8_t, kSeedEdgeFifoLimit> seedLookupTable{};
    std::size_t seedLookupEntryCount = 0u;
    BuildSeedLookupTable(seedAuxFrequencies, seedLookupTable, seedLookupEntryCount);
    std::array<int, 256u> seedLookupIndices{};
    seedLookupIndices.fill(-1);
    for (std::size_t index = 0u; index < seedLookupEntryCount; ++index) {
        seedLookupIndices[seedLookupTable[index]] = static_cast<int>(index);
    }

    std::vector<std::uint8_t> mainSymbols;
    std::vector<std::uint8_t> rawSeedAuxBytes;
    mainSymbols.reserve(mainEvents.size());
    for (const auto event : mainEvents) {
        if ((event & kSeedAuxEventMask) == 0u) {
            mainSymbols.push_back(static_cast<std::uint8_t>(event));
            continue;
        }
        const auto auxCode = static_cast<std::uint8_t>(event & 0xffu);
        const auto lookupIndex = seedLookupIndices[auxCode];
        if (lookupIndex >= 0) {
            mainSymbols.push_back(static_cast<std::uint8_t>(lookupIndex));
        } else {
            mainSymbols.push_back(15u);
            rawSeedAuxBytes.push_back(auxCode);
        }
    }

    std::vector<std::uint8_t> mainStream;
    std::vector<std::uint8_t> seedLeadStream;
    std::vector<std::uint8_t> seedAuxStream;
    if (!EncodeSymbolStream(mainSymbols, mainStream, error) ||
        !EncodeSymbolStream(seedLeadSymbols, seedLeadStream, error) ||
        !EncodeSymbolStream<256u>(rawSeedAuxBytes, seedAuxStream, error)) {
        return false;
    }

    output.reserve(
        64u + seedLookupEntryCount + mainStream.size() + seedLeadStream.size() +
        seedOrderBytes.size() + residualBytes.size() + seedAuxStream.size());
    output.insert(output.end(), kTopologyGrammarMagic.begin(), kTopologyGrammarMagic.end());
    output.push_back(kTopologyGrammarVersion);
    output.push_back(kTopologyGrammarIndexWidth);
    WriteVarUInt(output, cellCount);
    WriteVarUInt(output, connectivity.size());
    WriteVarUInt(output, initialNextVertex);
    WriteVarUInt(output, seedLookupEntryCount);
    output.insert(
        output.end(),
        seedLookupTable.begin(),
        seedLookupTable.begin() + static_cast<std::ptrdiff_t>(seedLookupEntryCount));
    WriteVarUInt(output, mainSymbols.size());
    WriteVarUInt(output, mainStream.size());
    WriteVarUInt(output, seedLeadSymbols.size());
    WriteVarUInt(output, seedLeadStream.size());
    WriteVarUInt(output, seedOrderBitCount);
    WriteVarUInt(output, seedOrderBytes.size());
    WriteVarUInt(output, residualBytes.size());
    WriteVarUInt(output, rawSeedAuxBytes.size());
    WriteVarUInt(output, seedAuxStream.size());
    output.insert(output.end(), mainStream.begin(), mainStream.end());
    output.insert(output.end(), seedLeadStream.begin(), seedLeadStream.end());
    output.insert(output.end(), seedOrderBytes.begin(), seedOrderBytes.end());
    output.insert(output.end(), residualBytes.begin(), residualBytes.end());
    output.insert(output.end(), seedAuxStream.begin(), seedAuxStream.end());

    g_topologyEncodeStats.blockCount.fetch_add(1u, std::memory_order_relaxed);
    g_topologyEncodeStats.cellCount.fetch_add(cellCount, std::memory_order_relaxed);
    g_topologyEncodeStats.connectivityCount.fetch_add(connectivity.size(), std::memory_order_relaxed);
    g_topologyEncodeStats.seedEdgeHitCount.fetch_add(seedEdgeHitCount, std::memory_order_relaxed);
    g_topologyEncodeStats.seedEdgeMissCount.fetch_add(seedEdgeMissCount, std::memory_order_relaxed);
    g_topologyEncodeStats.mainStreamBytes.fetch_add(mainStream.size(), std::memory_order_relaxed);
    g_topologyEncodeStats.seedLeadStreamBytes.fetch_add(seedLeadStream.size(), std::memory_order_relaxed);
    g_topologyEncodeStats.seedOrderBytes.fetch_add(seedOrderBytes.size(), std::memory_order_relaxed);
    g_topologyEncodeStats.residualBytes.fetch_add(residualBytes.size(), std::memory_order_relaxed);
    g_topologyEncodeStats.seedAuxSymbolCount.fetch_add(rawSeedAuxBytes.size(), std::memory_order_relaxed);
    g_topologyEncodeStats.seedAuxStreamBytes.fetch_add(seedAuxStream.size(), std::memory_order_relaxed);
    g_topologyEncodeStats.payloadBytes.fetch_add(output.size(), std::memory_order_relaxed);
    return true;
}

inline bool ReadHeaderSize(
    const std::span<const std::uint8_t> input,
    std::size_t& offset,
    std::size_t& value,
    std::string* error = nullptr) {
    std::uint64_t encoded = 0u;
    if (!ReadVarUInt(input, offset, encoded, error)) {
        return false;
    }
    if (encoded > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return validation::AssignError(error, "topology header value exceeds local size capacity");
    }
    value = static_cast<std::size_t>(encoded);
    return true;
}

inline bool TakeLane(
    const std::span<const std::uint8_t> input,
    std::size_t& offset,
    const std::size_t size,
    std::span<const std::uint8_t>& lane,
    std::string* error = nullptr) {
    if (offset > input.size() || size > input.size() - offset) {
        return validation::AssignError(error, "topology lane exceeds the encoded payload");
    }
    lane = input.subspan(offset, size);
    offset += size;
    return true;
}

inline bool DecodeConnectivityGrammar(
    const std::span<const std::uint8_t> input,
    const std::span<const IndexType> cellSizes,
    const std::size_t pointCount,
    const std::size_t cellCount,
    const std::size_t connectivityCount,
    const int fixedCellSize,
    std::vector<IndexType>& output,
    std::string* error = nullptr) {
    output.clear();
    if (input.size() < 6u) {
        return validation::AssignError(error, "topology grammar header is incomplete");
    }
    if (!std::equal(kTopologyGrammarMagic.begin(), kTopologyGrammarMagic.end(), input.begin())) {
        return validation::AssignError(error, "topology grammar format is invalid");
    }
    if (input[4] != kTopologyGrammarVersion) {
        return validation::AssignError(error, "版本不符合");
    }
    if (input[5] != kTopologyGrammarIndexWidth) {
        return validation::AssignError(error, "topology grammar index width is incompatible");
    }
    std::size_t offset = 6u;
    std::size_t encodedCellCount = 0u;
    std::size_t encodedConnectivityCount = 0u;
    std::size_t initialNextVertex = 0u;
    std::size_t seedLookupEntryCount = 0u;
    if (!ReadHeaderSize(input, offset, encodedCellCount, error) ||
        !ReadHeaderSize(input, offset, encodedConnectivityCount, error) ||
        !ReadHeaderSize(input, offset, initialNextVertex, error) ||
        !ReadHeaderSize(input, offset, seedLookupEntryCount, error)) {
        return false;
    }
    if (encodedCellCount != cellCount || encodedConnectivityCount != connectivityCount) {
        return validation::AssignError(error, "topology grammar counts do not match metadata");
    }
    if (initialNextVertex > std::numeric_limits<IndexType>::max() ||
        seedLookupEntryCount > kSeedEdgeFifoLimit ||
        seedLookupEntryCount > input.size() - offset) {
        return validation::AssignError(error, "topology grammar predictor metadata is invalid");
    }
    std::array<std::uint8_t, kSeedEdgeFifoLimit> seedLookupTable{};
    std::copy_n(input.begin() + static_cast<std::ptrdiff_t>(offset), seedLookupEntryCount, seedLookupTable.begin());
    offset += seedLookupEntryCount;

    std::size_t mainSymbolCount = 0u;
    std::size_t mainStreamSize = 0u;
    std::size_t seedLeadSymbolCount = 0u;
    std::size_t seedLeadStreamSize = 0u;
    std::size_t seedOrderBitCount = 0u;
    std::size_t seedOrderByteCount = 0u;
    std::size_t residualByteCount = 0u;
    std::size_t seedAuxSymbolCount = 0u;
    std::size_t seedAuxStreamSize = 0u;
    if (!ReadHeaderSize(input, offset, mainSymbolCount, error) ||
        !ReadHeaderSize(input, offset, mainStreamSize, error) ||
        !ReadHeaderSize(input, offset, seedLeadSymbolCount, error) ||
        !ReadHeaderSize(input, offset, seedLeadStreamSize, error) ||
        !ReadHeaderSize(input, offset, seedOrderBitCount, error) ||
        !ReadHeaderSize(input, offset, seedOrderByteCount, error) ||
        !ReadHeaderSize(input, offset, residualByteCount, error) ||
        !ReadHeaderSize(input, offset, seedAuxSymbolCount, error) ||
        !ReadHeaderSize(input, offset, seedAuxStreamSize, error)) {
        return false;
    }
    if (seedOrderByteCount != (seedOrderBitCount + 7u) / 8u) {
        return validation::AssignError(error, "topology seed-order stream length is invalid");
    }

    std::span<const std::uint8_t> mainStream;
    std::span<const std::uint8_t> seedLeadStream;
    std::span<const std::uint8_t> seedOrderBytes;
    std::span<const std::uint8_t> residualBytes;
    std::span<const std::uint8_t> seedAuxStream;
    if (!TakeLane(input, offset, mainStreamSize, mainStream, error) ||
        !TakeLane(input, offset, seedLeadStreamSize, seedLeadStream, error) ||
        !TakeLane(input, offset, seedOrderByteCount, seedOrderBytes, error) ||
        !TakeLane(input, offset, residualByteCount, residualBytes, error) ||
        !TakeLane(input, offset, seedAuxStreamSize, seedAuxStream, error) ||
        offset != input.size()) {
        return offset == input.size()
            ? false
            : validation::AssignError(error, "topology grammar payload has trailing bytes");
    }

    SymbolStreamReader<16u> mainReader;
    SymbolStreamReader<16u> seedLeadReader;
    SymbolStreamReader<256u> seedAuxReader;
    if (!mainReader.Reset(mainStream, mainSymbolCount, error) ||
        !seedLeadReader.Reset(seedLeadStream, seedLeadSymbolCount, error) ||
        !seedAuxReader.Reset(seedAuxStream, seedAuxSymbolCount, error)) {
        return false;
    }
    std::vector<std::size_t> cellOffsets;
    if (!BuildCellOffsets(
            cellCount,
            fixedCellSize,
            cellSizes,
            connectivityCount,
            cellOffsets,
            error)) {
        return false;
    }

    output.resize(connectivityCount);
    TopologyState state;
    state.nextVertex = initialNextVertex;
    state.deltaReference = static_cast<IndexType>(initialNextVertex);
    std::size_t seedOrderIndex = 0u;
    std::size_t residualOffset = 0u;

    const auto decodeGeneralVertex = [&](const auto& self,
                                         const std::uint8_t symbol,
                                         const IndexType* previousInCell,
                                         const IndexType* firstInCell,
                                         IndexType& value) -> bool {
        if (symbol == 0u) {
            if (state.nextVertex > std::numeric_limits<IndexType>::max()) {
                return validation::AssignError(error, "topology next-vertex predictor overflowed");
            }
            value = static_cast<IndexType>(state.nextVertex++);
            CommitVertex(state, value, true);
            return true;
        }
        if (symbol <= kGeneralVertexFifoLimit) {
            if (!ReadVertexFifo(state, symbol, kGeneralVertexFifoLimit, value, error)) {
                return false;
            }
            CommitVertexAt(state, value, false, static_cast<std::size_t>(symbol - 1u));
            return true;
        }
        const auto tokenReference = previousInCell != nullptr ? *previousInCell : state.deltaReference;
        if (symbol == 13u) {
            if (tokenReference == 0u) {
                return validation::AssignError(error, "topology minus-one predictor underflowed");
            }
            value = tokenReference - 1u;
        } else if (symbol == 14u) {
            if (tokenReference == std::numeric_limits<IndexType>::max()) {
                return validation::AssignError(error, "topology plus-one predictor overflowed");
            }
            value = tokenReference + 1u;
        } else if (symbol == 15u) {
            const auto residualReference = firstInCell != nullptr
                ? *firstInCell
                : state.deltaReference;
            if (!ReadSignedDelta(residualBytes, residualOffset, residualReference, value, error)) {
                return false;
            }
        } else {
            return validation::AssignError(error, "topology general vertex symbol is invalid");
        }
        CommitVertex(state, value, true);
        (void)self;
        return true;
    };
    const auto decodeSeedLeadVertex = [&](const std::uint8_t symbol, IndexType& value) -> bool {
        if (symbol == 0u) {
            if (state.nextVertex > std::numeric_limits<IndexType>::max()) {
                return validation::AssignError(error, "topology seed next-vertex predictor overflowed");
            }
            value = static_cast<IndexType>(state.nextVertex++);
            CommitVertex(state, value, true);
            return true;
        }
        if (symbol <= kGeneralVertexFifoLimit) {
            if (!ReadVertexFifo(state, symbol, kGeneralVertexFifoLimit, value, error)) {
                return false;
            }
            CommitVertexAt(state, value, false, static_cast<std::size_t>(symbol - 1u));
            return true;
        }
        const auto reference = state.deltaReference;
        if (symbol == 13u) {
            if (reference == 0u) {
                return validation::AssignError(error, "topology seed minus-one predictor underflowed");
            }
            value = reference - 1u;
        } else if (symbol == 14u) {
            if (reference == std::numeric_limits<IndexType>::max()) {
                return validation::AssignError(error, "topology seed plus-one predictor overflowed");
            }
            value = reference + 1u;
        } else if (symbol == 15u) {
            if (!ReadSignedDelta(residualBytes, residualOffset, reference, value, error)) {
                return false;
            }
        } else {
            return validation::AssignError(error, "topology seed-lead symbol is invalid");
        }
        CommitVertex(state, value, true);
        return true;
    };
    const auto decodeSeedTailVertex = [&](const std::uint8_t symbol,
                                          const IndexType* firstInCell,
                                          IndexType& value) -> bool {
        if (symbol == 0u) {
            if (state.nextVertex > std::numeric_limits<IndexType>::max()) {
                return validation::AssignError(error, "topology seed-tail next-vertex predictor overflowed");
            }
            value = static_cast<IndexType>(state.nextVertex++);
            CommitVertex(state, value, true);
            return true;
        }
        if (symbol <= kSeedVertexFifoLimit) {
            if (!ReadVertexFifo(state, symbol, kSeedVertexFifoLimit, value, error)) {
                return false;
            }
            CommitVertexAt(state, value, false, static_cast<std::size_t>(symbol - 1u));
            return true;
        }
        const auto residualReference = firstInCell != nullptr
            ? *firstInCell
            : state.deltaReference;
        if (symbol != 15u ||
            !ReadSignedDelta(residualBytes, residualOffset, residualReference, value, error)) {
            return symbol == 15u
                ? false
                : validation::AssignError(error, "topology seed-tail symbol is invalid");
        }
        CommitVertex(state, value, true);
        return true;
    };

    for (std::size_t cellIndex = 0u; cellIndex < cellCount; ++cellIndex) {
        auto cell = std::span<IndexType>(
            output.data() + cellOffsets[cellIndex],
            cellOffsets[cellIndex + 1u] - cellOffsets[cellIndex]);
        if (cell.empty()) {
            continue;
        }
        std::uint8_t symbol = 0u;
        if (cell.size() == 1u) {
            if (!mainReader.Read(symbol, error) ||
                !decodeGeneralVertex(decodeGeneralVertex, symbol, nullptr, nullptr, cell[0])) {
                return false;
            }
        } else if (cell.size() == 2u) {
            if (!mainReader.Read(symbol, error) ||
                !decodeGeneralVertex(decodeGeneralVertex, symbol, nullptr, nullptr, cell[0]) ||
                !mainReader.Read(symbol, error) ||
                !decodeGeneralVertex(decodeGeneralVertex, symbol, &cell[0], &cell[0], cell[1])) {
                return false;
            }
            PushEdgeFifo(state, cell[0], cell[1]);
        } else {
            if (!mainReader.Read(symbol, error)) {
                return false;
            }
            if (symbol < 15u) {
                EdgeKey edge;
                bool reversed = false;
                if (!ReadEdgeFifo(state, symbol, edge, error) ||
                    !ReadBit(seedOrderBytes, seedOrderIndex++, reversed)) {
                    return validation::AssignError(error, "topology seed-edge reference is invalid");
                }
                cell[0] = reversed ? edge.second : edge.first;
                cell[1] = reversed ? edge.first : edge.second;
                if (!mainReader.Read(symbol, error) ||
                    !decodeGeneralVertex(decodeGeneralVertex, symbol, &cell[1], &cell[0], cell[2])) {
                    return false;
                }
                PushEdgeFifo(state, cell[1], cell[2]);
            } else {
                std::uint8_t seedLeadSymbol = 0u;
                if (!seedLeadReader.Read(seedLeadSymbol, error) ||
                    !decodeSeedLeadVertex(seedLeadSymbol, cell[0]) ||
                    !mainReader.Read(symbol, error)) {
                    return false;
                }
                std::uint8_t auxCode = 0u;
                if (symbol < seedLookupEntryCount) {
                    auxCode = seedLookupTable[symbol];
                } else if (symbol == 15u) {
                    if (!seedAuxReader.Read(auxCode, error)) {
                        return false;
                    }
                } else {
                    return validation::AssignError(error, "topology seed lookup symbol is invalid");
                }
                if (!decodeSeedTailVertex(
                        static_cast<std::uint8_t>(auxCode >> 4u),
                        &cell[0],
                        cell[1]) ||
                    !decodeSeedTailVertex(
                        static_cast<std::uint8_t>(auxCode & 0x0fu),
                        &cell[0],
                        cell[2])) {
                    return false;
                }
                PushEdgeFifo(state, cell[0], cell[1]);
                PushEdgeFifo(state, cell[1], cell[2]);
            }
            for (std::size_t local = 3u; local < cell.size(); ++local) {
                if (!mainReader.Read(symbol, error) ||
                    !decodeGeneralVertex(
                        decodeGeneralVertex,
                        symbol,
                        &cell[local - 1u],
                        &cell[0],
                        cell[local])) {
                    return false;
                }
                PushEdgeFifo(state, cell[local - 1u], cell[local]);
            }
            PushEdgeFifo(state, cell.back(), cell.front());
        }
        for (const auto point : cell) {
            if (static_cast<std::size_t>(point) >= pointCount) {
                return validation::AssignError(error, "decoded topology point index is out of range");
            }
        }
    }

    if (!mainReader.Finish(error) ||
        !seedLeadReader.Finish(error) ||
        !seedAuxReader.Finish(error) ||
        seedOrderIndex != seedOrderBitCount ||
        residualOffset != residualBytes.size()) {
        output.clear();
        return error != nullptr && !error->empty()
            ? false
            : validation::AssignError(error, "topology grammar lane consumption is inconsistent");
    }
    return true;
}

inline bool EncodeConnectivity(
    const std::span<const IndexType> connectivity,
    const std::span<const IndexType> cellSizes,
    const std::size_t cellCount,
    const int fixedCellSize,
    const std::size_t pointCount,
    std::vector<std::uint8_t>& output,
    std::string* error = nullptr,
    const bool inputAlreadyValidated = false) {
    return EncodeConnectivityGrammar(
        connectivity,
        cellSizes,
        cellCount,
        fixedCellSize,
        pointCount,
        output,
        error,
        inputAlreadyValidated);
}

inline bool DecodeConnectivity(
    const std::span<const std::uint8_t> input,
    const std::span<const IndexType> cellSizes,
    const std::size_t pointCount,
    const std::size_t cellCount,
    const std::size_t connectivityCount,
    const int fixedCellSize,
    std::vector<IndexType>& output,
    std::string* error = nullptr) {
    return DecodeConnectivityGrammar(
        input,
        cellSizes,
        pointCount,
        cellCount,
        connectivityCount,
        fixedCellSize,
        output,
        error);
}

} // namespace datacodec::topocodec::blockcodec

#endif
