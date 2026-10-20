#ifndef iGameCellQualityMath_h
#define iGameCellQualityMath_h

/// =========================================================================
/// CellQuality 专用数学库（对齐 ParaView 6.1.1 / Sandia Verdict 源码）
/// =========================================================================
/// 【设计说明】
/// 本文件是 CellMeshMetricsFilter 的【私有】数学实现，按 PR review 要求
/// 从共享的 SurfaceMeshMetricsFilter / VolumeMeshMetricsFilter 中拆分出来：
///   - 共享文件已恢复为 upstream 原版，供 MeshQuality 等其他 filter 调用，
///     其行为与上游严格一致；
///   - 本文件中的全部公式对照 Verdict 官方源码（sandialabs/verdict，BSD-3）
///     重写 / 新增，并全程使用 double 精度（项目的 Point 是 Vector3f，
///     float 在"小量相减"时会产生抵消误差）。
///
/// 【公式来源标注】每个函数对应 Verdict 源码位置见各函数注释。
/// =========================================================================

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <algorithm>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN

/// CellQuality 数学命名空间：全部为无状态静态函数，仅供 CellMeshMetricsFilter 调用。
class CellQualityMath {
public:
    // ---------- 三角形（10 项 + 平均面积 2 项） ----------
    static double ComputeTriangleArea(const std::vector<iGame::Point>& points);              // tri_area (V_TriMetric.cpp:297)
    static double ComputeTriangleAspectRatio(const std::vector<iGame::Point>& points);       // tri_aspect_ratio
    static double ComputeTriangleCondition(const std::vector<iGame::Point>& points);         // tri_condition (V_TriMetric.cpp:579)
    static double ComputeTriangleDistortion(const std::vector<iGame::Point>& points);        // tri_distortion (线性三角形恒为 1)
    static double ComputeTriangleEdgeRatio(const std::vector<iGame::Point>& points);         // tri_edge_ratio
    static double ComputeTriangleEquiangleSkew(const std::vector<iGame::Point>& points);     // tri_equiangle_skew (V_TriMetric.cpp:555)
    static double ComputeTriangleMinAngle(const std::vector<iGame::Point>& points);          // tri_minimum_angle (V_TriMetric.cpp:422)
    static double ComputeTriangleRadiusRatio(const std::vector<iGame::Point>& points);       // tri_radius_ratio (V_TriMetric.cpp:207)
    static double ComputeTriangleScaledJacobian(const std::vector<iGame::Point>& points);    // tri_scaled_jacobian (V_TriMetric.cpp:617)
    static double ComputeTriangleShape(const std::vector<iGame::Point>& points);             // tri_shape (V_TriMetric.cpp:795)
    static double ComputeTriangleRelativeSizeSquared(const std::vector<iGame::Point>& points,
                                                     double averageArea);                    // tri_relative_size_squared (V_TriMetric.cpp:821)
    static double ComputeTriangleShapeAndSize(const std::vector<iGame::Point>& points,
                                              double averageArea);                           // tri_shape_and_size

    // ---------- 四边形（17 项 + 平均面积 3 项） ----------
    static double ComputeQuadArea(const std::vector<iGame::Point>& points);                  // quad_area (V_QuadMetric.cpp:762)
    static double ComputeQuadAspectRatio(const std::vector<iGame::Point>& points);           // quad_aspect_ratio (V_QuadMetric.cpp:448)
    static double ComputeQuadCondition(const std::vector<iGame::Point>& points);             // quad_condition (V_QuadMetric.cpp:1108)
    static double ComputeQuadDistortion(const std::vector<iGame::Point>& points);            // quad_distortion (V_QuadMetric.cpp:1396)
    static double ComputeQuadEdgeRatio(const std::vector<iGame::Point>& points);             // quad_edge_ratio
    static double ComputeQuadEquiangleSkew(const std::vector<iGame::Point>& points);         // quad_equiangle_skew (V_QuadMetric.cpp:1038)
    static double ComputeQuadJacobian(const std::vector<iGame::Point>& points);              // quad_jacobian (V_QuadMetric.cpp:1169)
    static double ComputeQuadMinAngle(const std::vector<iGame::Point>& points);              // quad_minimum_angle (V_QuadMetric.cpp:981)
    static double ComputeQuadOddy(const std::vector<iGame::Point>& points);                  // quad_oddy (V_QuadMetric.cpp:1059)
    static double ComputeQuadRadiusRatio(const std::vector<iGame::Point>& points);           // quad_radius_ratio (V_QuadMetric.cpp:489)
    static double ComputeQuadScaledJacobian(const std::vector<iGame::Point>& points);        // quad_scaled_jacobian (V_QuadMetric.cpp:1193)
    static double ComputeQuadShape(const std::vector<iGame::Point>& points);                 // quad_shape (V_QuadMetric.cpp:1267)
    static double ComputeQuadShear(const std::vector<iGame::Point>& points);                 // quad_shear (V_QuadMetric.cpp:1248)
    static double ComputeQuadSkew(const std::vector<iGame::Point>& points);                  // quad_skew
    static double ComputeQuadStretch(const std::vector<iGame::Point>& points);               // quad_stretch (V_QuadMetric.cpp:864)
    static double ComputeQuadTaper(const std::vector<iGame::Point>& points);                 // quad_taper
    static double ComputeQuadWarpage(const std::vector<iGame::Point>& points);               // quad_warpage (V_QuadMetric.cpp:726)
    static double ComputeQuadRelativeSizeSquared(const std::vector<iGame::Point>& points,
                                                 double averageArea);                        // quad_relative_size_squared (V_QuadMetric.cpp:1325)
    static double ComputeQuadShapeAndSize(const std::vector<iGame::Point>& points,
                                          double averageArea);                               // quad_shape_and_size (V_QuadMetric.cpp:1358)
    static double ComputeQuadShearAndSize(const std::vector<iGame::Point>& points,
                                          double averageArea);                               // quad_shear_and_size

    // ---------- 四面体（8 项 + 平均体积 2 项） ----------
    static double ComputeTetEdgeRatio(const std::vector<iGame::Point>& points);              // tet_edge_ratio
    static double ComputeTetVolume(const std::vector<iGame::Point>& points);                 // tet_volume（有符号，Verdict 口径）
    static double GetAspectRatioOfCell(const std::vector<iGame::Point>& points);             // tet_aspect_ratio (V_TetMetric.cpp:760)
    static double GetAspectRatioOfTet(const std::vector<iGame::Point>& points) { return GetAspectRatioOfCell(points); }
    static double GetJacobianOfCell(const std::vector<iGame::Point>& points);                // tet_jacobian (V_TetMetric.cpp:529)
    static double GetJacobianOfTet(const std::vector<iGame::Point>& points) { return GetJacobianOfCell(points); }
    static double GetCollapseRatioOfCell(const std::vector<iGame::Point>& points);           // tet_collapse_ratio
    static double GetCollapseRatioOfTet(const std::vector<iGame::Point>& points) { return GetCollapseRatioOfCell(points); }
    static double GetMinDihedralAngleOfTet(const std::vector<iGame::Point>& points);         // tet_minimum_angle (V_TetMetric.cpp:931)
    static double ComputeTetCondition(const std::vector<iGame::Point>& points);              // tet_condition (V_TetMetric.cpp:1391)
    static double ComputeTetDistortion(const std::vector<iGame::Point>& points);             // tet_distortion（线性四面体恒为 1）
    static double ComputeTetEquiangleSkew(const std::vector<iGame::Point>& points);          // tet_equiangle_skew (V_TetMetric.cpp:60)
    static double ComputeTetRadiusRatio(const std::vector<iGame::Point>& points);            // tet_radius_ratio (V_TetMetric.cpp:700)
    static double ComputeTetScaledJacobian(const std::vector<iGame::Point>& points);         // tet_scaled_jacobian (V_TetMetric.cpp:555)
    static double ComputeTetShape(const std::vector<iGame::Point>& points);                  // tet_shape (V_TetMetric.cpp:1440)
    static double ComputeTetRelativeSizeSquared(const std::vector<iGame::Point>& points,
                                                double averageVolume);                       // tet_relative_size_squared (V_TetMetric.cpp:1482)
    static double ComputeTetShapeAndSize(const std::vector<iGame::Point>& points,
                                         double averageVolume);                              // tet_shape_and_size (V_TetMetric.cpp:1516)

    // ---------- 六面体（17 项 + 平均体积 3 项） ----------
    static double ComputeHexVolume(const std::vector<iGame::Point>& points);                 // hex_volume
    static double ComputeHexEdgeRatio(const std::vector<iGame::Point>& points);              // hex_edge_ratio (V_HexMetric.cpp:714，含 :815-818 覆盖行为)
    static double ComputeHexJacobian(const std::vector<iGame::Point>& points);               // hex_jacobian
    static double ComputeHexSkew(const std::vector<iGame::Point>& points);                   // hex_skew
    static double ComputeHexTaper(const std::vector<iGame::Point>& points);                  // hex_taper
    static double ComputeHexStretch(const std::vector<iGame::Point>& points);                // hex_stretch
    static double ComputeHexDiagonal(const std::vector<iGame::Point>& points);               // hex_diagonal
    static double ComputeHexMaxEdgeRatio(const std::vector<iGame::Point>& points);           // hex_max_edge_ratio
    static double ComputeHexCondition(const std::vector<iGame::Point>& points);              // hex_condition (V_HexMetric.cpp:1822)
    static double ComputeHexDistortion(const std::vector<iGame::Point>& points);             // hex_distortion (V_HexMetric.cpp:2930)
    static double ComputeHexEquiangleSkew(const std::vector<iGame::Point>& points);          // hex_equiangle_skew (V_HexMetric.cpp:865)
    static double ComputeHexOddy(const std::vector<iGame::Point>& points);                   // hex_oddy (V_HexMetric.cpp:1473)
    static double ComputeHexScaledJacobian(const std::vector<iGame::Point>& points);         // hex_scaled_jacobian (V_HexMetric.cpp:1984)
    static double ComputeHexShape(const std::vector<iGame::Point>& points);                  // hex_shape (V_HexMetric.cpp:2590)
    static double ComputeHexShear(const std::vector<iGame::Point>& points);                  // hex_shear (V_HexMetric.cpp:2369)
    static double ComputeHexRelativeSizeSquared(const std::vector<iGame::Point>& points,
                                                double averageVolume);                       // hex_relative_size_squared (V_HexMetric.cpp:2787)
    static double ComputeHexShapeAndSize(const std::vector<iGame::Point>& points,
                                         double averageVolume);                              // hex_shape_and_size (V_HexMetric.cpp:2894)
    static double ComputeHexShearAndSize(const std::vector<iGame::Point>& points,
                                         double averageVolume);                              // hex_shear_and_size (V_HexMetric.cpp:2913)

    static constexpr double PI = 3.14159265358979323846;

private:
    CellQualityMath() = default; // 纯静态工具类，禁止实例化
};

IGAME_NAMESPACE_END

#endif // !iGameCellQualityMath_h
