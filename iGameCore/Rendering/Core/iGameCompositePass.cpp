#include "iGameCompositePass.h"

#include "iGameParallelContext.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

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
// binary-swap 合成辅助函数（对标 IceT `icetBSwapCompose` = radix-k 的 k=2 特例，
// 有序 BLEND / 无深度）
// ---------------------------------------------------------------------------
namespace {
// 预乘 alpha 图叠到背景色上，输出不透明 RGBA8（与稀疏/全图路径输出一致）。
void FlattenToBackground(const std::vector<unsigned char>& src, const float bg[3],
                         int n, std::vector<unsigned char>& out) {
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

// ---- 整幅图被平分成 p 块的「平坦均分」：把 n 个像素平分成 p 块，第 f 块长度。----
long long BinarySwapFlatLen(long long f, long long n, int p) {
    const long long base = n / p;
    const long long rem = n % p;
    return base + (f < rem ? 1 : 0);
}
// 第 f 块在 [0, n) 里的起始偏移。
long long BinarySwapFlatOffset(long long f, long long n, int p) {
    const long long base = n / p;
    const long long rem = n % p;
    return f * base + std::min(f, rem);
}

// 把当前块（长度 size，将最终裂成 eventual 个平坦分块）在本轮裂成 2 半的相对边界
// （3 个偏移：offsets[0]=0、offsets[1]=中点、offsets[2]=size）。等价于「把 size 平分
// 成 eventual 块、每 sub=eventual/2 块合并成一半」，从而保证递归二分 == 最终平坦均分
// （与 IceT icetSparseImageSplitChoosePartitions 一致）。
std::vector<long long> BinarySwapSplitOffsets(int eventual, long long size) {
    long long remainder = size % eventual;
    const int sub = eventual / 2;
    const long long lowerSize = (size / eventual) * sub;
    std::vector<long long> offs(3);
    long long off = 0;
    for (int j = 0; j < 2; ++j) {
        offs[static_cast<size_t>(j)] = off;
        off += lowerSize;
        if (remainder > sub) {
            off += sub;
            remainder -= sub;
        } else {
            off += remainder;
            remainder = 0;
        }
    }
    offs[2] = off;
    return offs;
}

// 按位反序：groupRank（深度序位置，近→远）→ 最终分块索引。binary-swap 每轮取
// groupRank 的一位做「数字」，⌈log₂P₂⌉ 轮递归后每个进程持有的分块索引 = groupRank
// 的按位反序（即 IceT radixkGetFinalPartitionIndex 的 k=2 特例）。
int BinarySwapFinalPartition(int groupRank, int rounds) {
    int fp = 0;
    for (int r = 0; r < rounds; ++r) {
        fp = (fp << 1) | ((groupRank >> r) & 1);
    }
    return fp;
}

// 把 work 里 [relStart, relStart+len) 的密集 RGBA8 打包成可发送的紧凑消息。
// 消息格式（接收端已知 len）：
//   u32 头 = 0             → 全空（无后续载荷）
//   u32 头 = 0xFFFFFFFF   → 密集（后续 len*4 字节原始 RGBA）
//   u32 头 = cnt (其它)    → 稀疏（后续 cnt 个 (u32 relIdx, u8 r,g,b,a)）
// 选择依据：稀疏编码 8 字节/像素，密集 4 字节/像素，取较小者（避免把密集块放大 2 倍）。
std::vector<unsigned char> BinarySwapPack(const std::vector<unsigned char>& work,
                                          long long relStart, long long len) {
    auto putU32 = [](std::vector<unsigned char>& b, std::uint32_t v) {
        b.push_back(static_cast<unsigned char>(v & 0xff));
        b.push_back(static_cast<unsigned char>((v >> 8) & 0xff));
        b.push_back(static_cast<unsigned char>((v >> 16) & 0xff));
        b.push_back(static_cast<unsigned char>((v >> 24) & 0xff));
    };

    long long cnt = 0;
    for (long long i = 0; i < len; ++i) {
        if (work[static_cast<size_t>(relStart + i) * 4 + 3] != 0) { ++cnt; }
    }

    std::vector<unsigned char> buf;
    buf.reserve(static_cast<size_t>(4 + len * 4));
    if (cnt == 0) {
        putU32(buf, 0u);
    } else if (cnt * 8 <= len * 4) {
        putU32(buf, static_cast<std::uint32_t>(cnt));
        for (long long i = 0; i < len; ++i) {
            const std::size_t b = static_cast<size_t>(relStart + i) * 4;
            if (work[b + 3] != 0) {
                putU32(buf, static_cast<std::uint32_t>(i));
                buf.push_back(work[b + 0]);
                buf.push_back(work[b + 1]);
                buf.push_back(work[b + 2]);
                buf.push_back(work[b + 3]);
            }
        }
    } else {
        putU32(buf, 0xFFFFFFFFu);
        for (long long i = 0; i < len; ++i) {
            const std::size_t b = static_cast<size_t>(relStart + i) * 4;
            buf.push_back(work[b + 0]);
            buf.push_back(work[b + 1]);
            buf.push_back(work[b + 2]);
            buf.push_back(work[b + 3]);
        }
    }
    return buf;
}

// 解包成长度为 len 的密集 RGBA8（非 0 像素按 relIdx 落位，其余保持 0）。
std::vector<unsigned char> BinarySwapUnpack(const std::vector<unsigned char>& buf,
                                            long long len) {
    auto getU32 = [&](std::size_t off) -> std::uint32_t {
        if (off + 4 > buf.size()) { return 0; }
        return static_cast<std::uint32_t>(buf[off]) |
               (static_cast<std::uint32_t>(buf[off + 1]) << 8) |
               (static_cast<std::uint32_t>(buf[off + 2]) << 16) |
               (static_cast<std::uint32_t>(buf[off + 3]) << 24);
    };

    std::vector<unsigned char> dense(static_cast<size_t>(len) * 4, 0);
    if (buf.size() < 4) { return dense; }
    const std::uint32_t h = getU32(0);
    if (h == 0) { return dense; }
    if (h == 0xFFFFFFFFu) {
        const std::size_t want = static_cast<size_t>(len) * 4;
        const std::size_t have = buf.size() - 4;
        if (have > 0) {
            std::memcpy(dense.data(), buf.data() + 4, std::min(want, have));
        }
        return dense;
    }
    std::size_t pos = 4;
    for (std::uint32_t t = 0; t < h; ++t) {
        if (pos + 8 > buf.size()) { break; }
        const std::uint32_t rel = getU32(pos);
        pos += 4;
        if (static_cast<long long>(rel) >= len) { pos += 4; continue; }
        const std::size_t b = static_cast<size_t>(rel) * 4;
        dense[b + 0] = buf[pos + 0];
        dense[b + 1] = buf[pos + 1];
        dense[b + 2] = buf[pos + 2];
        dense[b + 3] = buf[pos + 3];
        pos += 4;
    }
    return dense;
}

// 把两张「等长」的密集预乘 RGBA8 做 front-to-back over（front = 近，back = 远），
// float 累加（与 CompositeSparse 的逐像素公式一致），最后量化回 uint8。
std::vector<unsigned char> BinarySwapCompositeTwo(
        const std::vector<unsigned char>& front,
        const std::vector<unsigned char>& back, long long len) {
    std::vector<float> acc(static_cast<size_t>(len) * 4, 0.0f);
    for (const auto* p : {&front, &back}) {
        if (static_cast<long long>(p->size()) < len * 4) { continue; }
        for (long long i = 0; i < len; ++i) {
            const std::size_t b = static_cast<size_t>(i) * 4;
            const float fa = static_cast<float>((*p)[b + 3]) / 255.0f;
            if (fa <= 0.0f) { continue; }
            const float inv = 1.0f - acc[b + 3];
            if (inv <= 0.001f) { continue; }
            acc[b + 0] += inv * (static_cast<float>((*p)[b + 0]) / 255.0f);
            acc[b + 1] += inv * (static_cast<float>((*p)[b + 1]) / 255.0f);
            acc[b + 2] += inv * (static_cast<float>((*p)[b + 2]) / 255.0f);
            acc[b + 3] += inv * fa;
        }
    }
    auto toU = [](float v) {
        return static_cast<unsigned char>(
                std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    std::vector<unsigned char> out(static_cast<size_t>(len) * 4);
    for (long long i = 0; i < len; ++i) {
        const std::size_t b = static_cast<size_t>(i) * 4;
        out[b + 0] = toU(acc[b + 0]);
        out[b + 1] = toU(acc[b + 1]);
        out[b + 2] = toU(acc[b + 2]);
        out[b + 3] = toU(acc[b + 3]);
    }
    return out;
}
} // namespace

// ---------------------------------------------------------------------------
// binary-swap 合成（对标 IceT `icetBSwapCompose`，有序 BLEND / 无深度）
//
// 算法（见 legacy_doc/并行体渲染通信优化-实现步骤.md 阶段 4.2 与
// `bianry_swap算法实现.md`）：
//   1) AllGather 尺寸校验 + 块深度排序 → m_SortOrder（近→远）；
//      groupRank = 本进程在深度序中的位置（0=最前）。
//   2) 补齐到 2 的幂 P2 = next_pow2(P)；非 2 幂时「幽灵」位置视作全透明、不实际通信。
//   3) 工作图 = 本 rank 的整张局部图（预乘 alpha RGBA8）。
//   4) ⌈log₂P2⌉ 轮：第 r 轮按 groupRank 的第 r 位把当前块平分成两半，与
//      partner = groupRank ^ (1<<r) 交换「自己不负责的那半」、收「自己负责的那半」，
//      再按数字序（=深度序）front-to-back over 合成；partner 是幽灵则无通信、直接留半。
//   5) 结束后每个进程持有一块完全合成的分块（索引 = 按位反序的 groupRank），Gatherv 到
//      rank 0 拼成完整图（分块在最终图上平坦连续、互不重叠，rank0 无需再排序），
//      叠背景输出不透明 RGBA8 + 非空 ROI。
//
// 顺序正确性（Swizzle）：进程按「块深度近→远」得到 groupRank，每轮配对都发生在
// groupRank 相邻数字上——数字小者更靠前（近），因此「按数字序 front-to-back over」
// 恒保持透明度有序；这与 IceT 先 swizzle 再 bswap 完全等价。
// ---------------------------------------------------------------------------
bool iGameCompositePass::CompositeBinarySwap() {
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
            static_cast<int>(allDims[static_cast<size_t>(r) * 2 + 1]) !=
                    height) {
            dimsAgree = false;
            break;
        }
    }
    if (!dimsAgree || width <= 0 || height <= 0) {
        if (isRoot) {
            std::cerr << "[iGameCompositePass] binary-swap image size mismatch "
                         "or empty across ranks.\n";
        }
        return false;
    }

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
    int groupRank = -1;
    for (int g = 0; g < size; ++g) {
        if (m_SortOrder[static_cast<size_t>(g)] == rank) { groupRank = g; break; }
    }
    if (groupRank < 0) { return false; }

    const long long N = static_cast<long long>(width) * height;
    // 极端情况（进程数超过像素数）退回稀疏路径：binary-swap 的平坦均分会出现大量空块。
    if (size < 2 || N < static_cast<long long>(size)) {
        return CompositeSparse();
    }

    // 3) 补齐到 2 的幂（非 2 幂 P：不存在的 rank 视作全透明，实际不通信）。
    int P2 = 1;
    while (P2 < size) { P2 <<= 1; }
    const int rounds =
            (P2 > 1) ? static_cast<int>(std::log2(static_cast<double>(P2))) : 0;

    // 4) 工作图 = 本 rank 的整张局部图（预乘 alpha RGBA8）；截断/补零到恰好 N*4。
    std::vector<unsigned char> work = m_LocalRGBA;
    work.resize(static_cast<size_t>(N) * 4, 0);

    long long pieceLen = N;
    int step = 1;      // = 2^round，本轮组内「数字」的步长
    int remaining = P2; // 当前组大小（每轮减半）

    for (int r = 0; r < rounds; ++r) {
        const int myDigit = (groupRank / step) % 2;
        const int partner = groupRank ^ step;  // 翻转 groupRank 的第 r 位

        // 本轮把当前块平分成两半（与最终的平坦均分一致）。
        const std::vector<long long> offs =
                BinarySwapSplitOffsets(remaining, pieceLen);
        const long long myStart = offs[static_cast<size_t>(myDigit)];
        const long long myLen = offs[static_cast<size_t>(myDigit) + 1] - myStart;

        // 我保留的半张（自己负责的那半）。
        std::vector<unsigned char> myPiece(
                work.begin() + static_cast<size_t>(myStart) * 4,
                work.begin() + static_cast<size_t>(myStart + myLen) * 4);

        if (partner < size) {
            // 真实伙伴：发「我不负责的那半」，收「我负责的那半」。
            const int partnerDigit = 1 - myDigit;
            const long long otherStart = offs[static_cast<size_t>(partnerDigit)];
            const long long otherLen =
                    offs[static_cast<size_t>(partnerDigit) + 1] - otherStart;

            std::vector<unsigned char> sendBuf =
                    BinarySwapPack(work, otherStart, otherLen);
            const long long maxMsg = 4 + myLen * 4;  // 头 + 最坏情况（密集 myLen 像素）
            std::vector<unsigned char> recvBuf(static_cast<size_t>(maxMsg));
            const int worldPartner = m_SortOrder[static_cast<size_t>(partner)];
            const int recvReq =
                    ctx->Irecv(reinterpret_cast<char*>(recvBuf.data()),
                               static_cast<int>(maxMsg), worldPartner, r);
            const int sendReq =
                    ctx->Isend(reinterpret_cast<const char*>(sendBuf.data()),
                               static_cast<int>(sendBuf.size()), worldPartner, r);
            ctx->Wait(recvReq);
            std::vector<unsigned char> otherPiece =
                    BinarySwapUnpack(recvBuf, myLen);
            ctx->Wait(sendReq);

            // pieces[d] = 数字 d（=深度序）的贡献；按数字序 front-to-back over。
            std::vector<std::vector<unsigned char>> pieces(2);
            pieces[static_cast<size_t>(myDigit)] = std::move(myPiece);
            pieces[static_cast<size_t>(partnerDigit)] = std::move(otherPiece);
            work = BinarySwapCompositeTwo(pieces[0], pieces[1], myLen);
        } else {
            // 幽灵伙伴（补齐的非 2 幂空 rank，全透明）：无通信，直接留半张。
            work = std::move(myPiece);
        }

        pieceLen = myLen;
        remaining /= 2;
        step *= 2;
    }

    // 5) 最终分块索引 = 按位反序的 groupRank；Gatherv 到 rank 0 拼合。
    const int finalPartition = BinarySwapFinalPartition(groupRank, rounds);
    const long long finalLen = BinarySwapFlatLen(finalPartition, N, P2);
#if !defined(NDEBUG)
    if (pieceLen != finalLen ||
        static_cast<long long>(work.size()) != finalLen * 4) {
        std::cerr << "[iGameCompositePass] binary-swap piece size mismatch: "
                  << "tracked=" << pieceLen << " expected=" << finalLen << '\n';
    }
#endif

    // 由共享的 m_SortOrder 推出每个 rank 的分块偏移/长度（全员一致）。
    std::vector<int> recvCounts(static_cast<size_t>(size), 0);
    std::vector<int> displs(static_cast<size_t>(size), 0);
    for (int r2 = 0; r2 < size; ++r2) {
        int gr2 = -1;
        for (int g = 0; g < size; ++g) {
            if (m_SortOrder[static_cast<size_t>(g)] == r2) { gr2 = g; break; }
        }
        const int fp2 = BinarySwapFinalPartition(gr2, rounds);
        const long long len2 = BinarySwapFlatLen(fp2, N, P2);
        const long long off2 = BinarySwapFlatOffset(fp2, N, P2);
        recvCounts[static_cast<size_t>(r2)] = static_cast<int>(len2 * 4);
        displs[static_cast<size_t>(r2)] = static_cast<int>(off2 * 4);
    }

    m_ResultWidth = width;
    m_ResultHeight = height;
    std::vector<unsigned char> fullFrame;
    if (isRoot) { fullFrame.assign(static_cast<size_t>(N) * 4, 0); }
    ctx->Gatherv(reinterpret_cast<const char*>(work.data()),
                 recvCounts[static_cast<size_t>(rank)],
                 reinterpret_cast<char*>(fullFrame.empty() ? nullptr
                                                           : fullFrame.data()),
                 recvCounts.data(), displs.data(), 0);

    if (!isRoot) {
        ctx->Barrier();
        return true;
    }

    // rank 0：叠背景色输出不透明 RGBA8 + 非空 ROI（与其它合成路径口径一致）。
    m_ResultRGBA.assign(static_cast<size_t>(N) * 4, 0);
    FlattenToBackground(fullFrame, m_Background, width * height, m_ResultRGBA);
    ComputeResultROIFromFullFrame();
    ctx->Barrier();
    return true;
}

bool iGameCompositePass::Composite() {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();

    // 合成策略分流（所有 rank 依据同一组开关走到同一分支，不会失配）：
    //   1) --binary-swap：binary-swap 合成（O(log P) 轮、通信与合成分摊到所有 rank，
    //      无 rank0 单点汇聚，适合上千 rank；对标 IceT icetBSwapCompose）；
    //   2) 稀疏 ROI 合成（默认 / --direct）：只汇聚每个 rank 的非空像素外接矩形，
    //      上千 rank 下汇聚量与 rank 0 工作量都和 P 基本解耦。
    if (m_UseBinarySwapComposite && size > 1) {
        return CompositeBinarySwap();
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
        ComputeResultROIFromFullFrame();
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
                const unsigned char rr =
                        toUChar(ap[0] + m_Background[0] * inv);
                const unsigned char gg =
                        toUChar(ap[1] + m_Background[1] * inv);
                const unsigned char bb =
                        toUChar(ap[2] + m_Background[2] * inv);
                drow[x * 4 + 0] = rr;
                drow[x * 4 + 1] = gg;
                drow[x * 4 + 2] = bb;
                drow[x * 4 + 3] = 255;
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
    }
}

IGAME_NAMESPACE_END
