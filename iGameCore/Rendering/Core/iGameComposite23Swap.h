#ifndef IGAMEVIS_IGAMECOMPOSITE23SWAP_H
#define IGAMEVIS_IGAMECOMPOSITE23SWAP_H

#include "iGameMacro.h"

#include <algorithm>
#include <cstddef>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @namespace Composite23Swap
 * @brief 2-3 swap 图像合成（Hongfeng Yu et al., SC 2008）的**纯逻辑**部分。
 *
 * @details
 *  这里只做「合成树 + 图像平坦划分 + 分块归属顺序」的计算，不碰 MPI、不碰像素数据，
 *  因此 iGameCompositePass（块级路径）与 ParallelVolumePixelComposite（逐像素路径）
 *  可以共用同一份实现，也便于离线对拍。
 *
 *  算法要点（论文 §3.3）：
 *   - 任意 N（N>1）都能分解成 2 与 3 的和，所以合成树每个内部节点有 2 个或 3 个孩子；
 *     当 N 是 2 的幂时退化成一棵完全二叉树（即经典 binary-swap）。
 *   - 树的叶子是处理器（按深度序 / 合成序编号 0..N-1），每个内部节点对应一次合并。
 *   - **每个节点都把整幅图（ROI 子图）按自己的处理器数 n 平坦均分成 n 份**，第 p 份
 *     交给该节点 order[p] 号处理器。节点合并后，每个处理器手里始终是「整幅图的一个
 *     连续像素区间」，只是区间越来越小（1/n）。
 *   - 合并时，各孩子节点的旧分块与新分块是两套不同的平坦划分：旧分块可能横跨多个
 *     新分块（需要发给多个所有者），新分块也可能由多个旧分块拼成（需要从多个发送者
 *     接收）。孩子节点的 rank 区间在深度序上连续，所以合成顺序 = 孩子顺序。
 *   - OrderAssignment（论文 Algorithm 2）只决定「哪一份像素归哪个处理器」，用于让
 *     通信尽量均匀；它不改变混合顺序（混合顺序始终是深度序）。
 *
 *  复杂度：轮数 = ⌊log₂N⌋；每轮每个处理器最多与 4 个处理器通信（论文 §3.3 的界）。
 */
namespace Composite23Swap {

/** 第 index 份（共 parts 份）的像素数：余数优先分给前 rem 份。 */
inline long long FlatLen(int index, long long pixels, int parts) {
    if (parts <= 0) { return 0; }
    const long long base = pixels / parts;
    const long long rem = pixels % parts;
    return base + (static_cast<long long>(index) < rem ? 1 : 0);
}

/** 第 index 份在 [0, pixels) 里的起始偏移。 */
inline long long FlatOffset(int index, long long pixels, int parts) {
    if (parts <= 0) { return 0; }
    const long long base = pixels / parts;
    const long long rem = pixels % parts;
    return static_cast<long long>(index) * base +
           std::min<long long>(static_cast<long long>(index), rem);
}

/**
 * 平坦划分 [0, pixels) 成 parts 份后，与区间 [lo, hi) 相交的第一份的序号。
 * （份的偏移随序号单调递增，二分即可；hi 不参与二分，调用方按需截断。）
 */
inline int FirstPieceIntersecting(long long lo, long long pixels, int parts) {
    int low = 0;
    int high = parts; // [low, high)
    while (low < high) {
        const int mid = low + (high - low) / 2;
        const long long end = FlatOffset(mid, pixels, parts) +
                              FlatLen(mid, pixels, parts);
        if (end > lo) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return low;
}

/** ⌊log₂n⌋（n ≥ 1）。 */
inline int FloorLog2(int n) {
    int d = 0;
    while ((n >> (d + 1)) > 0) { ++d; }
    return d;
}

/** 合成树的一个节点：覆盖深度序里的连续 rank 区间 [lo, hi)。 */
struct Node {
    int lo{0};
    int hi{0};
    int parent{-1};
    int depth{0}; // 0 = 根（最后一次合并），depth = 叶子层
    std::vector<int> children; // 孩子节点下标（按深度序 = 混合顺序）
    std::vector<int> order;    // 本节点的分块归属顺序（groupRank 的排列）
};

/** 合成树 + 每层 rank→节点/位置的查表。 */
struct Tree {
    std::vector<Node> nodes; // nodes[0] = 根
    int ranks{0};
    int depth{0}; // 最大层号（叶子所在层）
    // 展平表：[d * ranks + groupRank] -> 该层包含它的节点下标 / 它在节点 order 里的位置。
    std::vector<int> nodeAtDepth;
    std::vector<int> posAtDepth;
};

namespace detail {

inline int BuildNode(Tree& tree, int lo, int hi, int parent, int depth) {
    const int index = static_cast<int>(tree.nodes.size());
    tree.nodes.push_back(Node{});
    {
        Node& node = tree.nodes[static_cast<std::size_t>(index)];
        node.lo = lo;
        node.hi = hi;
        node.parent = parent;
        node.depth = depth;
    }
    const int n = hi - lo;
    if (n <= 1) { return index; } // 叶子：一个处理器

    // 论文 Algorithm 1：2^d ≤ n < 2^(d+1)。d = ⌊log₂n⌋。
    const int d = FloorLog2(n);
    std::vector<int> childSizes;
    const int largerHalf = (n + 1) / 2;
    if (largerHalf < (1 << d)) {
        // 两孩子：⌊n/2⌋、⌈n/2⌉。
        childSizes = {n / 2, n - n / 2};
    } else {
        // 三孩子：⌊n/3⌋、⌊n/3⌋、⌈n/3⌉（l = m ≤ r）。
        const int third = n / 3;
        childSizes = {third, third, n - 2 * third};
    }

    int childLo = lo;
    for (int size : childSizes) {
        const int child = BuildNode(tree, childLo, childLo + size, index, depth + 1);
        tree.nodes[static_cast<std::size_t>(index)].children.push_back(child);
        childLo += size;
    }
    return index;
}

/** 论文 Algorithm 2：自底向上给每个节点生成分块归属顺序。 */
inline void AssignOrder(Tree& tree, int index) {
    Node& node = tree.nodes[static_cast<std::size_t>(index)];
    if (node.children.empty()) {
        node.order.assign(1, node.lo); // 叶子就是它自己
        return;
    }
    for (int child : node.children) { AssignOrder(tree, child); }

    std::vector<int> order = node.children; // 孩子节点下标
    if (order.size() == 2) {
        // 处理器多的那个孩子排前面；相等时保持左右顺序。
        const Node& a = tree.nodes[static_cast<std::size_t>(order[0])];
        const Node& b = tree.nodes[static_cast<std::size_t>(order[1])];
        if (b.order.size() > a.order.size()) { std::swap(order[0], order[1]); }
    } else if (order.size() == 3) {
        // 孩子规模是 (M, M, M+1)；M+1 那个（右孩子）更大时按 (右, 左, 中) 的顺序。
        const Node& l = tree.nodes[static_cast<std::size_t>(order[0])];
        const Node& r = tree.nodes[static_cast<std::size_t>(order[2])];
        if (r.order.size() > l.order.size()) {
            const int right = order[2];
            order[2] = order[1];
            order[1] = order[0];
            order[0] = right;
        }
    }

    // 轮流（round-robin）交叉合并各孩子的顺序，保持每个孩子内部的相对顺序。
    node.order.clear();
    node.order.reserve(static_cast<std::size_t>(node.hi - node.lo));
    std::size_t maxLen = 0;
    for (int child : order) {
        maxLen = std::max(
                maxLen, tree.nodes[static_cast<std::size_t>(child)].order.size());
    }
    for (std::size_t pos = 0; pos < maxLen; ++pos) {
        for (int child : order) {
            const std::vector<int>& childOrder =
                    tree.nodes[static_cast<std::size_t>(child)].order;
            if (pos < childOrder.size()) { node.order.push_back(childOrder[pos]); }
        }
    }
}

} // namespace detail

/**
 * 构造 N 个处理器的 2-3 swap 合成树（N ≥ 1）。
 * 所有处理器用同一份输入会得到逐位相同的树/顺序，因此可以据此确定性地配对。
 */
inline Tree Build(int rankCount) {
    Tree tree;
    tree.ranks = rankCount;
    if (rankCount <= 0) { return tree; }
    detail::BuildNode(tree, 0, rankCount, -1, 0);
    detail::AssignOrder(tree, 0);

    tree.depth = 0;
    for (const Node& node : tree.nodes) {
        tree.depth = std::max(tree.depth, node.depth);
    }
    const std::size_t tableSize =
            static_cast<std::size_t>(tree.depth + 1) * static_cast<std::size_t>(rankCount);
    tree.nodeAtDepth.assign(tableSize, -1);
    tree.posAtDepth.assign(tableSize, 0);
    for (std::size_t i = 0; i < tree.nodes.size(); ++i) {
        const Node& node = tree.nodes[i];
        for (std::size_t pos = 0; pos < node.order.size(); ++pos) {
            const int rank = node.order[pos];
            const std::size_t slot = static_cast<std::size_t>(node.depth) *
                                             static_cast<std::size_t>(rankCount) +
                                     static_cast<std::size_t>(rank);
            tree.nodeAtDepth[slot] = static_cast<int>(i);
            tree.posAtDepth[slot] = static_cast<int>(pos);
        }
    }
    return tree;
}

/** 该层（depth）包含 groupRank 的节点下标。 */
inline int NodeOf(const Tree& tree, int depth, int groupRank) {
    return tree.nodeAtDepth[static_cast<std::size_t>(depth) *
                                    static_cast<std::size_t>(tree.ranks) +
                            static_cast<std::size_t>(groupRank)];
}

/** groupRank 在该层节点 order 里的位置 = 它最终分到的分块序号。 */
inline int PosOf(const Tree& tree, int depth, int groupRank) {
    return tree.posAtDepth[static_cast<std::size_t>(depth) *
                                   static_cast<std::size_t>(tree.ranks) +
                           static_cast<std::size_t>(groupRank)];
}

} // namespace Composite23Swap

IGAME_NAMESPACE_END

#endif // IGAMEVIS_IGAMECOMPOSITE23SWAP_H
