#ifndef DATACODEC_CODEC_SUBCODEC_CANONICALHUFFMANCODEC_H
#define DATACODEC_CODEC_SUBCODEC_CANONICALHUFFMANCODEC_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace datacodec::codec {

template <std::size_t AlphabetSize>
inline std::array<std::uint32_t, AlphabetSize> BuildCanonicalHuffmanCodes(
    const std::array<std::uint8_t, AlphabetSize>& codeLengths) {
    std::array<std::uint32_t, AlphabetSize> codes{};
    std::uint32_t code = 0u;
    std::uint8_t previousLength = 0u;
    for (std::uint8_t length = 1u; length <= 32u; ++length) {
        code <<= static_cast<std::uint8_t>(length - previousLength);
        previousLength = length;
        for (std::size_t symbol = 0u; symbol < codeLengths.size(); ++symbol) {
            if (codeLengths[symbol] != length) {
                continue;
            }
            codes[symbol] = code;
            ++code;
        }
    }
    return codes;
}

template <std::size_t AlphabetSize>
inline bool BuildHuffmanCodeLengths(
    const std::array<std::uint64_t, AlphabetSize>& counts,
    std::array<std::uint8_t, AlphabetSize>& codeLengths) {
    codeLengths = {};

    struct Node {
        std::uint64_t weight{0u};
        int left{-1};
        int right{-1};
        int symbol{-1};
        bool active{false};
    };

    std::vector<Node> nodes;
    nodes.reserve(AlphabetSize == 0u ? 0u : AlphabetSize * 2u - 1u);
    for (std::size_t symbol = 0u; symbol < counts.size(); ++symbol) {
        if (counts[symbol] == 0u) {
            continue;
        }
        nodes.push_back(Node{
            .weight = counts[symbol],
            .symbol = static_cast<int>(symbol),
            .active = true,
        });
    }
    if (nodes.empty()) {
        return true;
    }
    if (nodes.size() == 1u) {
        codeLengths[static_cast<std::size_t>(nodes.front().symbol)] = 1u;
        return true;
    }

    auto findLightestActiveNode = [&]() -> int {
        int best = -1;
        for (std::size_t index = 0u; index < nodes.size(); ++index) {
            if (!nodes[index].active) {
                continue;
            }
            if (best < 0 ||
                nodes[index].weight < nodes[static_cast<std::size_t>(best)].weight ||
                (nodes[index].weight == nodes[static_cast<std::size_t>(best)].weight &&
                 nodes[index].symbol < nodes[static_cast<std::size_t>(best)].symbol)) {
                best = static_cast<int>(index);
            }
        }
        return best;
    };

    std::size_t activeCount = nodes.size();
    while (activeCount > 1u) {
        const auto first = findLightestActiveNode();
        if (first < 0) {
            return false;
        }
        nodes[static_cast<std::size_t>(first)].active = false;
        const auto second = findLightestActiveNode();
        if (second < 0) {
            return false;
        }
        nodes[static_cast<std::size_t>(second)].active = false;

        const auto parentWeight =
            nodes[static_cast<std::size_t>(first)].weight +
            nodes[static_cast<std::size_t>(second)].weight;
        nodes.push_back(Node{
            .weight = parentWeight,
            .left = first,
            .right = second,
            .symbol = nodes[static_cast<std::size_t>(first)].symbol <
                    nodes[static_cast<std::size_t>(second)].symbol
                ? nodes[static_cast<std::size_t>(first)].symbol
                : nodes[static_cast<std::size_t>(second)].symbol,
            .active = true,
        });
        --activeCount;
    }

    const auto assignDepth = [&](const auto& self, const int nodeIndex, const std::uint8_t depth) -> bool {
        const auto& node = nodes[static_cast<std::size_t>(nodeIndex)];
        if (node.symbol >= 0 && node.left < 0 && node.right < 0) {
            if (depth == 0u || depth > 32u) {
                return false;
            }
            codeLengths[static_cast<std::size_t>(node.symbol)] = depth;
            return true;
        }
        return self(self, node.left, static_cast<std::uint8_t>(depth + 1u)) &&
            self(self, node.right, static_cast<std::uint8_t>(depth + 1u));
    };
    return assignDepth(assignDepth, static_cast<int>(nodes.size() - 1u), 0u);
}

} // namespace datacodec::codec

#endif
