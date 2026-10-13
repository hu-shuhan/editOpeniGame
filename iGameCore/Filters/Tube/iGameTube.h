#pragma once

#include <iGameFilter.h>
#include <iGameVector.h>

IGAME_NAMESPACE_BEGIN

// TubeFilter
// 将一维线段（IG_LINE / IG_POLY_LINE，例如流线、折线、骨架线）
// 扫掠成具有真实厚度的三维圆管（输出 SurfaceMesh）。
//
// 算法（与 ParaView / VTK vtkTubeFilter 一致）：
//   在每个路径点上，先确定“垂直于前进方向(切线 T)的平面”，
//   再在该平面内画一个由半径 Radius 和边数 NumberOfSides 决定的正多边形截面，
//   相邻截面沿路径用四边形缝合，可选 Capping 封端。
//   截面朝向通过“平行传递”沿路径延续，保证不翻转、不扭转。
//
// 参数：
//   Radius         圆管半径（固定）
//   NumberOfSides  截面正多边形边数（>=3，越大越接近圆）
//   Capping        是否封闭首、尾端面
//   UseDefaultNormal 是否使用用户给定的 Default Normal 作为初始法向
//   DefaultNormal  初始法向参考（默认 0,0,1）；不使用时自动选择
class TubeFilter : public Filter {
public:
    I_OBJECT(TubeFilter);
    static Pointer New() { return new TubeFilter; }

    bool Execute() override;

    void SetRadius(double radius);
    double GetRadius() const;

    void SetNumberOfSides(int sides);
    int GetNumberOfSides() const;

    void SetCapping(bool capping);
    bool IsCapping() const;

    void SetUseDefaultNormal(bool useDefault);
    bool IsUsingDefaultNormal() const;

    void SetDefaultNormal(const Vector3d& normal);
    Vector3d GetDefaultNormal() const;

protected:
    TubeFilter();
    ~TubeFilter() override = default;

private:
    double  m_Radius{0.1};
    int     m_NumberOfSides{6};
    bool    m_Capping{true};
    bool    m_UseDefaultNormal{false};
    Vector3d m_DefaultNormal{0.0, 0.0, 1.0};
};

IGAME_NAMESPACE_END
