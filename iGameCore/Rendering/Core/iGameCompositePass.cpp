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
// 并行树合成（阶段 4，对标 IceT ICET_SINGLE_IMAGE_STRATEGY_TREE）
// ---------------------------------------------------------------------------
namespace {
// front-to-back "over"（两者均为预乘 alpha RGBA8）：out = front OVER back。
void ComposeOver(const std::vector<unsigned char>& front,
                 const std::vector<unsigned char>& back,
                 std::vector<unsigned char>& out, int n) {
    auto toU = [](float v) {
        return static_cast<unsigned char>(
                std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    for (int p = 0; p < n; ++p) {
        const std::size_t b = static_cast<std::size_t>(p) * 4;
        const float fa = static_cast<float>(front[b + 3]) / 255.0f;
        const float ba = static_cast<float>(back[b + 3]) / 255.0f;
        const float inv = 1.0f - fa;
        out[b + 0] = toU(static_cast<float>(front[b + 0]) / 255.0f +
                         static_cast<float>(back[b + 0]) / 255.0f * inv);
        out[b + 1] = toU(static_cast<float>(front[b + 1]) / 255.0f +
                         static_cast<float>(back[b + 1]) / 255.0f * inv);
        out[b + 2] = toU(static_cast<float>(front[b + 2]) / 255.0f +
                         static_cast<float>(back[b + 2]) / 255.0f * inv);
        out[b + 3] = toU(fa + ba * inv);
    }
}

// 预乘 alpha 图叠到背景色上，输出不透明 RGBA8（与 direct-send 路径输出一致）。
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
} // namespace

// ---------------------------------------------------------------------------
// radix-k 合成辅助函数（对标 IceT `src/strategies/radixk.c` / `radixkr.c` +
// `src/ice-t/image.c` 里的 sparse-image split / interlace / offset）。
//
// 本工程只做「有序 BLEND」（预乘 alpha、ICET_IMAGE_DEPTH_NONE 等价），因此这里
// 的合成不涉及深度缓冲，与稀疏/全图路径口径一致。
// ---------------------------------------------------------------------------
namespace {
// ---- 整幅图被平分成 p 块的「平坦均分」（IceT icetSparseImageSplitChoosePartitions
//      递归展开后的最终结果）----
// 把 n 个像素平分成 p 块，第 f 块长度。
long long RadixKFlatLen(long long f, long long n, int p) {
    const long long base = n / p;
    const long long rem = n % p;
    return base + (f < rem ? 1 : 0);
}
// 第 f 块在 [0, n) 里的起始偏移。
long long RadixKFlatOffset(long long f, long long n, int p) {
    const long long base = n / p;
    const long long rem = n % p;
    return f * base + std::min(f, rem);
}

// 表示 [0, p) 所需的最少二进制位数（IceT BIT_REVERSE 的字段宽 = ceil(log2 p)）。
int RadixKBits(int p) {
    int bits = 0;
    while ((1 << bits) < p) { ++bits; }
    return bits;
}
// 按 bits 位做按位反序（IceT BIT_REVERSE）。
int RadixKBitReverse(int x, int bits) {
    int r = 0;
    for (int i = 0; i < bits; ++i) {
        r = (r << 1) | (x & 1);
        x >>= 1;
    }
    return r;
}

// 把进程数 P 分解成 radix 因子序列（IceT radixkGetK，magic_k=8）：
//   1) 能整除 magic_k 就用 magic_k；
//   2) 否则在 [2, 2*magic_k) 内按「magic_k, magic_k-1, magic_k+1, ...」就近找因子；
//   3) 再否则从 2*magic_k 向上找到 sqrt(n)；
//   4) 都没有说明是（大）质数，直接用剩余数作 k。
// 结果保证各因子相乘 == P，且所有 rank 一致（只依赖 P）。
std::vector<int> RadixKFactor(int p) {
    constexpr int kMagicK = 8;
    std::vector<int> kFactors;
    if (p < 2) { return kFactors; }
    int nextDivide = p;
    while (nextDivide > 1) {
        int nextK = -1;
        if (nextDivide % kMagicK == 0) { nextK = kMagicK; }
        if (nextK == -1) {
            for (int it = 1; it < 2 * kMagicK; ++it) {
                const int tryK = (it % 2 == 1) ? (kMagicK + it / 2)
                                               : (kMagicK - it / 2);
                if (tryK < 2 || tryK >= 2 * kMagicK) { continue; }
                if (nextDivide % tryK == 0) { nextK = tryK; break; }
            }
        }
        if (nextK == -1) {
            const int maxK = static_cast<int>(std::floor(std::sqrt(
                    static_cast<double>(nextDivide))));
            for (int tryK = 2 * kMagicK; tryK < maxK; ++tryK) {
                if (nextDivide % tryK == 0) { nextK = tryK; break; }
            }
        }
        if (nextK == -1) { nextK = nextDivide; }
        kFactors.push_back(nextK);
        nextDivide /= nextK;
    }
    return kFactors;
}

// 数字反转：group_rank（compose_group 里的位置，近→远）→ 最终分块索引
// （IceT radixkGetFinalPartitionIndex）。最终分块索引的物理含义是「最终图上第几块」。
int RadixKFinalPartitionIndex(int groupRank, const std::vector<int>& kFactors) {
    int fp = 0;
    int step = 1;
    for (const int k : kFactors) {
        fp = fp * k + (groupRank / step) % k;
        step *= k;
    }
    return fp;
}

// 当前块（长度 size）在「最终会裂成 eventual 块」的前提下，本轮裂成 numPartitions 块
// 的相对边界（numPartitions+1 个偏移，offsets[0]=0）。这与 IceT
// icetSparseImageSplitChoosePartitions 完全一致：等价于「把 size 平分成 eventual 块、
// 每 sub=eventual/numPartitions 块合并成一个本轮子块」，从而保证递归分裂 == 最终平坦均分。
std::vector<long long> RadixKSplitOffsets(int numPartitions, int eventual,
                                          long long size) {
    long long remainder = size % eventual;
    const int sub = eventual / numPartitions;
    const long long lowerSize = (size / eventual) * sub;
    std::vector<long long> offs(static_cast<size_t>(numPartitions) + 1);
    long long off = 0;
    for (int j = 0; j < numPartitions; ++j) {
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
    offs[static_cast<size_t>(numPartitions)] = off;
    return offs;
}

// interlace 偏移（IceT icetGetInterlaceOffset）：interlaced 分块 partitionIndex 里
// 的像素，在「未 interlace 的原图」里应放回的起始偏移。interlace 是按位反序的块重排，
// 因此该偏移 = 原图里「按位反序等于 partitionIndex 的那一块」的起点。
long long RadixKInterlaceOffset(int partitionIndex, int p, long long n) {
    const long long lower = n / p;
    const long long rem = n % p;
    const int bits = RadixKBits(p);
    long long offset = 0;
    for (int op = 0; op < p; ++op) {
        int il = RadixKBitReverse(op, bits);
        if (p <= il) { il = op; }
        if (il == partitionIndex) { return offset; }
        offset += lower + (il < rem ? 1 : 0);
    }
    return 0; // 逻辑上不可达（partitionIndex 必被命中）
}

// interlace（IceT icetSparseImageInterlace）：把 n 像素的密集图按位反序重排 p 个块。
// in/out 均为密集 RGBA8（线性像素序 0..n-1）。
void RadixKInterlace(const std::vector<unsigned char>& in, int p,
                     std::vector<unsigned char>& out) {
    const long long n = static_cast<long long>(in.size()) / 4;
    const long long lower = n / p;
    const long long rem = n % p;
    const int bits = RadixKBits(p);

    // 每个「原块 op」在 in 里的起点：原块 op 的长度 = lower + (bit_reverse(op) < rem)。
    std::vector<long long> origStart(static_cast<size_t>(p) + 1, 0);
    for (int op = 0; op < p; ++op) {
        int il = RadixKBitReverse(op, bits);
        if (p <= il) { il = op; }
        const long long sz = lower + (il < rem ? 1 : 0);
        origStart[static_cast<size_t>(op) + 1] = origStart[static_cast<size_t>(op)] + sz;
    }

    out.resize(static_cast<size_t>(n) * 4);
    // 输出块 j = 原块 bit_reverse(j)；输出块 j 的起点 = j*lower + min(j, rem)。
    for (int j = 0; j < p; ++j) {
        int op = RadixKBitReverse(j, bits);
        if (p <= op) { op = j; }
        const long long sz = lower + (j < rem ? 1 : 0);
        const long long src = origStart[static_cast<size_t>(op)];
        const long long dst = static_cast<long long>(j) * lower +
                              std::min(static_cast<long long>(j), rem);
        std::memcpy(&out[static_cast<size_t>(dst) * 4],
                    &in[static_cast<size_t>(src) * 4],
                    static_cast<size_t>(sz) * 4);
    }
}

// 把 work 里 [relStart, relStart+len) 的密集 RGBA8 打包成可发送的紧凑消息。
// 消息格式（接收端已知 len）：
//   u32 头 = 0             → 全空（无后续载荷）
//   u32 头 = 0xFFFFFFFF   → 密集（后续 len*4 字节原始 RGBA）
//   u32 头 = cnt (其它)    → 稀疏（后续 cnt 个 (u32 relIdx, u8 r,g,b,a)）
// 选择依据：稀疏编码 8 字节/像素，密集 4 字节/像素，取较小者（避免把密集块放大 2 倍）。
std::vector<unsigned char> RadixKPack(const std::vector<unsigned char>& work,
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
std::vector<unsigned char> RadixKUnpack(const std::vector<unsigned char>& buf,
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

// 把 k 张「等长」的密集预乘 RGBA8（pieces 已按数字序 = 深度序 近→远 排好）做
// front-to-back over，float 累加（与 CompositeSparse 的逐像素公式一致），最后量化回
// uint8。pieces[0] 是最前（近），依次到 pieces[k-1] 最后（远）。
std::vector<unsigned char> RadixKCompositePieces(
        const std::vector<std::vector<unsigned char>>& pieces, long long len) {
    std::vector<float> acc(static_cast<size_t>(len) * 4, 0.0f);
    for (const auto& p : pieces) {
        if (static_cast<long long>(p.size()) < len * 4) { continue; }
        for (long long i = 0; i < len; ++i) {
            const std::size_t b = static_cast<size_t>(i) * 4;
            const float fa = static_cast<float>(p[b + 3]) / 255.0f;
            if (fa <= 0.0f) { continue; }
            const float inv = 1.0f - acc[b + 3];
            if (inv <= 0.001f) { continue; }
            acc[b + 0] += inv * (static_cast<float>(p[b + 0]) / 255.0f);
            acc[b + 1] += inv * (static_cast<float>(p[b + 1]) / 255.0f);
            acc[b + 2] += inv * (static_cast<float>(p[b + 2]) / 255.0f);
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

bool iGameCompositePass::CompositeTree() {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();
    const int rank = ctx->Rank();

    // 1) 尺寸校验（AllGather 全员一致，避免死锁）。
    double localDims[2] = {static_cast<double>(m_Width),
                           static_cast<double>(m_Height)};
    std::vector<double> allDims(static_cast<size_t>(size) * 2);
    ctx->AllGather(localDims, allDims.data(), 2);
    int width = m_Width, height = m_Height;
    bool dimsAgree = (size >= 1);
    for (int r = 0; r < size; ++r) {
        if (static_cast<int>(allDims[static_cast<size_t>(r) * 2]) != m_Width ||
            static_cast<int>(allDims[static_cast<size_t>(r) * 2 + 1]) !=
                    m_Height) {
            dimsAgree = false;
            break;
        }
    }
    if (!dimsAgree || width <= 0 || height <= 0) { return false; }

    // 2) 全局块序（AllGather 块深度 → m_SortOrder 近→远，全员一致）。
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
                  return a < b;
              });

    // 3) 我在近→远序列中的位置（0=最前）。
    int myPos = -1;
    for (int p = 0; p < size; ++p) {
        if (m_SortOrder[static_cast<size_t>(p)] == rank) { myPos = p; break; }
    }
    if (myPos < 0) { return false; }

    // 4) 有序二叉树合成：每轮「前位接收后方并 front OVER back 合成，后位发送后退出」。
    const int n = width * height;
    const int rgbaBytes = n * 4;
    std::vector<unsigned char> work = m_LocalRGBA;   // 当前累积图（预乘 alpha）
    std::vector<unsigned char> recv(static_cast<size_t>(rgbaBytes));
    std::vector<unsigned char> tmp(static_cast<size_t>(rgbaBytes));

    const int mRounds =
            (size > 1)
                    ? static_cast<int>(
                              std::ceil(std::log2(static_cast<double>(size))))
                    : 0;

    for (int r = 0; r < mRounds; ++r) {
        const int stride = 1 << r;
        if (myPos % (stride * 2) == 0) {
            // 前位：接收后方、front OVER back 合成。
            const int partnerPos = myPos + stride;
            if (partnerPos < size) {
                const int partnerRank =
                        m_SortOrder[static_cast<size_t>(partnerPos)];
                const int req = ctx->Irecv(reinterpret_cast<char*>(recv.data()),
                                           rgbaBytes, partnerRank, 0);
                ctx->Wait(req);
                ComposeOver(work, recv, tmp, n);
                work.swap(tmp);
            }
            // partnerPos >= size：无伙伴，保持 work。
        } else {
            // 后位：把当前累积图发给前位后退出本轮。
            const int frontRank =
                    m_SortOrder[static_cast<size_t>(myPos - stride)];
            const int req = ctx->Isend(reinterpret_cast<const char*>(work.data()),
                                       rgbaBytes, frontRank, 0);
            ctx->Wait(req);
            break;
        }
    }

    // 5) 前最位（myPos==0）持有完整预乘图；若非 rank 0，发给 rank 0。
    //    rank 0 叠背景色输出不透明 RGBA8（与 direct-send 路径口径一致）。
    if (myPos == 0 && rank != 0) {
        const int req = ctx->Isend(reinterpret_cast<const char*>(work.data()),
                                   rgbaBytes, 0, 1);
        ctx->Wait(req);
    }
    if (rank == 0) {
        if (myPos != 0) {
            const int frontRank = m_SortOrder[0];
            const int req = ctx->Irecv(reinterpret_cast<char*>(work.data()),
                                       rgbaBytes, frontRank, 1);
            ctx->Wait(req);
        }
        m_ResultWidth = width;
        m_ResultHeight = height;
        m_ResultRGBA.assign(static_cast<size_t>(rgbaBytes), 0);
        FlattenToBackground(work, m_Background, n, m_ResultRGBA);
        ComputeResultROIFromFullFrame();
    } else {
        m_ResultWidth = width;
        m_ResultHeight = height;
    }

    ctx->Barrier();
    return true;
}

bool iGameCompositePass::Composite() {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();

    // 合成策略分流（所有 rank 依据同一组开关走到同一分支，不会失配）：
    //   1) --tree：并行树合成（O(log P) 轮，适合每 rank 载荷本来就很大的场景）；
    //   2) --radix-k：radix-k 合成（O(log_k P) 轮、通信与合成分摊到所有 rank，
    //      无 rank0 单点汇聚，适合上万 rank；对标 IceT icetRadixkCompose）；
    //   3) 稀疏 ROI 合成（默认）：只汇聚每个 rank 的非空像素外接矩形，上千 rank 下
    //      汇聚量与 rank 0 工作量都和 P 基本解耦——这是 1000 rank 场景的默认路径；
    //   4) direct-send 全图汇聚（阶段 3 原路径，作为稀疏路径的兜底）。
    if (m_UseTreeComposite && size > 1) {
        return CompositeTree();
    }
    if (m_UseRadixKComposite && size > 1) {
        return CompositeRadixK();
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

// ---------------------------------------------------------------------------
// radix-k 合成（对标 IceT `icetRadixkCompose`，有序 BLEND / 无深度）
//
// 流程：
//   1) AllGather 尺寸校验 + 块深度排序 → m_SortOrder（近→远）。
//   2) compose_group[g] = m_SortOrder[g]（世界 rank 按深度序排列），group_rank = 本进程位置。
//   3) 把 P 分解成 radix 因子（RadixKFactor，magic_k=8）。
//   4) 工作图 = 本 rank 的整张局部图；若轮数 > 1 先做 interlace（按位反序块重排，负载均衡）。
//   5) 每轮：把当前块按 k 分裂成 k 个子块（RadixKSplitOffsets），与组内 k-1 个伙伴做
//      「转置」交换（我发子块 j 给伙伴 j、收伙伴 j 的子块 d），再按数字序（=深度序）
//      front-to-back over 合成；d = (group_rank/step)%k 是本轮数字。
//   6) 结束后每个进程持有一块完全合成的分块（索引 = 数字反转的 group_rank），Gatherv 到
//      rank 0 拼成完整图（interlace 时按 RadixKInterlaceOffset 落回原位置），叠背景输出。
//
// 顺序正确性（Swizzle）：组内伙伴按数字序排列，数字小者更靠前（近），因此「按数字序
// front-to-back over」即保持透明度有序；轮次递归后每个进程持有的分块索引是 group_rank
// 的数字反转，且分块在最终图上连续，Gatherv 即可无损重组。
// ---------------------------------------------------------------------------
bool iGameCompositePass::CompositeRadixK() {
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
            std::cerr << "[iGameCompositePass] radix-k image size mismatch or "
                         "empty across ranks.\n";
        }
        return false;
    }

    // 2) 全局块序（近 -> 远）。compose_group[g] = 世界 rank；groupRank = 我的位置。
    std::vector<double> allBlockDepth(static_cast<size_t>(size));
    ctx->AllGather(&m_BlockDepth, allBlockDepth.data(), 1);
    m_SortOrder.resize(static_cast<size_t>(size));
    for (int r = 0; r < size; ++r) { m_SortOrder[r] = r; }
    std::sort(m_SortOrder.begin(), m_SortOrder.end(), [&](int a, int b) {
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
    // 极端情况（进程数超过像素数 / 单进程）退回稀疏路径：radix-k 的平坦均分会出现大量
    // 空块，稀疏 Gatherv 更合适且已覆盖该情形。
    if (size < 2 || N < static_cast<long long>(size)) {
        return CompositeSparse();
    }

    // 3) radix 因子与轮数。
    const std::vector<int> kFactors = RadixKFactor(size);
    const int numRounds = static_cast<int>(kFactors.size());
    if (numRounds <= 0) { return CompositeSparse(); }

    // 4) 工作图 = 本 rank 的整张局部图（预乘 alpha RGBA8）。resize 同时处理
    //    「过长截断」与「过短补零」，保证恰好 width*height*4 字节。
    std::vector<unsigned char> work = m_LocalRGBA;
    work.resize(static_cast<size_t>(N) * 4, 0);

    // interlace（Swizzle 的负载均衡部分）：仅当不止一轮时（与 IceT 一致）。
    const bool useInterlace = (numRounds > 1);
    if (useInterlace) {
        std::vector<unsigned char> interlaced(static_cast<size_t>(N) * 4);
        RadixKInterlace(work, size, interlaced);
        work.swap(interlaced);
    }

    // 5) 逐轮分裂 + 转置交换 + 有序合成。
    int step = 1;
    long long pieceLen = N;
    int remaining = size;

    for (int r = 0; r < numRounds; ++r) {
        const int k = kFactors[r];
        const int myDigit = (groupRank / step) % k;

        // 本轮分裂边界（相对当前块），组内所有成员一致。
        const std::vector<long long> offs = RadixKSplitOffsets(k, remaining, pieceLen);
        const long long myStart = offs[myDigit];
        const long long myLen = offs[myDigit + 1] - offs[myDigit];

        // 本组第一个伙伴的 group_rank（IceT radixkGetPartners）。
        const int firstPartner =
                groupRank % step + (groupRank / (step * k)) * (step * k);
        const int tag = r;

        // 我收的永远是「伙伴 j 的子块 myDigit」，长度恒为 myLen。
        const long long maxMsg = 4 + myLen * 4; // 头 + 最坏情况（密集 myLen 像素）
        std::vector<std::vector<unsigned char>> recvRaw(static_cast<size_t>(k));
        std::vector<int> recvReq(static_cast<size_t>(k), -1);
        for (int j = 0; j < k; ++j) {
            if (j == myDigit) { continue; }
            recvRaw[static_cast<size_t>(j)].resize(static_cast<size_t>(maxMsg));
            const int partner = m_SortOrder[static_cast<size_t>(firstPartner + j * step)];
            recvReq[static_cast<size_t>(j)] =
                    ctx->Irecv(reinterpret_cast<char*>(recvRaw[static_cast<size_t>(j)].data()),
                               static_cast<int>(maxMsg), partner, tag);
        }

        // 发我的子块 j 给伙伴 j（j != myDigit）；子块 myDigit 留在本地。
        std::vector<std::vector<unsigned char>> sendBuf(static_cast<size_t>(k));
        std::vector<int> sendReq(static_cast<size_t>(k), -1);
        const unsigned char* wp = work.data();
        std::vector<unsigned char> myPiece(
                wp + static_cast<size_t>(myStart) * 4,
                wp + static_cast<size_t>(myStart + myLen) * 4);
        for (int j = 0; j < k; ++j) {
            if (j == myDigit) { continue; }
            const long long s0 = offs[j];
            const long long sl = offs[j + 1] - offs[j];
            sendBuf[static_cast<size_t>(j)] = RadixKPack(work, s0, sl);
            const int partner = m_SortOrder[static_cast<size_t>(firstPartner + j * step)];
            sendReq[static_cast<size_t>(j)] =
                    ctx->Isend(reinterpret_cast<const char*>(sendBuf[static_cast<size_t>(j)].data()),
                               static_cast<int>(sendBuf[static_cast<size_t>(j)].size()),
                               partner, tag);
        }

        // 收集 k 张等长子块（数字序 = 深度序），front-to-back over 合成。
        std::vector<std::vector<unsigned char>> pieces(static_cast<size_t>(k));
        for (int j = 0; j < k; ++j) {
            if (j == myDigit) {
                pieces[static_cast<size_t>(j)] = std::move(myPiece);
            } else {
                ctx->Wait(recvReq[static_cast<size_t>(j)]);
                pieces[static_cast<size_t>(j)] =
                        RadixKUnpack(recvRaw[static_cast<size_t>(j)], myLen);
            }
        }
        for (int j = 0; j < k; ++j) {
            if (j != myDigit) { ctx->Wait(sendReq[static_cast<size_t>(j)]); }
        }

        work = RadixKCompositePieces(pieces, myLen);
        pieceLen = myLen;
        remaining /= k;
        step *= k;
    }

    // 6) 最终分块索引（数字反转的 group_rank）→ 原图偏移 + 长度。
    const int finalPartition = RadixKFinalPartitionIndex(groupRank, kFactors);
    const long long finalLen = RadixKFlatLen(finalPartition, N, size);
    const long long finalOffset = useInterlace
            ? RadixKInterlaceOffset(finalPartition, size, N)
            : RadixKFlatOffset(finalPartition, N, size);

#if !defined(NDEBUG)
    if (pieceLen != finalLen || static_cast<long long>(work.size()) != finalLen * 4) {
        std::cerr << "[iGameCompositePass] radix-k piece size mismatch: "
                  << "tracked=" << pieceLen << " expected=" << finalLen << '\n';
    }
#endif

    // 7) 由共享的 m_SortOrder 确定性地推出每个 rank 的分块偏移/长度，Gatherv 到 rank 0。
    std::vector<int> recvCounts(static_cast<size_t>(size), 0);
    std::vector<int> displs(static_cast<size_t>(size), 0);
    for (int r2 = 0; r2 < size; ++r2) {
        int gr2 = -1;
        for (int g = 0; g < size; ++g) {
            if (m_SortOrder[static_cast<size_t>(g)] == r2) { gr2 = g; break; }
        }
        const int fp2 = RadixKFinalPartitionIndex(gr2, kFactors);
        const long long len2 = RadixKFlatLen(fp2, N, size);
        const long long off2 = useInterlace
                ? RadixKInterlaceOffset(fp2, size, N)
                : RadixKFlatOffset(fp2, N, size);
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
