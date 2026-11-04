#include "iGameCompositePass.h"

#include "iGameComposite23Swap.h"
#include "iGameParallelContext.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <limits>
#include <utility>

IGAME_NAMESPACE_BEGIN

void iGameCompositePass::SetLocalImage(int width, int height,
                                       const std::vector<unsigned char>& rgba,
                                       const std::vector<float>& depth) {
    m_Width = width < 0 ? 0 : width;
    m_Height = height < 0 ? 0 : height;
    m_LocalRGBA = rgba;
    m_LocalDepth = depth;
}

void iGameCompositePass::SetBackgroundColor(float r, float g, float b) {
    m_Background[0] = r;
    m_Background[1] = g;
    m_Background[2] = b;
}

double iGameCompositePass::ComputeBlockDepth(const double center[3],
                                             const double cameraPos[3],
                                             const double cameraFront[3]) {
    double v[3] = {center[0] - cameraPos[0], center[1] - cameraPos[1],
                   center[2] - cameraPos[2]};
    return v[0] * cameraFront[0] + v[1] * cameraFront[1] +
           v[2] * cameraFront[2];
}

// ---------------------------------------------------------------------------
// 2-3 swap 合成辅助函数（H. Yu, C. Wang, K.-L. Ma, SC 2008）
//
// 与 binary-swap 的关键区别：合成树每个内部节点有 2 或 3 个孩子，因此任意 P 都能
// 用（P = 2^K - 1 时也不会有一半处理器空闲）。树的构造、图像平坦划分与分块归属
// 顺序由 iGameComposite23Swap.h 提供（纯逻辑，供块级/逐像素两条路径共用）。
// ---------------------------------------------------------------------------
namespace {
namespace Swap23 = Composite23Swap;

// 预乘 alpha 图叠到背景色上，输出不透明 RGBA8（与稀疏/全图路径输出一致）。
void FlattenToBackground(const std::vector<unsigned char>& src, const float bg[3],
                         int n, std::vector<unsigned char>& out) {
    out.resize(static_cast<std::size_t>(n) * 4);
    auto toU = [](float v) {
        return static_cast<unsigned char>(
                std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    for (int p = 0; p < n; ++p) {
        const std::size_t b = static_cast<std::size_t>(p) * 4;
        const float a = static_cast<float>(src[b + 3]) / 255.0f;
        const float inv = 1.0f - a;
        out[b + 0] = toU(static_cast<float>(src[b + 0]) / 255.0f + bg[0] * inv);
        out[b + 1] = toU(static_cast<float>(src[b + 1]) / 255.0f + bg[1] * inv);
        out[b + 2] = toU(static_cast<float>(src[b + 2]) / 255.0f + bg[2] * inv);
        out[b + 3] = 255;
    }
}

// 整幅图填成不透明背景色（全空帧 / ROI 之外用）。
void FillOpaqueBackground(std::vector<unsigned char>& rgba, const float bg[3],
                          long long n) {
    auto toU = [](float v) {
        return static_cast<unsigned char>(
                std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    const unsigned char r = toU(bg[0]);
    const unsigned char g = toU(bg[1]);
    const unsigned char b = toU(bg[2]);
    rgba.assign(static_cast<std::size_t>(n) * 4, 0);
    for (long long p = 0; p < n; ++p) {
        const std::size_t o = static_cast<std::size_t>(p) * 4;
        rgba[o + 0] = r;
        rgba[o + 1] = g;
        rgba[o + 2] = b;
        rgba[o + 3] = 255;
    }
}

// rank 0 的收尾：roiFrame 是「预乘 alpha」的 ROI 合成结果（roiW*roiH*4）。
//   outFull = 全帧不透明 RGBA8（ROI 叠背景，其余填背景色）
//   outROI  = transparentROI ? 预乘 ROI（原样，供客户端叠背景）
//                            : 叠背景后的不透明 ROI
void ComposeROIOutputs(const std::vector<unsigned char>& roiFrame, int roiW,
                       int roiH, int x0, int y0, int fullW, int fullH,
                       const float bg[3], bool transparentROI,
                       std::vector<unsigned char>& outFull,
                       std::vector<unsigned char>& outROI) {
    FillOpaqueBackground(outFull, bg, static_cast<long long>(fullW) * fullH);
    std::vector<unsigned char> roiOpaque;
    FlattenToBackground(roiFrame, bg, roiW * roiH, roiOpaque);
    for (int y = 0; y < roiH; ++y) {
        std::memcpy(outFull.data() +
                            (static_cast<std::size_t>(y0 + y) * fullW + x0) * 4,
                    roiOpaque.data() + static_cast<std::size_t>(y) * roiW * 4,
                    static_cast<std::size_t>(roiW) * 4);
    }
    if (transparentROI) {
        outROI = roiFrame;
    } else {
        outROI = std::move(roiOpaque);
    }
}


void PutU32(std::vector<unsigned char>& b, std::uint32_t v) {
    b.push_back(static_cast<unsigned char>(v & 0xffu));
    b.push_back(static_cast<unsigned char>((v >> 8) & 0xffu));
    b.push_back(static_cast<unsigned char>((v >> 16) & 0xffu));
    b.push_back(static_cast<unsigned char>((v >> 24) & 0xffu));
}

std::uint32_t GetU32(const unsigned char* b, std::size_t off) {
    return static_cast<std::uint32_t>(b[off]) |
           (static_cast<std::uint32_t>(b[off + 1]) << 8) |
           (static_cast<std::uint32_t>(b[off + 2]) << 16) |
           (static_cast<std::uint32_t>(b[off + 3]) << 24);
}

// 把一段密集预乘 RGBA8 打包成可发送的紧凑消息体：
//   u32 头 = 0            → 全空（无后续载荷）
//   u32 头 = 0xFFFFFFFF   → 密集（后续 len*4 字节原始 RGBA）
//   u32 头 = cnt (其它)   → 稀疏（后续 cnt 个 (u32 relIdx, u8 r,g,b,a)）
// 选择依据：稀疏编码 8 字节/像素、密集 4 字节/像素，取较小者。
std::vector<unsigned char> PackDense(const unsigned char* rgba, long long len) {
    long long cnt = 0;
    for (long long i = 0; i < len; ++i) {
        if (rgba[static_cast<std::size_t>(i) * 4 + 3] != 0) { ++cnt; }
    }

    std::vector<unsigned char> buf;
    buf.reserve(static_cast<std::size_t>(4 + len * 4));
    if (cnt == 0) {
        PutU32(buf, 0u);
    } else if (cnt * 8 <= len * 4) {
        PutU32(buf, static_cast<std::uint32_t>(cnt));
        for (long long i = 0; i < len; ++i) {
            const std::size_t b = static_cast<std::size_t>(i) * 4;
            if (rgba[b + 3] != 0) {
                PutU32(buf, static_cast<std::uint32_t>(i));
                buf.push_back(rgba[b + 0]);
                buf.push_back(rgba[b + 1]);
                buf.push_back(rgba[b + 2]);
                buf.push_back(rgba[b + 3]);
            }
        }
    } else {
        PutU32(buf, 0xFFFFFFFFu);
        buf.insert(buf.end(), rgba, rgba + static_cast<std::size_t>(len) * 4);
    }
    return buf;
}

// 解包成长度为 len 的密集 RGBA8（非 0 像素按 relIdx 落位，其余保持 0）。
std::vector<unsigned char> UnpackDense(const unsigned char* buf, std::size_t size,
                                       long long len) {
    std::vector<unsigned char> dense(static_cast<std::size_t>(len) * 4, 0);
    if (size < 4) { return dense; }
    const std::uint32_t h = GetU32(buf, 0);
    if (h == 0) { return dense; }
    if (h == 0xFFFFFFFFu) {
        const std::size_t want = static_cast<std::size_t>(len) * 4;
        const std::size_t have = size - 4;
        if (have > 0) {
            std::memcpy(dense.data(), buf + 4, std::min(want, have));
        }
        return dense;
    }
    std::size_t pos = 4;
    for (std::uint32_t t = 0; t < h; ++t) {
        if (pos + 8 > size) { break; }
        const std::uint32_t rel = GetU32(buf, pos);
        pos += 4;
        if (static_cast<long long>(rel) >= len) { pos += 4; continue; }
        const std::size_t b = static_cast<std::size_t>(rel) * 4;
        dense[b + 0] = buf[pos + 0];
        dense[b + 1] = buf[pos + 1];
        dense[b + 2] = buf[pos + 2];
        dense[b + 3] = buf[pos + 3];
        pos += 4;
    }
    return dense;
}

// 把一段密集预乘 RGBA8 以 front-to-back over 累加进 acc 的 [off, off+len) 区间。
// （acc 是 float 累加缓冲：预乘颜色 + alpha；与 CompositeSparse 的公式一致。）
void OverDenseInto(float* acc, long long off, const unsigned char* dense,
                   long long len) {
    for (long long i = 0; i < len; ++i) {
        const std::size_t b = static_cast<std::size_t>(i) * 4;
        const float fa = static_cast<float>(dense[b + 3]) / 255.0f;
        if (fa <= 0.0f) { continue; }
        float* ap = acc + (off + i) * 4;
        const float inv = 1.0f - ap[3];
        if (inv <= 0.001f) { continue; }
        ap[0] += inv * (static_cast<float>(dense[b + 0]) / 255.0f);
        ap[1] += inv * (static_cast<float>(dense[b + 1]) / 255.0f);
        ap[2] += inv * (static_cast<float>(dense[b + 2]) / 255.0f);
        ap[3] += inv * fa;
    }
}

unsigned char ToUChar(float v) {
    return static_cast<unsigned char>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
}

} // namespace

// ---------------------------------------------------------------------------
// 2-3 swap 合成（任意 P；论文 SC 2008）
//
// 数据流：
//   1) AllGather 尺寸校验 + 各 rank 非空外接矩形求并集 ROI（与稀疏路径同口径）；
//   2) 块深度排序 → m_SortOrder（近→远）；groupRank = 本进程在深度序中的位置；
//   3) 构造 2-3 合成树（P 为 2 的幂时退化为完全二叉树）；
//   4) 工作图 = 本 rank 在并集 ROI 内的子图（预乘 alpha RGBA8），整幅图按平坦像素
//      数组处理；叶子层每个 rank 持有整幅图；
//   5) 自底向上 ⌊log₂P⌋ 轮：合并 2/3 个孩子时，把整幅图按合并后组的处理器数重新
//      平坦均分，按论文 Algorithm 2 的顺序分给组内处理器。每个处理器把自己的旧
//      区间切片发给新所有者，并收齐自己新区间需要的若干切片，按**孩子顺序**
//      front-to-back over（孩子是深度序里连续的 rank 区间 ⇒ 顺序正确）；
//   6) 结束后每个 rank 持有最终图的一块（Root 的 order 位置即分块序号），Gatherv
//      到 rank 0 按平坦偏移拼成 ROI，再写全帧不透明结果 + ROI 子矩形。
//
// 复杂度：⌊log₂P⌋ 轮；每轮每处理器最多与 4 个处理器通信（旧区间/新区间是两套
// 平坦划分，边界不对齐时最多跨 4 份），每处理器收发的像素总量 < 4/3 × ROI 大小。
//
// 有序合成：混合顺序始终是「孩子顺序 = 深度序的连续区间」，与 binary-swap 的
// swizzle 等价；OrderAssignment 只影响负载均衡，不参与前后判断。
// ---------------------------------------------------------------------------
bool iGameCompositePass::Composite23Swap() {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();
    const int rank = ctx->Rank();
    const bool isRoot = (rank == 0);

    // 1) 尺寸一致性校验（AllGather 全员一致，避免死锁）。
    double localDims[2] = {static_cast<double>(m_Width),
                           static_cast<double>(m_Height)};
    std::vector<double> allDims(static_cast<size_t>(size) * 2);
    ctx->AllGather(localDims, allDims.data(), 2);
    const int width = m_Width;
    const int height = m_Height;
    bool dimsAgree = (size >= 1);
    for (int r = 0; r < size; ++r) {
        if (static_cast<int>(allDims[static_cast<size_t>(r) * 2]) != width ||
            static_cast<int>(allDims[static_cast<size_t>(r) * 2 + 1]) != height) {
            dimsAgree = false;
            break;
        }
    }
    if (!dimsAgree || width <= 0 || height <= 0) {
        if (isRoot) {
            std::cerr << "[iGameCompositePass] 2-3 swap image size mismatch or "
                         "empty across ranks.\n";
        }
        return false;
    }

    // 1.5) ROI：各 rank 求非空像素外接矩形，AllGatherInt 后取并集；2-3 swap 只在
    //      「并集 ROI 子图」上做（ROI 外全透明像素不参与交换）。
    m_ResultWidth = width;
    m_ResultHeight = height;
    int lx0 = width, ly0 = height, lx1 = -1, ly1 = -1;
    if (m_LocalRGBA.size() >= static_cast<size_t>(width) * height * 4) {
        for (int y = 0; y < height; ++y) {
            const unsigned char* row =
                    m_LocalRGBA.data() + static_cast<size_t>(y) * width * 4;
            for (int x = 0; x < width; ++x) {
                if (row[x * 4 + 3] != 0) {
                    if (x < lx0) { lx0 = x; }
                    if (x > lx1) { lx1 = x; }
                    if (y < ly0) { ly0 = y; }
                    if (y > ly1) { ly1 = y; }
                }
            }
        }
    }
    const bool localEmpty = (lx1 < lx0 || ly1 < ly0);
    const int localROI[4] = {localEmpty ? 0 : lx0, localEmpty ? 0 : ly0,
                             localEmpty ? 0 : (lx1 - lx0 + 1),
                             localEmpty ? 0 : (ly1 - ly0 + 1)};
    std::vector<int> allROI(static_cast<size_t>(size) * 4, 0);
    ctx->AllGatherInt(localROI, allROI.data(), 4);

    int rx0 = width, ry0 = height, rx1 = -1, ry1 = -1;
    for (int r = 0; r < size; ++r) {
        const int rw = allROI[static_cast<size_t>(r) * 4 + 2];
        const int rh = allROI[static_cast<size_t>(r) * 4 + 3];
        if (rw <= 0 || rh <= 0) { continue; }
        const int rx = allROI[static_cast<size_t>(r) * 4 + 0];
        const int ry = allROI[static_cast<size_t>(r) * 4 + 1];
        rx0 = std::min(rx0, rx);
        ry0 = std::min(ry0, ry);
        rx1 = std::max(rx1, rx + rw - 1);
        ry1 = std::max(ry1, ry + rh - 1);
    }
    if (rx1 < rx0 || ry1 < ry0) {
        // 全透明：所有 rank 看到空并集，直接输出背景（不再做任何 P2P）。
        if (isRoot) {
            FillOpaqueBackground(m_ResultRGBA, m_Background,
                                 static_cast<long long>(width) * height);
            m_ResultROIX = m_ResultROIY = m_ResultROIW = m_ResultROIH = 0;
            m_ResultROIRGBA.clear();
        }
        ctx->Barrier();
        return true;
    }
    const int roiW = rx1 - rx0 + 1;
    const int roiH = ry1 - ry0 + 1;
    const long long N = static_cast<long long>(roiW) * roiH;

    // 2) 全局块序（近 -> 远）。groupRank = 本 rank 在深度序中的位置（0=最前）。
    std::vector<double> allBlockDepth(static_cast<size_t>(size));
    ctx->AllGather(&m_BlockDepth, allBlockDepth.data(), 1);
    m_SortOrder.resize(static_cast<size_t>(size));
    for (int r = 0; r < size; ++r) { m_SortOrder[static_cast<size_t>(r)] = r; }
    std::sort(m_SortOrder.begin(), m_SortOrder.end(),
              [&](int a, int b) {
                  if (allBlockDepth[static_cast<size_t>(a)] !=
                      allBlockDepth[static_cast<size_t>(b)]) {
                      return allBlockDepth[static_cast<size_t>(a)] <
                             allBlockDepth[static_cast<size_t>(b)];
                  }
                  return a < b;
              });
    std::vector<int> worldToGroup(static_cast<size_t>(size), -1);
    for (int g = 0; g < size; ++g) {
        worldToGroup[static_cast<size_t>(m_SortOrder[static_cast<size_t>(g)])] = g;
    }
    const int groupRank = worldToGroup[static_cast<size_t>(rank)];
    if (groupRank < 0) { return false; }

    // ROI 太小（像素数 < rank 数）时平坦划分会出现 0 长分块，退回稀疏 ROI 合成。
    // 该判断只依赖全局一致的 size/N/ROI，所有 rank 不会走岔。
    if (size < 2 || N < static_cast<long long>(size)) {
        return CompositeSparse();
    }
    if (N > static_cast<long long>(std::numeric_limits<int>::max()) / 8) {
        if (isRoot) {
            std::cerr << "[iGameCompositePass] 2-3 swap ROI too large for int "
                         "counts; falling back to sparse ROI compositing.\n";
        }
        return CompositeSparse();
    }

    // 3) 2-3 合成树（所有 rank 由同一个 P 构造，逐位一致）。
    const Swap23::Tree tree = Swap23::Build(size);
    if (tree.depth < 1) { return CompositeSparse(); }
    if (isRoot) {
        static bool logged = false;
        if (!logged) {
            logged = true;
            std::cout << "[iGameCompositePass] 2-3 swap: P=" << size
                      << " tree-depth=" << tree.depth << " root-children="
                      << tree.nodes[0].children.size() << " ROI=" << roiW << 'x'
                      << roiH << '\n';
        }
    }

    // 4) 工作图 = 本 rank 在「并集 ROI」内的子图（预乘 alpha RGBA8）。
    std::vector<unsigned char> work(static_cast<size_t>(N) * 4, 0);
    if (m_LocalRGBA.size() >= static_cast<size_t>(width) * height * 4) {
        const size_t rowBytes = static_cast<size_t>(roiW) * 4;
        for (int y = 0; y < roiH; ++y) {
            const unsigned char* src =
                    m_LocalRGBA.data() +
                    (static_cast<size_t>(ry0 + y) * width + rx0) * 4;
            std::memcpy(work.data() + static_cast<size_t>(y) * rowBytes, src,
                        rowBytes);
        }
    }

    // 5) 自底向上合并。层号 d：d = tree.depth 是叶子（每 rank 一幅整图），d = 0 是根。
    for (int d = tree.depth - 1; d >= 0; --d) {
        const int childIndex = Swap23::NodeOf(tree, d + 1, groupRank);
        const int parentIndex = Swap23::NodeOf(tree, d, groupRank);
        const Swap23::Node& child = tree.nodes[static_cast<size_t>(childIndex)];
        const Swap23::Node& parent = tree.nodes[static_cast<size_t>(parentIndex)];
        const int childRanks = child.hi - child.lo;
        const int parentRanks = parent.hi - parent.lo;

        // 旧区间：本 rank 在「孩子节点」里分到的分块。
        const int oldPos = Swap23::PosOf(tree, d + 1, groupRank);
        const long long oldLo = Swap23::FlatOffset(oldPos, N, childRanks);
        const long long oldLen = Swap23::FlatLen(oldPos, N, childRanks);
        const long long oldHi = oldLo + oldLen;
        if (static_cast<long long>(work.size()) != oldLen * 4) {
            // 内部错误：不能提前 return（其它 rank 还要继续集合通信）。按协议补齐。
            std::cerr << "[iGameCompositePass] 2-3 swap piece length mismatch "
                         "(rank "
                      << rank << ", depth " << d << "); padding.\n";
            work.resize(static_cast<size_t>(oldLen) * 4, 0);
        }

        // 新区间：本 rank 在「父节点」里分到的分块。
        const int newPos = Swap23::PosOf(tree, d, groupRank);
        const long long newLo = Swap23::FlatOffset(newPos, N, parentRanks);
        const long long newLen = Swap23::FlatLen(newPos, N, parentRanks);
        const long long newHi = newLo + newLen;

        // 5a) 发送：旧区间与父节点的哪些新分块相交（每份属于不同处理器）。
        //     用 deque 保证 Isend 用到的缓冲区地址不会因扩容而失效。
        std::deque<std::vector<unsigned char>> sendBufs;
        std::vector<int> sendReqs;
        for (int q = Swap23::FirstPieceIntersecting(oldLo, N, parentRanks);
             q < parentRanks; ++q) {
            const long long pieceLo = Swap23::FlatOffset(q, N, parentRanks);
            if (pieceLo >= oldHi) { break; }
            const long long pieceHi = pieceLo + Swap23::FlatLen(q, N, parentRanks);
            const long long is = std::max(pieceLo, oldLo);
            const long long ie = std::min(pieceHi, oldHi);
            if (ie <= is) { continue; }
            const int owner = parent.order[static_cast<size_t>(q)];
            if (owner == groupRank) { continue; } // 这一份归自己，无需发送

            std::vector<unsigned char> payload =
                    PackDense(work.data() + (is - oldLo) * 4, ie - is);
            std::vector<unsigned char> msg;
            msg.reserve(8 + payload.size());
            PutU32(msg, static_cast<std::uint32_t>(is));
            PutU32(msg, static_cast<std::uint32_t>(ie - is));
            msg.insert(msg.end(), payload.begin(), payload.end());
            const int partner = m_SortOrder[static_cast<size_t>(owner)];
            const int req = ctx->Isend(
                    reinterpret_cast<const char*>(msg.data()),
                    static_cast<int>(msg.size()), partner, d);
            sendBufs.push_back(std::move(msg));
            sendReqs.push_back(req);
        }

        // 5b) 接收：新区间由各孩子的旧分块拼成；每个孩子最多有 2~3 个旧分块与
        //     新区间相交（旧分块属于不同处理器，各发一段）。
        struct Incoming {
            int childSeq{0};       // 父节点 children 里的序号（混合顺序）
            int sender{0};         // groupRank
            long long start{0};    // 绝对像素下标
            long long len{0};
            std::vector<unsigned char> buf;
            int req{-1};
        };
        std::vector<Incoming> incoming;
        incoming.reserve(static_cast<size_t>(parent.children.size()) * 3);
        for (int ci = 0; ci < static_cast<int>(parent.children.size()); ++ci) {
            const Swap23::Node& cn =
                    tree.nodes[static_cast<size_t>(parent.children[static_cast<size_t>(ci)])];
            const int cnRanks = cn.hi - cn.lo;
            for (int q = Swap23::FirstPieceIntersecting(newLo, N, cnRanks); q < cnRanks;
                 ++q) {
                const long long pieceLo = Swap23::FlatOffset(q, N, cnRanks);
                if (pieceLo >= newHi) { break; }
                const long long pieceHi =
                        pieceLo + Swap23::FlatLen(q, N, cnRanks);
                const long long is = std::max(pieceLo, newLo);
                const long long ie = std::min(pieceHi, newHi);
                if (ie <= is) { continue; }
                const int sender = cn.order[static_cast<size_t>(q)];
                if (sender == groupRank) { continue; } // 本地片段，混合时直接取

                Incoming in;
                in.childSeq = ci;
                in.sender = sender;
                in.start = is;
                in.len = ie - is;
                const long long maxMsg = 4 + in.len * 4 + 8; // 消息头 + 最坏密集
                in.buf.resize(static_cast<size_t>(maxMsg));
                in.req = ctx->Irecv(
                        reinterpret_cast<char*>(in.buf.data()),
                        static_cast<int>(maxMsg),
                        m_SortOrder[static_cast<size_t>(sender)], d);
                incoming.push_back(std::move(in));
            }
        }

        // 5c) 混合：按孩子顺序（= 深度序的连续区间）front-to-back over。
        std::vector<float> acc(static_cast<size_t>(newLen) * 4, 0.0f);
        std::size_t incomingCursor = 0;
        for (int ci = 0; ci < static_cast<int>(parent.children.size()); ++ci) {
            const Swap23::Node& cn =
                    tree.nodes[static_cast<size_t>(parent.children[static_cast<size_t>(ci)])];
            const int cnRanks = cn.hi - cn.lo;
            for (int q = Swap23::FirstPieceIntersecting(newLo, N, cnRanks); q < cnRanks;
                 ++q) {
                const long long pieceLo = Swap23::FlatOffset(q, N, cnRanks);
                if (pieceLo >= newHi) { break; }
                const long long pieceHi =
                        pieceLo + Swap23::FlatLen(q, N, cnRanks);
                long long is = std::max(pieceLo, newLo);
                long long ie = std::min(pieceHi, newHi);
                if (ie <= is) { continue; }
                const int sender = cn.order[static_cast<size_t>(q)];
                if (sender == groupRank) {
                    // 本 rank 自己的旧分块，直接从 work 里切。
                    OverDenseInto(acc.data(), is - newLo,
                                  work.data() + (is - oldLo) * 4, ie - is);
                    continue;
                }

                // 找到对应的接收项（发送顺序与这里一致：孩子序 + 分块序号）。
                if (incomingCursor >= incoming.size() ||
                    incoming[incomingCursor].childSeq != ci) {
                    std::cerr << "[iGameCompositePass] 2-3 swap receive bookkeeping "
                                 "mismatch (rank "
                              << rank << ", depth " << d << ").\n";
                    continue;
                }
                Incoming& in = incoming[incomingCursor];
                ctx->Wait(in.req);
                long long gotStart = static_cast<long long>(GetU32(in.buf.data(), 0));
                long long gotLen = static_cast<long long>(GetU32(in.buf.data(), 4));
                if (gotStart != is || gotLen != ie - is) {
                    std::cerr << "[iGameCompositePass] 2-3 swap message range "
                                 "mismatch (rank "
                              << rank << ", depth " << d << ", expected [" << is
                              << ',' << ie << "), got [" << gotStart << ','
                              << (gotStart + gotLen) << "]).\n";
                    // 仍按发送方给的区间处理；越界则丢弃。
                    if (gotStart < newLo || gotStart + gotLen > newHi ||
                        gotLen <= 0) {
                        ++incomingCursor;
                        continue;
                    }
                    is = gotStart;
                    ie = gotStart + gotLen;
                }
                std::vector<unsigned char> dense =
                        UnpackDense(in.buf.data() + 8, in.buf.size() - 8, ie - is);
                OverDenseInto(acc.data(), is - newLo, dense.data(), ie - is);
                ++incomingCursor;
            }
        }
        for (int req : sendReqs) { ctx->Wait(req); }

        // 5d) 量化回 uint8，作为下一层（或收尾 Gatherv）的输入。
        work.resize(static_cast<size_t>(newLen) * 4);
        for (long long i = 0; i < newLen * 4; ++i) {
            work[static_cast<size_t>(i)] = ToUChar(acc[static_cast<size_t>(i)]);
        }
    }

    // 6) 最终分块序号 = 本 rank 在根节点 order 里的位置；Gatherv 到 rank 0 拼合。
    const int finalPos = Swap23::PosOf(tree, 0, groupRank);
    const long long finalLen = Swap23::FlatLen(finalPos, N, size);

    std::vector<int> recvCounts(static_cast<size_t>(size), 0);
    std::vector<int> displs(static_cast<size_t>(size), 0);
    for (int r2 = 0; r2 < size; ++r2) {
        const int g2 = worldToGroup[static_cast<size_t>(r2)];
        if (g2 < 0) { continue; }
        const int pos2 = Swap23::PosOf(tree, 0, g2);
        recvCounts[static_cast<size_t>(r2)] =
                static_cast<int>(Swap23::FlatLen(pos2, N, size) * 4);
        displs[static_cast<size_t>(r2)] =
                static_cast<int>(Swap23::FlatOffset(pos2, N, size) * 4);
    }

    const long long sendLen = recvCounts[static_cast<size_t>(rank)];
    if (static_cast<long long>(work.size()) < sendLen) {
        std::cerr << "[iGameCompositePass] 2-3 swap piece size mismatch; padding "
                     "before Gatherv.\n";
        work.resize(static_cast<size_t>(sendLen), 0);
    } else if (static_cast<long long>(work.size()) > sendLen) {
        work.resize(static_cast<size_t>(sendLen));
    }

    std::vector<unsigned char> roiFrame;
    if (isRoot) { roiFrame.assign(static_cast<size_t>(N) * 4, 0); }
    ctx->Gatherv(reinterpret_cast<const char*>(work.data()),
                 recvCounts[static_cast<size_t>(rank)],
                 reinterpret_cast<char*>(roiFrame.empty() ? nullptr
                                                          : roiFrame.data()),
                 recvCounts.data(), displs.data(), 0);

    if (!isRoot) {
        ctx->Barrier();
        return true;
    }

    // rank 0：拼出的 ROI 直接就是预乘 alpha 的合成结果；写出
    //   m_ResultRGBA    = 全帧不透明（ROI 之外填背景色）
    //   m_ResultROIRGBA = 预乘 ROI（C/S）/ 叠背景后的不透明 ROI（批渲染）
    m_ResultROIX = rx0;
    m_ResultROIY = ry0;
    m_ResultROIW = roiW;
    m_ResultROIH = roiH;
    ComposeROIOutputs(roiFrame, roiW, roiH, rx0, ry0, width, height,
                      m_Background, m_TransparentROI, m_ResultRGBA,
                      m_ResultROIRGBA);
    ctx->Barrier();
    return true;
}


bool iGameCompositePass::Composite() {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();

    // 合成策略分流（所有 rank 依据同一组开关走到同一分支，不会失配）：
    //   1) --2-3-swap：2-3 swap 合成（任意 P、O(log P) 轮、通信与合成分摊到所有
    //      rank，无 rank0 单点汇聚，适合上千 rank）；
    //   2) 稀疏 ROI 合成（默认 / --direct）：只汇聚每个 rank 的非空像素外接矩形，
    //      上千 rank 下汇聚量与 rank 0 工作量都和 P 基本解耦。
    if (m_Use23SwapComposite && size > 1) {
        return Composite23Swap();
    }
    if (m_UseSparseComposite) {
        return CompositeSparse();
    }
    return CompositeDenseGather();
}

bool iGameCompositePass::CompositeDenseGather() {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();
    const int rank = ctx->Rank();
    const bool isRoot = (rank == 0);

    // 1) 收集各 rank 的图像尺寸并校验一致（AllGather，所有 rank 结果一致，避免死锁）。
    double localDims[2] = {static_cast<double>(m_Width),
                           static_cast<double>(m_Height)};
    std::vector<double> allDims(static_cast<size_t>(size) * 2);
    ctx->AllGather(localDims, allDims.data(), 2);

    int width = m_Width;
    int height = m_Height;
    bool dimsAgree = (size >= 1);
    for (int r = 0; r < size; ++r) {
        if (static_cast<int>(allDims[static_cast<size_t>(r) * 2]) != m_Width ||
            static_cast<int>(allDims[static_cast<size_t>(r) * 2 + 1]) !=
                    m_Height) {
            dimsAgree = false;
            break;
        }
    }
    if (!dimsAgree || width <= 0 || height <= 0) {
        if (isRoot) {
            std::cerr << "[iGameCompositePass] image size mismatch or empty "
                         "across ranks.\n";
        }
        return false;
    }

    // 2) 收集各 rank 的超块深度，得到全局排序（近 -> 远）。
    std::vector<double> allBlockDepth(static_cast<size_t>(size));
    ctx->AllGather(&m_BlockDepth, allBlockDepth.data(), 1);

    m_SortOrder.resize(static_cast<size_t>(size));
    for (int r = 0; r < size; ++r) { m_SortOrder[r] = r; }
    std::sort(m_SortOrder.begin(), m_SortOrder.end(),
              [&](int a, int b) {
                  if (allBlockDepth[static_cast<size_t>(a)] !=
                      allBlockDepth[static_cast<size_t>(b)]) {
                      return allBlockDepth[static_cast<size_t>(a)] <
                             allBlockDepth[static_cast<size_t>(b)];
                  }
                  return a < b; // 确定性兜底
              });

    // 3) Gather 各 rank 的 RGBA 到 rank 0（blend 合成不需要深度）。
    const int pixelCount = width * height;
    const int rgbaCount = pixelCount * 4; // 每 rank 的字节数

    std::vector<unsigned char> allRGBA;
    if (isRoot) {
        allRGBA.resize(static_cast<size_t>(rgbaCount) * size);
    }

    ctx->Gather(reinterpret_cast<const char*>(m_LocalRGBA.data()),
                reinterpret_cast<char*>(allRGBA.data()), rgbaCount, 0);

    // 4) rank 0 按超块全局深度顺序 front-to-back 合成。
    if (isRoot) {
        m_ResultWidth = width;
        m_ResultHeight = height;
        CompositeOnRoot(size, allRGBA);
        if (!m_TransparentROI) { ComputeResultROIFromFullFrame(); }
    } else {
        m_ResultWidth = width;
        m_ResultHeight = height;
    }

    ctx->Barrier();
    return true;
}

// 从「已叠背景色的不透明全帧」里扫出非背景像素的外接矩形，并切出 ROI 子矩形。
// 只在 rank 0 调用；非背景 = RGB 任一通道非 0（背景色由调用方约定为黑色）。
void iGameCompositePass::ComputeResultROIFromFullFrame() {
    const int w = m_ResultWidth;
    const int h = m_ResultHeight;
    m_ResultROIX = 0;
    m_ResultROIY = 0;
    m_ResultROIW = 0;
    m_ResultROIH = 0;
    m_ResultROIRGBA.clear();
    if (w <= 0 || h <= 0 ||
        m_ResultRGBA.size() < static_cast<size_t>(w) * h * 4) {
        return;
    }

    int x0 = w, y0 = h, x1 = -1, y1 = -1;
    for (int y = 0; y < h; ++y) {
        const unsigned char* row =
                m_ResultRGBA.data() + static_cast<size_t>(y) * w * 4;
        for (int x = 0; x < w; ++x) {
            if (row[x * 4 + 0] || row[x * 4 + 1] || row[x * 4 + 2]) {
                if (x < x0) { x0 = x; }
                if (x > x1) { x1 = x; }
                if (y < y0) { y0 = y; }
                if (y > y1) { y1 = y; }
            }
        }
    }
    if (x1 < x0 || y1 < y0) { return; }

    m_ResultROIX = x0;
    m_ResultROIY = y0;
    m_ResultROIW = x1 - x0 + 1;
    m_ResultROIH = y1 - y0 + 1;
    m_ResultROIRGBA.resize(static_cast<size_t>(m_ResultROIW) * m_ResultROIH * 4);
    for (int y = 0; y < m_ResultROIH; ++y) {
        const unsigned char* src =
                m_ResultRGBA.data() +
                (static_cast<size_t>(y0 + y) * w + x0) * 4;
        std::memcpy(m_ResultROIRGBA.data() +
                            static_cast<size_t>(y) * m_ResultROIW * 4,
                    src, static_cast<size_t>(m_ResultROIW) * 4);
    }
}

// 从「预乘 alpha 全帧」里扫出 alpha!=0 的外接矩形，并切出预乘 RGBA 子矩形
// （alpha 保留，供客户端叠自己的背景；仅 rank 0 调用）。
void iGameCompositePass::ComputeResultROIFromPremultiplied(
        const std::vector<unsigned char>& src) {
    const int w = m_ResultWidth;
    const int h = m_ResultHeight;
    m_ResultROIX = 0;
    m_ResultROIY = 0;
    m_ResultROIW = 0;
    m_ResultROIH = 0;
    m_ResultROIRGBA.clear();
    if (w <= 0 || h <= 0 ||
        src.size() < static_cast<size_t>(w) * h * 4) {
        return;
    }

    int x0 = w, y0 = h, x1 = -1, y1 = -1;
    for (int y = 0; y < h; ++y) {
        const unsigned char* row = src.data() + static_cast<size_t>(y) * w * 4;
        for (int x = 0; x < w; ++x) {
            if (row[x * 4 + 3] != 0) {
                if (x < x0) { x0 = x; }
                if (x > x1) { x1 = x; }
                if (y < y0) { y0 = y; }
                if (y > y1) { y1 = y; }
            }
        }
    }
    if (x1 < x0 || y1 < y0) { return; }

    m_ResultROIX = x0;
    m_ResultROIY = y0;
    m_ResultROIW = x1 - x0 + 1;
    m_ResultROIH = y1 - y0 + 1;
    m_ResultROIRGBA.resize(static_cast<size_t>(m_ResultROIW) * m_ResultROIH * 4);
    for (int y = 0; y < m_ResultROIH; ++y) {
        const unsigned char* row =
                src.data() + (static_cast<size_t>(y0 + y) * w + x0) * 4;
        std::memcpy(m_ResultROIRGBA.data() +
                            static_cast<size_t>(y) * m_ResultROIW * 4,
                    row, static_cast<size_t>(m_ResultROIW) * 4);
    }
}

// ---------------------------------------------------------------------------
// 稀疏 ROI 合成（默认路径）
//
// 动机（对标 IceT 的 valid_pixels_viewport）：每个 rank 只持有体数据的一小块，渲染
// 出的图绝大部分像素 alpha=0；把整张 width×height×4 的图发给 rank 0 是纯浪费。
// 本路径：
//   1) 各 rank 求自己图上「alpha>0 像素」的外接矩形 ROI（4 个 int）；
//   2) AllGatherInt 汇总所有 rank 的 ROI（P×16 字节，P=1000 也只有 16KB）；
//   3) Gatherv 只把各自的 ROI 子矩形按 rank 序汇聚到 rank 0（总字节数 = Σ ROI 面积×4，
//      1000 rank 场景下通常只有 MB 量级，而非 4GB）；
//   4) rank 0 按超块全局深度序（front→back）把每个 rank 的 ROI 矩形 over 到累加缓冲。
//      复杂度 O(Σ ROI 面积) —— 与全图像素数×rank 数无关。
//   5) rank 0 把累加结果叠背景色，输出「全帧不透明 RGBA」+「ROI 子矩形」两份结果。
//
// 正确性：ROI 只裁「alpha 全 0 的像素」，而预乘 alpha 下 alpha=0 ⇒ RGB=0，因此裁掉的
// 像素在 over 合成里本来就不产生任何贡献；按超块深度序做 over 与逐像素循环在数学上完全
// 等价（超块互不重叠、可全局深度排序，见 iGameCompositePass 类注释）。
// ---------------------------------------------------------------------------
bool iGameCompositePass::CompositeSparse() {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();
    const int rank = ctx->Rank();
    const bool isRoot = (rank == 0);

    // 1) 尺寸一致性校验（AllGather 全员一致，避免死锁）。
    double localDims[2] = {static_cast<double>(m_Width),
                           static_cast<double>(m_Height)};
    std::vector<double> allDims(static_cast<size_t>(size) * 2);
    ctx->AllGather(localDims, allDims.data(), 2);

    const int width = m_Width;
    const int height = m_Height;
    bool dimsAgree = (size >= 1);
    for (int r = 0; r < size; ++r) {
        if (static_cast<int>(allDims[static_cast<size_t>(r) * 2]) != m_Width ||
            static_cast<int>(allDims[static_cast<size_t>(r) * 2 + 1]) !=
                    m_Height) {
            dimsAgree = false;
            break;
        }
    }
    if (!dimsAgree || width <= 0 || height <= 0) {
        if (isRoot) {
            std::cerr << "[iGameCompositePass] image size mismatch or empty "
                         "across ranks.\n";
        }
        return false;
    }

    // 2) 本 rank 的有效像素 ROI（alpha>0 的外接矩形）。
    int lx0 = width, ly0 = height, lx1 = -1, ly1 = -1;
    if (m_LocalRGBA.size() >= static_cast<size_t>(width) * height * 4) {
        for (int y = 0; y < height; ++y) {
            const unsigned char* row =
                    m_LocalRGBA.data() + static_cast<size_t>(y) * width * 4;
            for (int x = 0; x < width; ++x) {
                if (row[x * 4 + 3] != 0) {
                    if (x < lx0) { lx0 = x; }
                    if (x > lx1) { lx1 = x; }
                    if (y < ly0) { ly0 = y; }
                    if (y > ly1) { ly1 = y; }
                }
            }
        }
    }
    const bool localEmpty = (lx1 < lx0 || ly1 < ly0);
    const int localROI[4] = {localEmpty ? 0 : lx0, localEmpty ? 0 : ly0,
                             localEmpty ? 0 : (lx1 - lx0 + 1),
                             localEmpty ? 0 : (ly1 - ly0 + 1)};

    std::vector<int> allROI(static_cast<size_t>(size) * 4);
    ctx->AllGatherInt(localROI, allROI.data(), 4);

    // 3) 每 rank 载荷长度与偏移（所有 rank 用同一份 allROI 算出，完全一致）。
    std::vector<int> counts(static_cast<size_t>(size), 0);
    std::vector<int> displs(static_cast<size_t>(size), 0);
    long long total = 0;
    for (int r = 0; r < size; ++r) {
        const int rw = allROI[static_cast<size_t>(r) * 4 + 2];
        const int rh = allROI[static_cast<size_t>(r) * 4 + 3];
        counts[static_cast<size_t>(r)] =
                (rw > 0 && rh > 0) ? (rw * rh * 4) : 0;
        total += counts[static_cast<size_t>(r)];
    }

    // 兜底：Σ 载荷超过 int 上限（几乎只会出现在「每个 rank 都画满整屏」时）则退回全图汇聚。
    // 该判断只依赖 allROI，所有 rank 结论一致，不会走岔。
    if (total > static_cast<long long>(std::numeric_limits<int>::max())) {
        if (isRoot) {
            std::cerr << "[iGameCompositePass] sparse composite payload ("
                      << total << " bytes) exceeds int range; falling back to "
                                 "full-image gather.\n";
        }
        return CompositeDenseGather();
    }
    for (int r = 1; r < size; ++r) {
        displs[static_cast<size_t>(r)] =
                displs[static_cast<size_t>(r - 1)] +
                counts[static_cast<size_t>(r - 1)];
    }

    // 4) 全局块序（近 -> 远），所有 rank 一致。
    std::vector<double> allBlockDepth(static_cast<size_t>(size));
    ctx->AllGather(&m_BlockDepth, allBlockDepth.data(), 1);
    m_SortOrder.resize(static_cast<size_t>(size));
    for (int r = 0; r < size; ++r) { m_SortOrder[r] = r; }
    std::sort(m_SortOrder.begin(), m_SortOrder.end(),
              [&](int a, int b) {
                  if (allBlockDepth[static_cast<size_t>(a)] !=
                      allBlockDepth[static_cast<size_t>(b)]) {
                      return allBlockDepth[static_cast<size_t>(a)] <
                             allBlockDepth[static_cast<size_t>(b)];
                  }
                  return a < b; // 确定性兜底
              });

    // 5) 打包本 rank 的 ROI 子矩形（逐行连续拷贝）。
    std::vector<unsigned char> send;
    if (!localEmpty) {
        send.resize(static_cast<size_t>(counts[static_cast<size_t>(rank)]));
        for (int y = 0; y < localROI[3]; ++y) {
            const unsigned char* src =
                    m_LocalRGBA.data() +
                    (static_cast<size_t>(localROI[1] + y) * width + localROI[0]) * 4;
            std::memcpy(send.data() + static_cast<size_t>(y) * localROI[2] * 4,
                        src, static_cast<size_t>(localROI[2]) * 4);
        }
    }

    // 6) Gatherv 到 rank 0。rank0 的接收缓冲用整块连续内存，各 rank 按 displs 落位。
    std::vector<unsigned char> allPayload;
    if (isRoot) { allPayload.resize(static_cast<size_t>(total)); }
    ctx->Gatherv(reinterpret_cast<const char*>(send.empty() ? nullptr : send.data()),
                 counts[static_cast<size_t>(rank)],
                 reinterpret_cast<char*>(allPayload.empty() ? nullptr
                                                            : allPayload.data()),
                 counts.data(), displs.data(), 0);

    m_ResultWidth = width;
    m_ResultHeight = height;
    if (!isRoot) {
        ctx->Barrier();
        return true;
    }

    // 7) rank 0：并集 ROI + 按块序 over。
    int ux0 = width, uy0 = height, ux1 = -1, uy1 = -1;
    for (int r = 0; r < size; ++r) {
        const int rw = allROI[static_cast<size_t>(r) * 4 + 2];
        const int rh = allROI[static_cast<size_t>(r) * 4 + 3];
        if (rw <= 0 || rh <= 0) { continue; }
        const int rx = allROI[static_cast<size_t>(r) * 4 + 0];
        const int ry = allROI[static_cast<size_t>(r) * 4 + 1];
        ux0 = std::min(ux0, rx);
        uy0 = std::min(uy0, ry);
        ux1 = std::max(ux1, rx + rw - 1);
        uy1 = std::max(uy1, ry + rh - 1);
    }

    m_ResultROIX = 0;
    m_ResultROIY = 0;
    m_ResultROIW = 0;
    m_ResultROIH = 0;
    m_ResultROIRGBA.clear();
    m_ResultRGBA.assign(static_cast<size_t>(width) * height * 4, 0);
    // 全帧输出是「不透明」的：alpha 恒 255（背景黑）。
    for (size_t p = 3; p < m_ResultRGBA.size(); p += 4) { m_ResultRGBA[p] = 255; }

    if (ux1 >= ux0 && uy1 >= uy0) {
        const int uw = ux1 - ux0 + 1;
        const int uh = uy1 - uy0 + 1;
        // 累加缓冲：预乘颜色 + alpha（float，避免 UINT8 反复量化带来的合成误差）。
        std::vector<float> acc(static_cast<size_t>(uw) * uh * 4, 0.0f);

        for (int i = 0; i < size; ++i) {
            const int r = m_SortOrder[static_cast<size_t>(i)];
            const int cnt = counts[static_cast<size_t>(r)];
            if (cnt <= 0) { continue; }
            const int rx = allROI[static_cast<size_t>(r) * 4 + 0];
            const int ry = allROI[static_cast<size_t>(r) * 4 + 1];
            const int rw = allROI[static_cast<size_t>(r) * 4 + 2];
            const int rh = allROI[static_cast<size_t>(r) * 4 + 3];
            const unsigned char* src = allPayload.data() + displs[static_cast<size_t>(r)];
            const int bx = rx - ux0;
            const int by = ry - uy0;
            for (int y = 0; y < rh; ++y) {
                const unsigned char* srow = src + static_cast<size_t>(y) * rw * 4;
                float* arow = acc.data() +
                              (static_cast<size_t>(by + y) * uw + bx) * 4;
                for (int x = 0; x < rw; ++x) {
                    const float fa = static_cast<float>(srow[x * 4 + 3]) / 255.0f;
                    if (fa <= 0.0f) { continue; }
                    float* ap = arow + static_cast<size_t>(x) * 4;
                    const float inv = 1.0f - ap[3];
                    if (inv <= 0.001f) { continue; }
                    ap[0] += inv * (static_cast<float>(srow[x * 4 + 0]) / 255.0f);
                    ap[1] += inv * (static_cast<float>(srow[x * 4 + 1]) / 255.0f);
                    ap[2] += inv * (static_cast<float>(srow[x * 4 + 2]) / 255.0f);
                    ap[3] += inv * fa;
                }
            }
        }

        auto toUChar = [](float v) {
            return static_cast<unsigned char>(
                    std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
        };

        m_ResultROIX = ux0;
        m_ResultROIY = uy0;
        m_ResultROIW = uw;
        m_ResultROIH = uh;
        m_ResultROIRGBA.resize(static_cast<size_t>(uw) * uh * 4);
        for (int y = 0; y < uh; ++y) {
            const float* arow = acc.data() + static_cast<size_t>(y) * uw * 4;
            unsigned char* drow =
                    m_ResultROIRGBA.data() + static_cast<size_t>(y) * uw * 4;
            unsigned char* frow =
                    m_ResultRGBA.data() +
                    (static_cast<size_t>(uy0 + y) * width + ux0) * 4;
            for (int x = 0; x < uw; ++x) {
                const float* ap = arow + static_cast<size_t>(x) * 4;
                const float inv = 1.0f - ap[3];
                if (m_TransparentROI) {
                    // 预乘 alpha 输出：背景（alpha=0）交给客户端叠它自己的背景。
                    drow[x * 4 + 0] = toUChar(ap[0]);
                    drow[x * 4 + 1] = toUChar(ap[1]);
                    drow[x * 4 + 2] = toUChar(ap[2]);
                    drow[x * 4 + 3] = toUChar(ap[3]);
                } else {
                    drow[x * 4 + 0] = toUChar(ap[0] + m_Background[0] * inv);
                    drow[x * 4 + 1] = toUChar(ap[1] + m_Background[1] * inv);
                    drow[x * 4 + 2] = toUChar(ap[2] + m_Background[2] * inv);
                    drow[x * 4 + 3] = 255;
                }
                // 全帧结果始终是不透明背景（批渲染 / 交互窗口口径不变）。
                const unsigned char rr = toUChar(ap[0] + m_Background[0] * inv);
                const unsigned char gg = toUChar(ap[1] + m_Background[1] * inv);
                const unsigned char bb = toUChar(ap[2] + m_Background[2] * inv);
                frow[x * 4 + 0] = rr;
                frow[x * 4 + 1] = gg;
                frow[x * 4 + 2] = bb;
                frow[x * 4 + 3] = 255;
            }
        }
    }

    ctx->Barrier();
    return true;
}

void iGameCompositePass::CompositeOnRoot(
        int size, const std::vector<unsigned char>& allRGBA) {
    const int n = m_Width * m_Height;
    m_ResultRGBA.assign(static_cast<size_t>(n) * 4, 0);
    std::vector<unsigned char> premul;
    if (m_TransparentROI) { premul.assign(static_cast<size_t>(n) * 4, 0); }

    auto toUChar = [](float v) {
        const float c = std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f;
        return static_cast<unsigned char>(c);
    };

    // 块级有序合成（front-to-back "over"，预乘 alpha）：
    // m_SortOrder 已是「近→远」的全局超块序（Composite() 里由 AllGather 块深度算出，所有 rank 一致）。
    // 超块空间互不重叠且可全局排序，因此按块序 over 即等价于正确合成，无需逐像素深度排序，
    // 也不需要深度缓冲（对标 IceT 有序 BLEND 的 ICET_IMAGE_DEPTH_NONE）。
    for (int p = 0; p < n; ++p) {
        float c0 = 0.0f, c1 = 0.0f, c2 = 0.0f, a = 0.0f;
        for (int i = 0; i < size && a < 0.999f; ++i) {
            const int r = m_SortOrder[static_cast<size_t>(i)];
            const size_t base =
                    static_cast<size_t>(r) * n * 4 + static_cast<size_t>(p) * 4;
            const float fragA = static_cast<float>(allRGBA[base + 3]) / 255.0f;
            if (fragA <= 0.0f) { continue; }
            const float inv = 1.0f - a;
            c0 += (static_cast<float>(allRGBA[base + 0]) / 255.0f) * inv;
            c1 += (static_cast<float>(allRGBA[base + 1]) / 255.0f) * inv;
            c2 += (static_cast<float>(allRGBA[base + 2]) / 255.0f) * inv;
            a += fragA * inv;
        }

        // 叠到背景色上，输出不透明 RGBA。
        const float inv = 1.0f - a;
        const size_t out = static_cast<size_t>(p) * 4;
        m_ResultRGBA[out + 0] = toUChar(c0 + m_Background[0] * inv);
        m_ResultRGBA[out + 1] = toUChar(c1 + m_Background[1] * inv);
        m_ResultRGBA[out + 2] = toUChar(c2 + m_Background[2] * inv);
        m_ResultRGBA[out + 3] = 255;
        if (m_TransparentROI) {
            premul[out + 0] = toUChar(c0);
            premul[out + 1] = toUChar(c1);
            premul[out + 2] = toUChar(c2);
            premul[out + 3] = toUChar(a);
        }
    }

    if (m_TransparentROI) {
        ComputeResultROIFromPremultiplied(premul);
    }
}

IGAME_NAMESPACE_END
