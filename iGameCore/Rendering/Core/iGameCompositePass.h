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
 *   2. 所有 rank 的图像与深度 Gather 到 rank 0（MPI_Gather）；
 *   3. rank 0 对每个像素按深度从近到远排序，做 front-to-back "over" 合成（预乘 alpha），
 *      最后叠到背景色上得到不透明结果。
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

    int GetResultWidth() const { return m_ResultWidth; }
    int GetResultHeight() const { return m_ResultHeight; }
    /** 合成结果（不透明 RGBA8，含背景色），仅 rank 0 有效。 */
    const std::vector<unsigned char>& GetResultRGBA() const {
        return m_ResultRGBA;
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
    void CompositeOnRoot(int size, const std::vector<unsigned char>& allRGBA,
                         const std::vector<float>& allDepth);

    int m_Width{0};
    int m_Height{0};
    std::vector<unsigned char> m_LocalRGBA;
    std::vector<float> m_LocalDepth;
    double m_BlockDepth{0.0};
    float m_Background[3]{0.0f, 0.0f, 0.0f};

    std::vector<int> m_SortOrder;      // 全局排序（近 -> 远），所有 rank 一致
    std::vector<int> m_RankBlockOrder; // rank -> 在 m_SortOrder 中的位置
    std::vector<int> m_PixelOrder;     // 每像素复用的排序缓冲（size 个 rank 下标）
    int m_ResultWidth{0};
    int m_ResultHeight{0};
    std::vector<unsigned char> m_ResultRGBA; // 仅 rank 0
};

IGAME_NAMESPACE_END

#endif // IGAMEVIS_IGAMECOMPOSITEPASS_H
