#ifndef iGameVolumeResampleFilter_h
#define iGameVolumeResampleFilter_h

#include "iGameBoundingBox.h"
#include "iGameFilter.h"
#include "iGameFlatArray.h"
#include "iGameStructuredMesh.h"
#include <algorithm>
#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class iGameVolumeResampleFilter
 * @brief 体数据 -> 规则体素场重采样（Probe 等价物，阶段 2）。
 *
 * @details
 *  把任意体数据（VolumeMesh / UnstructuredMesh / StructuredMesh，或其多块组合）
 *  重采样成一个规则体素场 StructuredMesh：
 *    - 点按 (i + j*ni + k*ni*nj) 行主序排列，i 最快；
 *    - 标量场作为点标量（FloatArray，dim=1）附加到 AttributeSet；
 *    - 无效点（落在所有单元之外的采样点）通过 GetValidMask() 返回 UnsignedCharArray。
 *
 *  输出可直接作为 iGameVolumeRayCastGPU::SetInput 的输入（其假定体素行主序 + 点标量）。
 *  插值支持：四面体（重心）、六面体（三线性）；三棱柱/金字塔退化为最近节点；
 *  若输入已是 StructuredMesh，则在其自身网格上做三线性采样（假定均匀规则网格）。
 */
class iGameVolumeResampleFilter : public Filter {
public:
    I_OBJECT(iGameVolumeResampleFilter);
    static Pointer New() { return new iGameVolumeResampleFilter; }

    /** 目标体素网格分辨率（每个维度 >= 2）。 */
    void SetTargetDims(int ni, int nj, int nk) {
        m_Dims[0] = ni < 2 ? 2 : ni;
        m_Dims[1] = nj < 2 ? 2 : nj;
        m_Dims[2] = nk < 2 ? 2 : nk;
    }

    /** 目标字段名；为空则自动选择第一个标量/向量（点数据优先，其次单元数据）。 */
    void SetFieldName(const std::string& name) { m_FieldName = name; }

    /** 输出的规则体素场。 */
    StructuredMesh::Pointer GetStructuredMesh() const { return m_OutputMesh; }
    /** 无效点 mask（0=无效/在网格外，1=有效），长度 = ni*nj*nk。 */
    UnsignedCharArray::Pointer GetValidMask() const { return m_ValidMask; }

    /**
     * 输入分块里最细的一维点间距（Execute 后有效；无结构化输入时为 0）。
     *
     * 用途：判断目标分辨率是否相对源数据「欠采样」。输出间距明显大于源间距时，体数据里的
     * 高频结构会被点采样走样成断断续续的块状条纹——这正是并行体绘制顶面纹理不清的成因之一。
     */
    double GetSourceMinSpacing() const { return m_SourceMinSpacing; }

    /** 输出规则网格的三轴间距里最小的一维（Execute 后有效）。 */
    double GetOutputMinSpacing() const {
        const double m = std::min(m_OutputSpacing[0], m_OutputSpacing[1]);
        return std::min(m, m_OutputSpacing[2]);
    }

    bool Execute() override;

protected:
    iGameVolumeResampleFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~iGameVolumeResampleFilter() override = default;

private:
    int m_Dims[3]{64, 64, 64};
    std::string m_FieldName;
    StructuredMesh::Pointer m_OutputMesh{nullptr};
    UnsignedCharArray::Pointer m_ValidMask{nullptr};
    double m_SourceMinSpacing{0.0};          // 输入分块最细点间距（诊断用）
    double m_OutputSpacing[3]{1.0, 1.0, 1.0}; // 输出规则网格间距（诊断用）
};

IGAME_NAMESPACE_END

#endif // iGameVolumeResampleFilter_h
