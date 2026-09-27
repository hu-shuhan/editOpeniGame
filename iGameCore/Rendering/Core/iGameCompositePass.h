#ifndef IGAMEVIS_IGAMECOMPOSITEPASS_H
#define IGAMEVIS_IGAMECOMPOSITEPASS_H

#include "iGameObject.h"
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class iGameCompositePass
 * @brief 分布式有序合成器（阶段 3，IceT ordered compositing 的简化版）。
 *
 * @details
 *  每个 rank 先离屏渲染自己分到的那部分体数据，得到一张「预乘 alpha 的 RGBA8 图 +
 *  reversed-z float 深度图」（空像素 alpha=0、深度=0，即远平面），交给本类做跨进程
 *  合成。流程对标 TestPVolumeRender 的 vtkIceTCompositePass + ordered compositing：
 *
 *   1. 每 rank 上报自己超块中心沿相机方向的深度（AllGather 得到全局排序顺序，对标
 *      vtkOrderedCompositingHelper::ComputeSortOrder）；
 *   2. 所有 rank 的 RGBA 图像 Gather 到 rank 0（MPI_Gather）；
 *   3. rank 0 按「超块深度全局顺序（近→远）」做 front-to-back "over" 合成（预乘 alpha），
 *      最后叠到背景色上得到不透明结果。blend 合成不需要深度缓冲（对标 IceT 有序 BLEND
 *      的 ICET_IMAGE_DEPTH_NONE）。
 *
 *  关键正确性前提（对标 TestPVolumeRender）：所有 rank 必须使用同一相机 + 同一全局
 *  裁剪范围 + 同一传输函数标量范围，否则各 rank 深度不可直接比较、合成会错。
 *  本类是纯 CPU 图像合成，与渲染后端（GPU/CPU）无关。
 */
class iGameCompositePass : public Object {
public:
    I_OBJECT(iGameCompositePass);
    static Pointer New() { return new iGameCompositePass; }

    /**
     * 设置本 rank 渲染出的局部图像。
     * @param rgba 预乘 alpha 的 RGBA8，长度 width*height*4。
     * @param depth reversed-z 深度（near=1.0，far=0.0），长度 width*height。
     *        目前仅保留（供将来 z-buffer/调试），blend 合成不消费它。
     */
    void SetLocalImage(int width, int height,
                       const std::vector<unsigned char>& rgba,
                       const std::vector<float>& depth);

    /** 设置本 rank 超块中心沿相机方向的深度（用于 AllGather 全局排序）。 */
    void SetBlockDepth(double depth) { m_BlockDepth = depth; }
    double GetBlockDepth() const { return m_BlockDepth; }

    /** 合成结果的背景色（默认黑）。合成后叠到该背景上输出不透明 RGBA。 */
    void SetBackgroundColor(float r, float g, float b);

    /**
     * 执行分布式合成。所有 rank 必须同时调用。
     * @return 所有 rank 一致返回；false 表示各 rank 图像尺寸不一致（无法合成）。
     */
    bool Composite();

    /**
     * 是否使用「稀疏 ROI 合成」（默认 true，推荐）。
     *
     * @details
     *  每个 rank 先算自己图像上非透明像素的外接矩形（ROI），AllGather 各 rank 的 ROI
     *  （4 个 int）后，用 Gatherv 只把 ROI 子矩形发给 rank 0；rank 0 再按超块全局深度序
     *  把各 rank 的 ROI 矩形逐块 front-to-back over 到累加缓冲上。
     *
     *  为什么必须默认开：并行体绘制里每个 rank 只持有体数据的一小块，它在屏幕上通常只
     *  覆盖很小一片（本工程 19200 块 / 1000 rank 时约 20×20 像素），却要发送/接收
     *  1024×1024×4 = 4MB 的整图。1000 rank 的 direct-send 汇聚量因此是 4GB 量级，
     *  rank 0 还要做 O(P·像素) 的逐像素循环——这是上千 rank 下延迟爆炸的主因。
     *  只传 ROI 后汇聚量降到 MB 量级、rank 0 工作量降到 O(Σ ROI 面积)，与 P 基本解耦
     *  （对标 IceT 的 valid_pixels_viewport）。
     *
     *  稀疏路径在「所有 rank 的 ROI 字节数之和」超过 int 上限时自动回退到全图汇聚。
     */
    void SetUseSparseComposite(bool use) { m_UseSparseComposite = use; }
    bool GetUseSparseComposite() const { return m_UseSparseComposite; }

    /**
     * 是否使用 binary-swap 合成（默认 false，即用稀疏 ROI 合成）。
     *
     * @details
     *  对标 IceT 的 `ICET_SINGLE_IMAGE_STRATEGY_BSWAP`（即 radix-k 的 k=2 特例，
     *  `icetBSwapCompose`）：把整张图在 ⌈log₂P₂⌉ 轮内两两交换半张图并「前端 OVER 后端」
     *  合成，每进程每轮只收发「自己负责的那一半」，汇聚量 O(图像大小)、无 rank0 单点
     *  瓶颈；最后把 P 块合成结果 Gatherv 回 rank 0 拼成完整图。与「稀疏 ROI + Gatherv」
     *  的区别是：稀疏路径把 O(P) 条点对点全部打到 rank 0，binary-swap 把合成工作与
     *  通信分摊到所有 rank（P 到上千 rank 时更有意义）。
     *
     *  顺序正确性（Swizzle）：进程按「块深度近→远」排序得到 groupRank（0=最前），
     *  binary-swap 按 groupRank 做逐轮配对（partner = groupRank ^ 2^r），每轮组内
     *  「数字小者更靠前」，因此「按数字序 front-to-back over」即保持透明度有序；
     *  轮次结束后每个进程持有的分块索引是 groupRank 的「按位反序」，分块在最终图上
     *  平坦连续、互不重叠，Gatherv 即可无损重组。非 2 幂 P 用 next_pow2(P) 补齐，
     *  不存在的 rank 视作全透明（不实际通信）。
     */
    void SetUseBinarySwapComposite(bool use) { m_UseBinarySwapComposite = use; }
    bool GetUseBinarySwapComposite() const { return m_UseBinarySwapComposite; }

    int GetResultWidth() const { return m_ResultWidth; }
    int GetResultHeight() const { return m_ResultHeight; }
    /** 合成结果（不透明 RGBA8，含背景色），仅 rank 0 有效。 */
    const std::vector<unsigned char>& GetResultRGBA() const {
        return m_ResultRGBA;
    }

    /**
     * 合成结果里「非背景像素」的外接矩形（仅 rank 0 有效）。
     * 服务端可直接拿它做帧裁剪（sendFrame 的 roiX/roiY/roiW/roiH），无需再扫一遍全图。
     * ROI 为空时 roiW/roiH 为 0。
     */
    int GetResultROIX() const { return m_ResultROIX; }
    int GetResultROIY() const { return m_ResultROIY; }
    int GetResultROIW() const { return m_ResultROIW; }
    int GetResultROIH() const { return m_ResultROIH; }
    /** 合成结果中 ROI 子矩形的不透明 RGBA8（roiW*roiH*4），仅 rank 0 有效。 */
    const std::vector<unsigned char>& GetResultROIRGBA() const {
        return m_ResultROIRGBA;
    }

    /** 全局深度排序顺序（rank 编号，从近到远），所有 rank 一致。 */
    const std::vector<int>& GetSortOrder() const { return m_SortOrder; }

    /**
     * 计算点 center 沿相机前方向 front 的相机深度（= dot(center - cameraPos,
     * front)）。值越小越靠近相机，用于各超块的全局排序。
     */
    static double ComputeBlockDepth(const double center[3],
                                    const double cameraPos[3],
                                    const double cameraFront[3]);

protected:
    iGameCompositePass() = default;
    ~iGameCompositePass() override = default;

private:
    void CompositeOnRoot(int size, const std::vector<unsigned char>& allRGBA);
    // 稀疏 ROI 合成（默认路径）。内部在极端情况下会回退到 CompositeDenseGather()。
    bool CompositeSparse();
    // direct-send 全图汇聚合成（作为稀疏路径的回退）。
    bool CompositeDenseGather();
    // binary-swap 合成（对标 IceT icetBSwapCompose，O(log P) 轮、无单点汇聚）。
    bool CompositeBinarySwap();
    // 从已合成的全帧结果里扫出非背景像素外接矩形，填充 m_ResultROI*（仅 rank 0）。
    void ComputeResultROIFromFullFrame();

    int m_Width{0};
    int m_Height{0};
    std::vector<unsigned char> m_LocalRGBA;
    std::vector<float> m_LocalDepth;
    double m_BlockDepth{0.0};
    float m_Background[3]{0.0f, 0.0f, 0.0f};

    std::vector<int> m_SortOrder;      // 全局排序（近 -> 远），所有 rank 一致
    bool m_UseSparseComposite{true};   // 稀疏 ROI 合成开关（默认开）
    bool m_UseBinarySwapComposite{false};  // binary-swap 合成开关（默认关）
    int m_ResultWidth{0};
    int m_ResultHeight{0};
    std::vector<unsigned char> m_ResultRGBA; // 仅 rank 0

    // 结果的有效区（仅 rank 0）。稀疏路径直接产出，全图路径在收尾时扫描一次得到。
    int m_ResultROIX{0};
    int m_ResultROIY{0};
    int m_ResultROIW{0};
    int m_ResultROIH{0};
    std::vector<unsigned char> m_ResultROIRGBA;
};

IGAME_NAMESPACE_END

#endif // IGAMEVIS_IGAMECOMPOSITEPASS_H
