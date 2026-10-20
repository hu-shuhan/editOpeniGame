#ifndef iGameTensorPrincipalInvariantsFilter_h
#define iGameTensorPrincipalInvariantsFilter_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class TensorPrincipalInvariantsFilter
 * @brief 对**对称张量**数组逐元求主值（特征值）与主方向（特征向量）。
 *
 * 语义要点：
 *   - 输入张量按**分量数**判定维数，分量顺序固定：
 *       3 分量 → 2D 对称张量：XX, YY, XY；
 *       6 分量 → 3D 对称张量：XX, YY, ZZ, XY, YZ, XZ；
 *   - 用对称特征分解求主值，**按从大到小排序**（Sigma 1 最大）；
 *   - **主方向的符号按固定惯例规范化**（第一分量绝对值最大者居首、对角元取正、
 *     det>0 凑成右手系；各向同性给单位阵、两个主值相等时用叉乘重建该平面内的方向）：
 *     特征向量存在 v / -v 二义性，定下这套惯例才能保证「同一输入永远给同一结果」；
 *   - **重根（两个 / 三个主值相等）的判定用相对容差**（相对张量自身的量级，即三个
 *     主值的最大绝对值）：主值是相对量，整体乘一个常数（换单位）不应改变主方向，
 *     因此不能用带绝对下限的容差，否则小量级张量会被误判成重根、主方向退化成坐标轴；
 *   - 输出 6 个数组，命名规则为 `<原名> - Sigma N` / `<原名> - Sigma N (Vector)`：
 *       `<名> - Sigma 1/2/3`          —— 第 1/2/3 主值（标量，降序）
 *       `<名> - Sigma 1/2/3 (Vector)` —— 第 1/2/3 主方向（3 分量）
 *     写入顺序固定为：先 3 个向量，再 3 个标量；
 *   - `ScaleVectors`（默认 false）：主方向是否按对应主值缩放（false 时是单位向量）；
 *   - 点数据 / 单元数据都支持，由 `SetArrayAttachment()` 指定（默认 IG_POINT）；
 *   - 张量分量含 NaN 时，该元组的输出全部写 NaN（便于在界面上识别）。
 *
 * 【3 分量的判定】
 *   iGame 的数组没有"分量名"概念，因此用**属性类型 IG_TENSOR** 作为判据：
 *   6 分量一律视为张量；3 分量只有被标记为 IG_TENSOR 时才当作 2D 张量。
 *
 * 【独立输出节点】
 *   输入完全不被修改：点/单元/类型/全部属性数组逐个深拷贝到新建的 UnstructuredMesh，
 *   再把 6 个结果数组追加到输出（点或单元数据，与输入张量的挂载位置相同）。
 *   几何一律原样保留 —— 表面网格里的**独立线段**（不属于任何面的边）也会带到输出，
 *   它们是只增不减的几何；输入里与结果数组同名的数据只按「挂载位置一致」替换，
 *   另一侧同名的数组属于用户数据，原样保留。
 *
 * 【失败与空结果（绝不静默跳过）】
 *   无输入 / 输入为空 / 数组不存在 / 分量数不是 3 或 6 /
 *   数组元组数与网格点数（或单元数）不一致：返回 false，并通过 GetMessage() 说明原因。
 *   （表面网格保留独立线段而使单元数增加时，若单元张量只覆盖原有面单元，
 *     线段单元写 NaN 并在 GetMessage() 中说明，不算失败。）
 */
class TensorPrincipalInvariantsFilter : public Filter {

public:
    I_OBJECT(TensorPrincipalInvariantsFilter);
    static Pointer New() { return new TensorPrincipalInvariantsFilter; }
    bool Execute() override;

    /// 待处理的张量数组名（必填；不存在时 Execute 返回 false）
    void SetTensorArrayName(const std::string& name) { m_ArrayName = name; }
    const std::string& GetTensorArrayName() const { return m_ArrayName; }

    /// 张量所在位置：IG_POINT（默认）或 IG_CELL
    void SetArrayAttachment(IGenum attachment) { m_Attachment = attachment; }
    IGenum GetArrayAttachment() const { return m_Attachment; }

    /// 主方向是否按主值缩放（默认 false）
    void SetScaleVectors(bool on) { m_ScaleVectors = on; }
    bool GetScaleVectors() const { return m_ScaleVectors; }

    /// 最近一次执行的信息（失败原因、提示等），供界面显示
    const std::string& GetMessage() const { return m_Message; }

    /// 本次会写出的 6 个数组名（顺序 = 写入顺序：3 个向量 + 3 个标量），供界面展示
    std::vector<std::string> GetOutputArrayNames() const;

    /**
     * 判断一个数组能否作为输入张量：
     *   6 分量 → 视为 3D 对称张量（不额外校验）；
     *   3 分量 → 仅当属性类型为 IG_TENSOR 时视为 2D 对称张量
     *            （iGame 数组没有"分量名"概念，因此用属性类型判定）。
     */
    static bool IsSymmetricTensor(int dimension, IGenum attributeType);

    /// 主值数组名（`<原名> - Sigma N`）
    static std::string SigmaValueArrayName(const std::string& baseName, int index);
    /// 主方向数组名（`<原名> - Sigma N (Vector)`）
    static std::string SigmaVectorArrayName(const std::string& baseName, int index);

protected:
    TensorPrincipalInvariantsFilter();
    ~TensorPrincipalInvariantsFilter() override = default;

    /// 执行主体逻辑（不含异常捕获，由 Execute 包裹）
    bool ExecuteInternal();

    std::string m_ArrayName;
    IGenum m_Attachment = IG_POINT;
    bool m_ScaleVectors = false;

    /// 最近一次执行的信息
    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif // iGameTensorPrincipalInvariantsFilter_h
