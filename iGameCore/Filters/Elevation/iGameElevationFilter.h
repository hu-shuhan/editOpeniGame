#pragma once

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 高程标量场过滤器（DIME #19，参数与计算对齐 ParaView Elevation）
 *
 * 低点与高点定义一条有向标尺线段：v = 高点 − 低点 给出投影方向，
 * 低点是参数 t = 0 的锚点、高点是 t = 1 的锚点。每个点 p 的输出为：
 *
 *   t = clamp( ((p − 低点) · v) / |v|² , 0, 1 )
 *   Elevation = RangeLow + t × (RangeHigh − RangeLow)   （标量范围，默认 [0,1]）
 *
 * - t 在 [0,1] 两端饱和：低于低点输出 RangeLow，高于高点输出 RangeHigh，
 *   任何输入都不产生 NaN；低点/高点垂直于线段方向的分量在点积中归零，
 *   不影响结果；
 * - 标尺固定、不随数据自适应：锚点不变时移动/修改点位会改变输出值与着色；
 * - 取色范围（dataRange）由过滤器自管且只扩不缩（grow-only，与 ParaView
 *   颜色条行为一致）：首次执行挂载输出数据的实际范围，此后每次执行用新
 *   数据范围单调扩张——标量范围改大再改小，颜色条保持历史最大范围；
 * - 独立输出：生成新的输出数据对象（几何与输入共享、属性集独立），输入
 *   保持原样；输入未变时重复执行复用已有输出对象（仅替换数组并保留取色
 *   范围），模型树中始终只有一个独立输出节点。
 */
class ElevationFilter : public Filter {
public:
    I_OBJECT(ElevationFilter);
    static Pointer New() { return new ElevationFilter; }

    // 设置标尺线段低点（t = 0 锚点）；默认 (0, 0, 0)，与高点重合时 Execute 被拒绝
    void SetLowPoint(double x, double y, double z);
    void SetLowPoint(const Vector3f& p);

    // 设置标尺线段高点（t = 1 锚点）；默认 (0, 0, 1)
    void SetHighPoint(double x, double y, double z);
    void SetHighPoint(const Vector3f& p);

    const Vector3f& GetLowPoint() const { return m_LowPoint; }
    const Vector3f& GetHighPoint() const { return m_HighPoint; }

    // 设置标量范围（输出值域）；要求 low < high，非法输入被拒绝并保持原值
    void SetScalarRange(double low, double high);
    double GetScalarRangeLow() const { return m_RangeLow; }
    double GetScalarRangeHigh() const { return m_RangeHigh; }

    void SetArrayName(const std::string& name);
    const std::string& GetArrayName() const { return m_ArrayName; }

    bool Execute() override;

protected:
    ElevationFilter();
    ~ElevationFilter() override = default;

private:
    Vector3f m_LowPoint{0.f, 0.f, 0.f};   // 标尺线段低点（t = 0 锚点）
    Vector3f m_HighPoint{0.f, 0.f, 1.f};  // 标尺线段高点（t = 1 锚点）
    double m_RangeLow{0.0};               // 标量范围下限（输出值域）
    double m_RangeHigh{1.0};              // 标量范围上限（输出值域）
    std::string m_ArrayName{"Elevation"};

    // grow-only 取色范围的历史最大范围（跨多次执行单调扩张，只扩不缩）
    double m_RunningMin{0.0};
    double m_RunningMax{0.0};
    bool m_RunningRangeValid{false};

    // 输出对象复用：输入未变时重复执行复用已有输出（模型树保持单一节点）
    DataObject::Pointer m_LastInput{};
    DataObject::Pointer m_Output{};
};

IGAME_NAMESPACE_END
