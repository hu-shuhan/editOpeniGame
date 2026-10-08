#pragma once
// ParallelVolumePixelComposite.h —— 并行体绘制「逐像素」图像合成（--pixelwise）。
//
// 这是与 iGameCore 的 iGameCompositePass（--blockwise，块级有序 over）并列的**第二条**
// 合成路径，全部逻辑留在 pvr 模块内、不改动 iGameCore，便于单独回退与对拍。
//
// ---------------------------------------------------------------------------
// 两种口径的区别
// ---------------------------------------------------------------------------
//   --blockwise（默认，iGameCompositePass）：
//       每个 rank 只上报一个深度 —— 超块中心沿相机方向的距离
//       （iGameCompositePass::ComputeBlockDepth(blockCenter, camPos, front)，值**小**=近）。
//       rank0 按这个全局块序把各 rank 的图整幅 over。屏幕投影上互相重叠的超块，其前后
//       关系只有这一个顺序，逐像素看是**近似**的。
//
//   --pixelwise（本文件）：
//       每个像素用它**自己的首命中深度**（光线步进写回的 reversed-z，值**大**=近）决定
//       各 rank 贡献的先后 ⇒ 重叠区的前后关系逐像素正确，更接近单进程参考图像。
//
// 代价（本文件存在的意义就是让你量它）：
//   1) 载荷变多：ROI 子矩形除了 RGBA8 还要带一份 float 深度（+4 B/像素）；
//   2) rank0 计算：不再是"按块序线性 over"，而是"逐像素收集候选 → 按深度排序 → over"；
//      为了不让它退化成 O(像素 × P)，用 32×32 屏幕 tile 建立"哪些 rank 与该 tile 相交"
//      的索引，每像素只在该 tile 的候选表里查；
//   3) 集合通信次数其实**更少**：稀疏路径只要 1 次 AllGatherInt + 1 次 Gatherv + 1 Barrier
//      （块级稀疏路径是 3 次 AllGather + Gatherv + Barrier，因为还要交换尺寸与块深度）。
//
// 两条策略都支持（与 --direct / --binary-swap 正交组合）：
//   SetUseBinarySwapComposite(false) → 逐像素 + 稀疏 ROI 汇聚（CompositeSparse）
//   SetUseBinarySwapComposite(true)  → 逐像素 + binary-swap 交换（CompositeBinarySwap，
//                                      ⌈log₂P⌉ 轮两两交换半张图，每轮在像素级按深度定前后；
//                                      只支持 2 的幂 P，非 2 幂自动退回上面那条）
//
// ---------------------------------------------------------------------------
// 深度口径（关键，两套口径方向相反，别搞混）
// ---------------------------------------------------------------------------
//   · 像素深度（本文件用）：reversed-z，**大 = 近**，0 = 远/空。
//     来源：iGameVolumeRayCastCPU::RayCastPixel 的"首命中深度回写"，以及 --gpu 路径
//     Scene::CaptureParallelVolumeFrame 读回的 GL 深度缓冲（同一 reversed-z 约定）。
//   · 块深度（块级路径用）：dot(center - camPos, front)，**小 = 近**。本文件只在
//     binary-swap 里用它算 groupRank（配对与最终分块的映射），不参与前后判断。
//
// ---------------------------------------------------------------------------
// 正确性边界
// ---------------------------------------------------------------------------
//   · 逐像素排序用的是"每个 rank 在该像素上的**首命中**深度 + 该 rank 沿光线已合成好的
//     RGBA"，这正是 sort-last 图像合成的标准口径；它比块级序更细，但**不等于**逐采样
//     排序（后者需要传输体素/采样，不是图像合成）。
//   · binary-swap 的逐像素合并是一棵 log P 的合并树：只有当两半的深度区间不交错时，
//     结果与"全局逐像素排序"完全一致；交错的采样无法在 2 张已合成图像之间还原（这是
//     图像合成固有的近似，块级路径同样存在，只是更粗）。要完全精确请用逐像素 + --direct。
//   · 每轮合并都会把 RGBA 量化回 uint8（与 iGameCore 的 binary-swap 一致）。
// =============================================================================

#include "iGameParallelContext.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

namespace iGamePVPixel {

// ===========================================================================
// detail：纯函数（只依赖 std，不碰 MPI / iGame），便于离线对拍与单测
// ===========================================================================
namespace detail {

// 256 级查表：把 [0,1] 的 float 量化成 uint8（与 iGameCore 的 +0.5 取整口径一致）。
inline unsigned char ToUChar(float v) {
    return static_cast<unsigned char>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
}

// 预乘 alpha 的 over：acc 是 float 累加缓冲（预乘颜色 + alpha，长度 4），
// px 是预乘 RGBA8。与 iGameCompositePass::CompositeSparse 的逐像素公式一致：
//   inv = 1 - acc.a;  acc.rgb += inv * px.rgb;  acc.a += inv * px.a;
inline void OverPremul(float* acc, const unsigned char* px) {
    const float fa = static_cast<float>(px[3]) / 255.0f;
    if (fa <= 0.0f) { return; }
    const float inv = 1.0f - acc[3];
    if (inv <= 0.001f) { return; } // 已饱和
    acc[0] += inv * (static_cast<float>(px[0]) / 255.0f);
    acc[1] += inv * (static_cast<float>(px[1]) / 255.0f);
    acc[2] += inv * (static_cast<float>(px[2]) / 255.0f);
    acc[3] += inv * fa;
}

// 一个 rank 的"本地片"：ROI 矩形（全帧坐标）+ ROI 内**密集** RGBA8 + ROI 内逐像素深度。
// rgba 长度 = w*h*4，depth 长度 = w*h（reversed-z，大=近）。空片的 w/h 为 0。
struct Piece {
    int x0{0};
    int y0{0};
    int w{0};
    int h{0};
    std::vector<unsigned char> rgba;
    std::vector<float> depth;

    bool Empty() const { return w <= 0 || h <= 0; }
};

// 从 work（长度 N 的密集 RGBA + 深度）里切出 [start, start+len)。
inline void SlicePiece(const std::vector<unsigned char>& rgba,
                       const std::vector<float>& depth, long long start, long long len,
                       std::vector<unsigned char>& outRGBA,
                       std::vector<float>& outDepth) {
    outRGBA.assign(static_cast<std::size_t>(len) * 4, 0);
    outDepth.assign(static_cast<std::size_t>(len), 0.0f);
    if (len <= 0) { return; }
    if (rgba.size() >= static_cast<std::size_t>(start + len) * 4) {
        std::memcpy(outRGBA.data(),
                    rgba.data() + static_cast<std::size_t>(start) * 4,
                    static_cast<std::size_t>(len) * 4);
    }
    if (depth.size() >= static_cast<std::size_t>(start + len)) {
        std::memcpy(outDepth.data(), depth.data() + static_cast<std::size_t>(start),
                    static_cast<std::size_t>(len) * sizeof(float));
    }
}

inline void PutU32(std::vector<unsigned char>& b, std::uint32_t v) {
    b.push_back(static_cast<unsigned char>(v & 0xffu));
    b.push_back(static_cast<unsigned char>((v >> 8) & 0xffu));
    b.push_back(static_cast<unsigned char>((v >> 16) & 0xffu));
    b.push_back(static_cast<unsigned char>((v >> 24) & 0xffu));
}

inline std::uint32_t GetU32(const std::vector<unsigned char>& b, std::size_t off) {
    if (off + 4 > b.size()) { return 0; }
    return static_cast<std::uint32_t>(b[off]) |
           (static_cast<std::uint32_t>(b[off + 1]) << 8) |
           (static_cast<std::uint32_t>(b[off + 2]) << 16) |
           (static_cast<std::uint32_t>(b[off + 3]) << 24);
}

// binary-swap 一轮交换用的打包（对标 iGameCore 的自适应打包，多加一份深度）：
//   u32 头 = 0            → 全空
//   u32 头 = 0xFFFFFFFF   → 密集：len*4 字节 RGBA8 + len*4 字节 float 深度（8 B/像素）
//   u32 头 = cnt          → 稀疏：cnt × (u32 relIdx + RGBA8 + f32 深度)（12 B/非空像素）
// 取两者较小者，避免把"几乎全空"的半张图按密集传。
inline std::vector<unsigned char> PackPiece(const std::vector<unsigned char>& rgba,
                                            const std::vector<float>& depth, long long len) {
    long long cnt = 0;
    for (long long i = 0; i < len; ++i) {
        if (rgba[static_cast<std::size_t>(i) * 4 + 3] != 0) { ++cnt; }
    }
    std::vector<unsigned char> buf;
    if (cnt == 0) {
        PutU32(buf, 0u);
        return buf;
    }
    if (cnt * 12 < len * 8) {
        PutU32(buf, static_cast<std::uint32_t>(cnt));
        for (long long i = 0; i < len; ++i) {
            const std::size_t b = static_cast<std::size_t>(i) * 4;
            if (rgba[b + 3] == 0) { continue; }
            PutU32(buf, static_cast<std::uint32_t>(i));
            buf.push_back(rgba[b + 0]);
            buf.push_back(rgba[b + 1]);
            buf.push_back(rgba[b + 2]);
            buf.push_back(rgba[b + 3]);
            const float d = depth.empty() ? 0.0f : depth[static_cast<std::size_t>(i)];
            const unsigned char* db = reinterpret_cast<const unsigned char*>(&d);
            buf.insert(buf.end(), db, db + sizeof(float));
        }
    } else {
        PutU32(buf, 0xFFFFFFFFu);
        const std::size_t want = static_cast<std::size_t>(len) * 4;
        buf.insert(buf.end(), rgba.begin(),
                   rgba.begin() + static_cast<std::ptrdiff_t>(want));
        for (long long i = 0; i < len; ++i) {
            const float d = depth.empty() ? 0.0f : depth[static_cast<std::size_t>(i)];
            const unsigned char* db = reinterpret_cast<const unsigned char*>(&d);
            buf.insert(buf.end(), db, db + sizeof(float));
        }
    }
    return buf;
}

// 解包成长度 len 的密集 RGBA8 + 深度（未出现的像素保持 alpha=0、深度 0=远）。
inline bool UnpackPiece(const std::vector<unsigned char>& buf, long long len,
                        std::vector<unsigned char>& outRGBA,
                        std::vector<float>& outDepth) {
    outRGBA.assign(static_cast<std::size_t>(len) * 4, 0);
    outDepth.assign(static_cast<std::size_t>(len), 0.0f);
    if (buf.size() < 4) { return false; }
    const std::uint32_t h = GetU32(buf, 0);
    if (h == 0u) { return true; }
    if (h == 0xFFFFFFFFu) {
        const std::size_t want = static_cast<std::size_t>(len) * 4;
        if (buf.size() < 4 + want * 2) { return false; }
        std::memcpy(outRGBA.data(), buf.data() + 4, want);
        std::memcpy(outDepth.data(), buf.data() + 4 + want, want);
        return true;
    }
    std::size_t pos = 4;
    for (std::uint32_t t = 0; t < h; ++t) {
        if (pos + 12 > buf.size()) { break; }
        const std::uint32_t rel = GetU32(buf, pos);
        pos += 4;
        if (static_cast<long long>(rel) < len) {
            const std::size_t b = static_cast<std::size_t>(rel) * 4;
            std::memcpy(&outRGBA[b], buf.data() + pos, 4);
            std::memcpy(&outDepth[rel], buf.data() + pos + 4, sizeof(float));
        }
        pos += 8;
    }
    return true;
}

// binary-swap 一轮的**逐像素**合并：同一像素上谁的深度大（reversed-z：近）谁在前。
//   aFirstOnTie = 深度相等时是否让 a 在前（用于确定性，避免 MPI 双方得出不同结果）。
// 输出 = 合并后的密集 RGBA8 + 深度（深度取两者中更近的那个，供下一轮继续比较）。
inline void MergePiecesByDepth(const std::vector<unsigned char>& aRGBA,
                               const std::vector<float>& aDepth,
                               const std::vector<unsigned char>& bRGBA,
                               const std::vector<float>& bDepth, long long len,
                               bool aFirstOnTie, std::vector<unsigned char>& outRGBA,
                               std::vector<float>& outDepth) {
    outRGBA.assign(static_cast<std::size_t>(len) * 4, 0);
    outDepth.assign(static_cast<std::size_t>(len), 0.0f);
    for (long long i = 0; i < len; ++i) {
        const std::size_t b = static_cast<std::size_t>(i) * 4;
        const unsigned char* pa = &aRGBA[b];
        const unsigned char* pb = &bRGBA[b];
        const bool ea = (pa[3] == 0);
        const bool eb = (pb[3] == 0);
        if (ea && eb) { continue; }
        const float da = aDepth.empty() ? 0.0f : aDepth[static_cast<std::size_t>(i)];
        const float db = bDepth.empty() ? 0.0f : bDepth[static_cast<std::size_t>(i)];
        float acc[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float d = 0.0f;
        if (ea) {
            OverPremul(acc, pb);
            d = db;
        } else if (eb) {
            OverPremul(acc, pa);
            d = da;
        } else {
            // reversed-z：大 = 近 ⇒ 深度大的先 over。
            const bool aNearer = (da > db) || (da == db && aFirstOnTie);
            if (aNearer) {
                OverPremul(acc, pa);
                OverPremul(acc, pb);
                d = da;
            } else {
                OverPremul(acc, pb);
                OverPremul(acc, pa);
                d = db;
            }
        }
        outRGBA[b + 0] = ToUChar(acc[0]);
        outRGBA[b + 1] = ToUChar(acc[1]);
        outRGBA[b + 2] = ToUChar(acc[2]);
        outRGBA[b + 3] = ToUChar(acc[3]);
        outDepth[static_cast<std::size_t>(i)] = d;
    }
}

// 逐像素合成 + 稀疏 ROI 汇聚的 rank0 端核心（纯函数，方便离线对拍）：
//   入参 pieces 是各 rank 的本地片（只有 rank0 调用时才有内容）；
//   输出 outFull（w*h*4，叠背景、不透明）、outROI（4 个 int）与 outROIRGBA。
//   平均/最大候选数通过 maxCand / sumCand 回传，便于观察 rank0 的计算量。
inline void CompositePiecesDepthSorted(const std::vector<Piece>& pieces, int width,
                                       int height, const float bg[3], int tileSize,
                                       std::vector<unsigned char>& outFull, int outROI[4],
                                       std::vector<unsigned char>& outROIRGBA,
                                       int& maxCand, double& sumCand) {
    maxCand = 0;
    sumCand = 0.0;
    outFull.assign(static_cast<std::size_t>(width) * height * 4, 0);
    for (std::size_t p = 3; p < outFull.size(); p += 4) {
        outFull[p] = 255; // 全帧不透明（与 iGameCore 各路径口径一致）
    }
    outROI[0] = outROI[1] = outROI[2] = outROI[3] = 0;
    outROIRGBA.clear();

    // 1) 并集 ROI（与块级稀疏路径一致：各 rank ROI 的并集，而不是重新扫描结果图）
    int ux0 = width, uy0 = height, ux1 = -1, uy1 = -1;
    for (const Piece& pc : pieces) {
        if (pc.Empty()) { continue; }
        ux0 = std::min(ux0, pc.x0);
        uy0 = std::min(uy0, pc.y0);
        ux1 = std::max(ux1, pc.x0 + pc.w - 1);
        uy1 = std::max(uy1, pc.y0 + pc.h - 1);
    }
    if (ux1 < ux0 || uy1 < uy0) { return; } // 全空：全帧=背景，ROI 为空
    const int uw = ux1 - ux0 + 1;
    const int uh = uy1 - uy0 + 1;

    // 2) 屏幕 tile → 与该 tile 相交的 rank 列表（每像素只查一次它所在的 tile）
    const int ts = tileSize > 0 ? tileSize : 32;
    const int tcols = (uw + ts - 1) / ts;
    const int trows = (uh + ts - 1) / ts;
    std::vector<std::vector<int>> tiles(static_cast<std::size_t>(tcols) * trows);
    for (int r = 0; r < static_cast<int>(pieces.size()); ++r) {
        const Piece& pc = pieces[static_cast<std::size_t>(r)];
        if (pc.Empty()) { continue; }
        const int tx0 = (pc.x0 - ux0) / ts;
        const int tx1 = (pc.x0 + pc.w - 1 - ux0) / ts;
        const int ty0 = (pc.y0 - uy0) / ts;
        const int ty1 = (pc.y0 + pc.h - 1 - uy0) / ts;
        for (int ty = ty0; ty <= ty1; ++ty) {
            for (int tx = tx0; tx <= tx1; ++tx) {
                tiles[static_cast<std::size_t>(ty) * tcols + tx].push_back(r);
            }
        }
    }

    // 3) 逐像素：收集候选 → 按深度降序（大=近）→ front-to-back 预乘 over
    struct Cand {
        float d;
        const unsigned char* px;
        int rank;
    };
    std::vector<Cand> cand;
    cand.reserve(16);
    outROIRGBA.assign(static_cast<std::size_t>(uw) * uh * 4, 0);
    for (int y = uy0; y <= uy1; ++y) {
        for (int x = ux0; x <= ux1; ++x) {
            const std::vector<int>& list =
                    tiles[static_cast<std::size_t>((y - uy0) / ts) * tcols +
                          static_cast<std::size_t>((x - ux0) / ts)];
            cand.clear();
            for (int r : list) {
                const Piece& pc = pieces[static_cast<std::size_t>(r)];
                if (x < pc.x0 || x >= pc.x0 + pc.w || y < pc.y0 ||
                    y >= pc.y0 + pc.h) {
                    continue;
                }
                const std::size_t li =
                        static_cast<std::size_t>(y - pc.y0) * pc.w + (x - pc.x0);
                const unsigned char* px = pc.rgba.data() + li * 4;
                if (px[3] == 0) { continue; } // 该像素上这个 rank 没有贡献
                const float d = pc.depth.empty() ? 0.0f : pc.depth[li];
                cand.push_back(Cand{d, px, r});
            }
            if (static_cast<int>(cand.size()) > maxCand) {
                maxCand = static_cast<int>(cand.size());
            }
            sumCand += static_cast<double>(cand.size());

            float acc[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            if (!cand.empty()) {
                std::sort(cand.begin(), cand.end(), [](const Cand& a, const Cand& b) {
                    if (a.d != b.d) { return a.d > b.d; } // reversed-z：大 = 近
                    return a.rank < b.rank;               // 深度相等：按 rank 定序（确定性）
                });
                for (const Cand& c : cand) { OverPremul(acc, c.px); }
            }
            const float inv = 1.0f - acc[3];
            const unsigned char rr = ToUChar(acc[0] + bg[0] * inv);
            const unsigned char gg = ToUChar(acc[1] + bg[1] * inv);
            const unsigned char bb = ToUChar(acc[2] + bg[2] * inv);
            const std::size_t ro =
                    (static_cast<std::size_t>(y - uy0) * uw + (x - ux0)) * 4;
            outROIRGBA[ro + 0] = rr;
            outROIRGBA[ro + 1] = gg;
            outROIRGBA[ro + 2] = bb;
            outROIRGBA[ro + 3] = 255;
            const std::size_t fo = (static_cast<std::size_t>(y) * width + x) * 4;
            outFull[fo + 0] = rr;
            outFull[fo + 1] = gg;
            outFull[fo + 2] = bb;
            outFull[fo + 3] = 255;
        }
    }
    outROI[0] = ux0;
    outROI[1] = uy0;
    outROI[2] = uw;
    outROI[3] = uh;
}

// 预乘图叠背景 → 不透明 RGBA8（与 iGameCore 的 FlattenToBackground 一致）。
inline void FlattenToBackground(const std::vector<unsigned char>& src, const float bg[3],
                                long long n, std::vector<unsigned char>& out) {
    out.assign(static_cast<std::size_t>(n) * 4, 0);
    for (long long p = 0; p < n; ++p) {
        const std::size_t b = static_cast<std::size_t>(p) * 4;
        const float a = static_cast<float>(src[b + 3]) / 255.0f;
        const float inv = 1.0f - a;
        out[b + 0] = ToUChar(static_cast<float>(src[b + 0]) / 255.0f + bg[0] * inv);
        out[b + 1] = ToUChar(static_cast<float>(src[b + 1]) / 255.0f + bg[1] * inv);
        out[b + 2] = ToUChar(static_cast<float>(src[b + 2]) / 255.0f + bg[2] * inv);
        out[b + 3] = 255;
    }
}

// 从"已叠背景的不透明全帧"里扫非背景像素外接矩形（判定与 iGameCore 一致：
// RGB 任一通道非 0 即非背景；背景色由调用方约定为黑色）。
inline void ComputeROIFromFullFrame(const std::vector<unsigned char>& full, int w, int h,
                                    int roi[4], std::vector<unsigned char>& outROIRGBA) {
    roi[0] = roi[1] = roi[2] = roi[3] = 0;
    outROIRGBA.clear();
    if (w <= 0 || h <= 0 || full.size() < static_cast<std::size_t>(w) * h * 4) { return; }
    int x0 = w, y0 = h, x1 = -1, y1 = -1;
    for (int y = 0; y < h; ++y) {
        const unsigned char* row = full.data() + static_cast<std::size_t>(y) * w * 4;
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
    roi[0] = x0;
    roi[1] = y0;
    roi[2] = x1 - x0 + 1;
    roi[3] = y1 - y0 + 1;
    outROIRGBA.resize(static_cast<std::size_t>(roi[2]) * roi[3] * 4);
    for (int y = 0; y < roi[3]; ++y) {
        const unsigned char* src =
                full.data() + (static_cast<std::size_t>(y0 + y) * w + x0) * 4;
        std::memcpy(outROIRGBA.data() + static_cast<std::size_t>(y) * roi[2] * 4, src,
                    static_cast<std::size_t>(roi[2]) * 4);
    }
}

// ---- binary-swap 的平坦均分 / 二分边界 / 按位反序（与 iGameCore 同算法）----
inline long long FlatLen(long long f, long long n, int p) {
    const long long base = n / p;
    const long long rem = n % p;
    return base + (f < rem ? 1 : 0);
}

inline long long FlatOffset(long long f, long long n, int p) {
    const long long base = n / p;
    const long long rem = n % p;
    return f * base + std::min(f, rem);
}

inline std::vector<long long> SplitOffsets(int eventual, long long size) {
    long long remainder = size % eventual;
    const int sub = eventual / 2;
    const long long lowerSize = (size / eventual) * sub;
    std::vector<long long> offs(3);
    long long off = 0;
    for (int j = 0; j < 2; ++j) {
        offs[static_cast<std::size_t>(j)] = off;
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

inline int FinalPartition(int groupRank, int rounds) {
    int fp = 0;
    for (int r = 0; r < rounds; ++r) {
        fp = (fp << 1) | ((groupRank >> r) & 1);
    }
    return fp;
}

} // namespace detail

// ===========================================================================
// PixelCompositePass —— 与 iGameCompositePass 同形的接口子集，便于在调用点二选一
// ===========================================================================
class PixelCompositePass {
public:
    void SetLocalImage(int width, int height, const std::vector<unsigned char>& rgba,
                       const std::vector<float>& depth) {
        m_Width = width < 0 ? 0 : width;
        m_Height = height < 0 ? 0 : height;
        m_LocalRGBA = rgba;
        m_LocalDepth = depth;
    }

    /** 本 rank 超块中心沿相机方向的深度（只用于 binary-swap 的 groupRank 配对/分块映射）。 */
    void SetBlockDepth(double depth) { m_BlockDepth = depth; }

    void SetBackgroundColor(float r, float g, float b) {
        m_Background[0] = r;
        m_Background[1] = g;
        m_Background[2] = b;
    }

    /** true = 走 binary-swap 交换（对标 --binary-swap）；false = 走稀疏 ROI 汇聚（--direct）。 */
    void SetUseBinarySwapComposite(bool use) { m_UseBinarySwap = use; }
    bool GetUseBinarySwapComposite() const { return m_UseBinarySwap; }

    /** 每像素候选索引的 tile 边长（默认 32；越小索引越细、内存越多）。 */
    void SetTileSize(int tileSize) { m_TileSize = tileSize > 0 ? tileSize : 32; }

    /** 执行逐像素合成；所有 rank 必须同时调用（与 iGameCompositePass::Composite 同约定）。 */
    bool Composite() {
        m_LastMaxCand = 0;
        m_LastAvgCand = 0.0;
        return m_UseBinarySwap ? CompositeBinarySwap() : CompositeSparse();
    }

    int GetResultWidth() const { return m_ResultWidth; }
    int GetResultHeight() const { return m_ResultHeight; }

    /** 全帧不透明 RGBA8（含背景），仅 rank 0 有效。 */
    const std::vector<unsigned char>& GetResultRGBA() const { return m_ResultRGBA; }

    int GetResultROIX() const { return m_ResultROIX; }
    int GetResultROIY() const { return m_ResultROIY; }
    int GetResultROIW() const { return m_ResultROIW; }
    int GetResultROIH() const { return m_ResultROIH; }

    /** 合成结果中 ROI 子矩形的不透明 RGBA8（roiW*roiH*4），仅 rank 0 有效。 */
    const std::vector<unsigned char>& GetResultROIRGBA() const { return m_ResultROIRGBA; }

    /** 上一帧的诊断量（仅 rank 0）：逐像素候选的最大值/平均值，配合性能分析用。 */
    int GetLastMaxCandidates() const { return m_LastMaxCand; }
    double GetLastAvgCandidates() const { return m_LastAvgCand; }

private:
    // 逐像素 + 稀疏 ROI 汇聚：AllGatherInt(ROI) → Gatherv(RGBA+depth) → rank0 逐像素排序 over。
    bool CompositeSparse() {
        auto ctx = iGame::ParallelContext::Instance();
        const int size = ctx->Size();
        const int rank = ctx->Rank();
        const bool isRoot = (rank == 0);
        if (m_Width <= 0 || m_Height <= 0) { return false; }

        // 1) 本 rank 的 ROI：alpha != 0 的外接矩形（与块级稀疏路径同口径）
        const int w = m_Width;
        const int h = m_Height;
        int lx0 = w, ly0 = h, lx1 = -1, ly1 = -1;
        if (m_LocalRGBA.size() >= static_cast<std::size_t>(w) * h * 4) {
            for (int y = 0; y < h; ++y) {
                const unsigned char* row =
                        m_LocalRGBA.data() + static_cast<std::size_t>(y) * w * 4;
                for (int x = 0; x < w; ++x) {
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

        // 2) 交换 ROI（一次 AllGatherInt；逐像素路径不需要块深度，所以比块级路径少 2 次集合通信）
        std::vector<int> allROI(static_cast<std::size_t>(size) * 4, 0);
        ctx->AllGatherInt(localROI, allROI.data(), 4);

        // 3) 载荷 = ROI 子矩形 RGBA8（roiW*roiH*4）+ 逐像素深度（roiW*roiH*4）
        std::vector<int> counts(static_cast<std::size_t>(size), 0);
        std::vector<int> displs(static_cast<std::size_t>(size), 0);
        long long total = 0;
        for (int r = 0; r < size; ++r) {
            const int rw = allROI[static_cast<std::size_t>(r) * 4 + 2];
            const int rh = allROI[static_cast<std::size_t>(r) * 4 + 3];
            counts[static_cast<std::size_t>(r)] =
                    (rw > 0 && rh > 0) ? (rw * rh * 8) : 0;
            total += counts[static_cast<std::size_t>(r)];
        }
        if (total > static_cast<long long>(std::numeric_limits<int>::max())) {
            // 只在"每个 rank 都画满整屏"时才会出现（对比：块级稀疏路径同样会退回全图 Gather，
            // 那条路在 iGameCore 里；逐像素路径没有全图兜底，直接让调用方看到失败）。
            if (isRoot) {
                std::cerr << "[pixelwise] payload (" << total
                          << " B) exceeds the int range; use --blockwise instead.\n";
            }
            return false;
        }
        for (int r = 1; r < size; ++r) {
            displs[static_cast<std::size_t>(r)] =
                    displs[static_cast<std::size_t>(r - 1)] +
                    counts[static_cast<std::size_t>(r - 1)];
        }

        // 4) 打包本 rank 的 ROI 子矩形（RGBA 块 + 深度块，逐行连续拷贝）
        std::vector<unsigned char> send;
        if (!localEmpty) {
            const int rw = localROI[2];
            const int rh = localROI[3];
            const std::size_t plane = static_cast<std::size_t>(rw) * rh * 4;
            send.assign(plane * 2, 0);
            for (int y = 0; y < rh; ++y) {
                const std::size_t srcRow =
                        (static_cast<std::size_t>(localROI[1] + y) * w + localROI[0]) * 4;
                std::memcpy(send.data() + static_cast<std::size_t>(y) * rw * 4,
                            m_LocalRGBA.data() + srcRow,
                            static_cast<std::size_t>(rw) * 4);
                // 深度可能缺失/偏短（例如某些后端没回深度）：缺的部分保持 0（远）
                if (m_LocalDepth.size() >= (static_cast<std::size_t>(localROI[1] + y) * w +
                                            localROI[0] + rw)) {
                    std::memcpy(send.data() + plane + static_cast<std::size_t>(y) * rw * 4,
                                m_LocalDepth.data() +
                                        static_cast<std::size_t>(localROI[1] + y) * w +
                                        localROI[0],
                                static_cast<std::size_t>(rw) * sizeof(float));
                }
            }
        }

        // 5) Gatherv 到 rank0（recvCounts/displs 由同一份 allROI 算出，所有 rank 一致）
        std::vector<unsigned char> allPayload;
        if (isRoot) { allPayload.resize(static_cast<std::size_t>(total)); }
        ctx->Gatherv(reinterpret_cast<const char*>(send.empty() ? nullptr : send.data()),
                     counts[static_cast<std::size_t>(rank)],
                     reinterpret_cast<char*>(allPayload.empty() ? nullptr
                                                                : allPayload.data()),
                     counts.data(), displs.data(), 0);

        m_ResultWidth = w;
        m_ResultHeight = h;
        if (!isRoot) {
            ctx->Barrier();
            return true;
        }

        // 6) rank0：把每 rank 的载荷还原成 Piece，做逐像素深度排序 over
        std::vector<detail::Piece> pieces(static_cast<std::size_t>(size));
        for (int r = 0; r < size; ++r) {
            const int rw = allROI[static_cast<std::size_t>(r) * 4 + 2];
            const int rh = allROI[static_cast<std::size_t>(r) * 4 + 3];
            if (rw <= 0 || rh <= 0) { continue; }
            detail::Piece& pc = pieces[static_cast<std::size_t>(r)];
            pc.x0 = allROI[static_cast<std::size_t>(r) * 4 + 0];
            pc.y0 = allROI[static_cast<std::size_t>(r) * 4 + 1];
            pc.w = rw;
            pc.h = rh;
            const std::size_t plane = static_cast<std::size_t>(rw) * rh * 4;
            const unsigned char* src = allPayload.data() + displs[static_cast<std::size_t>(r)];
            pc.rgba.assign(src, src + plane);
            pc.depth.resize(static_cast<std::size_t>(rw) * rh);
            std::memcpy(pc.depth.data(), src + plane, plane);
        }
        int roi[4] = {0, 0, 0, 0};
        detail::CompositePiecesDepthSorted(pieces, w, h, m_Background, m_TileSize,
                                           m_ResultRGBA, roi, m_ResultROIRGBA,
                                           m_LastMaxCand, m_LastAvgCand);
        m_ResultROIX = roi[0];
        m_ResultROIY = roi[1];
        m_ResultROIW = roi[2];
        m_ResultROIH = roi[3];
        const double px = static_cast<double>(std::max(0, roi[2])) * std::max(0, roi[3]);
        std::cerr << "[pixelwise] sparse payload=" << total << "B unionROI=" << roi[2] << 'x'
                  << roi[3] << " maxCandidates=" << m_LastMaxCand << " avg="
                  << (px > 0.0 ? m_LastAvgCand / px : 0.0) << '\n';
        ctx->Barrier();
        return true;
    }

    // 逐像素 + binary-swap：⌈log₂P⌉ 轮两两交换半张图（RGBA+depth），每轮按深度定前后。
    bool CompositeBinarySwap() {
        auto ctx = iGame::ParallelContext::Instance();
        const int size = ctx->Size();
        const int rank = ctx->Rank();
        const bool isRoot = (rank == 0);
        if (m_Width <= 0 || m_Height <= 0) { return false; }

        // 非 2 的幂：把不存在的 rank 当全透明会丢数据（需要 telescoping，未实现），
        // 直接退回逐像素稀疏 ROI 合成（结果始终正确）。
        if ((size & (size - 1)) != 0) {
            if (isRoot) {
                std::cerr << "[pixelwise] binary-swap requires a power-of-2 rank count (got "
                          << size << "); falling back to per-pixel sparse-ROI compositing.\n";
            }
            return CompositeSparse();
        }

        // 1) 尺寸一致性校验（AllGather，所有 rank 结论一致）
        const double localDims[2] = {static_cast<double>(m_Width),
                                     static_cast<double>(m_Height)};
        std::vector<double> allDims(static_cast<std::size_t>(size) * 2, 0.0);
        ctx->AllGather(localDims, allDims.data(), 2);
        for (int r = 0; r < size; ++r) {
            if (static_cast<int>(allDims[static_cast<std::size_t>(r) * 2]) != m_Width ||
                static_cast<int>(allDims[static_cast<std::size_t>(r) * 2 + 1]) != m_Height) {
                if (isRoot) {
                    std::cerr << "[pixelwise] image size mismatch across ranks; "
                                 "cannot composite.\n";
                }
                return false;
            }
        }

        // 2) 全局块序（近→远；块深度小 = 近）→ groupRank（只用于配对与最终分块映射）
        std::vector<double> allBlockDepth(static_cast<std::size_t>(size), 0.0);
        ctx->AllGather(&m_BlockDepth, allBlockDepth.data(), 1);
        m_SortOrder.resize(static_cast<std::size_t>(size));
        for (int r = 0; r < size; ++r) { m_SortOrder[static_cast<std::size_t>(r)] = r; }
        std::sort(m_SortOrder.begin(), m_SortOrder.end(), [&](int a, int b) {
            if (allBlockDepth[static_cast<std::size_t>(a)] !=
                allBlockDepth[static_cast<std::size_t>(b)]) {
                return allBlockDepth[static_cast<std::size_t>(a)] <
                       allBlockDepth[static_cast<std::size_t>(b)];
            }
            return a < b;
        });
        int groupRank = -1;
        for (int g = 0; g < size; ++g) {
            if (m_SortOrder[static_cast<std::size_t>(g)] == rank) {
                groupRank = g;
                break;
            }
        }
        if (groupRank < 0) { return false; }

        const long long N = static_cast<long long>(m_Width) * m_Height;
        if (size < 2 || N < static_cast<long long>(size)) { return CompositeSparse(); }

        const int rounds =
                static_cast<int>(std::log2(static_cast<double>(size)));
        std::vector<unsigned char> workRGBA = m_LocalRGBA;
        workRGBA.resize(static_cast<std::size_t>(N) * 4, 0);
        std::vector<float> workDepth = m_LocalDepth;
        workDepth.resize(static_cast<std::size_t>(N), 0.0f);

        long long pieceLen = N;
        int step = 1;
        int remaining = size;
        for (int r = 0; r < rounds; ++r) {
            const int myDigit = (groupRank / step) % 2;
            const int partner = groupRank ^ step; // 翻转 groupRank 的第 r 位
            const std::vector<long long> offs = detail::SplitOffsets(remaining, pieceLen);
            const long long myStart = offs[static_cast<std::size_t>(myDigit)];
            const long long myLen =
                    offs[static_cast<std::size_t>(myDigit) + 1] - myStart;
            const long long otherStart = offs[static_cast<std::size_t>(1 - myDigit)];
            const long long otherLen =
                    offs[static_cast<std::size_t>(2 - myDigit)] - otherStart;

            // 我留下的一半 / 我发给伙伴的一半（都带深度）
            std::vector<unsigned char> myRGBA, otherRGBA;
            std::vector<float> myDepth, otherDepth;
            detail::SlicePiece(workRGBA, workDepth, myStart, myLen, myRGBA, myDepth);
            detail::SlicePiece(workRGBA, workDepth, otherStart, otherLen, otherRGBA,
                               otherDepth);

            const std::vector<unsigned char> sendBuf =
                    detail::PackPiece(otherRGBA, otherDepth, otherLen);
            const long long maxMsg = 4 + myLen * 8; // 头 + 最坏情况（密集 8 B/像素）
            std::vector<unsigned char> recvBuf(static_cast<std::size_t>(maxMsg));
            const int worldPartner = m_SortOrder[static_cast<std::size_t>(partner)];
            const int recvReq = ctx->Irecv(reinterpret_cast<char*>(recvBuf.data()),
                                          static_cast<int>(maxMsg), worldPartner, r);
            const int sendReq =
                    ctx->Isend(reinterpret_cast<const char*>(sendBuf.data()),
                               static_cast<int>(sendBuf.size()), worldPartner, r);
            ctx->Wait(recvReq);
            std::vector<unsigned char> recvRGBA;
            std::vector<float> recvDepth;
            if (!detail::UnpackPiece(recvBuf, myLen, recvRGBA, recvDepth)) {
                ctx->Wait(sendReq);
                return false;
            }
            // 逐像素按深度合并（深度大 = 近的在前；深度相等时本方在前，保证双方一致）
            detail::MergePiecesByDepth(myRGBA, myDepth, recvRGBA, recvDepth, myLen, true,
                                       workRGBA, workDepth);
            ctx->Wait(sendReq);

            pieceLen = myLen;
            remaining /= 2;
            step *= 2;
        }

        // 3) 最终分块索引 = 按位反序的 groupRank；Gatherv 到 rank0 拼合（只需 RGBA）
        const int finalPartition = detail::FinalPartition(groupRank, rounds);
        const long long finalLen = detail::FlatLen(finalPartition, N, size);
        std::vector<int> recvCounts(static_cast<std::size_t>(size), 0);
        std::vector<int> displs(static_cast<std::size_t>(size), 0);
        for (int r2 = 0; r2 < size; ++r2) {
            int gr2 = -1;
            for (int g = 0; g < size; ++g) {
                if (m_SortOrder[static_cast<std::size_t>(g)] == r2) {
                    gr2 = g;
                    break;
                }
            }
            const int fp2 = detail::FinalPartition(gr2, rounds);
            recvCounts[static_cast<std::size_t>(r2)] =
                    static_cast<int>(detail::FlatLen(fp2, N, size) * 4);
            displs[static_cast<std::size_t>(r2)] =
                    static_cast<int>(detail::FlatOffset(fp2, N, size) * 4);
        }
        if (static_cast<long long>(workRGBA.size()) !=
            static_cast<long long>(recvCounts[static_cast<std::size_t>(rank)])) {
            if (isRoot) {
                std::cerr << "[pixelwise] binary-swap piece length is inconsistent "
                             "(internal error).\n";
            }
            return false;
        }

        m_ResultWidth = m_Width;
        m_ResultHeight = m_Height;
        std::vector<unsigned char> fullFrame;
        if (isRoot) { fullFrame.assign(static_cast<std::size_t>(N) * 4, 0); }
        ctx->Gatherv(reinterpret_cast<const char*>(workRGBA.data()),
                     recvCounts[static_cast<std::size_t>(rank)],
                     reinterpret_cast<char*>(fullFrame.empty() ? nullptr
                                                               : fullFrame.data()),
                     recvCounts.data(), displs.data(), 0);

        if (!isRoot) {
            ctx->Barrier();
            return true;
        }

        // 4) rank0：叠背景 + 扫非背景外接矩形（与块级全图路径口径一致）
        detail::FlattenToBackground(fullFrame, m_Background, N, m_ResultRGBA);
        int roi[4] = {0, 0, 0, 0};
        detail::ComputeROIFromFullFrame(m_ResultRGBA, m_Width, m_Height, roi,
                                        m_ResultROIRGBA);
        m_ResultROIX = roi[0];
        m_ResultROIY = roi[1];
        m_ResultROIW = roi[2];
        m_ResultROIH = roi[3];
        ctx->Barrier();
        return true;
    }

    int m_Width{0};
    int m_Height{0};
    std::vector<unsigned char> m_LocalRGBA;
    std::vector<float> m_LocalDepth;
    double m_BlockDepth{0.0};
    float m_Background[3]{0.0f, 0.0f, 0.0f};
    bool m_UseBinarySwap{false};
    int m_TileSize{32};

    std::vector<int> m_SortOrder; // 近→远（binary-swap 的 groupRank 顺序）
    int m_ResultWidth{0};
    int m_ResultHeight{0};
    std::vector<unsigned char> m_ResultRGBA;    // 仅 rank 0
    int m_ResultROIX{0};
    int m_ResultROIY{0};
    int m_ResultROIW{0};
    int m_ResultROIH{0};
    std::vector<unsigned char> m_ResultROIRGBA; // 仅 rank 0
    int m_LastMaxCand{0};
    double m_LastAvgCand{0.0};
};

} // namespace iGamePVPixel
