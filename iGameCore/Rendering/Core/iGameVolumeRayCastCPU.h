#ifndef IGAMEVIS_IGAMEVOLUMERAYCASTCPU_H
#define IGAMEVIS_IGAMEVOLUMERAYCASTCPU_H

#include "iGameObject.h"
#include "iGameStructuredMesh.h"
#include "iGameVolumeTransferFunction.h"
#include "igm/igm.h"

#include <limits>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class iGameVolumeRayCastCPU
 * @brief 纯 CPU 光线步进体渲染器（阶段 4 生产后端）。
 *
 * @details
 *  对标 TestPVolumeRender 的 MyCustomVolumePass 数学语义，但补齐其在
 *  report_rendering_frame.md 里指出的全部性能短板（当时是单线程 + 逐样本实时求
 *  传输函数 + 每步除法）：
 *    - 像素/光线级并行：外层用 ThreadPool::parallelFor 按像素行摊薄；
 *    - 1D LUT 查表：复用 iGameVolumeTransferFunction::BakeLUT()，采样点直接查表，
 *      不做实时 GetColor/GetOpacity；
 *    - 增量步进：维持 grid 空间坐标增量前进（gx += dirGrid.x * step），避免每步除法；
 *    - 空体素跳过：对体素场建砖块 min/max 结构，透明/无效砖块整体跳过；
 *    - front-to-back 合成 + 提前终止（aAccum > 0.995）；
 *    - LOD 三开关：maxSamples / pixelStride / screenROI（对标 MyCustomVolumePass）。
 *
 *  本类不依赖 OpenGL/GLFW：输入与 GPU 后端完全相同的重采样产物 StructuredMesh（或
 *  单块结构化网格），输出预乘 alpha 的 RGBA8 图 + reversed-z float 深度图到 CPU 缓冲，
 *  可直接交给 iGameCompositePass 做跨进程合成（CPU 后端天然无头，超算可用）。
 *
 *  与 GPU 后端（iGameVolumeRayCastGPU）的一致性：
 *    - 共用同一套传输函数 LUT 生成与标量范围归一化；
 *    - 光线生成/包围盒求交/深度回写与 VolumeRayCast.frag 逐像素同语义（reversed-z：
 *      near=1.0、far=0.0），便于同数据、同 LUT、同相机下做 GPU/CPU 逐像素对照。
 */
class iGameVolumeRayCastCPU : public Object {
public:
    I_OBJECT(iGameVolumeRayCastCPU);
    static Pointer New() { return new iGameVolumeRayCastCPU; }

    /**
     * 设置输入规则体素场（StructuredMesh，点标量，行主序 i 最快）。
     * 字段选择口径与 GPU 后端一致：激活属性 -> 点标量/向量 -> 单元标量/向量
     * （单元数据自动做 CellDataToPointData）。失败（非 3D 或无可用标量）返回 false。
     */
    bool SetInput(StructuredMesh::Pointer mesh);

    /**
     * 可选：无效点 mask（0=无效/透明，1=有效），来自
     * iGameVolumeResampleFilter::GetValidMask()。用于屏蔽落在原网格之外的采样点，
     * 避免出现"蓝色颗粒/点状面"（对标 MyCustomVolumePass 的 vtkValidPointMask）。
     */
    void SetValidMask(UnsignedCharArray::Pointer mask);

    /** 设置传输函数并烘焙 1D LUT（CPU 内存）。标量范围取自 tf->GetScalarMin/Max。 */
    void SetTransferFunction(iGameVolumeTransferFunction::Pointer tf);

    /** 覆盖标量归一化范围（并行体绘制用 AllReduce 后的全局范围）。 */
    void SetScalarRange(double scalarMin, double scalarMax);

    /**
     * 渲染到 CPU 图像缓冲。
     * @param view        相机视图矩阵（lookAtRH）。
     * @param proj        相机投影矩阵（perspectiveRH_OZ / orthoRH_OZ，reversed-z）。
     * @param modelMatrix 模型矩阵（局部空间 -> 世界空间；并行体绘制通常为单位阵）。
     * @param viewport    输出分辨率（像素）。
     * @param outRGBA     输出预乘 alpha RGBA8，长度 viewport.x*viewport.y*4，
     *                    左下角原点、行主序（py=0 为底行）。
     * @param outDepth    输出 reversed-z float 深度（near=1.0、far=0.0），
     *                    长度 viewport.x*viewport.y；空像素深度为 0（远平面）。
     */
    void Render(const igm::mat4& view, const igm::mat4& proj,
                const igm::mat4& modelMatrix, const igm::uvec2& viewport,
                std::vector<unsigned char>& outRGBA,
                std::vector<float>& outDepth);

    // ---------------- 质量 / LOD 参数（对标 MyCustomVolumePass） ----------------

    /** 每条光线的最大采样步数（默认 512）。 */
    void SetMaxSamples(int maxSamples) {
        m_MaxSamples = maxSamples < 1 ? 1 : maxSamples;
    }
    int GetMaxSamples() const { return m_MaxSamples; }

    /** 局部空间步长；<=0 时按 maxSamples 自动推导（默认 0=自动）。 */
    void SetStepSize(float stepSize) { m_StepSize = stepSize; }
    float GetStepSize() const { return m_StepSize; }

    /** 像素步进（>1 时低清渲染，最近邻放大填满，默认 1）。 */
    void SetPixelStride(int stride) { m_PixelStride = stride < 1 ? 1 : stride; }
    int GetPixelStride() const { return m_PixelStride; }

    /** 是否只渲染体在屏幕上的包围盒区域（screen ROI，默认 false）。 */
    void SetUseScreenROI(bool use) { m_UseScreenROI = use; }
    bool GetUseScreenROI() const { return m_UseScreenROI; }

    /**
     * Beer-Lambert 单位距离修正（对标 VTK ScalarOpacityUnitDistance）。
     *  <=0（默认）：禁用 Beer-Lambert，直接用 LUT 不透明度合成（alphaStep = opacity），
     *               与 GPU 后端（VolumeRayCast.frag）完全一致、尺度无关；
     *  >0：启用 Beer-Lambert，alphaStep = 1 - exp(-opacity * step / d)。
     *      注意 d 需与体数据坐标单位同量级（如包围盒对角线），否则会整体偏透明/偏不透明。
     */
    void SetScalarOpacityUnitDistance(double d) { m_UnitDistance = d; }
    double GetScalarOpacityUnitDistance() const { return m_UnitDistance; }

    /** 是否启用空体素跳过（砖块 min/max 空间跳跃，默认 true）。 */
    void SetEmptySpaceSkippingEnabled(bool enabled) {
        m_EmptySpaceSkip = enabled;
        if (!enabled) { m_BricksDirty = false; }
    }
    bool GetEmptySpaceSkippingEnabled() const { return m_EmptySpaceSkip; }

    /** 上传标量场的真实数据范围（用于标量范围未初始化时的回退）。 */
    float GetDataMin() const { return m_DataMin; }
    float GetDataMax() const { return m_DataMax; }

protected:
    iGameVolumeRayCastCPU();
    ~iGameVolumeRayCastCPU() override;

private:
    // 标量场提取（SetInput 内部）：点标量 FloatArray 缓冲（ni*nj*nk 个 float）。
    bool ExtractScalarField(StructuredMesh::Pointer mesh);

    // (重)建砖块 min/max 结构（空体素跳过用）；仅在需要且启用时调用。
    void EnsureBricks();

    // 在标量归一化域上求值：采样值 -> 归一化 [0,1] -> LUT RGBA（含线性插值）。
    // outColor 为颜色（未预乘），outAlpha 为不透明度 [0,1]。
    void SampleLUT(float scalar, float outColor[3], float& outAlpha) const;

    // 光线与 AABB 求交（局部空间，对标 VolumeRayCast.frag 的 intersectBox）。
    static bool IntersectBox(const igm::vec3& origin, const igm::vec3& dir,
                             const igm::vec3& boxMin, const igm::vec3& boxMax,
                             float& tEnter, float& tExit);

    // 逐像素光线步进：把结果（预乘 RGBA + reversed-z 深度）写入 rgba/depth。
    void RayCastPixel(int px, int py, int w, int h,
                      const igm::mat4& invViewProj, const igm::mat4& invModel,
                      const igm::mat4& projViewModel, const float* scalarData,
                      const unsigned char* maskData, size_t maskCount,
                      float* rgba, float& depth);

    // ---------- 输入状态 ----------
    StructuredMesh::Pointer m_Input{nullptr};
    int m_InputAttributeIndex{-1};   // 提取时的激活属性；切换字段需重提
    std::vector<float> m_ScalarField; // 点标量缓冲（float，行主序 i 最快）
    int m_Dims[3]{0, 0, 0};
    igm::vec3 m_BoxMin{0.0f};
    igm::vec3 m_BoxMax{1.0f};
    float m_DataMin{0.0f};
    float m_DataMax{1.0f};

    // ---------- 传输函数 / LUT ----------
    iGameVolumeTransferFunction::Pointer m_TransferFunction{nullptr};
    std::vector<unsigned char> m_LUT; // RGBA8，resolution*4
    int m_LUTResolution{0};
    double m_ScalarMin{0.0};
    double m_ScalarMax{1.0};
    float m_OpacityThreshold{-std::numeric_limits<float>::max()}; // 首个不透明标量值

    // ---------- 有效点 mask ----------
    UnsignedCharArray::Pointer m_ValidMask{nullptr};

    // ---------- 空体素跳过（砖块 min/max） ----------
    int m_BrickSize{8};
    int m_BrickDims[3]{0, 0, 0};
    std::vector<float> m_BrickMax;        // 每砖块（仅有效点）的最大标量
    std::vector<unsigned char> m_BrickActive; // 每砖块是否含至少一个有效点
    bool m_BricksDirty{true};

    // ---------- 质量 / LOD ----------
    int m_MaxSamples{512};
    float m_StepSize{0.0f};
    int m_PixelStride{1};
    bool m_UseScreenROI{false};
    double m_UnitDistance{0.0}; // <=0 禁用 Beer-Lambert（默认，与 GPU 一致）
    bool m_EmptySpaceSkip{true};
};

IGAME_NAMESPACE_END

#endif // IGAMEVIS_IGAMEVOLUMERAYCASTCPU_H
