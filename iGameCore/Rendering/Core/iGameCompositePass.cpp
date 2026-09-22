#include "iGameCompositePass.h"

#include "iGameParallelContext.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

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

bool iGameCompositePass::Composite() {
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

    m_RankBlockOrder.resize(static_cast<size_t>(size));
    for (int i = 0; i < size; ++i) {
        m_RankBlockOrder[static_cast<size_t>(m_SortOrder[i])] = i;
    }

    // 3) Gather 各 rank 的 RGBA 与深度到 rank 0。
    const int pixelCount = width * height;
    const int rgbaCount = pixelCount * 4; // 每 rank 的字节数
    const int depthCount = pixelCount;    // 每 rank 的 float 数

    std::vector<unsigned char> allRGBA;
    std::vector<float> allDepth;
    if (isRoot) {
        allRGBA.resize(static_cast<size_t>(rgbaCount) * size);
        allDepth.resize(static_cast<size_t>(depthCount) * size);
    }

    ctx->Gather(reinterpret_cast<const char*>(m_LocalRGBA.data()),
                reinterpret_cast<char*>(allRGBA.data()), rgbaCount, 0);
    ctx->Gather(m_LocalDepth.data(), allDepth.data(), depthCount, 0);

    // 4) rank 0 逐像素深度排序 + front-to-back 合成。
    if (isRoot) {
        m_ResultWidth = width;
        m_ResultHeight = height;
        CompositeOnRoot(size, allRGBA, allDepth);
    } else {
        m_ResultWidth = width;
        m_ResultHeight = height;
    }

    ctx->Barrier();
    return true;
}

void iGameCompositePass::CompositeOnRoot(
        int size, const std::vector<unsigned char>& allRGBA,
        const std::vector<float>& allDepth) {
    const int n = m_Width * m_Height;
    m_ResultRGBA.assign(static_cast<size_t>(n) * 4, 0);
    m_PixelOrder.resize(static_cast<size_t>(size));

    auto toUChar = [](float v) {
        const float c = std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f;
        return static_cast<unsigned char>(c);
    };

    for (int p = 0; p < n; ++p) {
        // 初始化并选择排序：深度大（近）优先，深度相同时按超块全局顺序 + rank 兜底。
        for (int r = 0; r < size; ++r) { m_PixelOrder[r] = r; }
        for (int i = 0; i < size; ++i) {
            int best = i;
            for (int j = i + 1; j < size; ++j) {
                const int rj = m_PixelOrder[j];
                const int rb = m_PixelOrder[best];
                const float dj =
                        allDepth[static_cast<size_t>(rj) * n + p];
                const float db =
                        allDepth[static_cast<size_t>(rb) * n + p];
                if (dj > db ||
                    (dj == db &&
                     m_RankBlockOrder[static_cast<size_t>(rj)] <
                             m_RankBlockOrder[static_cast<size_t>(rb)])) {
                    best = j;
                }
            }
            if (best != i) { std::swap(m_PixelOrder[i], m_PixelOrder[best]); }
        }

        // front-to-back "over"（预乘 alpha）。
        float c0 = 0.0f, c1 = 0.0f, c2 = 0.0f, a = 0.0f;
        for (int i = 0; i < size && a < 0.999f; ++i) {
            const int r = m_PixelOrder[i];
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
