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
    //   2) 稀疏 ROI 合成（默认）：只汇聚每个 rank 的非空像素外接矩形，上千 rank 下
    //      汇聚量与 rank 0 工作量都和 P 基本解耦——这是 1000 rank 场景的默认路径；
    //   3) direct-send 全图汇聚（阶段 3 原路径，作为稀疏路径的兜底）。
    if (m_UseTreeComposite && size > 1) {
        return CompositeTree();
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
