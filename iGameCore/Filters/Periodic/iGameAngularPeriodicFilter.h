#ifndef iGameAngularPeriodicFilter_h
#define iGameAngularPeriodicFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"

#include <array>

IGAME_NAMESPACE_BEGIN

// 角度周期复制过滤器：把一个网格绕指定轴旋转复制 N 份（角度周期），
// 输出一个合并后的 UnstructuredMesh（包含原始网格 + 各旋转副本）。
//
// 参数语义与 ParaView 的 vtkAngularPeriodicFilter 对齐：
//   - 周期角度 angle(deg)：相邻两份之间的旋转角度（不是总角度）；
//   - 周期数量 copies   ：总份数（含第 0 份原始网格）；
//   - 第 i 份绕轴旋转 i×angle 度。
// 例如 angle=90、copies=4，得到 0°、90°、180°、270° 四份。
//
// 份数模式对齐 vtkPeriodicFilter::IterationMode：
//   - DIRECT_NB：使用 SetNumberOfCopies 指定的份数；
//   - MAX      ：自动取不超过整周的最大份数 floor(360 / angle)。
// 覆盖情况（整周闭合 / 缺口 / 重叠）通过 GetCoverageInfo() 返回，默认只提示不拦截；
// 设 SetRequireFullPeriod(true) 后，非整周闭合将导致 Execute 失败。
//
// 几何旋转的同时，点/单元属性按语义同步旋转：
//   - IG_VECTOR / IG_NORMAL（3 分量）：作为方向向量随几何旋转；
//   - IG_TENSOR（9 分量或 6 分量对称张量）：按 R·T·Rᵀ 旋转；
//   - 其余类型（标量、纹理坐标、RGB 等）以及无符号整型数组按份原样复制。
class AngularPeriodicFilter : public Filter {
public:
    I_OBJECT(AngularPeriodicFilter);
    static Pointer New() { return new AngularPeriodicFilter; }

    // 份数模式（对齐 vtkPeriodicFilter::IterationMode）：
    //   DIRECT_NB：使用用户指定的周期数量；
    //   MAX      ：自动生成不超过整周（360°）的最大份数 floor(360/angle)。
    enum IterationMode {
        ITERATION_MODE_DIRECT_NB = 0,
        ITERATION_MODE_MAX = 1
    };

    // 设置旋转轴：过点 origin，方向 axis（自动归一化）
    void SetRotationAxis(const Point& origin, const Vector3d& axis);
    // 设置周期数量（总份数，含原始网格）
    void SetNumberOfCopies(int n) { m_NumberOfCopies = n; }
    // 设置周期角度（度）：相邻两份之间的旋转角度，第 i 份旋转 i×angle 度
    void SetAngle(float angle) { m_Angle = angle; }
    // 设置份数模式
    void SetIterationMode(int mode) { m_IterationMode = mode; }
    int GetIterationMode() const { return m_IterationMode; }
    void SetIterationModeToDirectNb() { m_IterationMode = ITERATION_MODE_DIRECT_NB; }
    void SetIterationModeToMax() { m_IterationMode = ITERATION_MODE_MAX; }
    // 是否要求整周闭合（份数×角度 == 360°）；为 true 且不闭合时 Execute 失败
    void SetRequireFullPeriod(bool require) { m_RequireFullPeriod = require; }
    bool GetRequireFullPeriod() const { return m_RequireFullPeriod; }

    bool Execute() override;

    std::string GetMessage() const { return m_Message; }
    // 覆盖信息：整周闭合 / 缺口 x° / 重叠 x°，以及各份角度（Execute 后有效）
    std::string GetCoverageInfo() const { return m_CoverageInfo; }
    // 实际生成的份数（MAX 模式下可能与 SetNumberOfCopies 不同）
    int GetEffectiveNumberOfCopies() const { return m_EffectiveCopies; }

private:
    // 绕轴（m_AxisOrigin, m_AxisNormalized）旋转的 3x3 矩阵（行主序）
    using RotMat = std::array<double, 9>;
    // 用旋转矩阵 R 把点 p 绕轴旋转
    Point RotatePoint(const Point& p, const RotMat& rotation) const;

protected:
    AngularPeriodicFilter();
    ~AngularPeriodicFilter() override = default;

    Point m_AxisOrigin{0.f, 0.f, 0.f};
    Vector3d m_AxisNormalized{0.0, 0.0, 1.0};
    int m_NumberOfCopies{4};
    int m_IterationMode{ITERATION_MODE_DIRECT_NB};
    bool m_RequireFullPeriod{false};
    int m_EffectiveCopies{4};
    float m_Angle{90.0f};

    std::string m_Message{""};
    std::string m_CoverageInfo{""};
};

IGAME_NAMESPACE_END
#endif
