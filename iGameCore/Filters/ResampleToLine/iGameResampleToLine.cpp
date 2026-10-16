#include "iGameResampleToLine.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>

IGAME_NAMESPACE_BEGIN

namespace {

// 自动容差比例：容差 = 包围盒对角线 × kAutoToleranceRatio
constexpr double kAutoToleranceRatio = 1e-6;
// 形函数参数坐标容差
constexpr double kParamTol = 1e-6;
// 支持插值的最大单元点数
constexpr int kMaxCellPointNum = 32;

/** 3x3 行列式（等价于 c1 . (c2 x c3)） */
inline double Det3(const double c1[3], const double c2[3], const double c3[3]) {
    return c1[0] * c2[1] * c3[2] + c2[0] * c3[1] * c1[2] + c3[0] * c1[1] * c2[2] -
           c1[0] * c3[1] * c2[2] - c2[0] * c1[1] * c3[2] - c3[0] * c2[1] * c1[2];
}

/**
 * 单元插值类型：
 *   - 线性面单元 → 线性插值（重心坐标 / 双线性 / 扇形三角化）
 *   - 体单元     → 均值坐标（Mean Value Coordinates）
 *   - 二次单元   → 二次形函数（Newton 参数坐标反解）
 */
enum ShapeKind {
    SHAPE_NONE = 0,
    // 线性面单元
    SHAPE_LINEAR_TRIANGLE,
    SHAPE_LINEAR_QUAD,
    SHAPE_LINEAR_POLYGON,
    // 二次面单元
    SHAPE_QUADRATIC_TRIANGLE, // 6 节点
    SHAPE_QUADRATIC_QUAD,     // 8 节点
    // 体单元
    SHAPE_TETRA,              // 4 节点：解析重心坐标（等价于均值坐标，精确）
    SHAPE_MVC_VOLUME,         // 六面体/棱柱/金字塔/多面体：均值坐标
    // 二次体单元
    SHAPE_QUADRATIC_TETRA,    // 10 节点
    SHAPE_QUADRATIC_HEX,      // 20 节点
    SHAPE_QUADRATIC_PRISM,    // 15 节点（三棱柱 / 楔形）
    SHAPE_QUADRATIC_PYRAMID,  // 13 节点
    // 高次单元（节点序与形函数按 VTK 硬编码；VTK 中 pcoords 为 [0,1]，此处直接使用其 [-1,1] 内部形式）
    SHAPE_BIQUADRATIC_QUAD,       // 9 节点：0-3 角点、4-7 棱中点(0-1)(1-2)(2-3)(3-0)、8 面心
    SHAPE_QUADRATIC_LINEAR_QUAD,  // 6 节点：0-3 角点、4=棱(0,1)中点、5=棱(2,3)中点
    SHAPE_QUADRATIC_LINEAR_WEDGE, // 12 节点：0-5 角点、6-8 底面棱中点、9-11 顶面棱中点
    SHAPE_TRIQUADRATIC_HEX,       // 27 节点：0-7 角点、8-19 棱中点、20-25 面心、26 体心
};

/** 是否走「二次 / 高次形函数」插值路径 */
inline bool IsQuadraticKind(ShapeKind kind) {
    switch (kind) {
        case SHAPE_QUADRATIC_TRIANGLE:
        case SHAPE_QUADRATIC_QUAD:
        case SHAPE_QUADRATIC_TETRA:
        case SHAPE_QUADRATIC_HEX:
        case SHAPE_QUADRATIC_PRISM:
        case SHAPE_QUADRATIC_PYRAMID:
        case SHAPE_BIQUADRATIC_QUAD:
        case SHAPE_QUADRATIC_LINEAR_QUAD:
        case SHAPE_QUADRATIC_LINEAR_WEDGE:
        case SHAPE_TRIQUADRATIC_HEX:
            return true;
        default:
            return false;
    }
}

/** 二次「面」单元（走平面投影 + 2D Newton） */
inline bool IsQuadraticFaceKind(ShapeKind kind) {
    return kind == SHAPE_QUADRATIC_TRIANGLE || kind == SHAPE_QUADRATIC_QUAD || kind == SHAPE_BIQUADRATIC_QUAD ||
           kind == SHAPE_QUADRATIC_LINEAR_QUAD;
}

/** 四个角点的四边形类单元（用于构造投影基） */
inline bool IsQuadKind(ShapeKind kind) {
    return kind == SHAPE_LINEAR_QUAD || kind == SHAPE_QUADRATIC_QUAD || kind == SHAPE_BIQUADRATIC_QUAD ||
           kind == SHAPE_QUADRATIC_LINEAR_QUAD;
}

/** 按单元类型与点数判定插值方式 */
ShapeKind ClassifyCell(IGenum cellType, int npts) {
    switch (cellType) {
        case IG_TRIANGLE:
            return (npts == 3) ? SHAPE_LINEAR_TRIANGLE : SHAPE_NONE;
        case IG_QUAD:
            return (npts == 4) ? SHAPE_LINEAR_QUAD : SHAPE_NONE;
        case IG_POLYGON:
            return (npts >= 3) ? SHAPE_LINEAR_POLYGON : SHAPE_NONE;
        case IG_TETRA:
            return (npts == 4) ? SHAPE_TETRA : SHAPE_NONE;
        case IG_HEXAHEDRON:
        case IG_PRISM:
        case IG_PYRAMID:
        case IG_VOLUME:
        case IG_POLYHEDRON:
            return (npts >= 4) ? SHAPE_MVC_VOLUME : SHAPE_NONE;
        case IG_QUADRATIC_TRIANGLE:
        case IG_BIQUADRATIC_TRIANGLE:
            return (npts == 6) ? SHAPE_QUADRATIC_TRIANGLE : SHAPE_NONE;
        case IG_QUADRATIC_QUAD:
            return (npts == 8) ? SHAPE_QUADRATIC_QUAD : SHAPE_NONE;
        case IG_QUADRATIC_TETRA:
            return (npts == 10) ? SHAPE_QUADRATIC_TETRA : SHAPE_NONE;
        case IG_QUADRATIC_HEXAHEDRON:
            return (npts == 20) ? SHAPE_QUADRATIC_HEX : SHAPE_NONE;
        case IG_QUADRATIC_PRISM:
            return (npts == 15) ? SHAPE_QUADRATIC_PRISM : SHAPE_NONE;
        case IG_QUADRATIC_PYRAMID:
            return (npts == 13) ? SHAPE_QUADRATIC_PYRAMID : SHAPE_NONE;
        // 以下节点序见 VTK：vtkBiQuadraticQuad / vtkQuadraticLinearQuad /
        // vtkQuadraticLinearWedge / vtkTriQuadraticHexahedron
        case IG_BIQUADRATIC_QUAD:
            return (npts == 9) ? SHAPE_BIQUADRATIC_QUAD : SHAPE_NONE;
        case IG_QUADRATIC_LINEAR_QUAD:
            return (npts == 6) ? SHAPE_QUADRATIC_LINEAR_QUAD : SHAPE_NONE;
        case IG_QUADRATIC_LINEAR_WEDGE:
            return (npts == 12) ? SHAPE_QUADRATIC_LINEAR_WEDGE : SHAPE_NONE;
        case IG_TRIQUADRATIC_HEXAHEDRON:
            return (npts == 27) ? SHAPE_TRIQUADRATIC_HEX : SHAPE_NONE;
        default:
            return SHAPE_NONE;
    }
}

/** 是否为「二次 / 高次」单元族（用于退化提示统计） */
bool IsHighOrderCell(IGenum cellType) {
    switch (cellType) {
        case IG_QUADRATIC_EDGE:
        case IG_QUADRATIC_TRIANGLE:
        case IG_QUADRATIC_QUAD:
        case IG_QUADRATIC_POLYGON:
        case IG_QUADRATIC_TETRA:
        case IG_QUADRATIC_HEXAHEDRON:
        case IG_QUADRATIC_PRISM:
        case IG_QUADRATIC_PYRAMID:
        case IG_BIQUADRATIC_QUAD:
        case IG_TRIQUADRATIC_HEXAHEDRON:
        case IG_TRIQUADRATIC_PYRAMID:
        case IG_QUADRATIC_LINEAR_QUAD:
        case IG_QUADRATIC_LINEAR_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        case IG_BIQUADRATIC_TRIANGLE:
        case IG_LAGRANGE_CURVE:
        case IG_LAGRANGE_TRIANGLE:
        case IG_LAGRANGE_QUADRILATERAL:
        case IG_LAGRANGE_TETRAHEDRON:
        case IG_LAGRANGE_HEXAHEDRON:
        case IG_LAGRANGE_PRISM:
        case IG_LAGRANGE_PYRAMID:
            return true;
        default:
            return false;
    }
}

/** 不支持的高次「面」单元退化到线性角点时，需要使用的角点数量（0 = 无法退化） */
int LinearCornerCount(IGenum cellType, int npts) {
    switch (cellType) {
        case IG_TRIANGLE:
        case IG_QUADRATIC_TRIANGLE:
        case IG_BIQUADRATIC_TRIANGLE:
        case IG_LAGRANGE_TRIANGLE:
            return (npts >= 3) ? 3 : 0;
        case IG_QUAD:
        case IG_QUADRATIC_QUAD:
        case IG_BIQUADRATIC_QUAD:
        case IG_QUADRATIC_LINEAR_QUAD:
        case IG_LAGRANGE_QUADRILATERAL:
            return (npts >= 4) ? 4 : 0;
        case IG_POLYGON:
        case IG_QUADRATIC_POLYGON:
            return (npts >= 3) ? npts : 0;
        default:
            return 0;
    }
}

/* ================================================================== */
/* 形函数：线性四边形                                                  */
/* ================================================================== */
void LinearQuadShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1];
    sf[0] = (1.0 - r) * (1.0 - s);
    sf[1] = r * (1.0 - s);
    sf[2] = r * s;
    sf[3] = (1.0 - r) * s;
}

void LinearQuadDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1];
    d[0] = -(1.0 - s);
    d[1] = (1.0 - s);
    d[2] = s;
    d[3] = -s;
    d[4] = -(1.0 - r);
    d[5] = -r;
    d[6] = r;
    d[7] = (1.0 - r);
}

/* ================================================================== */
/* 形函数：二次单元                                                    */
/* ================================================================== */

/** 二次三角形（6 节点）：参数坐标 r,s ∈ [0,1], r+s ≤ 1 */
void QuadraticTriangleShape(const double pc[3], double* sf) {
    const double L0 = 1.0 - pc[0] - pc[1];
    const double L1 = pc[0];
    const double L2 = pc[1];
    sf[0] = L0 * (2.0 * L0 - 1.0);
    sf[1] = L1 * (2.0 * L1 - 1.0);
    sf[2] = L2 * (2.0 * L2 - 1.0);
    sf[3] = 4.0 * L0 * L1; // 棱 0-1
    sf[4] = 4.0 * L1 * L2; // 棱 1-2
    sf[5] = 4.0 * L2 * L0; // 棱 2-0
}

void QuadraticTriangleDerivs(const double pc[3], double* d) {
    const double L[3] = {1.0 - pc[0] - pc[1], pc[0], pc[1]};
    const double dL[3][2] = {{-1.0, -1.0}, {1.0, 0.0}, {0.0, 1.0}};
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 2; ++k) { d[k * 6 + i] = (4.0 * L[i] - 1.0) * dL[i][k]; }
    }
    const int pairs[3][2] = {{0, 1}, {1, 2}, {2, 0}};
    for (int e = 0; e < 3; ++e) {
        const int a = pairs[e][0], b = pairs[e][1];
        for (int k = 0; k < 2; ++k) { d[k * 6 + 3 + e] = 4.0 * (dL[a][k] * L[b] + L[a] * dL[b][k]); }
    }
}

/** 二次四边形（8 节点，Serendipity）：参数坐标 r,s ∈ [0,1] */
void QuadraticQuadShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1];
    sf[0] = (1.0 - r) * (1.0 - s) * (1.0 - 2.0 * r - 2.0 * s);
    sf[1] = r * (1.0 - s) * (2.0 * r - 2.0 * s - 1.0);
    sf[2] = r * s * (2.0 * r + 2.0 * s - 3.0);
    sf[3] = (1.0 - r) * s * (-2.0 * r + 2.0 * s - 1.0);
    sf[4] = 4.0 * r * (1.0 - r) * (1.0 - s);
    sf[5] = 4.0 * r * s * (1.0 - s);
    sf[6] = 4.0 * r * s * (1.0 - r);
    sf[7] = 4.0 * s * (1.0 - r) * (1.0 - s);
}

void QuadraticQuadDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1];
    // d/dr
    d[0] = (1.0 - s) * (4.0 * r + 2.0 * s - 3.0);
    d[1] = (1.0 - s) * (4.0 * r - 2.0 * s - 1.0);
    d[2] = s * (4.0 * r + 2.0 * s - 3.0);
    d[3] = s * (4.0 * r - 2.0 * s - 1.0);
    d[4] = 4.0 * (1.0 - 2.0 * r) * (1.0 - s);
    d[5] = 4.0 * s * (1.0 - s);
    d[6] = 4.0 * s * (1.0 - 2.0 * r);
    d[7] = -4.0 * s * (1.0 - s);
    // d/ds
    d[8] = (1.0 - r) * (2.0 * r + 4.0 * s - 3.0);
    d[9] = r * (-2.0 * r + 4.0 * s - 1.0);
    d[10] = r * (2.0 * r + 4.0 * s - 3.0);
    d[11] = (1.0 - r) * (-2.0 * r + 4.0 * s - 1.0);
    d[12] = -4.0 * r * (1.0 - r);
    d[13] = 4.0 * r * (1.0 - 2.0 * s);
    d[14] = 4.0 * r * (1.0 - r);
    d[15] = 4.0 * (1.0 - r) * (1.0 - 2.0 * s);
}

/** 二次四面体（10 节点）：参数坐标 (L1,L2,L3)，L0 = 1 - L1 - L2 - L3 */
void QuadraticTetraShape(const double pc[3], double* sf) {
    const double L[4] = {1.0 - pc[0] - pc[1] - pc[2], pc[0], pc[1], pc[2]};
    for (int i = 0; i < 4; ++i) { sf[i] = L[i] * (2.0 * L[i] - 1.0); }
    const int pairs[6][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};
    for (int e = 0; e < 6; ++e) { sf[4 + e] = 4.0 * L[pairs[e][0]] * L[pairs[e][1]]; }
}

void QuadraticTetraDerivs(const double pc[3], double* d) {
    const double L[4] = {1.0 - pc[0] - pc[1] - pc[2], pc[0], pc[1], pc[2]};
    const double dL[4][3] = {{-1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    for (int i = 0; i < 4; ++i) {
        for (int k = 0; k < 3; ++k) { d[k * 10 + i] = (4.0 * L[i] - 1.0) * dL[i][k]; }
    }
    const int pairs[6][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};
    for (int e = 0; e < 6; ++e) {
        const int a = pairs[e][0], b = pairs[e][1];
        for (int k = 0; k < 3; ++k) { d[k * 10 + 4 + e] = 4.0 * (dL[a][k] * L[b] + L[a] * dL[b][k]); }
    }
}

// 二次六面体（20 节点）节点自然坐标符号表（VTK 节点顺序）
const int kQuadHexSign[20][3] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1},
        {0, -1, -1},  {1, 0, -1},  {0, 1, -1},  {-1, 0, -1},  {0, -1, 1},  {1, 0, 1},  {0, 1, 1},  {-1, 0, 1},
        {-1, -1, 0},  {1, -1, 0},  {1, 1, 0},   {-1, 1, 0}};

/** 二次六面体（20 节点，Serendipity）：参数坐标 r,s,t ∈ [-1,1] */
void QuadraticHexShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1], t = pc[2];
    for (int i = 0; i < 8; ++i) {
        const double A = 1.0 + r * kQuadHexSign[i][0];
        const double B = 1.0 + s * kQuadHexSign[i][1];
        const double C = 1.0 + t * kQuadHexSign[i][2];
        const double S = kQuadHexSign[i][0] * r + kQuadHexSign[i][1] * s + kQuadHexSign[i][2] * t - 2.0;
        sf[i] = 0.125 * A * B * C * S;
    }
    for (int i = 8; i < 20; ++i) {
        const int sx = kQuadHexSign[i][0], sy = kQuadHexSign[i][1], sz = kQuadHexSign[i][2];
        if (sx == 0) {
            sf[i] = 0.25 * (1.0 - r * r) * (1.0 + s * sy) * (1.0 + t * sz);
        } else if (sy == 0) {
            sf[i] = 0.25 * (1.0 + r * sx) * (1.0 - s * s) * (1.0 + t * sz);
        } else {
            sf[i] = 0.25 * (1.0 + r * sx) * (1.0 + s * sy) * (1.0 - t * t);
        }
    }
}

void QuadraticHexDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1], t = pc[2];
    for (int i = 0; i < 8; ++i) {
        const int sx = kQuadHexSign[i][0], sy = kQuadHexSign[i][1], sz = kQuadHexSign[i][2];
        const double A = 1.0 + r * sx, B = 1.0 + s * sy, C = 1.0 + t * sz;
        const double S = sx * r + sy * s + sz * t - 2.0;
        d[0 * 20 + i] = 0.125 * B * C * sx * (S + A);
        d[1 * 20 + i] = 0.125 * A * C * sy * (S + B);
        d[2 * 20 + i] = 0.125 * A * B * sz * (S + C);
    }
    for (int i = 8; i < 20; ++i) {
        const int sx = kQuadHexSign[i][0], sy = kQuadHexSign[i][1], sz = kQuadHexSign[i][2];
        if (sx == 0) {
            d[0 * 20 + i] = -0.5 * r * (1.0 + s * sy) * (1.0 + t * sz);
            d[1 * 20 + i] = 0.25 * (1.0 - r * r) * sy * (1.0 + t * sz);
            d[2 * 20 + i] = 0.25 * (1.0 - r * r) * (1.0 + s * sy) * sz;
        } else if (sy == 0) {
            d[0 * 20 + i] = 0.25 * sx * (1.0 - s * s) * (1.0 + t * sz);
            d[1 * 20 + i] = -0.5 * s * (1.0 + r * sx) * (1.0 + t * sz);
            d[2 * 20 + i] = 0.25 * (1.0 + r * sx) * (1.0 - s * s) * sz;
        } else {
            d[0 * 20 + i] = 0.25 * sx * (1.0 + s * sy) * (1.0 - t * t);
            d[1 * 20 + i] = 0.25 * (1.0 + r * sx) * sy * (1.0 - t * t);
            d[2 * 20 + i] = -0.5 * t * (1.0 + r * sx) * (1.0 + s * sy);
        }
    }
}

/* ------------------------------------------------------------------ */
/* 二次三棱柱（15 节点）                                               */
/* ------------------------------------------------------------------ */
// 节点顺序（与本工程 QuadraticPrism::edges 一致）：
//   0,1,2 底面角点；3,4,5 顶面角点；
//   6(0-1), 7(1-2), 8(2-0) 底面棱中点；9(3-4), 10(4-5), 11(5-3) 顶面棱中点；
//   12(0-3), 13(1-4), 14(2-5) 竖棱中点。
// 三角形部分用 6 节点二次三角形基函数 T[0..5]，底面层因子 phi、顶面层因子 psi、
// 竖向气泡 b = 4t(1-t)：Sum(N) == 1，且在每个节点取 Kronecker 值（已逐点验证）。
namespace {
// 节点 → 二次三角形基函数索引（-1 表示竖棱中点）
const int kPrismTIndex[15] = {0, 1, 2, 0, 1, 2, 3, 4, 5, 3, 4, 5, -1, -1, -1};
// 0 = 底面层(phi)，1 = 顶面层(psi)，-1 = 竖棱
const int kPrismLevel[15] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, -1, -1, -1};

/** 二次三角形的 6 个基函数及其偏导（r,s 方向） */
void Prism15TriangleBasis(const double pc[3], double T[6], double dT[6][3], double L[3], double dL[3][3]) {
    L[0] = 1.0 - pc[0] - pc[1];
    L[1] = pc[0];
    L[2] = pc[1];
    dL[0][0] = -1.0; dL[0][1] = -1.0; dL[0][2] = 0.0;
    dL[1][0] = 1.0;  dL[1][1] = 0.0;  dL[1][2] = 0.0;
    dL[2][0] = 0.0;  dL[2][1] = 1.0;  dL[2][2] = 0.0;

    T[0] = L[0] * (2.0 * L[0] - 1.0);
    T[1] = L[1] * (2.0 * L[1] - 1.0);
    T[2] = L[2] * (2.0 * L[2] - 1.0);
    T[3] = 4.0 * L[0] * L[1]; // 棱 0-1
    T[4] = 4.0 * L[1] * L[2]; // 棱 1-2
    T[5] = 4.0 * L[2] * L[0]; // 棱 2-0
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) { dT[i][k] = (4.0 * L[i] - 1.0) * dL[i][k]; }
    }
    const int pairs[3][2] = {{0, 1}, {1, 2}, {2, 0}};
    for (int e = 0; e < 3; ++e) {
        const int a = pairs[e][0], b = pairs[e][1];
        for (int k = 0; k < 3; ++k) { dT[3 + e][k] = 4.0 * (dL[a][k] * L[b] + L[a] * dL[b][k]); }
    }
}
} // namespace

void QuadraticPrism15Shape(const double pc[3], double* sf) {
    double T[6], dT[6][3], L[3], dL[3][3];
    Prism15TriangleBasis(pc, T, dT, L, dL);

    const double t = pc[2];
    const double b = 4.0 * t * (1.0 - t);
    const double phi = 1.0 - t - 0.5 * b; // 底面层
    const double psi = t - 0.5 * b;       // 顶面层

    for (int i = 0; i < 15; ++i) {
        const int ti = kPrismTIndex[i];
        if (ti >= 0) {
            sf[i] = T[ti] * (kPrismLevel[i] == 1 ? psi : phi);
        } else {
            sf[i] = L[i - 12] * b;
        }
    }
}

void QuadraticPrism15Derivs(const double pc[3], double* d) {
    double T[6], dT[6][3], L[3], dL[3][3];
    Prism15TriangleBasis(pc, T, dT, L, dL);

    const double t = pc[2];
    const double b = 4.0 * t * (1.0 - t);
    const double db = 4.0 - 8.0 * t;
    const double phi = 1.0 - t - 0.5 * b, dphi = -1.0 - 0.5 * db;
    const double psi = t - 0.5 * b, dpsi = 1.0 - 0.5 * db;

    for (int i = 0; i < 15; ++i) {
        const int ti = kPrismTIndex[i];
        if (ti >= 0) {
            const bool top = (kPrismLevel[i] == 1);
            const double f = top ? psi : phi;
            const double df = top ? dpsi : dphi;
            for (int k = 0; k < 3; ++k) { d[k * 15 + i] = dT[ti][k] * f; }
            d[2 * 15 + i] = T[ti] * df; // 三角形基函数与 t 无关，直接赋值
        } else {
            const int li = i - 12;
            for (int k = 0; k < 3; ++k) { d[k * 15 + i] = dL[li][k] * b; }
            d[2 * 15 + i] = L[li] * db;
        }
    }
}

/* ------------------------------------------------------------------ */
/* 二次金字塔（13 节点）                                               */
/* ------------------------------------------------------------------ */
// 节点顺序（与本工程 QuadraticPyramid::edges 一致）：
//   0,1,2,3 底面角点；4 顶点；5(0-1),6(1-2),7(2-3),8(3-0) 底棱中点；
//   9(0-4),10(1-4),11(2-4),12(3-4) 侧棱中点。
// 底面用 8 节点二次四边形 Q[0..7]，侧棱用线性四边形角点函数 q[0..3]：
//   phi = (1-t)(1-2t)（底面层）、g = t(2t-1)(5-4t)（顶点）、h = 8t(1-t)^2（侧棱）
//   Sum(N) == 1，且在每个节点取 Kronecker 值（已逐点验证）。
void QuadraticPyramid13Shape(const double pc[3], double* sf) {
    const double pc2[3] = {pc[0], pc[1], 0.0};
    double Q[8], q[4];
    QuadraticQuadShape(pc2, Q);
    LinearQuadShape(pc2, q);

    const double t = pc[2];
    const double phi = (1.0 - t) * (1.0 - 2.0 * t);
    const double g = t * (2.0 * t - 1.0) * (5.0 - 4.0 * t);
    const double h = 8.0 * t * (1.0 - t) * (1.0 - t);

    sf[0] = Q[0] * phi;
    sf[1] = Q[1] * phi;
    sf[2] = Q[2] * phi;
    sf[3] = Q[3] * phi;
    sf[5] = Q[4] * phi;
    sf[6] = Q[5] * phi;
    sf[7] = Q[6] * phi;
    sf[8] = Q[7] * phi;

    sf[4] = g;

    sf[9] = h * q[0];
    sf[10] = h * q[1];
    sf[11] = h * q[2];
    sf[12] = h * q[3];
}

void QuadraticPyramid13Derivs(const double pc[3], double* d) {
    const double pc2[3] = {pc[0], pc[1], 0.0};
    double Q[8], q[4], dQ[3 * 8], dq[3 * 4];
    QuadraticQuadShape(pc2, Q);
    LinearQuadShape(pc2, q);
    QuadraticQuadDerivs(pc2, dQ);
    LinearQuadDerivs(pc2, dq);

    const double t = pc[2];
    const double phi = (1.0 - t) * (1.0 - 2.0 * t), dphi = -3.0 + 4.0 * t;
    const double dg = -24.0 * t * t + 28.0 * t - 5.0;
    const double h = 8.0 * t * (1.0 - t) * (1.0 - t);
    const double dh = 8.0 * (1.0 - t) * (1.0 - 3.0 * t);

    const int base[8] = {0, 1, 2, 3, 5, 6, 7, 8}; // 单元节点 → Q 索引
    for (int i = 0; i < 8; ++i) {
        const int n = base[i];
        for (int k = 0; k < 2; ++k) { d[k * 13 + n] = dQ[k * 8 + i] * phi; }
        d[2 * 13 + n] = Q[i] * dphi;
    }

    for (int k = 0; k < 3; ++k) { d[k * 13 + 4] = 0.0; }
    d[2 * 13 + 4] = dg;

    const int lat[4] = {9, 10, 11, 12}; // 单元节点 → q 索引
    for (int i = 0; i < 4; ++i) {
        const int n = lat[i];
        for (int k = 0; k < 2; ++k) { d[k * 13 + n] = dq[k * 4 + i] * h; }
        d[2 * 13 + n] = q[i] * dh;
    }
}

/* ------------------------------------------------------------------ */
/* 高次单元（按 VTK 硬编码）                                           */
/* ------------------------------------------------------------------ */
// VTK 的二次单元形函数内部统一在 [-1,1] 上定义（对外 pcoords 为 [0,1]，
// 通过 r = 2*(pcoords - 0.5) 转换）。这里直接使用其 [-1,1] 形式，公式与
// VTK 源码逐行一致，因此插值结果与 VTK 相同。
namespace {
/** 一维二次 Lagrange（节点 -1, 0, +1）：对应 VTK 的 g1/g2/g3 */
inline void LagrangeG(double x, double g[3], double dg[3]) {
    g[0] = -0.5 * x * (1.0 - x);  // 节点 -1
    g[1] = (1.0 + x) * (1.0 - x); // 节点 0
    g[2] = 0.5 * x * (1.0 + x);   // 节点 +1
    dg[0] = x - 0.5;
    dg[1] = -2.0 * x;
    dg[2] = x + 0.5;
}
} // namespace

// 9 节点双二次四边形：VTK vtkBiQuadraticQuad
// 节点 → (gx 索引, gy 索引)，索引 0 = -1、1 = 0、2 = +1
namespace {
const int kBiQuadIdx[9][2] = {{0, 0}, {2, 0}, {2, 2}, {0, 2}, {1, 0}, {2, 1}, {1, 2}, {0, 1}, {1, 1}};
} // namespace

void BiQuadraticQuadShape(const double pc[3], double* sf) {
    double gx[3], dx[3], gy[3], dy[3];
    LagrangeG(pc[0], gx, dx);
    LagrangeG(pc[1], gy, dy);
    for (int i = 0; i < 9; ++i) { sf[i] = gx[kBiQuadIdx[i][0]] * gy[kBiQuadIdx[i][1]]; }
}

void BiQuadraticQuadDerivs(const double pc[3], double* d) {
    double gx[3], dx[3], gy[3], dy[3];
    LagrangeG(pc[0], gx, dx);
    LagrangeG(pc[1], gy, dy);
    for (int i = 0; i < 9; ++i) {
        d[0 * 9 + i] = dx[kBiQuadIdx[i][0]] * gy[kBiQuadIdx[i][1]];
        d[1 * 9 + i] = gx[kBiQuadIdx[i][0]] * dy[kBiQuadIdx[i][1]];
    }
}

// 6 节点「二次-线性」四边形：VTK vtkQuadraticLinearQuad
// 节点 0-3 角点 (-1,-1)(1,-1)(1,1)(-1,1)，4=(0,-1)，5=(0,1)：x 方向二次、y 方向线性
void QuadraticLinearQuadShape(const double pc[3], double* sf) {
    double gx[3], dx[3];
    LagrangeG(pc[0], gx, dx);
    const double m0 = 0.5 * (1.0 - pc[1]); // y = -1
    const double m1 = 0.5 * (1.0 + pc[1]); // y = +1
    sf[0] = gx[0] * m0;
    sf[1] = gx[2] * m0;
    sf[4] = gx[1] * m0;
    sf[3] = gx[0] * m1;
    sf[2] = gx[2] * m1;
    sf[5] = gx[1] * m1;
}

void QuadraticLinearQuadDerivs(const double pc[3], double* d) {
    double gx[3], dx[3];
    LagrangeG(pc[0], gx, dx);
    const double m0 = 0.5 * (1.0 - pc[1]);
    const double m1 = 0.5 * (1.0 + pc[1]);
    const double dm0 = -0.5, dm1 = 0.5;
    d[0 * 6 + 0] = dx[0] * m0;
    d[0 * 6 + 1] = dx[2] * m0;
    d[0 * 6 + 4] = dx[1] * m0;
    d[0 * 6 + 3] = dx[0] * m1;
    d[0 * 6 + 2] = dx[2] * m1;
    d[0 * 6 + 5] = dx[1] * m1;
    d[1 * 6 + 0] = gx[0] * dm0;
    d[1 * 6 + 1] = gx[2] * dm0;
    d[1 * 6 + 4] = gx[1] * dm0;
    d[1 * 6 + 3] = gx[0] * dm1;
    d[1 * 6 + 2] = gx[2] * dm1;
    d[1 * 6 + 5] = gx[1] * dm1;
}

// 12 节点「二次-线性」楔形：VTK vtkQuadraticLinearWedge（形函数逐行照抄 VTK，
// 仅去掉 VTK 末尾对 [0,1] 参数的 *2 缩放，因为此处直接使用 [-1,1]）
void QuadraticLinearWedgeShape(const double pc[3], double* sf) {
    const double x = pc[0], y = pc[1], z = pc[2];
    // 角点（两个二次三角形）
    sf[0] = (x + y) * 0.5 * (x + y + 1.0) * (1 - z) * 0.5;
    sf[1] = x * (x + 1.0) * 0.5 * (1 - z) * 0.5;
    sf[2] = y * (1.0 + y) * 0.5 * (1 - z) * 0.5;
    sf[3] = (x + y) * 0.5 * (x + y + 1.0) * (1 + z) * 0.5;
    sf[4] = x * (x + 1.0) * 0.5 * (1 + z) * 0.5;
    sf[5] = y * (1.0 + y) * 0.5 * (1 + z) * 0.5;
    // 三角形棱中点
    sf[6] = -(x + 1) * (x + y) * (1 - z) * 0.5;
    sf[7] = (x + 1) * (y + 1) * (1 - z) * 0.5;
    sf[8] = -(y + 1) * (x + y) * (1 - z) * 0.5;
    sf[9] = -(x + 1) * (x + y) * (1 + z) * 0.5;
    sf[10] = (x + 1) * (y + 1) * (1 + z) * 0.5;
    sf[11] = -(y + 1) * (x + y) * (1 + z) * 0.5;
}

void QuadraticLinearWedgeDerivs(const double pc[3], double* d) {
    const double x = pc[0], y = pc[1], z = pc[2];
    // d/dx（对应 VTK InterpolationDerivs 的 x 分块，去掉 *2）
    d[0 * 12 + 0] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 - z) * 0.5;
    d[0 * 12 + 1] = (1.0 + 2.0 * x) * 0.5 * (1.0 - z) * 0.5;
    d[0 * 12 + 2] = 0.0;
    d[0 * 12 + 3] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 + z) * 0.5;
    d[0 * 12 + 4] = (1.0 + 2.0 * x) * 0.5 * (1.0 + z) * 0.5;
    d[0 * 12 + 5] = 0.0;
    d[0 * 12 + 6] = -(2.0 * x + y + 1.0) * (1.0 - z) * 0.5;
    d[0 * 12 + 7] = (y + 1.0) * (1.0 - z) * 0.5;
    d[0 * 12 + 8] = -(y + 1.0) * (1.0 - z) * 0.5;
    d[0 * 12 + 9] = -(2.0 * x + y + 1.0) * (1.0 + z) * 0.5;
    d[0 * 12 + 10] = (y + 1.0) * (1.0 + z) * 0.5;
    d[0 * 12 + 11] = -(y + 1.0) * (1.0 + z) * 0.5;
    // d/dy
    d[1 * 12 + 0] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 - z) * 0.5;
    d[1 * 12 + 1] = 0.0;
    d[1 * 12 + 2] = (1.0 + 2.0 * y) * 0.5 * (1.0 - z) * 0.5;
    d[1 * 12 + 3] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 + z) * 0.5;
    d[1 * 12 + 4] = 0.0;
    d[1 * 12 + 5] = (1.0 + 2.0 * y) * 0.5 * (1.0 + z) * 0.5;
    d[1 * 12 + 6] = -(x + 1.0) * (1.0 - z) * 0.5;
    d[1 * 12 + 7] = (x + 1.0) * (1.0 - z) * 0.5;
    d[1 * 12 + 8] = -(x + 2.0 * y + 1.0) * (1.0 - z) * 0.5;
    d[1 * 12 + 9] = -(x + 1.0) * (1.0 + z) * 0.5;
    d[1 * 12 + 10] = (x + 1.0) * (1.0 + z) * 0.5;
    d[1 * 12 + 11] = -(x + 2.0 * y + 1.0) * (1.0 + z) * 0.5;
    // d/dz
    d[2 * 12 + 0] = (x + y) * 0.5 * (x + y + 1.0) * -0.5;
    d[2 * 12 + 1] = x * (x + 1.0) * 0.5 * -0.5;
    d[2 * 12 + 2] = y * (1.0 + y) * 0.5 * -0.5;
    d[2 * 12 + 3] = (x + y) * 0.5 * (x + y + 1.0) * 0.5;
    d[2 * 12 + 4] = x * (x + 1.0) * 0.5 * 0.5;
    d[2 * 12 + 5] = y * (1.0 + y) * 0.5 * 0.5;
    d[2 * 12 + 6] = -(x + 1.0) * (x + y) * -0.5;
    d[2 * 12 + 7] = (x + 1.0) * (y + 1.0) * -0.5;
    d[2 * 12 + 8] = -(y + 1.0) * (x + y) * -0.5;
    d[2 * 12 + 9] = -(x + 1.0) * (x + y) * 0.5;
    d[2 * 12 + 10] = (x + 1.0) * (y + 1.0) * 0.5;
    d[2 * 12 + 11] = -(y + 1.0) * (x + y) * 0.5;
}

// 27 节点三二次六面体：VTK vtkTriQuadraticHexahedron（张量积二次 Lagrange）
// 节点序 0-7 角点、8-19 棱中点、20-25 面心、26 体心（VTK 源码中的赋值顺序照抄）
void TriQuadraticHexShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1], t = pc[2];
    double g1r = -0.5 * r * (1 - r), g1s = -0.5 * s * (1 - s), g1t = -0.5 * t * (1 - t);
    double g2r = (1 + r) * (1 - r), g2s = (1 + s) * (1 - s), g2t = (1 + t) * (1 - t);
    double g3r = 0.5 * r * (1 + r), g3s = 0.5 * s * (1 + s), g3t = 0.5 * t * (1 + t);

    // 八个角点
    sf[0] = g1r * g1s * g1t;
    sf[1] = g3r * g1s * g1t;
    sf[2] = g3r * g3s * g1t;
    sf[3] = g1r * g3s * g1t;
    sf[4] = g1r * g1s * g3t;
    sf[5] = g3r * g1s * g3t;
    sf[6] = g3r * g3s * g3t;
    sf[7] = g1r * g3s * g3t;
    // 棱中点
    sf[8] = g2r * g1s * g1t;
    sf[9] = g3r * g2s * g1t;
    sf[10] = g2r * g3s * g1t;
    sf[11] = g1r * g2s * g1t;
    sf[12] = g2r * g1s * g3t;
    sf[13] = g3r * g2s * g3t;
    sf[14] = g2r * g3s * g3t;
    sf[15] = g1r * g2s * g3t;
    sf[16] = g1r * g1s * g2t;
    sf[17] = g3r * g1s * g2t;
    sf[18] = g3r * g3s * g2t;
    sf[19] = g1r * g3s * g2t;
    // 面心（此处的赋值顺序与 VTK 源码一致）
    sf[22] = g2r * g1s * g2t;
    sf[21] = g3r * g2s * g2t;
    sf[23] = g2r * g3s * g2t;
    sf[20] = g1r * g2s * g2t;
    sf[24] = g2r * g2s * g1t;
    sf[25] = g2r * g2s * g3t;
    // 体心
    sf[26] = g2r * g2s * g2t;
}

void TriQuadraticHexDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1], t = pc[2];
    double g1r = -0.5 * r * (1 - r), g1s = -0.5 * s * (1 - s), g1t = -0.5 * t * (1 - t);
    double g2r = (1 + r) * (1 - r), g2s = (1 + s) * (1 - s), g2t = (1 + t) * (1 - t);
    double g3r = 0.5 * r * (1 + r), g3s = 0.5 * s * (1 + s), g3t = 0.5 * t * (1 + t);
    const double g1r_r = r - 0.5, g1s_s = s - 0.5, g1t_t = t - 0.5;
    const double g2r_r = -2 * r, g2s_s = -2 * s, g2t_t = -2 * t;
    const double g3r_r = r + 0.5, g3s_s = s + 0.5, g3t_t = t + 0.5;

    // d/dr
    d[0 * 27 + 0] = g1r_r * g1s * g1t;
    d[0 * 27 + 1] = g3r_r * g1s * g1t;
    d[0 * 27 + 2] = g3r_r * g3s * g1t;
    d[0 * 27 + 3] = g1r_r * g3s * g1t;
    d[0 * 27 + 4] = g1r_r * g1s * g3t;
    d[0 * 27 + 5] = g3r_r * g1s * g3t;
    d[0 * 27 + 6] = g3r_r * g3s * g3t;
    d[0 * 27 + 7] = g1r_r * g3s * g3t;
    d[0 * 27 + 8] = g2r_r * g1s * g1t;
    d[0 * 27 + 9] = g3r_r * g2s * g1t;
    d[0 * 27 + 10] = g2r_r * g3s * g1t;
    d[0 * 27 + 11] = g1r_r * g2s * g1t;
    d[0 * 27 + 12] = g2r_r * g1s * g3t;
    d[0 * 27 + 13] = g3r_r * g2s * g3t;
    d[0 * 27 + 14] = g2r_r * g3s * g3t;
    d[0 * 27 + 15] = g1r_r * g2s * g3t;
    d[0 * 27 + 16] = g1r_r * g1s * g2t;
    d[0 * 27 + 17] = g3r_r * g1s * g2t;
    d[0 * 27 + 18] = g3r_r * g3s * g2t;
    d[0 * 27 + 19] = g1r_r * g3s * g2t;
    d[0 * 27 + 20] = g1r_r * g2s * g2t;
    d[0 * 27 + 21] = g3r_r * g2s * g2t;
    d[0 * 27 + 22] = g2r_r * g1s * g2t;
    d[0 * 27 + 23] = g2r_r * g3s * g2t;
    d[0 * 27 + 24] = g2r_r * g2s * g1t;
    d[0 * 27 + 25] = g2r_r * g2s * g3t;
    d[0 * 27 + 26] = g2r_r * g2s * g2t;

    // d/ds
    d[1 * 27 + 0] = g1r * g1s_s * g1t;
    d[1 * 27 + 1] = g3r * g1s_s * g1t;
    d[1 * 27 + 2] = g3r * g3s_s * g1t;
    d[1 * 27 + 3] = g1r * g3s_s * g1t;
    d[1 * 27 + 4] = g1r * g1s_s * g3t;
    d[1 * 27 + 5] = g3r * g1s_s * g3t;
    d[1 * 27 + 6] = g3r * g3s_s * g3t;
    d[1 * 27 + 7] = g1r * g3s_s * g3t;
    d[1 * 27 + 8] = g2r * g1s_s * g1t;
    d[1 * 27 + 9] = g3r * g2s_s * g1t;
    d[1 * 27 + 10] = g2r * g3s_s * g1t;
    d[1 * 27 + 11] = g1r * g2s_s * g1t;
    d[1 * 27 + 12] = g2r * g1s_s * g3t;
    d[1 * 27 + 13] = g3r * g2s_s * g3t;
    d[1 * 27 + 14] = g2r * g3s_s * g3t;
    d[1 * 27 + 15] = g1r * g2s_s * g3t;
    d[1 * 27 + 16] = g1r * g1s_s * g2t;
    d[1 * 27 + 17] = g3r * g1s_s * g2t;
    d[1 * 27 + 18] = g3r * g3s_s * g2t;
    d[1 * 27 + 19] = g1r * g3s_s * g2t;
    d[1 * 27 + 20] = g1r * g2s_s * g2t;
    d[1 * 27 + 21] = g3r * g2s_s * g2t;
    d[1 * 27 + 22] = g2r * g1s_s * g2t;
    d[1 * 27 + 23] = g2r * g3s_s * g2t;
    d[1 * 27 + 24] = g2r * g2s_s * g1t;
    d[1 * 27 + 25] = g2r * g2s_s * g3t;
    d[1 * 27 + 26] = g2r * g2s_s * g2t;

    // d/dt
    d[2 * 27 + 0] = g1r * g1s * g1t_t;
    d[2 * 27 + 1] = g3r * g1s * g1t_t;
    d[2 * 27 + 2] = g3r * g3s * g1t_t;
    d[2 * 27 + 3] = g1r * g3s * g1t_t;
    d[2 * 27 + 4] = g1r * g1s * g3t_t;
    d[2 * 27 + 5] = g3r * g1s * g3t_t;
    d[2 * 27 + 6] = g3r * g3s * g3t_t;
    d[2 * 27 + 7] = g1r * g3s * g3t_t;
    d[2 * 27 + 8] = g2r * g1s * g1t_t;
    d[2 * 27 + 9] = g3r * g2s * g1t_t;
    d[2 * 27 + 10] = g2r * g3s * g1t_t;
    d[2 * 27 + 11] = g1r * g2s * g1t_t;
    d[2 * 27 + 12] = g2r * g1s * g3t_t;
    d[2 * 27 + 13] = g3r * g2s * g3t_t;
    d[2 * 27 + 14] = g2r * g3s * g3t_t;
    d[2 * 27 + 15] = g1r * g2s * g3t_t;
    d[2 * 27 + 16] = g1r * g1s * g2t_t;
    d[2 * 27 + 17] = g3r * g1s * g2t_t;
    d[2 * 27 + 18] = g3r * g3s * g2t_t;
    d[2 * 27 + 19] = g1r * g3s * g2t_t;
    d[2 * 27 + 20] = g1r * g2s * g2t_t;
    d[2 * 27 + 21] = g3r * g2s * g2t_t;
    d[2 * 27 + 22] = g2r * g1s * g2t_t;
    d[2 * 27 + 23] = g2r * g3s * g2t_t;
    d[2 * 27 + 24] = g2r * g2s * g1t_t;
    d[2 * 27 + 25] = g2r * g2s * g3t_t;
    d[2 * 27 + 26] = g2r * g2s * g2t_t;
}

/* ================================================================== */
/* 形函数统一入口                                                      */
/* ================================================================== */
int ShapePointCount(ShapeKind kind) {
    switch (kind) {
        case SHAPE_LINEAR_QUAD: return 4;
        case SHAPE_QUADRATIC_TRIANGLE: return 6;
        case SHAPE_QUADRATIC_QUAD: return 8;
        case SHAPE_TETRA: return 4;
        case SHAPE_QUADRATIC_TETRA: return 10;
        case SHAPE_QUADRATIC_HEX: return 20;
        case SHAPE_QUADRATIC_PRISM: return 15;
        case SHAPE_QUADRATIC_PYRAMID: return 13;
        case SHAPE_BIQUADRATIC_QUAD: return 9;
        case SHAPE_QUADRATIC_LINEAR_QUAD: return 6;
        case SHAPE_QUADRATIC_LINEAR_WEDGE: return 12;
        case SHAPE_TRIQUADRATIC_HEX: return 27;
        default: return 0;
    }
}

void ShapeFunctions(ShapeKind kind, const double pc[3], double* sf) {
    switch (kind) {
        case SHAPE_LINEAR_QUAD: LinearQuadShape(pc, sf); break;
        case SHAPE_QUADRATIC_TRIANGLE: QuadraticTriangleShape(pc, sf); break;
        case SHAPE_QUADRATIC_QUAD: QuadraticQuadShape(pc, sf); break;
        case SHAPE_QUADRATIC_TETRA: QuadraticTetraShape(pc, sf); break;
        case SHAPE_QUADRATIC_HEX: QuadraticHexShape(pc, sf); break;
        case SHAPE_QUADRATIC_PRISM: QuadraticPrism15Shape(pc, sf); break;
        case SHAPE_QUADRATIC_PYRAMID: QuadraticPyramid13Shape(pc, sf); break;
        case SHAPE_BIQUADRATIC_QUAD: BiQuadraticQuadShape(pc, sf); break;
        case SHAPE_QUADRATIC_LINEAR_QUAD: QuadraticLinearQuadShape(pc, sf); break;
        case SHAPE_QUADRATIC_LINEAR_WEDGE: QuadraticLinearWedgeShape(pc, sf); break;
        case SHAPE_TRIQUADRATIC_HEX: TriQuadraticHexShape(pc, sf); break;
        default: break;
    }
}

void ShapeDerivs(ShapeKind kind, const double pc[3], double* d) {
    switch (kind) {
        case SHAPE_LINEAR_QUAD: LinearQuadDerivs(pc, d); break;
        case SHAPE_QUADRATIC_TRIANGLE: QuadraticTriangleDerivs(pc, d); break;
        case SHAPE_QUADRATIC_QUAD: QuadraticQuadDerivs(pc, d); break;
        case SHAPE_QUADRATIC_TETRA: QuadraticTetraDerivs(pc, d); break;
        case SHAPE_QUADRATIC_HEX: QuadraticHexDerivs(pc, d); break;
        case SHAPE_QUADRATIC_PRISM: QuadraticPrism15Derivs(pc, d); break;
        case SHAPE_QUADRATIC_PYRAMID: QuadraticPyramid13Derivs(pc, d); break;
        case SHAPE_BIQUADRATIC_QUAD: BiQuadraticQuadDerivs(pc, d); break;
        case SHAPE_QUADRATIC_LINEAR_QUAD: QuadraticLinearQuadDerivs(pc, d); break;
        case SHAPE_QUADRATIC_LINEAR_WEDGE: QuadraticLinearWedgeDerivs(pc, d); break;
        case SHAPE_TRIQUADRATIC_HEX: TriQuadraticHexDerivs(pc, d); break;
        default: break;
    }
}

void ShapeInitParams(ShapeKind kind, double pc[3]) {
    pc[0] = pc[1] = pc[2] = 0.0;
    if (kind == SHAPE_QUADRATIC_TETRA) {
        pc[0] = pc[1] = pc[2] = 0.25;
    } else if (kind == SHAPE_QUADRATIC_TRIANGLE) {
        pc[0] = pc[1] = 1.0 / 3.0;
    } else if (kind == SHAPE_LINEAR_QUAD || kind == SHAPE_QUADRATIC_QUAD) {
        pc[0] = pc[1] = 0.5;
    } else if (kind == SHAPE_QUADRATIC_PRISM) {
        pc[0] = pc[1] = 1.0 / 3.0;
        pc[2] = 0.5;
    } else if (kind == SHAPE_QUADRATIC_PYRAMID) {
        pc[0] = pc[1] = 0.5;
        pc[2] = 0.5;
    }
    // SHAPE_QUADRATIC_HEX 中心为 (0,0,0)
}

/** 参数坐标是否落在单元自然域内 */
bool ShapeDomainOk(ShapeKind kind, const double pc[3], double tol) {
    switch (kind) {
        case SHAPE_LINEAR_QUAD:
        case SHAPE_QUADRATIC_QUAD:
            return pc[0] >= -tol && pc[0] <= 1.0 + tol && pc[1] >= -tol && pc[1] <= 1.0 + tol;
        case SHAPE_QUADRATIC_PRISM:
            return pc[0] >= -tol && pc[1] >= -tol && (pc[0] + pc[1]) <= 1.0 + tol && pc[2] >= -tol &&
                   pc[2] <= 1.0 + tol;
        case SHAPE_QUADRATIC_PYRAMID:
            return pc[0] >= -tol && pc[0] <= 1.0 + tol && pc[1] >= -tol && pc[1] <= 1.0 + tol && pc[2] >= -tol &&
                   pc[2] <= 1.0 + tol;
        case SHAPE_QUADRATIC_TRIANGLE:
            return pc[0] >= -tol && pc[1] >= -tol && (pc[0] + pc[1]) <= 1.0 + tol;
        case SHAPE_QUADRATIC_TETRA: {
            const double L0 = 1.0 - pc[0] - pc[1] - pc[2];
            return L0 >= -tol && pc[0] >= -tol && pc[1] >= -tol && pc[2] >= -tol && L0 <= 1.0 + tol;
        }
        case SHAPE_QUADRATIC_HEX:
            return pc[0] >= -1.0 - tol && pc[0] <= 1.0 + tol && pc[1] >= -1.0 - tol && pc[1] <= 1.0 + tol &&
                   pc[2] >= -1.0 - tol && pc[2] <= 1.0 + tol;
        // 以下 kind 的参数坐标直接使用 VTK 内部的 [-1,1] 形式
        case SHAPE_BIQUADRATIC_QUAD:
        case SHAPE_QUADRATIC_LINEAR_QUAD:
            return pc[0] >= -1.0 - tol && pc[0] <= 1.0 + tol && pc[1] >= -1.0 - tol && pc[1] <= 1.0 + tol;
        case SHAPE_TRIQUADRATIC_HEX:
        case SHAPE_QUADRATIC_LINEAR_WEDGE:
            return pc[0] >= -1.0 - tol && pc[0] <= 1.0 + tol && pc[1] >= -1.0 - tol && pc[1] <= 1.0 + tol &&
                   pc[2] >= -1.0 - tol && pc[2] <= 1.0 + tol;
        default:
            return false;
    }
}

/** 二次 3D 单元的 Newton 逆映射（参数坐标 → 权重 + 残差） */
bool NewtonQuadratic3D(ShapeKind kind, const std::vector<Point>& pts, const Point& target, double pc[3],
                       std::vector<double>& weights, double& residual) {
    const int npts = ShapePointCount(kind);
    if (npts <= 0 || static_cast<int>(pts.size()) < npts) { return false; }

    ShapeInitParams(kind, pc);

    double sf[kMaxCellPointNum];
    double dN[3 * kMaxCellPointNum];

    for (int iter = 0; iter < 32; ++iter) {
        ShapeFunctions(kind, pc, sf);
        ShapeDerivs(kind, pc, dN);

        double rcol[3] = {0.0, 0.0, 0.0};
        double scol[3] = {0.0, 0.0, 0.0};
        double tcol[3] = {0.0, 0.0, 0.0};
        double fcol[3] = {0.0, 0.0, 0.0};
        for (int i = 0; i < npts; ++i) {
            const Point& x = pts[static_cast<size_t>(i)];
            for (int j = 0; j < 3; ++j) {
                const double xv = static_cast<double>(x[j]);
                fcol[j] += sf[i] * xv;
                rcol[j] += dN[i] * xv;
                scol[j] += dN[i + npts] * xv;
                tcol[j] += dN[i + 2 * npts] * xv;
            }
        }
        for (int j = 0; j < 3; ++j) { fcol[j] -= static_cast<double>(target[j]); }

        const double detJ = Det3(rcol, scol, tcol);
        if (std::fabs(detJ) < 1e-30) { break; }

        const double negF[3] = {-fcol[0], -fcol[1], -fcol[2]};
        const double dp0 = Det3(negF, scol, tcol) / detJ;
        const double dp1 = Det3(rcol, negF, tcol) / detJ;
        const double dp2 = Det3(rcol, scol, negF) / detJ;

        pc[0] += dp0;
        pc[1] += dp1;
        pc[2] += dp2;

        if (std::fabs(dp0) + std::fabs(dp1) + std::fabs(dp2) < 1e-11) { break; }
        if (std::fabs(pc[0]) > 1e3 || std::fabs(pc[1]) > 1e3 || std::fabs(pc[2]) > 1e3) { return false; }
    }

    ShapeFunctions(kind, pc, sf);
    weights.assign(sf, sf + npts);

    Point x(0.f, 0.f, 0.f);
    for (int i = 0; i < npts; ++i) { x += pts[static_cast<size_t>(i)] * static_cast<float>(sf[i]); }
    residual = static_cast<double>((x - target).length());
    return true;
}

/** 面单元（线性四边形 / 二次四边形 / 二次三角形）的 2D Newton（投影到单元平面） */
bool NewtonParametric2D(ShapeKind kind, const std::vector<Point>& pts, const Point& p, double pc[2],
                        std::vector<double>& weights, double& residual) {
    const int npts = ShapePointCount(kind);
    if (npts <= 0 || static_cast<int>(pts.size()) < npts) { return false; }

    // 用角点构造局部正交基
    const int lastCorner = IsQuadKind(kind) ? 3 : 2;
    Vector3f n = CrossProduct(pts[lastCorner] - pts[0], pts[1] - pts[0]);
    const double nl = n.length();
    if (nl < 1e-20) { return false; }
    n = n / static_cast<float>(nl);

    Vector3f u = pts[1] - pts[0];
    u = u - n * DotProduct(u, n);
    double ul = u.length();
    if (ul < 1e-20) { return false; }
    u = u / static_cast<float>(ul);
    const Vector3f v = CrossProduct(n, u);

    std::vector<double> qx(static_cast<size_t>(npts)), qy(static_cast<size_t>(npts));
    for (int i = 0; i < npts; ++i) {
        const Vector3f diff = pts[static_cast<size_t>(i)] - pts[0];
        qx[i] = static_cast<double>(DotProduct(diff, u));
        qy[i] = static_cast<double>(DotProduct(diff, v));
    }
    const Vector3f pd = p - pts[0];
    const double tx = static_cast<double>(DotProduct(pd, u));
    const double ty = static_cast<double>(DotProduct(pd, v));

    double p3[3] = {0.0, 0.0, 0.0};
    ShapeInitParams(kind, p3);

    double sf[kMaxCellPointNum];
    double dN[3 * kMaxCellPointNum];

    for (int iter = 0; iter < 32; ++iter) {
        ShapeFunctions(kind, p3, sf);
        ShapeDerivs(kind, p3, dN);

        double F[2] = {0.0, 0.0};
        double J[2][2] = {{0.0, 0.0}, {0.0, 0.0}};
        for (int i = 0; i < npts; ++i) {
            F[0] += sf[i] * qx[i];
            F[1] += sf[i] * qy[i];
            J[0][0] += dN[i] * qx[i];
            J[1][0] += dN[i] * qy[i];
            J[0][1] += dN[i + npts] * qx[i];
            J[1][1] += dN[i + npts] * qy[i];
        }
        F[0] -= tx;
        F[1] -= ty;

        const double detJ = J[0][0] * J[1][1] - J[0][1] * J[1][0];
        if (std::fabs(detJ) < 1e-30) { break; }

        const double dr = (-F[0] * J[1][1] + F[1] * J[0][1]) / detJ;
        const double ds = (-F[1] * J[0][0] + F[0] * J[1][0]) / detJ;
        p3[0] += dr;
        p3[1] += ds;

        if (std::fabs(dr) + std::fabs(ds) < 1e-12) { break; }
        if (p3[0] < -4.0 || p3[0] > 5.0 || p3[1] < -4.0 || p3[1] > 5.0) { break; }
    }

    pc[0] = p3[0];
    pc[1] = p3[1];

    ShapeFunctions(kind, p3, sf);
    weights.assign(sf, sf + npts);

    Point x(0.f, 0.f, 0.f);
    for (int i = 0; i < npts; ++i) { x += pts[static_cast<size_t>(i)] * static_cast<float>(sf[i]); }
    residual = static_cast<double>((x - p).length());
    return true;
}

/* ================================================================== */
/* 均值坐标（Mean Value Coordinates）                                  */
/* ================================================================== */

/**
 * 单个多边形面对采样点的均值坐标贡献。
 * 算法与 iGameStreamTracer / VTK vtkMeanValueCoordinatesInterpolator 一致：
 * 把面的顶点方向投影到单位球面，再用球面多边形的角度权重求和。
 */
bool MeanValueFaceWeights(const std::vector<Point>& face, const Point& x, std::vector<double>& w) {
    const int n = static_cast<int>(face.size());
    if (n < 3) { return false; }
    w.assign(static_cast<size_t>(n), 0.0);

    const double eps = 1e-14;
    std::vector<double> dist(static_cast<size_t>(n), 0.0);
    std::vector<Vector3f> u(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i) {
        const Vector3f d = face[static_cast<size_t>(i)] - x;
        const double l = d.length();
        if (l < 1e-12) { // 采样点与该面顶点重合
            w.assign(static_cast<size_t>(n), 0.0);
            w[static_cast<size_t>(i)] = 1.0;
            return true;
        }
        dist[i] = l;
        u[i] = d / static_cast<float>(l);
    }

    // 球面多边形的「面积向量」
    Vector3f vsum(0.f, 0.f, 0.f);
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        Vector3f cross = CrossProduct(u[i], u[j]);
        const double cl = cross.length();
        if (cl > eps) { cross = cross / static_cast<float>(cl); }
        const double chord = static_cast<double>((u[j] - u[i]).length());
        const double angle = 2.0 * std::asin(std::min(1.0, chord * 0.5));
        vsum += cross * static_cast<float>(0.5 * angle);
    }
    const double vNorm = vsum.length();
    if (vNorm < eps) { return false; }

    Vector3f v = vsum / static_cast<float>(vNorm);
    if (DotProduct(v, u[0]) < 0.f) { v = -v; }

    std::vector<double> alpha(static_cast<size_t>(n), 0.0);
    std::vector<double> theta(static_cast<size_t>(n), 0.0);
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        Vector3f n0 = CrossProduct(u[i], v);
        Vector3f n1 = CrossProduct(u[j], v);
        const double l0 = n0.length();
        const double l1 = n1.length();
        if (l0 > eps) { n0 = n0 / static_cast<float>(l0); }
        if (l1 > eps) { n1 = n1 / static_cast<float>(l1); }
        const double chord = static_cast<double>((n0 - n1).length());
        alpha[i] = 2.0 * std::asin(std::min(1.0, chord * 0.5));
        if (DotProduct(CrossProduct(n0, n1), v) < 0.f) { alpha[i] = -alpha[i]; }

        const double chordU = static_cast<double>((u[i] - v).length());
        theta[i] = 2.0 * std::asin(std::min(1.0, chordU * 0.5));
    }

    // 退化情形 1：采样点落在该面所在平面内
    for (int i = 0; i < n; ++i) {
        if (std::fabs(theta[i]) < 1e-12) {
            w[i] = vNorm / dist[i];
            return true;
        }
    }

    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        const int prev = (i - 1 + n) % n;
        const double ti = std::tan(theta[i]);
        if (std::fabs(ti) < 1e-12) { return false; }
        sum += (1.0 / ti) * (std::tan(alpha[i] * 0.5) + std::tan(alpha[prev] * 0.5));
    }

    if (std::fabs(sum) < eps) {
        // 退化情形 2：改用与球面角度相关的备选权重
        std::vector<double> tmp(static_cast<size_t>(n), 0.0);
        double sumW = 0.0;
        for (int i = 0; i < n; ++i) {
            const int prev = (i - 1 + n) % n;
            tmp[i] = (std::tan(theta[i] * 0.5) + std::tan(theta[prev] * 0.5)) / dist[i];
            sumW += tmp[i];
        }
        if (std::fabs(sumW) < eps) { return false; }
        for (int i = 0; i < n; ++i) { w[i] = tmp[i] / sumW; }
        return true;
    }

    for (int i = 0; i < n; ++i) {
        const int prev = (i - 1 + n) % n;
        const double st = std::sin(theta[i]);
        if (std::fabs(st) < 1e-12) { return false; }
        w[i] = vNorm / sum / dist[i] / st * (std::tan(alpha[i] * 0.5) + std::tan(alpha[prev] * 0.5));
    }
    return true;
}

/* ================================================================== */
/* 基础几何工具                                                        */
/* ================================================================== */

/** 点到线段的最近点（返回距离平方） */
void ClosestPointOnSegment(const Point& p, const Point& a, const Point& b, Point& closest, double& distSq) {
    const Vector3f ab = b - a;
    const double denom = static_cast<double>(DotProduct(ab, ab));
    if (denom < 1e-24) {
        closest = a;
        distSq = static_cast<double>((p - a).squaredLength());
        return;
    }
    double t = static_cast<double>(DotProduct(p - a, ab)) / denom;
    t = std::min(1.0, std::max(0.0, t));
    closest = a + ab * static_cast<float>(t);
    distSq = static_cast<double>((closest - p).squaredLength());
}

/** 点到三角形的最近点（返回距离平方） */
void ClosestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c, Point& closest,
                            double& distSq) {
    const Vector3f ab = b - a;
    const Vector3f ac = c - a;
    const Vector3f ap = p - a;

    const double d1 = static_cast<double>(DotProduct(ab, ap));
    const double d2 = static_cast<double>(DotProduct(ac, ap));
    if (d1 <= 0.0 && d2 <= 0.0) {
        closest = a;
        distSq = static_cast<double>(DotProduct(ap, ap));
        return;
    }

    const Vector3f bp = p - b;
    const double d3 = static_cast<double>(DotProduct(ab, bp));
    const double d4 = static_cast<double>(DotProduct(ac, bp));
    if (d3 >= 0.0 && d4 <= d3) {
        closest = b;
        distSq = static_cast<double>(DotProduct(bp, bp));
        return;
    }

    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
        const double t = (d1 - d3) != 0.0 ? d1 / (d1 - d3) : 0.0;
        closest = a + ab * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    const Vector3f cp = p - c;
    const double d5 = static_cast<double>(DotProduct(ab, cp));
    const double d6 = static_cast<double>(DotProduct(ac, cp));
    if (d6 >= 0.0 && d5 <= d6) {
        closest = c;
        distSq = static_cast<double>(DotProduct(cp, cp));
        return;
    }

    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
        const double t = (d2 - d6) != 0.0 ? d2 / (d2 - d6) : 0.0;
        closest = a + ac * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
        const double denom = (d4 - d3) + (d5 - d6);
        const double t = denom != 0.0 ? (d4 - d3) / denom : 0.0;
        closest = b + (c - b) * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    const Vector3f n = CrossProduct(ab, ac);
    const double denom = static_cast<double>(DotProduct(n, n));
    if (denom > 1e-24) {
        const double t = static_cast<double>(DotProduct(n, ap)) / denom;
        closest = p - n * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    closest = a;
    double minDistSq = static_cast<double>(DotProduct(ap, ap));
    const double distB = static_cast<double>(DotProduct(bp, bp));
    if (distB < minDistSq) {
        closest = b;
        minDistSq = distB;
    }
    const double distC = static_cast<double>(DotProduct(cp, cp));
    if (distC < minDistSq) {
        closest = c;
        minDistSq = distC;
    }
    distSq = minDistSq;
}

/** 三角形重心坐标（按投影点计算），同时返回点到三角面的距离 */
bool TriangleBarycentric(const Point& p, const Point& a, const Point& b, const Point& c, double w[3],
                         double& planeDist) {
    const Vector3f n = CrossProduct(b - a, c - a);
    const double nn = static_cast<double>(DotProduct(n, n));
    if (nn < 1e-24) { return false; }

    const Vector3f ap = p - a;
    const double t = static_cast<double>(DotProduct(n, ap)) / nn;
    planeDist = std::fabs(t) * std::sqrt(nn);
    const Vector3f proj = p - n * static_cast<float>(t);

    const Vector3f v0 = b - a;
    const Vector3f v1 = c - a;
    const Vector3f v2 = proj - a;
    const double d00 = static_cast<double>(DotProduct(v0, v0));
    const double d01 = static_cast<double>(DotProduct(v0, v1));
    const double d11 = static_cast<double>(DotProduct(v1, v1));
    const double d20 = static_cast<double>(DotProduct(v2, v0));
    const double d21 = static_cast<double>(DotProduct(v2, v1));
    const double denom = d00 * d11 - d01 * d01;
    if (std::fabs(denom) < 1e-24) { return false; }

    const double l1 = (d11 * d20 - d01 * d21) / denom;
    const double l2 = (d00 * d21 - d01 * d20) / denom;
    w[0] = 1.0 - l1 - l2;
    w[1] = l1;
    w[2] = l2;
    return true;
}

/** 权重规整：去掉微小负值并归一化（线性 / 二次插值用） */
void NormalizeWeights(std::vector<double>& w) {
    double sum = 0.0;
    for (double& v : w) {
        if (v < 0.0) { v = 0.0; }
        sum += v;
    }
    if (sum <= 1e-30) {
        const double v = w.empty() ? 0.0 : 1.0 / static_cast<double>(w.size());
        for (double& x : w) { x = v; }
        return;
    }
    for (double& v : w) { v /= sum; }
}

/** 权重仅归一化（均值坐标允许出现负权重） */
bool NormalizeKeepSign(std::vector<double>& w) {
    double sum = 0.0;
    for (const double v : w) { sum += v; }
    if (std::fabs(sum) <= 1e-30) { return false; }
    for (double& v : w) { v /= sum; }
    return true;
}

/** 按输入数组类型创建同类型的输出数组（保留数组数据类型） */
ArrayObject::Pointer CreateArrayObject(IGenum arrayType) {
    switch (arrayType) {
        case IG_FloatArray:
            return FloatArray::New();
        case IG_DoubleArray:
            return DoubleArray::New();
        case IG_IntArray:
            return IntArray::New();
        case IG_UnsignedIntArray:
            return UnsignedIntArray::New();
        case IG_CharArray:
            return CharArray::New();
        case IG_UnsignedCharArray:
            return UnsignedCharArray::New();
        case IG_ShortArray:
            return ShortArray::New();
        case IG_UnsignedShortArray:
            return UnsignedShortArray::New();
        case IG_LongLongArray:
            return LongLongArray::New();
        case IG_UnsignedLongLongArray:
            return UnsignedLongLongArray::New();
        default:
            return FloatArray::New();
    }
}

/** 广义绕数（Generalized Winding Number）：判断点是否在闭合三角面片边界内部 */
double WindingNumber(const std::vector<Point>& triA, const std::vector<Point>& triB, const std::vector<Point>& triC,
                     const Point& p) {
    double total = 0.0;
    const size_t n = triA.size();
    for (size_t t = 0; t < n; ++t) {
        const double A[3] = {triA[t][0] - p[0], triA[t][1] - p[1], triA[t][2] - p[2]};
        const double B[3] = {triB[t][0] - p[0], triB[t][1] - p[1], triB[t][2] - p[2]};
        const double C[3] = {triC[t][0] - p[0], triC[t][1] - p[1], triC[t][2] - p[2]};
        const double la = std::sqrt(A[0] * A[0] + A[1] * A[1] + A[2] * A[2]);
        const double lb = std::sqrt(B[0] * B[0] + B[1] * B[1] + B[2] * B[2]);
        const double lc = std::sqrt(C[0] * C[0] + C[1] * C[1] + C[2] * C[2]);
        if (la < 1e-30 || lb < 1e-30 || lc < 1e-30) { continue; }

        const double num = A[0] * (B[1] * C[2] - B[2] * C[1]) - A[1] * (B[0] * C[2] - B[2] * C[0]) +
                           A[2] * (B[0] * C[1] - B[1] * C[0]);
        const double den = la * lb * lc + (A[0] * B[0] + A[1] * B[1] + A[2] * B[2]) * lc +
                           (B[0] * C[0] + B[1] * C[1] + B[2] * C[2]) * la +
                           (C[0] * A[0] + C[1] * A[1] + C[2] * A[2]) * lb;
        total += 2.0 * std::atan2(num, den);
    }
    return total / (4.0 * 3.14159265358979323846);
}

/* ================================================================== */
/* 体单元边界面工具（面 = pts 下标序列）                               */
/* ================================================================== */
using CellFaceList = std::vector<std::vector<int>>;

/**
 * 收集单元边界面。
 * globalIds == nullptr 时认为面的点 id 就是 pts 下标（角点线性单元用）；
 * 否则按网格全局 id 映射到 pts 下标（多面体可能重复出现同一顶点，取首个匹配）。
 */
void CollectCellFaces(Cell* cell, const igIndex* globalIds, int npts, CellFaceList& faces) {
    faces.clear();
    if (cell == nullptr) { return; }
    const int nfaces = cell->GetNumberOfFaces();
    if (nfaces <= 0) { return; }

    for (int f = 0; f < nfaces; ++f) {
        auto face = cell->GetFace(f);
        if (face == nullptr) { continue; }
        const int fn = face->GetNumberOfPoints();
        if (fn < 3) { continue; }

        std::vector<int> ids;
        ids.reserve(static_cast<size_t>(fn));
        for (int k = 0; k < fn; ++k) {
            const igIndex fid = face->GetPointId(k);
            int local = -1;
            if (globalIds == nullptr) {
                local = static_cast<int>(fid);
            } else {
                for (int i = 0; i < npts; ++i) {
                    if (globalIds[i] == fid) {
                        local = i;
                        break;
                    }
                }
            }
            if (local >= 0 && local < npts) { ids.push_back(local); }
        }
        if (ids.size() >= 3) { faces.push_back(std::move(ids)); }
    }
}

/** 边界面扇形三角化 */
void TriangulateFaces(const std::vector<Point>& pts, const CellFaceList& faces, std::vector<Point>& triA,
                      std::vector<Point>& triB, std::vector<Point>& triC) {
    triA.clear();
    triB.clear();
    triC.clear();
    for (const auto& face : faces) {
        if (face.size() < 3) { continue; }
        for (size_t k = 1; k + 1 < face.size(); ++k) {
            triA.push_back(pts[static_cast<size_t>(face[0])]);
            triB.push_back(pts[static_cast<size_t>(face[k])]);
            triC.push_back(pts[static_cast<size_t>(face[k + 1])]);
        }
    }
}

/** 点到单元边界的最小距离 */
double DistanceToFaces(const std::vector<Point>& pts, const CellFaceList& faces, const Point& p, Point& closest) {
    double best = DBL_MAX;
    closest = p;
    for (const auto& face : faces) {
        if (face.size() < 3) { continue; }
        for (size_t k = 1; k + 1 < face.size(); ++k) {
            Point c(0.f, 0.f, 0.f);
            double d2 = 0.0;
            ClosestPointOnTriangle(p, pts[static_cast<size_t>(face[0])], pts[static_cast<size_t>(face[k])],
                                   pts[static_cast<size_t>(face[k + 1])], c, d2);
            if (d2 < best) {
                best = d2;
                closest = c;
            }
        }
    }
    return (best == DBL_MAX) ? DBL_MAX : std::sqrt(best);
}

/** 闭合边界面片 → 点是否在体内（贴面 或 广义绕数 |w| > 0.5） */
bool ContainsByFaces(const std::vector<Point>& pts, const CellFaceList& faces, const Point& p, double distTol) {
    Point closest;
    if (DistanceToFaces(pts, faces, p, closest) <= distTol) { return true; }

    std::vector<Point> triA, triB, triC;
    TriangulateFaces(pts, faces, triA, triB, triC);
    if (triA.empty()) { return false; }
    return std::fabs(WindingNumber(triA, triB, triC, p)) > 0.5;
}

/** 逐面均值坐标（权重按 pts 下标返回，含归一化与线性精度校验） */
bool MeanValueWeightsFromFaces(const std::vector<Point>& pts, const CellFaceList& faces, const Point& p, double distTol,
                               std::vector<double>& w) {
    w.assign(pts.size(), 0.0);

    std::vector<double> faceWeights;
    std::vector<Point> facePts;
    bool any = false;
    for (const auto& face : faces) {
        if (face.size() < 3) { continue; }
        facePts.clear();
        for (const int idx : face) { facePts.push_back(pts[static_cast<size_t>(idx)]); }
        if (!MeanValueFaceWeights(facePts, p, faceWeights)) { continue; }
        for (size_t k = 0; k < face.size(); ++k) { w[static_cast<size_t>(face[k])] += faceWeights[k]; }
        any = true;
    }
    if (!any) { return false; }
    if (!NormalizeKeepSign(w)) { return false; }

    // 线性精度校验：均值坐标应能重建采样点位置，否则退化为距离反比权重
    Point rebuild(0.f, 0.f, 0.f);
    for (size_t i = 0; i < pts.size(); ++i) { rebuild += pts[i] * static_cast<float>(w[i]); }
    const double err = static_cast<double>((rebuild - p).length());
    double scale = 0.0;
    for (size_t i = 0; i < pts.size(); ++i) { scale = std::max(scale, static_cast<double>((pts[i] - p).length())); }
    if (err > std::max(distTol * 10.0, scale * 1e-3)) {
        for (size_t i = 0; i < pts.size(); ++i) {
            const double d2 = static_cast<double>((pts[i] - p).squaredLength());
            w[i] = 1.0 / (d2 + 1e-30);
        }
        if (!NormalizeKeepSign(w)) { return false; }
    }
    return true;
}

/** 由单元类型得到「角点线性体单元」的对象与角点数（用于高次体单元的退化处理） */
int LinearCornerVolumeCell(IGenum cellType, Cell::Pointer& cell) {
    switch (cellType) {
        case IG_QUADRATIC_TETRA:
        case IG_LAGRANGE_TETRAHEDRON:
            cell = Tetra::New();
            return 4;
        case IG_QUADRATIC_HEXAHEDRON:
        case IG_TRIQUADRATIC_HEXAHEDRON:
        case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        case IG_LAGRANGE_HEXAHEDRON:
            cell = Hexahedron::New();
            return 8;
        case IG_QUADRATIC_PRISM:
        case IG_QUADRATIC_LINEAR_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_WEDGE:
        case IG_LAGRANGE_PRISM:
            cell = Prism::New();
            return 6;
        case IG_QUADRATIC_PYRAMID:
        case IG_TRIQUADRATIC_PYRAMID:
        case IG_LAGRANGE_PYRAMID:
            cell = Pyramid::New();
            return 5;
        default:
            cell = nullptr;
            return 0;
    }
}

} // namespace

/* ------------------------------------------------------------------ */
/* 参数设置                                                            */
/* ------------------------------------------------------------------ */
void ResampleToLine::setOrigTarget(const Vector3d& p0, const Vector3d& p1, const int& x) {
    orig = Point(static_cast<float>(p0[0]), static_cast<float>(p0[1]), static_cast<float>(p0[2]));
    target = Point(static_cast<float>(p1[0]), static_cast<float>(p1[1]), static_cast<float>(p1[2]));
    n = x;
}

/* ------------------------------------------------------------------ */
/* 主流程                                                              */
/* ------------------------------------------------------------------ */
bool ResampleToLine::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "未选择输入数据";
        return false;
    }
    if (n < 2) {
        m_Message = "n must be greater than 1";
        return false;
    }

    // 优先使用 UnstructuredMesh；SurfaceMesh / VolumeMesh 先转换
    auto mesh = DynamicCast<UnstructuredMesh>(input);
    if (mesh == nullptr) { mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(input); }
    if (mesh == nullptr) {
        m_Message = "please input UnstructuredMesh / SurfaceMesh / VolumeMesh";
        return false;
    }
    if (mesh->GetNumberOfPoints() == 0 || mesh->GetNumberOfCells() == 0) {
        m_Message = "Mesh has no cells or points.";
        return false;
    }

    const BoundingBox& bbox = mesh->GetBoundingBox();
    const double diag = std::max(bbox.diag(), 1e-12);

    // 容差：自动（包围盒对角线 × 1e-6）或手动
    m_EffectiveTolerance = m_AutoTolerance ? (diag * kAutoToleranceRatio) : (m_Tolerance * diag);
    const double distTol = m_EffectiveTolerance;

    m_UnsupportedQuadraticCellCount = 0;

    // 1. 沿线均匀生成采样点
    Points::Pointer samples = Points::New();
    std::vector<SampleLocation> locations(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(n - 1);
        const Point p(static_cast<float>((1.0 - t) * orig[0] + t * target[0]),
                      static_cast<float>((1.0 - t) * orig[1] + t * target[1]),
                      static_cast<float>((1.0 - t) * orig[2] + t * target[2]));
        samples->AddPoint(p);
        locations[static_cast<size_t>(i)].point = p;
    }

    // 2. 均匀网格加速结构
    const double cellNum = static_cast<double>(mesh->GetNumberOfCells());
    igIndex gridDim = static_cast<igIndex>(std::cbrt(cellNum) + 0.5);
    gridDim = std::max<igIndex>(4, std::min<igIndex>(64, gridDim));
    const igIndex nx = gridDim, ny = gridDim, nz = gridDim;
    auto grid = BuildUniformGrid(mesh, bbox, nx, ny, nz);

    // 3. 逐采样点定位单元（不再吸附最近单元：不在任何单元内即标记无效）
    m_SampleCellIds.assign(static_cast<size_t>(n), -1);
    m_SampleValidMask.assign(static_cast<size_t>(n), 0);
    int validCount = 0;
    for (int i = 0; i < n; ++i) {
        SampleLocation loc;
        if (LocateSample(mesh, samples->GetPoint(i), bbox, grid, nx, ny, nz, distTol, loc) && loc.cellId >= 0) {
            locations[static_cast<size_t>(i)] = loc;
            m_SampleCellIds[static_cast<size_t>(i)] = loc.cellId;
            m_SampleValidMask[static_cast<size_t>(i)] = 1;
            ++validCount;
        }
    }

    // 4. 插值 Point Data / 复制 Cell Data / 有效点掩码
    AttributeSet* inAttr = mesh->GetAttributeSet();
    AttributeSet::Pointer outAttr = AttributeSet::New();
    InterpolatePointData(inAttr, outAttr, locations, n);
    CopyCellData(inAttr, outAttr, locations, n);
    AddValidPointMask(outAttr, n);

    // 5. 生成折线输出
    BuildPolyLineOutputs(samples, outAttr, n);

    m_Message = "ResampleToLine: samples=" + std::to_string(n) + ", valid=" + std::to_string(validCount) +
                ", invalid=" + std::to_string(n - validCount) +
                ", tolerance=" + std::to_string(m_EffectiveTolerance) +
                (m_AutoTolerance ? " (auto = bboxDiag*1e-6)" : " (manual)");
    if (m_UnsupportedQuadraticCellCount > 0) {
        m_Message += ", degraded high-order cells=" + std::to_string(m_UnsupportedQuadraticCellCount);
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* 均匀网格                                                            */
/* ------------------------------------------------------------------ */
std::vector<std::vector<igIndex>> ResampleToLine::BuildUniformGrid(const UnstructuredMesh::Pointer& mesh,
                                                                  const BoundingBox& bbox, igIndex nx, igIndex ny,
                                                                  igIndex nz) {
    double voxelX = (bbox.max[0] - bbox.min[0]) / static_cast<double>(nx);
    double voxelY = (bbox.max[1] - bbox.min[1]) / static_cast<double>(ny);
    double voxelZ = (bbox.max[2] - bbox.min[2]) / static_cast<double>(nz);
    if (voxelX < 1e-12) { voxelX = 1.0; }
    if (voxelY < 1e-12) { voxelY = 1.0; }
    if (voxelZ < 1e-12) { voxelZ = 1.0; }

    std::vector<std::vector<igIndex>> grid(static_cast<size_t>(nx) * ny * nz);

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    for (igIndex cellId = 0; cellId < static_cast<igIndex>(mesh->GetNumberOfCells()); ++cellId) {
        const int npts = mesh->GetCellPointIds(cellId, ptIds);
        if (npts <= 0) { continue; }

        double minv[3] = {DBL_MAX, DBL_MAX, DBL_MAX};
        double maxv[3] = {-DBL_MAX, -DBL_MAX, -DBL_MAX};
        for (int k = 0; k < npts; ++k) {
            const Point& p = mesh->GetPoint(ptIds[k]);
            for (int j = 0; j < 3; ++j) {
                const double v = static_cast<double>(p[j]);
                minv[j] = std::min(minv[j], v);
                maxv[j] = std::max(maxv[j], v);
            }
        }

        const igIndex ixMin = std::max<igIndex>(0, static_cast<igIndex>((minv[0] - bbox.min[0]) / voxelX));
        const igIndex iyMin = std::max<igIndex>(0, static_cast<igIndex>((minv[1] - bbox.min[1]) / voxelY));
        const igIndex izMin = std::max<igIndex>(0, static_cast<igIndex>((minv[2] - bbox.min[2]) / voxelZ));
        const igIndex ixMax = std::min<igIndex>(nx - 1, static_cast<igIndex>((maxv[0] - bbox.min[0]) / voxelX));
        const igIndex iyMax = std::min<igIndex>(ny - 1, static_cast<igIndex>((maxv[1] - bbox.min[1]) / voxelY));
        const igIndex izMax = std::min<igIndex>(nz - 1, static_cast<igIndex>((maxv[2] - bbox.min[2]) / voxelZ));

        for (igIndex ix = ixMin; ix <= ixMax; ++ix) {
            for (igIndex iy = iyMin; iy <= iyMax; ++iy) {
                for (igIndex iz = izMin; iz <= izMax; ++iz) {
                    grid[static_cast<size_t>(ix) + static_cast<size_t>(iy) * nx + static_cast<size_t>(iz) * nx * ny]
                            .push_back(cellId);
                }
            }
        }
    }

    return grid;
}

/* ------------------------------------------------------------------ */
/* 采样点定位                                                          */
/* ------------------------------------------------------------------ */
bool ResampleToLine::LocateSample(const UnstructuredMesh::Pointer& mesh, const Point& p, const BoundingBox& bbox,
                                  const std::vector<std::vector<igIndex>>& grid, igIndex nx, igIndex ny, igIndex nz,
                                  double distTol, SampleLocation& out) {
    double voxelX = (bbox.max[0] - bbox.min[0]) / static_cast<double>(nx);
    double voxelY = (bbox.max[1] - bbox.min[1]) / static_cast<double>(ny);
    double voxelZ = (bbox.max[2] - bbox.min[2]) / static_cast<double>(nz);
    if (voxelX < 1e-12) { voxelX = 1.0; }
    if (voxelY < 1e-12) { voxelY = 1.0; }
    if (voxelZ < 1e-12) { voxelZ = 1.0; }

    const igIndex ix = std::max<igIndex>(0, std::min<igIndex>(nx - 1, static_cast<igIndex>((p[0] - bbox.min[0]) / voxelX)));
    const igIndex iy = std::max<igIndex>(0, std::min<igIndex>(ny - 1, static_cast<igIndex>((p[1] - bbox.min[1]) / voxelY)));
    const igIndex iz = std::max<igIndex>(0, std::min<igIndex>(nz - 1, static_cast<igIndex>((p[2] - bbox.min[2]) / voxelZ)));

    std::vector<igIndex> candidates;
    const igIndex maxRadius = std::max<igIndex>(nx, std::max<igIndex>(ny, nz));
    for (igIndex r = 0; r <= maxRadius && candidates.empty(); ++r) {
        for (igIndex dx = -r; dx <= r; ++dx) {
            for (igIndex dy = -r; dy <= r; ++dy) {
                for (igIndex dz = -r; dz <= r; ++dz) {
                    if (std::max(std::abs(dx), std::max(std::abs(dy), std::abs(dz))) != r) { continue; }
                    const igIndex cx = ix + dx, cy = iy + dy, cz = iz + dz;
                    if (cx < 0 || cx >= nx || cy < 0 || cy >= ny || cz < 0 || cz >= nz) { continue; }
                    const auto& list = grid[static_cast<size_t>(cx) + static_cast<size_t>(cy) * nx +
                                            static_cast<size_t>(cz) * nx * ny];
                    candidates.insert(candidates.end(), list.begin(), list.end());
                }
            }
        }
    }
    if (candidates.empty()) { return false; }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

    // 只接受真正包含采样点的单元；不再在容差外寻找最近点
    for (const igIndex cellId : candidates) {
        std::vector<double> weights;
        if (!ComputeCellWeights(mesh, cellId, p, distTol, weights)) { continue; }
        if (weights.empty()) { continue; }

        igIndex ptIds[IGAME_CELL_MAX_SIZE];
        const int npts = mesh->GetCellPointIds(cellId, ptIds);

        out.cellId = cellId;
        out.point = p;
        out.pointIds.assign(ptIds, ptIds + npts);
        out.weights = std::move(weights);
        return true;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* 单元权重分发                                                        */
/* ------------------------------------------------------------------ */
bool ResampleToLine::ComputeCellWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                        double distTol, std::vector<double>& weights) {
    weights.clear();

    auto cells = mesh->GetCells();
    if (cells == nullptr) { return false; }
    const IGuint cellSize = cells->GetCellSize(cellId);
    if (cellSize == 0 || cellSize > static_cast<IGuint>(kMaxCellPointNum)) { return false; }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts <= 0) { return false; }

    const IGenum cellType = mesh->GetCellType(cellId);
    ShapeKind kind = ClassifyCell(cellType, npts);
    if (kind == SHAPE_NONE) {
        // 暂不支持的高次单元：退化处理（并在结果信息中提示）
        if (IsHighOrderCell(cellType)) { ++m_UnsupportedQuadraticCellCount; }
        if (Cell::GetCellDimension(static_cast<igIndex>(cellType)) == 3 && npts >= 4) {
            // 高次体单元：用其线性角点单元的面拓扑做均值坐标（含绕数包含判定）
            if (ComputeDegradedVolumeWeights(mesh, cellId, p, distTol, weights)) { return true; }
            return ComputeMeanValueWeights(mesh, cellId, p, distTol, weights);
        }
        const int corners = LinearCornerCount(cellType, npts);
        if (corners == 0) { return false; }
        return ComputeLinearFaceWeights(mesh, cellId, p, distTol, weights);
    }

    if (kind == SHAPE_LINEAR_TRIANGLE || kind == SHAPE_LINEAR_QUAD || kind == SHAPE_LINEAR_POLYGON) {
        return ComputeLinearFaceWeights(mesh, cellId, p, distTol, weights);
    }
    if (kind == SHAPE_TETRA || kind == SHAPE_MVC_VOLUME) {
        return ComputeMeanValueWeights(mesh, cellId, p, distTol, weights);
    }
    if (IsQuadraticKind(kind)) {
        return ComputeQuadraticWeights(mesh, cellId, p, distTol, weights);
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* 线性面单元：线性插值（重心坐标 / 双线性 / 扇形三角化）               */
/* ------------------------------------------------------------------ */
bool ResampleToLine::ComputeLinearFaceWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                              double distTol, std::vector<double>& weights) {
    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts < 3) { return false; }

    const IGenum cellType = mesh->GetCellType(cellId);
    const ShapeKind kind = ClassifyCell(cellType, npts);
    int used = npts;
    if (kind == SHAPE_LINEAR_TRIANGLE || cellType == IG_QUADRATIC_TRIANGLE || cellType == IG_BIQUADRATIC_TRIANGLE ||
        cellType == IG_LAGRANGE_TRIANGLE) {
        used = 3;
    } else if (kind == SHAPE_LINEAR_QUAD || cellType == IG_QUADRATIC_QUAD || cellType == IG_BIQUADRATIC_QUAD ||
               cellType == IG_QUADRATIC_LINEAR_QUAD || cellType == IG_LAGRANGE_QUADRILATERAL) {
        used = 4;
    }
    if (used < 3 || used > npts) { return false; }

    std::vector<Point> pts(static_cast<size_t>(used));
    for (int i = 0; i < used; ++i) { pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]); }

    std::vector<double> local;
    if (used == 3) {
        double w[3];
        double planeDist = 0.0;
        if (!TriangleBarycentric(p, pts[0], pts[1], pts[2], w, planeDist)) { return false; }
        if (planeDist > distTol) { return false; }
        if (w[0] < -kParamTol || w[1] < -kParamTol || w[2] < -kParamTol) { return false; }
        local.assign(w, w + 3);
        NormalizeWeights(local);
    } else if (used == 4) {
        double pc[2] = {0.5, 0.5};
        double residual = 0.0;
        if (!NewtonParametric2D(SHAPE_LINEAR_QUAD, pts, p, pc, local, residual)) { return false; }
        if (residual > distTol) { return false; }
        if (pc[0] < -kParamTol || pc[0] > 1.0 + kParamTol || pc[1] < -kParamTol || pc[1] > 1.0 + kParamTol) {
            return false;
        }
        NormalizeWeights(local);
    } else {
        // 多边形：以 0 号点为扇心做扇形三角剖分
        bool found = false;
        for (int k = 1; k + 1 < used && !found; ++k) {
            double w[3];
            double planeDist = 0.0;
            if (!TriangleBarycentric(p, pts[0], pts[static_cast<size_t>(k)], pts[static_cast<size_t>(k + 1)], w,
                                     planeDist)) {
                continue;
            }
            if (planeDist <= distTol && w[0] >= -kParamTol && w[1] >= -kParamTol && w[2] >= -kParamTol) {
                local.assign(static_cast<size_t>(used), 0.0);
                local[0] = w[0];
                local[static_cast<size_t>(k)] = w[1];
                local[static_cast<size_t>(k + 1)] = w[2];
                found = true;
            }
        }
        if (!found) { return false; }
        NormalizeWeights(local);
    }

    // 权重向量长度与单元点数一致（高次单元的角点回退时其余为 0）
    weights.assign(static_cast<size_t>(npts), 0.0);
    for (int i = 0; i < used; ++i) { weights[static_cast<size_t>(i)] = local[static_cast<size_t>(i)]; }
    return true;
}

/* ------------------------------------------------------------------ */
/* 体单元：包含判定                                                    */
/* ------------------------------------------------------------------ */
bool ResampleToLine::IsPointInVolumeCell(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                         double distTol) {
    auto cell = mesh->GetCell(cellId);
    if (cell == nullptr) { return false; }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts < 4) { return false; }

    std::vector<Point> pts(static_cast<size_t>(npts));
    for (int i = 0; i < npts; ++i) { pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]); }

    CellFaceList faces;
    CollectCellFaces(cell, ptIds, npts, faces);
    if (faces.empty()) { return false; }
    return ContainsByFaces(pts, faces, p, distTol);
}

/* ------------------------------------------------------------------ */
/* 体单元：均值坐标插值                                                */
/* ------------------------------------------------------------------ */
bool ResampleToLine::ComputeMeanValueWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                             double distTol, std::vector<double>& weights) {
    auto cell = mesh->GetCell(cellId);
    if (cell == nullptr) { return false; }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts < 4 || npts > kMaxCellPointNum) { return false; }

    std::vector<Point> pts(static_cast<size_t>(npts));
    for (int i = 0; i < npts; ++i) { pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]); }

    const IGenum cellType = mesh->GetCellType(cellId);

    // 四面体：解析重心坐标（包含判定与插值权重，数学上等同均值坐标）
    if (cellType == IG_TETRA && npts == 4) {
        const Point& a = pts[0];
        const Point& b = pts[1];
        const Point& c = pts[2];
        const Point& d = pts[3];
        double p10[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        double p20[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        double p30[3] = {d[0] - a[0], d[1] - a[1], d[2] - a[2]};
        double rhs[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};

        const double det = Det3(p10, p20, p30);
        if (std::fabs(det) < 1e-30) { return false; }

        double w[4];
        w[1] = Det3(rhs, p20, p30) / det;
        w[2] = Det3(p10, rhs, p30) / det;
        w[3] = Det3(p10, p20, rhs) / det;
        w[0] = 1.0 - w[1] - w[2] - w[3];

        const double scale = std::max({1e-12, static_cast<double>((a - b).length()),
                                       static_cast<double>((a - c).length()), static_cast<double>((a - d).length())});
        const double baryTol = std::max(kParamTol, distTol / scale);
        if (w[0] < -baryTol || w[1] < -baryTol || w[2] < -baryTol || w[3] < -baryTol) { return false; }

        weights.assign(static_cast<size_t>(npts), 0.0);
        for (int i = 0; i < 4; ++i) { weights[static_cast<size_t>(i)] = w[i]; }
        return NormalizeKeepSign(weights);
    }

    // 其他体单元：包含判定（贴面 / 绕数）+ 逐面均值坐标
    CellFaceList faces;
    CollectCellFaces(cell, ptIds, npts, faces);
    if (faces.empty()) { return false; }
    if (!ContainsByFaces(pts, faces, p, distTol)) { return false; }

    std::vector<double> w;
    if (!MeanValueWeightsFromFaces(pts, faces, p, distTol, w)) { return false; }

    weights = std::move(w);
    return true;
}

/* ------------------------------------------------------------------ */
/* 高次体单元退化：用线性角点单元的面表做均值坐标                      */
/* ------------------------------------------------------------------ */
bool ResampleToLine::ComputeDegradedVolumeWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                                 double distTol, std::vector<double>& weights) {
    weights.clear();

    const IGenum cellType = mesh->GetCellType(cellId);
    Cell::Pointer linear;
    const int corners = LinearCornerVolumeCell(cellType, linear);
    if (corners == 0 || linear == nullptr) { return false; }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts < corners) { return false; }

    // 用局部下标填充角点单元：其面表给出的点 id 即 pts 下标
    linear->Reset();
    std::vector<Point> pts(static_cast<size_t>(corners));
    for (int i = 0; i < corners; ++i) {
        pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]);
        linear->m_PointIds->AddId(i);
        linear->m_Points->AddPoint(pts[static_cast<size_t>(i)]);
    }

    CellFaceList faces;
    CollectCellFaces(linear.get(), nullptr, corners, faces);
    if (faces.empty()) { return false; }
    if (!ContainsByFaces(pts, faces, p, distTol)) { return false; }

    std::vector<double> w;
    if (!MeanValueWeightsFromFaces(pts, faces, p, distTol, w)) { return false; }

    // 权重向量长度与单元点数一致（角点之外为 0）
    weights.assign(static_cast<size_t>(npts), 0.0);
    for (int i = 0; i < corners; ++i) { weights[static_cast<size_t>(i)] = w[static_cast<size_t>(i)]; }
    return true;
}

/* ------------------------------------------------------------------ */
/* 二次单元：二次形函数插值                                            */
/* ------------------------------------------------------------------ */
bool ResampleToLine::ComputeQuadraticWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                             double distTol, std::vector<double>& weights) {
    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts <= 0) { return false; }

    const IGenum cellType = mesh->GetCellType(cellId);
    const ShapeKind kind = ClassifyCell(cellType, npts);
    const int need = ShapePointCount(kind);
    if (need == 0 || npts != need) { return false; }

    std::vector<Point> pts(static_cast<size_t>(npts));
    for (int i = 0; i < npts; ++i) { pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]); }

    if (IsQuadraticFaceKind(kind)) {
        double pc[2] = {0.5, 0.5};
        double residual = 0.0;
        std::vector<double> w;
        if (!NewtonParametric2D(kind, pts, p, pc, w, residual)) { return false; }
        const double pc3[3] = {pc[0], pc[1], 0.0};
        if (!ShapeDomainOk(kind, pc3, kParamTol)) { return false; }
        if (residual > distTol) { return false; }
        NormalizeWeights(w);
        weights = std::move(w);
        return true;
    }

    double pc[3] = {0.0, 0.0, 0.0};
    double residual = 0.0;
    std::vector<double> w;
    if (!NewtonQuadratic3D(kind, pts, p, pc, w, residual)) { return false; }
    if (!ShapeDomainOk(kind, pc, kParamTol)) { return false; }
    if (residual > distTol) { return false; }
    NormalizeWeights(w);
    weights = std::move(w);
    return true;
}

/* ------------------------------------------------------------------ */
/* 点到单元边界的距离                                                  */
/* ------------------------------------------------------------------ */
double ResampleToLine::DistanceToCellBoundary(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                              Point& closest) {
    double best = DBL_MAX;
    closest = p;

    auto cell = mesh->GetCell(cellId);
    if (cell == nullptr) { return best; }

    const int nfaces = cell->GetNumberOfFaces();
    for (int f = 0; f < nfaces; ++f) {
        auto face = cell->GetFace(f);
        if (face == nullptr) { continue; }
        const int fn = face->GetNumberOfPoints();
        if (fn < 3) {
            for (int k = 0; k + 1 < fn; ++k) {
                Point c(0.f, 0.f, 0.f);
                double d2 = 0.0;
                ClosestPointOnSegment(p, face->GetPoint(k), face->GetPoint(k + 1), c, d2);
                if (d2 < best) {
                    best = d2;
                    closest = c;
                }
            }
            continue;
        }
        for (int k = 1; k + 1 < fn; ++k) {
            Point c(0.f, 0.f, 0.f);
            double d2 = 0.0;
            ClosestPointOnTriangle(p, face->GetPoint(0), face->GetPoint(k), face->GetPoint(k + 1), c, d2);
            if (d2 < best) {
                best = d2;
                closest = c;
            }
        }
    }

    if (best == DBL_MAX) {
        const int npts = cell->GetNumberOfPoints();
        for (int i = 0; i < npts; ++i) {
            const double d2 = static_cast<double>((cell->GetPoint(i) - p).squaredLength());
            if (d2 < best) {
                best = d2;
                closest = cell->GetPoint(i);
            }
        }
    }
    return (best == DBL_MAX) ? DBL_MAX : std::sqrt(best);
}

/* ------------------------------------------------------------------ */
/* 属性插值                                                            */
/* ------------------------------------------------------------------ */
void ResampleToLine::InterpolatePointData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                                          const std::vector<SampleLocation>& locations, int sampleNum) {
    if (inSet == nullptr || outSet == nullptr) { return; }
    auto all = inSet->GetAllAttributes();
    if (all == nullptr) { return; }

    std::vector<double> values;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_POINT) { continue; }

        auto src = attr.pointer;
        const int dim = src->GetDimension();
        if (dim <= 0) { continue; }

        // 保留合理的数据类型：浮点数组沿用原类型，整型等插值后使用 float
        IGenum outArrayType = src->GetArrayType();
        if (outArrayType != IG_FloatArray && outArrayType != IG_DoubleArray) { outArrayType = IG_FloatArray; }

        auto dst = CreateArrayObject(outArrayType);
        if (dst == nullptr) { continue; }
        dst->SetName(src->GetName());
        dst->SetDimension(dim);
        dst->Resize(static_cast<IGsize>(sampleNum));

        values.assign(static_cast<size_t>(dim), 0.0);
        for (int j = 0; j < sampleNum; ++j) {
            std::fill(values.begin(), values.end(), 0.0); // 无效采样点填 0
            const SampleLocation& loc = locations[static_cast<size_t>(j)];
            const size_t npts = loc.pointIds.size();
            if (loc.cellId >= 0 && npts > 0 && loc.weights.size() == npts) {
                for (size_t k = 0; k < npts; ++k) {
                    const double w = loc.weights[k];
                    if (w == 0.0) { continue; }
                    const igIndex ptId = loc.pointIds[k];
                    for (int c = 0; c < dim; ++c) {
                        values[static_cast<size_t>(c)] += w * src->GetElementValue(ptId, c);
                    }
                }
            }
            dst->SetElement(static_cast<IGsize>(j), values.data());
        }

        outSet->AddAttribute(attr.type, IG_POINT, dst, attr.GetDataRange());
    }
}

void ResampleToLine::CopyCellData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                                  const std::vector<SampleLocation>& locations, int sampleNum) {
    if (inSet == nullptr || outSet == nullptr) { return; }
    auto all = inSet->GetAllAttributes();
    if (all == nullptr) { return; }

    std::vector<double> values;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_CELL) { continue; }

        auto src = attr.pointer;
        const int dim = src->GetDimension();
        if (dim <= 0) { continue; }

        // Cell Data 是复制关系，完整保留原数组数据类型与分量数
        auto dst = CreateArrayObject(src->GetArrayType());
        if (dst == nullptr) { continue; }
        dst->SetName(src->GetName());
        dst->SetDimension(dim);
        dst->Resize(static_cast<IGsize>(sampleNum));

        const IGsize cellNum = src->GetNumberOfElements();
        values.assign(static_cast<size_t>(dim), 0.0);
        for (int j = 0; j < sampleNum; ++j) {
            std::fill(values.begin(), values.end(), 0.0);
            const igIndex cellId = locations[static_cast<size_t>(j)].cellId;
            if (cellId >= 0 && static_cast<IGsize>(cellId) < cellNum) {
                src->GetElement(static_cast<IGsize>(cellId), values.data());
            }
            dst->SetElement(static_cast<IGsize>(j), values.data());
        }

        // 复制到采样点上，因此附着类型为 IG_POINT
        outSet->AddAttribute(attr.type, IG_POINT, dst, attr.GetDataRange());
    }
}

void ResampleToLine::AddValidPointMask(AttributeSet::Pointer outSet, int sampleNum) {
    if (outSet == nullptr) { return; }

    auto mask = UnsignedCharArray::New();
    mask->SetName("validpointmask");
    mask->SetDimension(1);
    mask->Resize(static_cast<IGsize>(sampleNum));
    for (int i = 0; i < sampleNum; ++i) {
        const unsigned char v = (static_cast<size_t>(i) < m_SampleValidMask.size())
                                        ? m_SampleValidMask[static_cast<size_t>(i)]
                                        : static_cast<unsigned char>(0);
        mask->SetValue(static_cast<IGsize>(i), static_cast<double>(v));
    }

    // validpointmask 附着在采样点上：1 = 可插值，0 = 无效
    outSet->AddAttribute(IG_SCALAR, IG_POINT, mask);
}

/* ------------------------------------------------------------------ */
/* 折线输出                                                            */
/* ------------------------------------------------------------------ */
/** 复制一份属性条目（数组共享，属性条目独立），避免共享 AttributeSet 时宿主对象混乱 */
namespace {
AttributeSet::Pointer CloneAttributeSetEntries(const AttributeSet::Pointer& srcSet) {
    auto dstSet = AttributeSet::New();
    if (srcSet == nullptr) { return dstSet; }

    auto all = srcSet->GetAllAttributes();
    if (all == nullptr) { return dstSet; }

    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        dstSet->AddAttribute(attr.type, attr.attachmentType, attr.pointer, attr.dataRange);
    }
    return dstSet;
}
} // namespace

void ResampleToLine::BuildPolyLineOutputs(const Points::Pointer& samples, AttributeSet::Pointer attrSet,
                                          int sampleNum) {
    // (1) UnstructuredMesh 折线：IG_LINE 单元，保持与既有流程（菜单/示例）兼容
    auto lineMesh = UnstructuredMesh::New();
    lineMesh->SetName("resample_to_line");
    lineMesh->SetPoints(samples);

    auto cells = CellArray::New();
    auto types = UnsignedIntArray::New();
    for (int i = 0; i + 1 < sampleNum; ++i) {
        cells->AddCellId2(i, i + 1);
        types->AddValue(IG_LINE);
    }
    lineMesh->SetCells(cells, types);
    lineMesh->SetAttributeSet(CloneAttributeSetEntries(attrSet));
    lineMesh->SetViewStyle(IG_WIREFRAME);

    m_LineMesh = lineMesh;
    SetOutput(0, lineMesh);

    // (2) SurfaceMesh 折线：真正的折线数据（点 + 边），可直接渲染为折线
    auto poly = SurfaceMesh::New();
    poly->SetName("resample_to_line");
    poly->SetPoints(samples);

    // 先给出空的面片数组：SurfaceMesh 的拓扑/可绘制数据接口要求面片数组非空，
    // 且其时间戳要早于边数组，避免后续 RequestEditStatus() 依据面片重建边而丢掉折线
    poly->SetFaces(CellArray::New());

    auto edges = CellArray::New();
    for (int i = 0; i + 1 < sampleNum; ++i) { edges->AddCellId2(i, i + 1); }
    poly->SetEdges(edges);
    poly->SetAttributeSet(attrSet);
    poly->SetViewStyle(IG_WIREFRAME);
    // 折线没有面片，关闭抽壳/简化渲染，避免空面片网格参与简化
    poly->SetShellRenderingOption(false);

    m_PolyLine = poly;
    SetOutput(1, poly);
}

IGAME_NAMESPACE_END
