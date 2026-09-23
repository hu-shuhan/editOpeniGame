// ============================================================================
// ProbeLocator — ProbeFilter 的单元定位（find cell）几何函数集合
//
// 约定（v2，按设计文档）：
//   - 面单元支持三角形、四边形；体单元支持四面体、六面体，其余单元类型直接返回未命中；
//   - 容差为相对值（默认 kProbeDefaultTolerance）：
//       * 插值参数允许超出 [0, 1] 至多 tolerance；
//       * 面单元投影距离 / 插值残差允许至多 tolerance * 模型包围盒对角线；
//   - 四边形用双线性映射反解、六面体用三线性映射反解（默认单元均为凸）；
//   - 命中时输出单元顶点的插值权重，调用方据此对点属性插值。
// ============================================================================
#pragma once
#include <iGameCell.h>
#include <iGamePoints.h>
#include <iGameVector.h>

IGAME_NAMESPACE_BEGIN

// ---- 几何容差 ----
// 默认相对容差：用户未设置容差时由 filter 自动使用
inline constexpr double kProbeDefaultTolerance = 1e-6;
// 数值下限：避免退化单元被误判
inline constexpr double kProbeNumericalEps = 1e-12;

// 单次命中的单元信息（三角形/四边形/四面体/六面体）
struct ProbeCellHit {
    bool found{false};
    int numVertices{0}; // 3/4 = 面单元, 4/8 = 体单元
    int localVertIds[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; // 单元内顶点序号
    double weights[8] = {0.0, 0.0, 0.0, 0.0,
                         0.0, 0.0, 0.0, 0.0}; // 插值权重（重心坐标/双线性/三线性权重）
};

// 判断查询点 q 是否在单元 cell 上（v2：三角形/四边形/四面体/六面体，其余类型未命中）。
// tolerance: 相对容差（无量纲）；
// bboxDiag: 模型包围盒对角线，用于把相对容差换算成投影距离/插值残差；
// 命中时填充 hit 并返回 true，否则返回 false。
bool EvaluatePosition(const Cell* cell, const Point& q, double tolerance,
                      double bboxDiag, ProbeCellHit& hit);

IGAME_NAMESPACE_END
