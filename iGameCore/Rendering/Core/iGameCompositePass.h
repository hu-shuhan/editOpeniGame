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
 *   2. 默认只把各 rank 的「非空像素外接矩形（ROI）」汇聚到 rank 0（稀疏 ROI 路径）；
 *      也可选 2-3 swap：在并集 ROI 上按 2/3 路合成树做 O(log P) 轮交换，合成工作
 *      摊到所有 rank（任意 P 都支持）；
 *   3. 按「超块深度全局顺序（近→远）」做 front-to-back "over" 合成（预乘 alpha），
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
     * ROI 输出是否保留「预乘 alpha + 透明背景」（默认 false：叠背景色输出不透明 ROI）。
     *
     * @details
     *  置 true 时 GetResultROIRGBA() 里的 alpha 是体数据累计不透明度（背景处 alpha=0），
     *  RGB 为预乘颜色，供 C/S 前端把 ROI 叠到自己的背景（例如垂直渐变背景）上：
     *      dst = src + dst * (1 - src.a)
     *  全帧结果 GetResultRGBA() 仍然是不透明背景（批渲染/交互窗口口径不变）。
     */
    void SetTransparentROI(bool use) { m_TransparentROI = use; }
    bool GetTransparentROI() const { return m_TransparentROI; }

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
     * 是否使用 2-3 swap 合成（默认 false，即用稀疏 ROI 合成）。
     *
     * @details
     *  对标 H. Yu, C. Wang, K.-L. Ma, *Massively Parallel Volume Rendering Using
     *  2-3 Swap Image Compositing*, SC 2008：先由各 rank 的局部非空外接矩形
     *  AllGatherInt 出「并集 ROI」，再在 ROI 子图上按一棵 2-3 合成树做
     *  ⌊log₂P⌋ 轮合并（每个内部节点有 2 或 3 个孩子），每轮把整幅 ROI 重新平坦
     *  均分给组内处理器、只交换「自己不负责的那些区间」，收到后按深度序
     *  front-to-back over；最后把 P 块合成结果 Gatherv 回 rank 0 拼成 ROI。
     *
     *  与 binary-swap 的本质区别：**允许任意 P**（不要求 2 的幂）。任意整数都能
     *  分解成 2 和 3 的和，因此当 P = 2^K - 1 时也不会出现「一半处理器空闲」或
     *  「最后一轮单个处理器收所有数据」的退化；P 为 2 的幂时退化成经典
     *  binary-swap。所有处理器在每一轮都参与，通信/合成工作分摊到所有 rank，
     *  无 rank0 单点瓶颈。
     *
     *  顺序正确性：进程按「块深度近→远」排序得到 groupRank（0=最前）。合成树的
     *  孩子始终是深度序里连续的 rank 区间，合并时按**孩子顺序**做 front-to-back
     *  over，因此混合顺序严格是深度序；节点的处理器顺序（论文 Algorithm 2 的
     *  OrderAssignment）只决定「哪一份像素归哪个处理器」，用于均衡通信/计算量。
     *
     *  注意：它仍是 ⌊log₂P⌋ 轮**全局同步的阻塞 P2P**（每轮每处理器最多与 4 个
     *  处理器通信），任意一对慢/拥塞都会把整帧拖住；1024 rank 交互场景下默认的
     *  稀疏 ROI 路径仍可能更快，也更不容易出现交互假死。
     */
    void SetUse23SwapComposite(bool use) { m_Use23SwapComposite = use; }
    bool GetUse23SwapComposite() const { return m_Use23SwapComposite; }

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
    /**
     * 合成结果中 ROI 子矩形（roiW*roiH*4），仅 rank 0 有效。
     * 默认叠背景色后输出不透明 RGBA8；SetTransparentROI(true) 时是「预乘 alpha + 透明
     * 背景」（alpha=0 为背景），供客户端叠自己的背景。
     */
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
    // 2-3 swap 合成（论文 SC 2008；任意 P，O(log P) 轮、无单点汇聚）。
    bool Composite23Swap();
    // 从已合成的全帧结果里扫出非背景像素外接矩形，填充 m_ResultROI*（仅 rank 0）。
    void ComputeResultROIFromFullFrame();
    // 从「预乘 alpha 全帧」里扫出 alpha!=0 的外接矩形并切出预乘 ROI（仅 rank 0）。
    void ComputeResultROIFromPremultiplied(const std::vector<unsigned char>& src);

    int m_Width{0};
    int m_Height{0};
    std::vector<unsigned char> m_LocalRGBA;
    std::vector<float> m_LocalDepth;
    double m_BlockDepth{0.0};
    float m_Background[3]{0.0f, 0.0f, 0.0f};
    bool m_TransparentROI{false}; // ROI 是否保留预乘 alpha（供客户端叠自己的背景）

    std::vector<int> m_SortOrder;      // 全局排序（近 -> 远），所有 rank 一致
    bool m_UseSparseComposite{true};   // 稀疏 ROI 合成开关（默认开）
    bool m_Use23SwapComposite{false};  // 2-3 swap 合成开关（默认关）
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
