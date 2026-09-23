// ============================================================================
// ProbeFilter — 在指定位置探测数据（点定位 + 插值）
//
// 数据流（v3 设计）:
//   输入 0：模型网格（面网格 / 体网格），提供单元与点属性；
//   输入 1：查询点点集（PointSet），用于点定位 + 插值；
//   输出 0：与输入 1 为同一个对象 —— 在查询点集上原地添加插值属性与
//           ValidPointMask，不新建输出点集，重复执行不会产生新对象。
//
// 用法:
//   auto f = ProbeFilter::New();
//   f->SetInput(model);        // 模型网格
//   f->SetInput(1, queryPts);  // 查询点点集（Execute 后原地带上插值属性）
//   f->SetTolerance(1e-6);     // 可选；默认自动使用 kProbeDefaultTolerance
//   f->Execute();
//   auto result = f->GetOutput();  // 与 queryPts 为同一对象
//
// 算法（v1）:
//   1. 对每个查询点遍历模型单元（面网格只查面单元、体网格只查体单元），
//      调用 EvaluatePosition 判定；v1 仅支持三角形 / 四面体，其余类型未命中；
//   2. 命中后按重心坐标对模型全部点属性做线性插值，写入查询点；
//   3. 新增点属性 ValidPointMask：找到单元 = 1，未找到 = 0；
//      未找到单元时插值属性填 0；
//   4. 模型无属性 / 无单元不报错，流程照常执行。
//
// 球体随机采样（交互层使用）:
//   GenerateSpherePoints(points, center, radius, n) 在以 center 为球心、
//   radius 为半径的球体内均匀随机采样 n 个点，原地写入 points。
//   radius == 0 时按退化球处理：n 个点全部生成在 center 位置。
// ============================================================================
#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>

IGAME_NAMESPACE_BEGIN
class ProbeFilter : public Filter {
public:
    I_OBJECT(ProbeFilter);
    static Pointer New() { return new ProbeFilter; }

    bool Execute() override;

    // ---- 容差（相对值，默认自动）----
    void SetTolerance(double value) { m_Tolerance = value; }
    double GetTolerance() const { return m_Tolerance; }
    void SetAutoTolerance() { m_Tolerance = -1.0; }
    bool HasAutoTolerance() const { return m_Tolerance < 0.0; }

    // ---- 球体随机采样（原地修改 points，供 UI / 测试复用）----
    // radius == 0 时所有点落在 center 位置；radius < 0 时不生成任何点。
    // seed == 0 时使用随机种子；否则使用固定种子（便于复现）。
    static void GenerateSpherePoints(PointSet::Pointer points, const Point& center,
                                     float radius, int count, unsigned seed = 0);

protected:
    ProbeFilter();
    ~ProbeFilter() override = default;

private:
    double m_Tolerance{-1.0};  // < 0 表示自动计算
};
IGAME_NAMESPACE_END
