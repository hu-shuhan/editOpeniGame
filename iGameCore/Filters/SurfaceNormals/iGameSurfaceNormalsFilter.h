#pragma once

#include <iGameFilter.h>
#include <iGameSurfaceMesh.h>

IGAME_NAMESPACE_BEGIN

// SurfaceNormalsFilter
// 计算表面网格（Poly Data：三角形 / 四边形 / 多边形面片）的面法向量与点法向量。
// 参数语义与 ParaView / VTK vtkPolyDataNormals 对齐。
//
// 输出属性：
//   面数据 (IG_CELL):
//     - "Normals"           : 3 分量法向量 (IG_NORMAL)，单位化
//     - "Normals_Magnitude" : 1 分量标量 (IG_SCALAR)，法向量模长
//   点数据 (IG_POINT):
//     - "Normals"           : 3 分量法向量 (IG_NORMAL)，单位化
//     - "Normals_Magnitude" : 1 分量标量 (IG_SCALAR)，法向量模长
//
// 仅支持 SurfaceMesh（多边形表面网格）；其他类型返回 false。
class SurfaceNormalsFilter : public Filter {
public:
    I_OBJECT(SurfaceNormalsFilter);
    static Pointer New() { return new SurfaceNormalsFilter; }

    bool Execute() override;

    /// 是否计算点法向量，默认 true。
    void SetComputePointNormals(bool value) { m_ComputePointNormals = value; }
    bool GetComputePointNormals() const { return m_ComputePointNormals; }

    /// 是否计算面法向量，默认 true。
    void SetComputeCellNormals(bool value) { m_ComputeCellNormals = value; }
    bool GetComputeCellNormals() const { return m_ComputeCellNormals; }

    /// 是否按特征角分裂锐边上的共享顶点，默认 true。
    void SetSplitting(bool value) { m_Splitting = value; }
    bool GetSplitting() const { return m_Splitting; }

    /// 锐边特征角，单位为度，默认 30。
    void SetFeatureAngle(double degrees) { m_FeatureAngle = degrees; }
    double GetFeatureAngle() const { return m_FeatureAngle; }

    /// 是否翻转最终法向量，默认 false。
    void SetFlipNormals(bool value) { m_FlipNormals = value; }
    bool GetFlipNormals() const { return m_FlipNormals; }

    /// 是否自动统一相邻面的环绕方向，默认 true。
    void SetConsistency(bool value) { m_Consistency = value; }
    bool GetConsistency() const { return m_Consistency; }

protected:
    SurfaceNormalsFilter();
    ~SurfaceNormalsFilter() override = default;

private:
    bool m_ComputePointNormals{true};
    bool m_ComputeCellNormals{true};
    bool m_Splitting{true};
    double m_FeatureAngle{30.0};
    bool m_FlipNormals{false};
    bool m_Consistency{true};
};

IGAME_NAMESPACE_END
