/**
 * @class   iGameYieldCriteria
 * @brief   在输入数据集的张量属性上计算屈服准则（Yield Criteria）。
 *
 *          对每个点（或单元）的对称应力张量求主应力，再按所选准则输出结果数组：
 *            - 主应力 (Principal Stress)：主应力 σ1 ≥ σ2 ≥ σ3（标量）与对应主方向（单位矢量）
 *            - Tresca 准则：σ1 - σ3
 *            - von Mises 准则：sqrt(0.5 * ((σ1-σ2)² + (σ2-σ3)² + (σ3-σ1)²))
 *
 *          支持一次选择多个张量属性，逐个计算并把结果数组追加到输出上（数组名以各自的
 *          张量名为前缀）；张量属性为 6 分量对称张量（xx, yy, zz, xy, yz, zx）或 9 分量
 *          完整张量。输出为输入网格的深拷贝，几何与拓扑不变。
 */

#pragma once

#include "iGameFilter.h"
#include "iGameDataObject.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class YieldCriteriaFilter : public Filter {
public:
    I_OBJECT(YieldCriteriaFilter)
    static Pointer New() { return new YieldCriteriaFilter; }

    /// 支持的屈服准则
    enum Criterion {
        PRINCIPAL_STRESS = 0, // 主应力（含主方向）
        TRESCA = 1,           // Tresca：σ1 - σ3
        VON_MISES = 2,        // von Mises 等效应力
    };

    bool Execute() override;

    /// 参与计算的张量属性名（可多个，点数据与单元数据可混合）；为空则自动取一个
    void SetTensorArrayNames(const std::vector<std::string>& names);
    const std::vector<std::string>& GetTensorArrayNames() const;

    /// 便捷接口：只设置一个张量属性（等价于 SetTensorArrayNames({name})）
    void SetTensorArrayName(const std::string& name);
    /// 返回第一个张量属性名（未设置时为空串）
    std::string GetTensorArrayName() const;

    /// 参与计算的屈服准则（可多个）；为空则输出输入数据的副本，不追加结果数组。
    void SetCriteria(const std::vector<Criterion>& criteria);
    const std::vector<Criterion>& GetCriteria() const;

    /// 便捷接口：只设置一个准则（等价于 SetCriteria({criterion})）
    void SetCriterion(Criterion criterion);
    /// 返回第一个准则；列表为空时返回主应力作为便捷接口的默认值。
    const Criterion& GetCriterion() const;

    /// 上一次执行的说明（失败原因 / 实际处理的张量属性与准则）
    const std::string& GetStatusMessage() const;

protected:
    YieldCriteriaFilter();
    ~YieldCriteriaFilter() override = default;

    std::vector<std::string> m_TensorArrayNames;
    std::vector<Criterion> m_Criteria{PRINCIPAL_STRESS};
    std::string m_StatusMessage;
};

/**
 * @brief 求 3x3 对称矩阵的特征值（降序）与特征向量。
 *        输入为行主序 3x3 对称矩阵，使用 Jacobi 旋转法。
 * @param a        输入矩阵（会被就地修改）
 * @param values   输出特征值，降序排列
 * @param vectors  输出特征向量（行主序 3x3，第 i 行是 values[i] 对应的单位特征向量）
 */
void ComputeSymmetricEigen(double a[3][3], double values[3], double vectors[3][3]);

IGAME_NAMESPACE_END
