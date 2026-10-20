#include "iGameCellQualityMath.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>
#include <vector>

IGAME_NAMESPACE_BEGIN

// ===========================================================================
// 匿名命名空间：Verdict 内部辅助函数与常量（全程 double 运算）
// ===========================================================================
namespace {

constexpr double kVerictDblMax = 1e30; // Verdict 的 VERDICT_DBL_MAX
constexpr double kVerictDblMin = 1e-30; // Verdict 的 VERDICT_DBL_MIN
constexpr double kVdMax = 1e30;
constexpr double kVdMin = 1e-30;

inline void ToD(const iGame::Point& p, double out[3]) {
    out[0] = static_cast<double>(p[0]);
    out[1] = static_cast<double>(p[1]);
    out[2] = static_cast<double>(p[2]);
}

inline void ToD8(const std::vector<iGame::Point>& p, int n, double c[][3]) {
    for (int i = 0; i < n; ++i) {
        c[i][0] = static_cast<double>(p[i][0]);
        c[i][1] = static_cast<double>(p[i][1]);
        c[i][2] = static_cast<double>(p[i][2]);
    }
}

inline void SubD(const double a[3], const double b[3], double out[3]) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}
inline void SubV(const double a[3], const double b[3], double o[3]) {
    o[0] = a[0] - b[0]; o[1] = a[1] - b[1]; o[2] = a[2] - b[2];
}

inline double LenD(const double v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
inline double LenV(const double v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

inline double LenSqD(const double v[3]) { return v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; }
inline double LenSqV(const double v[3]) { return v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; }

inline double DotD(const double u[3], const double v[3]) { return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]; }
inline double DotV(const double u[3], const double v[3]) { return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]; }

inline void CrossD(const double u[3], const double v[3], double out[3]) {
    out[0] = u[1] * v[2] - u[2] * v[1];
    out[1] = u[2] * v[0] - u[0] * v[2];
    out[2] = u[0] * v[1] - u[1] * v[0];
}
inline void CrossV(const double u[3], const double v[3], double o[3]) {
    o[0] = u[1] * v[2] - u[2] * v[1];
    o[1] = u[2] * v[0] - u[0] * v[2];
    o[2] = u[0] * v[1] - u[1] * v[0];
}

template <int N>
inline double ElemScaling(const double c[][3]) {
    double mn[3] = {c[0][0], c[0][1], c[0][2]};
    double mx[3] = {c[0][0], c[0][1], c[0][2]};
    for (int i = 1; i < N; ++i) {
        for (int k = 0; k < 3; ++k) {
            if (c[i][k] < mn[k]) { mn[k] = c[i][k]; }
            if (c[i][k] > mx[k]) { mx[k] = c[i][k]; }
        }
    }
    double d[3] = {mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]};
    const double len = LenD(d);
    return (len < kVerictDblMin) ? 1.0 : len;
}

template <int N>
inline double ScaleOf(const double c[][3]) {
    return ElemScaling<N>(c);
}

inline void MakeQuadEdges(const double c[][3], double e[4][3]) {
    SubD(c[1], c[0], e[0]);
    SubD(c[2], c[1], e[1]);
    SubD(c[3], c[2], e[2]);
    SubD(c[0], c[3], e[3]);
}

inline void SignedCornerAreas(const double c[][3], double areas[4]) {
    double e[4][3];
    MakeQuadEdges(c, e);

    double cn[4][3];
    CrossD(e[3], e[0], cn[0]);
    CrossD(e[0], e[1], cn[1]);
    CrossD(e[1], e[2], cn[2]);
    CrossD(e[2], e[3], cn[3]);

    double pa0[3], pa1[3], nc[3];
    for (int k = 0; k < 3; ++k) { pa0[k] = e[0][k] - e[2][k]; }
    for (int k = 0; k < 3; ++k) { pa1[k] = e[1][k] - e[3][k]; }
    CrossD(pa0, pa1, nc);
    const double nl = LenD(nc);
    if (nl > kVerictDblMin) {
        for (int k = 0; k < 3; ++k) { nc[k] /= nl; }
    } else {
        nc[0] = nc[1] = nc[2] = 0.0;
    }

    for (int i = 0; i < 4; ++i) { areas[i] = DotD(nc, cn[i]); }
}

inline void QuadMinMaxAngle(const double c[][3], double out[2]) {
    const bool collapsed = (c[3][0] == c[2][0] && c[3][1] == c[2][1] && c[3][2] == c[2][2]);
    if (collapsed) {
        double te[3][3];
        SubD(c[1], c[0], te[0]);
        SubD(c[2], c[1], te[1]);
        SubD(c[0], c[2], te[2]);
        double mn = 360.0, mx = 0.0;
        for (int i = 0; i < 3; ++i) {
            const int j = (i + 1) % 3;
            const double li = LenD(te[i]), lj = LenD(te[j]);
            if (li <= kVerictDblMin || lj <= kVerictDblMin) { continue; }
            const double ang = std::acos(std::clamp(-DotD(te[i], te[j]) / (li * lj), -1.0, 1.0)) * 180.0 / 3.14159265358979323846;
            if (ang < mn) { mn = ang; }
            if (ang > mx) { mx = ang; }
        }
        out[0] = mn;
        out[1] = mx;
        return;
    }

    double e[4][3];
    MakeQuadEdges(c, e);

    double mn = 360.0, mx = 0.0;
    for (int i = 0; i < 4; ++i) {
        const int j = (i + 1) % 4;
        const double li = LenD(e[i]);
        const double lj = LenD(e[j]);
        if (li <= kVerictDblMin || lj <= kVerictDblMin) { continue; }
        const double ang = std::acos(std::clamp(-DotD(e[i], e[j]) / (li * lj), -1.0, 1.0)) * 180.0 / 3.14159265358979323846;
        if (ang < mn) { mn = ang; }
        if (ang > mx) { mx = ang; }
    }

    double areas[4];
    SignedCornerAreas(c, areas);
    if (areas[0] < 0.0 || areas[1] < 0.0 || areas[2] < 0.0 || areas[3] < 0.0) {
        mx = 360.0 - mx;
    }

    out[0] = mn;
    out[1] = mx;
}

inline double TriConditionCore(const double c[][3]) {
    double v1[3], v2[3];
    SubD(c[1], c[0], v1);
    SubD(c[2], c[0], v2);

    double cr[3];
    CrossD(v1, v2, cr);
    const double areaX2 = LenD(cr);
    if (areaX2 == 0.0) { return kVerictDblMax; }

    const double condition = (DotD(v1, v1) + DotD(v2, v2) - DotD(v1, v2)) / (areaX2 * std::sqrt(3.0));
    return std::min(condition, kVerictDblMax);
}

inline double TriScaledJacobianCore(const double c[][3]) {
    double e[3][3];
    SubD(c[1], c[0], e[0]);
    SubD(c[2], c[0], e[1]);
    SubD(c[2], c[1], e[2]);

    const double l0 = LenD(e[0]), l1 = LenD(e[1]), l2 = LenD(e[2]);

    double first[3], second[3], cr[3];
    SubD(e[1], e[0], first);
    SubD(e[2], e[0], second);
    CrossD(first, second, cr);
    double jacobian = LenD(cr);

    double maxProd = l0 * l1;
    const double p12 = l1 * l2, p02 = l0 * l2;
    if (p12 > maxProd) { maxProd = p12; }
    if (p02 > maxProd) { maxProd = p02; }

    if (maxProd < kVerictDblMin) { return 0.0; }

    jacobian *= 2.0 / std::sqrt(3.0);
    jacobian /= maxProd;

    return (jacobian > 0.0) ? std::min(jacobian, kVerictDblMax) : std::max(jacobian, -kVerictDblMax);
}

inline double TriRadiusRatioCore(const double c[][3]) {
    double v[3][3];
    SubD(c[1], c[0], v[0]);
    SubD(c[2], c[1], v[1]);
    SubD(c[0], c[2], v[2]);

    const double cs = ElemScaling<3>(c);
    for (int i = 0; i < 3; ++i) { for (int k = 0; k < 3; ++k) { v[i][k] /= cs; } }

    const double a1 = LenD(v[0]), b1 = LenD(v[1]), c1 = LenD(v[2]);

    double cr[3];
    CrossD(v[0], v[1], cr);
    const double den = LenSqD(cr);
    if (den < kVerictDblMin) { return kVerictDblMax; }

    const double rr = 0.25 * a1 * b1 * c1 * (a1 + b1 + c1) / den;
    return (rr > 0.0) ? std::min(rr, kVerictDblMax) : std::max(rr, -kVerictDblMax);
}

inline void HexDerivD(const double c[8][3], double xi, double et, double ze,
                      double dxi[3], double det_[3], double dze[3]) {
    const double dN[8][3] = {
            {-0.125 * (1 - et) * (1 - ze), -0.125 * (1 - xi) * (1 - ze), -0.125 * (1 - xi) * (1 - et)},
            {0.125 * (1 - et) * (1 - ze), -0.125 * (1 + xi) * (1 - ze), -0.125 * (1 + xi) * (1 - et)},
            {0.125 * (1 + et) * (1 - ze), 0.125 * (1 + xi) * (1 - ze), -0.125 * (1 + xi) * (1 + et)},
            {-0.125 * (1 + et) * (1 - ze), 0.125 * (1 - xi) * (1 - ze), -0.125 * (1 - xi) * (1 + et)},
            {-0.125 * (1 - et) * (1 + ze), -0.125 * (1 - xi) * (1 + ze), 0.125 * (1 - xi) * (1 - et)},
            {0.125 * (1 - et) * (1 + ze), -0.125 * (1 + xi) * (1 + ze), 0.125 * (1 + xi) * (1 - et)},
            {0.125 * (1 + et) * (1 + ze), 0.125 * (1 + xi) * (1 + ze), 0.125 * (1 + xi) * (1 + et)},
            {-0.125 * (1 + et) * (1 + ze), 0.125 * (1 - xi) * (1 + ze), 0.125 * (1 - xi) * (1 + et)}};
    for (int k = 0; k < 3; ++k) { dxi[k] = 0.0; det_[k] = 0.0; dze[k] = 0.0; }
    for (int j = 0; j < 8; ++j) {
        for (int k = 0; k < 3; ++k) {
            dxi[k] += dN[j][0] * c[j][k];
            det_[k] += dN[j][1] * c[j][k];
            dze[k] += dN[j][2] * c[j][k];
        }
    }
}

constexpr double kHexPt[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                                 {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};

constexpr int kHexCornerBase[8] = {0, 1, 2, 3, 4, 5, 6, 7};
constexpr int kHexCornerNbr[8][3] = {{1, 3, 4}, {2, 0, 5}, {3, 1, 6}, {0, 2, 7},
                                     {7, 5, 0}, {4, 6, 1}, {5, 7, 2}, {6, 4, 3}};

inline void HexCornerEdges(const double c[8][3], int i, double e0[3], double e1[3], double e2[3]) {
    const int b = kHexCornerBase[i];
    SubV(c[kHexCornerNbr[i][0]], c[b], e0);
    SubV(c[kHexCornerNbr[i][1]], c[b], e1);
    SubV(c[kHexCornerNbr[i][2]], c[b], e2);
}

inline double ConditionComp(const double xxi[3], const double xet[3], const double xze[3]) {
    double cr[3];
    CrossV(xet, xze, cr);
    const double det = DotV(xxi, cr);
    if (det <= kVdMin) { return kVdMax; }

    const double term1 = DotV(xxi, xxi) + DotV(xet, xet) + DotV(xze, xze);

    double c1[3], c2[3], c3[3];
    CrossV(xxi, xet, c1);
    CrossV(xet, xze, c2);
    CrossV(xze, xxi, c3);
    const double term2 = DotV(c1, c1) + DotV(c2, c2) + DotV(c3, c3);

    return std::sqrt(term1 * term2) / det;
}

inline double OddyCompHex(const double xxi[3], const double xet[3], const double xze[3]) {
    const double g11 = DotV(xxi, xxi), g12 = DotV(xxi, xet), g13 = DotV(xxi, xze);
    const double g22 = DotV(xet, xet), g23 = DotV(xet, xze), g33 = DotV(xze, xze);

    double cr[3];
    CrossV(xet, xze, cr);
    const double rtG = DotV(xxi, cr);

    if (rtG <= kVdMin) { return kVdMax; }

    const double normG2 = g11 * g11 + 2.0 * g12 * g12 + 2.0 * g13 * g13 +
                          g22 * g22 + 2.0 * g23 * g23 + g33 * g33;
    const double normJ2 = g11 + g22 + g33;
    return (normG2 - (1.0 / 3.0) * normJ2 * normJ2) / std::pow(rtG, 4.0 / 3.0);
}

struct HexWeight {
    double detw;
    double scale;
    bool valid;
};

inline HexWeight HexGetWeight(double averageSize) {
    HexWeight w{0.0, 0.0, false};
    if (averageSize <= 0.0) { return w; }
    w.scale = std::pow(averageSize, 1.0 / 3.0);
    w.detw = w.scale * w.scale * w.scale;
    w.valid = true;
    return w;
}

inline double GetCollapseRatioOfVertex(Point v1, Point v2, Point v3, double volume) {
    double area = ((v2 - v1).cross(v3 - v1)).norm();
    double high = volume / area * 6;

    double maxLength = std::max((v2 - v1).norm(), (v3 - v1).norm());
    maxLength = std::max(maxLength, (v3 - v2).norm());

    return high / maxLength;
}

inline std::vector<double> GetMinAndMaxLenOfCell(const std::vector<iGame::Point>& points) {
    double minLength = DBL_MAX, maxLength = 0.0;
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            double length = (points[i] - points[j]).norm();
            minLength = (std::min)(minLength, length);
            maxLength = (std::max)(maxLength, length);
        }
    }
    std::vector<double> result;
    result.push_back(minLength);
    result.push_back(maxLength);
    return result;
}

} // namespace

// ==================== 三角形指标 ====================
double CellQualityMath::ComputeTriangleArea(const std::vector<Point>& points) {
    auto sub = [&](int a, int b, double out[3]) {
        out[0] = static_cast<double>(points[a][0]) - points[b][0];
        out[1] = static_cast<double>(points[a][1]) - points[b][1];
        out[2] = static_cast<double>(points[a][2]) - points[b][2];
    };
    double e1[3], e2[3], cr[3];
    sub(1, 0, e1);
    sub(2, 0, e2);
    cr[0] = e1[1] * e2[2] - e1[2] * e2[1];
    cr[1] = e1[2] * e2[0] - e1[0] * e2[2];
    cr[2] = e1[0] * e2[1] - e1[1] * e2[0];
    return 0.5 * std::sqrt(cr[0] * cr[0] + cr[1] * cr[1] + cr[2] * cr[2]);
}

double CellQualityMath::ComputeTriangleAspectRatio(const std::vector<Point>& points) {
    double a = (points[1] - points[0]).norm();
    double b = (points[2] - points[1]).norm();
    double c = (points[0] - points[2]).norm();
    double area = ComputeTriangleArea(points);

    return (std::max({a, b, c})) * (a + b + c) / (4.0 * sqrt(3.0) * area);
}

double CellQualityMath::ComputeTriangleCondition(const std::vector<Point>& points) {
    double c[3][3];
    for (int i = 0; i < 3; ++i) { ToD(points[i], c[i]); }
    return TriConditionCore(c);
}

double CellQualityMath::ComputeTriangleDistortion(const std::vector<Point>& points) {
    (void) points;
    return 1.0; // Verdict：直边三角形即目标形状，畸变恒为 1
}

double CellQualityMath::ComputeTriangleEdgeRatio(const std::vector<Point>& points) {
    double a = (points[1] - points[0]).norm();
    double b = (points[2] - points[1]).norm();
    double c = (points[0] - points[2]).norm();

    return std::max({a, b, c}) / std::min({a, b, c});
}

double CellQualityMath::ComputeTriangleEquiangleSkew(const std::vector<Point>& points) {
    double c[3][3];
    for (int i = 0; i < 3; ++i) { ToD(points[i], c[i]); }

    // Verdict 的 sides：sides[0]=P1-P0, sides[1]=P2-P1, sides[2]=P2-P0, sides[3]=-sides[1]
    double sides[4][3];
    SubD(c[1], c[0], sides[0]);
    SubD(c[2], c[1], sides[1]);
    SubD(c[2], c[0], sides[2]);
    for (int k = 0; k < 3; ++k) { sides[3][k] = -sides[1][k]; }

    const double l0 = LenSqD(sides[0]), l1 = LenSqD(sides[1]), l2 = LenSqD(sides[2]);
    if (l0 == 0.0 || l1 == 0.0 || l2 == 0.0) { return 0.0; }

    auto interior = [](const double a[3], const double b[3]) {
        const double la = LenD(a), lb = LenD(b);
        if (la <= kVerictDblMin || lb <= kVerictDblMin) { return 0.0; }
        return std::acos(std::clamp(DotD(a, b) / (la * lb), -1.0, 1.0)) * 180.0 / PI;
    };

    // tri_minimum_angle：找最短边，算其对角的角
    int idxMin = 0;
    if (l1 < l0) { idxMin = 1; }
    if (l2 < ((idxMin == 0) ? l0 : l1)) { idxMin = 2; }
    double minAngle;
    if (idxMin == 0) { minAngle = interior(sides[2], sides[1]); }
    else if (idxMin == 1) { minAngle = interior(sides[0], sides[2]); }
    else { minAngle = interior(sides[0], sides[3]); }

    // tri_maximum_angle：找最长边，算其对角的角
    int idxMax = 0;
    if (l1 > l0) { idxMax = 1; }
    if (l2 > ((idxMax == 0) ? l0 : l1)) { idxMax = 2; }
    double maxAngle;
    if (idxMax == 0) { maxAngle = interior(sides[2], sides[1]); }
    else if (idxMax == 1) { maxAngle = interior(sides[0], sides[2]); }
    else { maxAngle = interior(sides[0], sides[3]); }

    const double skewMax = (maxAngle - 60.0) / 120.0;
    const double skewMin = (60.0 - minAngle) / 60.0;
    return (skewMax > skewMin) ? skewMax : skewMin;
}

double CellQualityMath::ComputeTriangleMinAngle(const std::vector<Point>& points) {
    auto sub = [&](int a, int b, double out[3]) {
        out[0] = static_cast<double>(points[a][0]) - points[b][0];
        out[1] = static_cast<double>(points[a][1]) - points[b][1];
        out[2] = static_cast<double>(points[a][2]) - points[b][2];
    };
    auto dot = [](const double u[3], const double v[3]) { return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]; };
    auto sq = [](const double v[3]) { return v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; };

    double sides[4][3];
    sub(1, 0, sides[0]);   // P1 - P0
    sub(2, 1, sides[1]);   // P2 - P1
    sub(2, 0, sides[2]);   // P2 - P0
    for (int k = 0; k < 3; ++k) { sides[3][k] = -sides[1][k]; }

    const double l0 = sq(sides[0]), l1 = sq(sides[1]), l2 = sq(sides[2]);
    if (l0 == 0.0 || l1 == 0.0 || l2 == 0.0) { return 0.0; }

    int shortSide = 0;
    if (l1 < l0) { shortSide = 1; }
    if (l2 < (shortSide == 0 ? l0 : l1)) { shortSide = 2; }

    const double* a = nullptr;
    const double* b = nullptr;
    if (shortSide == 0) { a = sides[2]; b = sides[1]; }
    else if (shortSide == 1) { a = sides[0]; b = sides[2]; }
    else { a = sides[0]; b = sides[3]; }

    const double la = std::sqrt(sq(a));
    const double lb = std::sqrt(sq(b));
    if (la <= 0.0 || lb <= 0.0) { return 0.0; }

    const double cosA = std::clamp(dot(a, b) / (la * lb), -1.0, 1.0);
    return std::acos(cosA) * 180.0 / PI;
}

double CellQualityMath::ComputeTriangleRadiusRatio(const std::vector<Point>& points) {
    double c[3][3];
    for (int i = 0; i < 3; ++i) { ToD(points[i], c[i]); }
    return TriRadiusRatioCore(c);
}

double CellQualityMath::ComputeTriangleScaledJacobian(const std::vector<Point>& points) {
    double c[3][3];
    for (int i = 0; i < 3; ++i) { ToD(points[i], c[i]); }
    return TriScaledJacobianCore(c);
}

double CellQualityMath::ComputeTriangleShape(const std::vector<Point>& points) {
    const double cs = ComputeTriangleCondition(points);
    const double shape = (cs <= kVerictDblMin) ? kVerictDblMax : (1.0 / cs);
    return (shape > 0.0) ? std::min(shape, kVerictDblMax) : std::max(shape, -kVerictDblMax);
}

double CellQualityMath::ComputeTriangleRelativeSizeSquared(
        const std::vector<Point>& points, double averageArea) {
    if (averageArea <= 0.0) { return 0.0; }
    // tri_get_weight 构造的三角形，其 detw 等于 2 * average_tri_area
    const double detw = 2.0 * averageArea;
    if (detw == 0.0) { return 0.0; }

    double c[3][3];
    for (int i = 0; i < 3; ++i) { ToD(points[i], c[i]); }

    double xxi[3], xet[3], cr[3];
    SubD(c[0], c[1], xxi); // P0 - P1
    SubD(c[0], c[2], xet); // P0 - P2
    CrossD(xxi, xet, cr);
    const double deta = LenD(cr);
    if (deta == 0.0 || detw == 0.0) { return 0.0; }

    double size = deta / detw;
    size = size * size;
    const double rel = std::min(size, 1.0 / size);
    return (rel > 0.0) ? std::min(rel, kVerictDblMax) : std::max(rel, -kVerictDblMax);
}

double CellQualityMath::ComputeTriangleShapeAndSize(const std::vector<Point>& points,
                                                             double averageArea) {
    const double shape = ComputeTriangleShape(points);
    const double size = ComputeTriangleRelativeSizeSquared(points, averageArea);
    return shape * size;
}


// ==================== 四边形指标 ====================
double CellQualityMath::ComputeQuadArea(const std::vector<Point>& points) {
    double c[4][3];
    for (int i = 0; i < 4; ++i) {
        c[i][0] = static_cast<double>(points[i][0]);
        c[i][1] = static_cast<double>(points[i][1]);
        c[i][2] = static_cast<double>(points[i][2]);
    }
    // 复用文件末尾的 signed_corner_areas 实现（在同名静态函数之前需要前向调用，
    // 这里直接内联一份等价计算，避免改动函数顺序）。
    auto sub = [&](int a, int b, double o[3]) {
        o[0] = c[a][0] - c[b][0]; o[1] = c[a][1] - c[b][1]; o[2] = c[a][2] - c[b][2];
    };
    auto crossv = [](const double u[3], const double v[3], double o[3]) {
        o[0] = u[1] * v[2] - u[2] * v[1];
        o[1] = u[2] * v[0] - u[0] * v[2];
        o[2] = u[0] * v[1] - u[1] * v[0];
    };
    auto dotv = [](const double u[3], const double v[3]) { return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]; };
    auto lenv = [](const double v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };

    double e[4][3];
    sub(1, 0, e[0]);
    sub(2, 1, e[1]);
    sub(3, 2, e[2]);
    sub(0, 3, e[3]);

    double cn[4][3];
    crossv(e[3], e[0], cn[0]);
    crossv(e[0], e[1], cn[1]);
    crossv(e[1], e[2], cn[2]);
    crossv(e[2], e[3], cn[3]);

    double pa0[3], pa1[3], nc[3];
    for (int k = 0; k < 3; ++k) { pa0[k] = e[0][k] - e[2][k]; }
    for (int k = 0; k < 3; ++k) { pa1[k] = e[1][k] - e[3][k]; }
    crossv(pa0, pa1, nc);
    const double nl = lenv(nc);
    if (nl > 0.0) { for (int k = 0; k < 3; ++k) { nc[k] /= nl; } }

    double sum = 0.0;
    for (int i = 0; i < 4; ++i) { sum += dotv(nc, cn[i]); }

    double area = 0.25 * sum;
    return (area > 0.0) ? area : 0.0;
}

double CellQualityMath::ComputeQuadAspectRatio(const std::vector<Point>& points) {
    auto sub = [&](int a, int b, double out[3]) {
        out[0] = static_cast<double>(points[a][0]) - points[b][0];
        out[1] = static_cast<double>(points[a][1]) - points[b][1];
        out[2] = static_cast<double>(points[a][2]) - points[b][2];
    };
    auto len = [](const double v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };
    auto cross = [](const double u[3], const double v[3], double out[3]) {
        out[0] = u[1] * v[2] - u[2] * v[1];
        out[1] = u[2] * v[0] - u[0] * v[2];
        out[2] = u[0] * v[1] - u[1] * v[0];
    };
    auto dot = [](const double u[3], const double v[3]) { return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]; };

    // 4 条边（Verdict make_quad_edges 的同一约定）
    double e[4][3];
    sub(1, 0, e[0]);
    sub(2, 1, e[1]);
    sub(3, 2, e[2]);
    sub(0, 3, e[3]);

    const double l0 = len(e[0]), l1 = len(e[1]), l2 = len(e[2]), l3 = len(e[3]);
    double ma = (l0 > l1) ? l0 : l1;
    double mb = (l2 > l3) ? l2 : l3;
    const double hm = (ma > mb) ? ma : mb;

    // signed_corner_areas：中心法向 · 各顶点角法向（未归一化叉积）
    double cn[4][3];
    cross(e[3], e[0], cn[0]);
    cross(e[0], e[1], cn[1]);
    cross(e[1], e[2], cn[2]);
    cross(e[2], e[3], cn[3]);

    double pa0[3], pa1[3], nc[3];
    for (int k = 0; k < 3; ++k) { pa0[k] = e[0][k] - e[2][k]; }
    for (int k = 0; k < 3; ++k) { pa1[k] = e[1][k] - e[3][k]; }
    cross(pa0, pa1, nc);
    const double nlen = len(nc);
    if (nlen <= 0.0) { return 1e30; }
    for (int k = 0; k < 3; ++k) { nc[k] /= nlen; }

    const double denom = dot(nc, cn[0]) + dot(nc, cn[1]) + dot(nc, cn[2]) + dot(nc, cn[3]);
    if (std::fabs(denom) < 1e-300) { return 1e30; }

    const double aspectRatio = hm * (l0 + l1 + l2 + l3) / denom;
    return (aspectRatio < 1e30) ? aspectRatio : 1e30;
}

double CellQualityMath::ComputeQuadCondition(const std::vector<Point>& points) {
    // 对齐 Verdict quad_condition（V_QuadMetric.cpp:1108-1162）：
    //   char_size = 包围盒对角线；areas[i] = signed_corner_areas[i] / char_size²
    //   对每个顶点 i：xxi = (P_i - P_{i+1})/cs，xet = (P_i - P_{i+3})/cs
    //                 condition_i = (|xxi|² + |xet|²) / areas[i]
    //   返回 max(condition_i) / 2
    // 旧实现误用"拆成两个三角形取 condition 最大值"，与该定义无关。
    double c[4][3];
    for (int i = 0; i < 4; ++i) { ToD(points[i], c[i]); }

    const double cs = ElemScaling<4>(c);
    double areas[4];
    SignedCornerAreas(c, areas);
    for (int i = 0; i < 4; ++i) { areas[i] /= (cs * cs); }

    double maxCond = 0.0;
    for (int i = 0; i < 4; ++i) {
        double xxi[3], xet[3];
        SubD(c[i], c[(i + 1) % 4], xxi);
        SubD(c[i], c[(i + 3) % 4], xet);
        for (int k = 0; k < 3; ++k) { xxi[k] /= cs; xet[k] /= cs; }

        double cond;
        if (areas[i] < kVerictDblMin) {
            cond = kVerictDblMax;
        } else {
            cond = (LenSqD(xxi) + LenSqD(xet)) / areas[i];
        }
        if (cond > maxCond) { maxCond = cond; }
    }
    if (maxCond >= kVerictDblMax) { return kVerictDblMax; }
    return maxCond / 2.0;
}

double CellQualityMath::ComputeQuadDistortion(const std::vector<Point>& points) {
    double c[4][3];
    for (int i = 0; i < 4; ++i) { ToD(points[i], c[i]); }

    // 2x2 高斯点（±1/√3），权重均为 1
    const double g = 1.0 / std::sqrt(3.0);
    double gp[4][2] = {{-g, -g}, {g, -g}, {g, g}, {-g, g}};

    // 面法向：严格照搬 Verdict quad_normal（V_QuadMetric.cpp:204-260）
    // 先取节点 0 与节点 2 的法向，同向则取平均；否则再用节点 1 判定。
    // 旧实现简单用两条对角线叉积，会让 distortion 系统性偏大约 0.1%。
    double faceNormal[3];
    {
        auto nodeNormalAt = [&](int i0, int i1, int i2, double out[3]) {
            double a[3], b[3];
            SubD(c[i1], c[i0], a);
            SubD(c[i2], c[i0], b);
            CrossD(a, b, out);
            const double l = LenD(out);
            if (l > kVerictDblMin) { for (int k = 0; k < 3; ++k) { out[k] /= l; } }
        };
        double n0[3], n2[3];
        nodeNormalAt(0, 1, 3, n0);
        nodeNormalAt(2, 3, 1, n2);

        if (DotD(n0, n2) > 0.0) {
            for (int k = 0; k < 3; ++k) { faceNormal[k] = 0.5 * (n0[k] + n2[k]); }
        } else {
            double n1[3];
            nodeNormalAt(1, 2, 0, n1);
            if (DotD(n0, n1) > 0.0) {
                for (int k = 0; k < 3; ++k) { faceNormal[k] = 0.5 * (n0[k] + n1[k]); }
            } else {
                for (int k = 0; k < 3; ++k) { faceNormal[k] = 0.5 * (n2[k] + n1[k]); }
            }
        }
    }

    // 形函数导数：dN/dxi, dN/det
    auto deriv = [](double xi, double et, double dNdxi[4], double dNdet[4]) {
        dNdxi[0] = -0.25 * (1 - et); dNdxi[1] = 0.25 * (1 - et);
        dNdxi[2] = 0.25 * (1 + et);  dNdxi[3] = -0.25 * (1 + et);
        dNdet[0] = -0.25 * (1 - xi); dNdet[1] = -0.25 * (1 + xi);
        dNdet[2] = 0.25 * (1 + xi);  dNdet[3] = 0.25 * (1 - xi);
    };

    // 1) element_area = Σ |aa × bb|
    double elementArea = 0.0;
    for (int k = 0; k < 4; ++k) {
        double dNdxi[4], dNdet[4];
        deriv(gp[k][0], gp[k][1], dNdxi, dNdet);
        double aa[3] = {0, 0, 0}, bb[3] = {0, 0, 0};
        for (int j = 0; j < 4; ++j) {
            for (int m = 0; m < 3; ++m) {
                aa[m] += dNdxi[j] * c[j][m];
                bb[m] += dNdet[j] * c[j][m];
            }
        }
        double cr[3];
        CrossD(aa, bb, cr);
        elementArea += LenD(cr); // 权重为 1
    }

    // 2) 节点法向
    // 注意：节点处要用【角点】自然坐标 (±1,±1)，
    // 不能用高斯点坐标 (±1/√3,±1/√3) —— 后者会让 distortion 整体偏大。
    const double kNodeXi[4] = {-1.0, 1.0, 1.0, -1.0};
    const double kNodeEt[4] = {-1.0, -1.0, 1.0, 1.0};
    double nodeNormal[4][3];
    for (int i = 0; i < 4; ++i) {
        double dNdxi[4], dNdet[4];
        deriv(kNodeXi[i], kNodeEt[i], dNdxi, dNdet);
        double aa[3] = {0, 0, 0}, bb[3] = {0, 0, 0};
        for (int j = 0; j < 4; ++j) {
            for (int m = 0; m < 3; ++m) {
                aa[m] += dNdxi[j] * c[j][m];
                bb[m] += dNdet[j] * c[j][m];
            }
        }
        CrossD(aa, bb, nodeNormal[i]);
        const double l = LenD(nodeNormal[i]);
        if (l > kVerictDblMin) { for (int m = 0; m < 3; ++m) { nodeNormal[i][m] /= l; } }
    }

    // 3) 是否平面
    bool flat = true;
    for (int i = 0; i < 4; ++i) {
        if (std::fabs(DotD(nodeNormal[0], nodeNormal[i])) < 0.99) { flat = false; break; }
    }

    const double thickness = 0.001 * std::sqrt(elementArea);
    const double zl0 = flat ? 0.0 : 0.5773502691896;
    const int nz = flat ? 1 : 2;

    double distortion = kVerictDblMax;
    // 4a) 遍历高斯点
    for (int k = 0; k < 4; ++k) {
        double zl = zl0;
        for (int iz = 0; iz < nz; ++iz) {
            zl = -zl;
            const double tz = zl * thickness / 2.0;
            double dNdxi[4], dNdet[4];
            deriv(gp[k][0], gp[k][1], dNdxi, dNdet);
            double aa[3] = {0, 0, 0}, bb[3] = {0, 0, 0}, cc[3] = {0, 0, 0};
            for (int j = 0; j < 4; ++j) {
                double xin[3] = {c[j][0] + tz * nodeNormal[j][0],
                                 c[j][1] + tz * nodeNormal[j][1],
                                 c[j][2] + tz * nodeNormal[j][2]};
                for (int m = 0; m < 3; ++m) {
                    aa[m] += dNdxi[j] * xin[m];
                    bb[m] += dNdet[j] * xin[m];
                    cc[m] += (dNdxi[j] * 0.0) * 0.0; // 高斯点处 thickness_gauss = 形函数*thickness/2
                }
            }
            // Verdict 在高斯点循环里：thickness_gauss = shape_function[ife][ja]*thickness/2
            double shape[4];
            const double xi = gp[k][0], et = gp[k][1];
            shape[0] = 0.25 * (1 - xi) * (1 - et);
            shape[1] = 0.25 * (1 + xi) * (1 - et);
            shape[2] = 0.25 * (1 + xi) * (1 + et);
            shape[3] = 0.25 * (1 - xi) * (1 + et);
            for (int j = 0; j < 4; ++j) {
                const double tg = shape[j] * thickness / 2.0;
                for (int m = 0; m < 3; ++m) { cc[m] += tg * nodeNormal[j][m]; }
            }

            double nAtPt[3];
            CrossD(aa, bb, nAtPt);
            const double sign = (DotD(faceNormal, nAtPt) > 0.0) ? 1.0 : -1.0;
            const double distrt = sign * DotD(cc, nAtPt);
            if (distrt < distortion) { distortion = distrt; }
        }
    }
    // 4b) 遍历节点
    for (int ja = 0; ja < 4; ++ja) {
        double zl = zl0;
        for (int iz = 0; iz < nz; ++iz) {
            zl = -zl;
            const double tz = zl * thickness / 2.0;
            double aa[3] = {0, 0, 0}, bb[3] = {0, 0, 0}, cc[3] = {0, 0, 0};
            for (int jai = 0; jai < 4; ++jai) {
                double dNdxi[4], dNdet[4];
                // 节点处必须用角点自然坐标 (±1,±1)，不是高斯点坐标
                deriv(kNodeXi[ja], kNodeEt[ja], dNdxi, dNdet);
                double xin[3] = {c[jai][0] + tz * nodeNormal[ja][0],
                                 c[jai][1] + tz * nodeNormal[ja][1],
                                 c[jai][2] + tz * nodeNormal[ja][2]};
                for (int m = 0; m < 3; ++m) {
                    aa[m] += dNdxi[jai] * xin[m];
                    bb[m] += dNdet[jai] * xin[m];
                }
                const double tg = (jai == ja) ? thickness / 2.0 : 0.0;
                for (int m = 0; m < 3; ++m) { cc[m] += tg * nodeNormal[jai][m]; }
            }
            double nAtPt[3];
            CrossD(aa, bb, nAtPt);
            const double sign = (DotD(faceNormal, nAtPt) > 0.0) ? 1.0 : -1.0;
            const double distrt = sign * DotD(cc, nAtPt);
            if (distrt < distortion) { distortion = distrt; }
        }
    }

    if (elementArea * thickness != 0.0) {
        distortion *= 8.0 / (elementArea * thickness);
    } else {
        distortion *= 8.0;
    }
    return distortion;
}

double CellQualityMath::ComputeQuadEdgeRatio(const std::vector<Point>& points) {
    std::vector<double> v;
    v.push_back((points[1] - points[0]).norm());
    v.push_back((points[2] - points[1]).norm());
    v.push_back((points[3] - points[2]).norm());
    v.push_back((points[0] - points[3]).norm());

    return *std::max_element(v.begin(), v.end()) / *std::min_element(v.begin(), v.end());
}

double CellQualityMath::ComputeQuadEquiangleSkew(const std::vector<Point>& points) {
    double c[4][3];
    for (int i = 0; i < 4; ++i) { ToD(points[i], c[i]); }
    double mm[2];
    QuadMinMaxAngle(c, mm);
    const double skewMax = (mm[1] - 90.0) / 90.0;
    const double skewMin = (90.0 - mm[0]) / 90.0;
    return (skewMax > skewMin) ? skewMax : skewMin;
}

double CellQualityMath::ComputeQuadJacobian(const std::vector<Point>& points) {
    // 对齐 Verdict quad_jacobian（V_QuadMetric.cpp:1169-1186）：
    //   jacobian = min(0..3) of [ 中心法向 · 各顶点角法向 ]
    //   顶点角法向是【未归一化】的叉积（等于 2 倍三角形面积）。
    // 旧实现对角法向多做了 .normalized()，于是平面四边形的结果恒为 ±1，
    // 丢了面积量纲：2x1 矩形应得 2.0，旧实现得 1.0。
    const Point e0 = points[1] - points[0];
    const Point e1 = points[2] - points[1];
    const Point e2 = points[3] - points[2];
    const Point e3 = points[0] - points[3];

    const Point cn0 = e3.cross(e0);
    const Point cn1 = e0.cross(e1);
    const Point cn2 = e1.cross(e2);
    const Point cn3 = e2.cross(e3);

    const Point pa0 = e0 - e2;
    const Point pa1 = e1 - e3;
    Point n_c = pa0.cross(pa1);
    const double nLen = n_c.norm();
    if (nLen < 1e-30) { return 0.0; } // 退化四边形
    n_c = n_c / nLen;

    const double a0 = n_c.dot(cn0);
    const double a1 = n_c.dot(cn1);
    const double a2 = n_c.dot(cn2);
    const double a3 = n_c.dot(cn3);

    return std::min(std::min(a0, a1), std::min(a2, a3));
}

double CellQualityMath::ComputeQuadMinAngle(const std::vector<Point>& points) {
    auto sub = [&](int a, int b, double out[3]) {
        out[0] = static_cast<double>(points[a][0]) - points[b][0];
        out[1] = static_cast<double>(points[a][1]) - points[b][1];
        out[2] = static_cast<double>(points[a][2]) - points[b][2];
    };
    double e[4][3];
    sub(1, 0, e[0]);
    sub(2, 1, e[1]);
    sub(3, 2, e[2]);
    sub(0, 3, e[3]);

    double mn = 360.0;
    for (int i = 0; i < 4; ++i) {
        const int j = (i + 1) % 4;
        const double li = std::sqrt(e[i][0] * e[i][0] + e[i][1] * e[i][1] + e[i][2] * e[i][2]);
        const double lj = std::sqrt(e[j][0] * e[j][0] + e[j][1] * e[j][1] + e[j][2] * e[j][2]);
        if (li <= 0.0 || lj <= 0.0) { continue; }
        const double d = e[i][0] * e[j][0] + e[i][1] * e[j][1] + e[i][2] * e[j][2];
        const double angle = std::acos(std::clamp(-d / (li * lj), -1.0, 1.0)) * 180.0 / PI;
        if (angle < mn) { mn = angle; }
    }
    return mn;
}

double CellQualityMath::ComputeQuadOddy(const std::vector<Point>& points) {
    double c[4][3];
    for (int i = 0; i < 4; ++i) { ToD(points[i], c[i]); }
    const double cs = ElemScaling<4>(c);
    for (int i = 0; i < 4; ++i) { for (int k = 0; k < 3; ++k) { c[i][k] /= cs; } }

    double maxOddy = 0.0;
    for (int i = 0; i < 4; ++i) {
        double f[3], s[3];
        SubD(c[i], c[(i + 1) % 4], f);
        SubD(c[i], c[(i + 3) % 4], s);

        const double g11 = DotD(f, f);
        const double g12 = DotD(f, s);
        const double g22 = DotD(s, s);
        const double g = g11 * g22 - g12 * g12;

        double cur;
        if (g < kVerictDblMin) {
            cur = kVerictDblMax;
        } else {
            cur = ((g11 - g22) * (g11 - g22) + 4.0 * g12 * g12) / (2.0 * g);
        }
        if (cur > maxOddy) { maxOddy = cur; }
    }
    return (maxOddy > 0.0) ? std::min(maxOddy, kVerictDblMax) : std::max(maxOddy, -kVerictDblMax);
}

double CellQualityMath::ComputeQuadRadiusRatio(const std::vector<Point>& points) {
    double c[4][3];
    for (int i = 0; i < 4; ++i) { ToD(points[i], c[i]); }
    const double cs = ElemScaling<4>(c);

    double e[4][3];
    MakeQuadEdges(c, e);
    for (int i = 0; i < 4; ++i) { for (int k = 0; k < 3; ++k) { e[i][k] /= cs; } }

    const double a2 = LenSqD(e[0]), b2 = LenSqD(e[1]), c2 = LenSqD(e[2]), d2 = LenSqD(e[3]);

    double dg[3];
    SubD(c[2], c[0], dg);
    for (int k = 0; k < 3; ++k) { dg[k] /= cs; }
    const double m2 = LenSqD(dg);

    SubD(c[3], c[1], dg);
    for (int k = 0; k < 3; ++k) { dg[k] /= cs; }
    const double n2 = LenSqD(dg);

    double t0 = (a2 > b2) ? a2 : b2;
    double t1 = (c2 > d2) ? c2 : d2;
    double t2 = (m2 > n2) ? m2 : n2;
    double h2 = (t0 > t1) ? t0 : t1;
    h2 = (h2 > t2) ? h2 : t2;

    double ab[3], bc[3], cd[3], da[3];
    CrossD(e[0], e[1], ab);
    CrossD(e[1], e[2], bc);
    CrossD(e[2], e[3], cd);
    CrossD(e[3], e[0], da);

    t0 = LenD(da);
    t1 = LenD(ab);
    t2 = LenD(bc);
    const double t3 = LenD(cd);
    t0 = (t0 < t1) ? t0 : t1;
    t2 = (t2 < t3) ? t2 : t3;
    t0 = (t0 < t2) ? t0 : t2;

    if (t0 < kVerictDblMin) { return kVerictDblMax; }
    const double coeff = 1.0 / (2.0 * std::sqrt(2.0));
    const double rr = coeff * std::sqrt((a2 + b2 + c2 + d2) * h2) / t0;
    return (rr > 0.0) ? std::min(rr, kVerictDblMax) : std::max(rr, -kVerictDblMax);
}

double CellQualityMath::ComputeQuadScaledJacobian(const std::vector<Point>& points) {
    double c[4][3];
    for (int i = 0; i < 4; ++i) { ToD(points[i], c[i]); }

    double ca[4];
    SignedCornerAreas(c, ca);

    double e[4][3];
    MakeQuadEdges(c, e);
    const double cs = ElemScaling<4>(c);
    for (int i = 0; i < 4; ++i) { ca[i] /= (cs * cs); }
    for (int i = 0; i < 4; ++i) { for (int k = 0; k < 3; ++k) { e[i][k] /= cs; } }

    const double l[4] = {LenD(e[0]), LenD(e[1]), LenD(e[2]), LenD(e[3])};
    if (l[0] < kVerictDblMin || l[1] < kVerictDblMin || l[2] < kVerictDblMin || l[3] < kVerictDblMin) {
        return 0.0;
    }

    double mn = ca[0] / (l[0] * l[3]);
    const double s1 = ca[1] / (l[1] * l[0]);
    const double s2 = ca[2] / (l[2] * l[1]);
    const double s3 = ca[3] / (l[3] * l[2]);
    if (s1 < mn) { mn = s1; }
    if (s2 < mn) { mn = s2; }
    if (s3 < mn) { mn = s3; }
    return (mn > 0.0) ? std::min(mn, kVerictDblMax) : std::max(mn, -kVerictDblMax);
}

double CellQualityMath::ComputeQuadShape(const std::vector<Point>& points) {
    double c[4][3];
    for (int i = 0; i < 4; ++i) { ToD(points[i], c[i]); }

    double ca[4];
    SignedCornerAreas(c, ca);

    double e[4][3];
    MakeQuadEdges(c, e);
    const double cs = ElemScaling<4>(c);
    for (int i = 0; i < 4; ++i) { ca[i] /= (cs * cs); }
    // Verdict 的 apply_elem_scaling_on_edges 会就地把 edges 也除以 char_size，
    // 所以下面的 length_squared 是【归一化后】的边平方。少除这一步会差两个数量级。
    for (int i = 0; i < 4; ++i) { for (int k = 0; k < 3; ++k) { e[i][k] /= cs; } }

    const double ls[4] = {LenSqD(e[0]), LenSqD(e[1]), LenSqD(e[2]), LenSqD(e[3])};
    if (ls[0] <= kVerictDblMin || ls[1] <= kVerictDblMin || ls[2] <= kVerictDblMin || ls[3] <= kVerictDblMin) {
        return 0.0;
    }

    double mn = ca[0] / (ls[0] + ls[3]);
    const double s1 = ca[1] / (ls[1] + ls[0]);
    const double s2 = ca[2] / (ls[2] + ls[1]);
    const double s3 = ca[3] / (ls[3] + ls[2]);
    if (s1 < mn) { mn = s1; }
    if (s2 < mn) { mn = s2; }
    if (s3 < mn) { mn = s3; }
    mn *= 2.0;
    if (mn < kVerictDblMin) { mn = 0.0; }
    return (mn > 0.0) ? std::min(mn, kVerictDblMax) : std::max(mn, -kVerictDblMax);
}

double CellQualityMath::ComputeQuadShear(const std::vector<Point>& points) {
    const double sj = ComputeQuadScaledJacobian(points);
    return (sj <= kVerictDblMin) ? 0.0 : std::min(sj, kVerictDblMax);
}

double CellQualityMath::ComputeQuadSkew(const std::vector<Point>& points) {
    Point x_1 = (points[1] - points[0]) + (points[2] - points[3]);
    Point x_2 = (points[2] - points[1]) + (points[3] - points[0]);
    Point x_1_norm = x_1 / x_1.norm();
    Point x_2_norm = x_2 / x_2.norm();

    return std::abs(x_1_norm * x_2_norm);
}

double CellQualityMath::ComputeQuadStretch(const std::vector<Point>& points) {
    // 包围盒对角线（Verdict elem_scaling）
    Point bmin = points[0];
    Point bmax = points[0];
    for (int i = 1; i < 4; ++i) {
        for (int k = 0; k < 3; ++k) {
            if (points[i][k] < bmin[k]) { bmin[k] = points[i][k]; }
            if (points[i][k] > bmax[k]) { bmax[k] = points[i][k]; }
        }
    }
    double charSize = (bmax - bmin).norm();
    if (charSize < 1e-30) { charSize = 1.0; }

    const double inv = 1.0 / charSize;
    const Point e0 = (points[1] - points[0]) * inv;
    const Point e1 = (points[2] - points[1]) * inv;
    const Point e2 = (points[3] - points[2]) * inv;
    const Point e3 = (points[0] - points[3]) * inv;

    const double l0 = e0.squaredNorm();
    const double l1 = e1.squaredNorm();
    const double l2 = e2.squaredNorm();
    const double l3 = e3.squaredNorm();

    const double d02 = ((points[2] - points[0]) * inv).squaredNorm();
    const double d13 = ((points[3] - points[1]) * inv).squaredNorm();
    const double diag = (d02 > d13) ? d02 : d13;

    if (diag < 1e-300) { return 0.0; }

    double minLen2 = (l0 < l1) ? l0 : l1;
    const double m23 = (l2 < l3) ? l2 : l3;
    if (m23 < minLen2) { minLen2 = m23; }

    return std::sqrt(2.0) * std::sqrt(minLen2 / diag);
}

double CellQualityMath::ComputeQuadTaper(const std::vector<Point>& points) {
    Point x_1 = (points[1] - points[0]) + (points[2] - points[3]);
    Point x_2 = (points[2] - points[1]) + (points[3] - points[0]);
    Point x_12 = (points[0] - points[1]) + (points[2] - points[3]);

    return x_12.norm() / std::min(x_1.norm(), x_2.norm());
}

double CellQualityMath::ComputeQuadWarpage(const std::vector<Point>& points) {
    double c[4][3];
    for (int i = 0; i < 4; ++i) { ToD(points[i], c[i]); }

    double e[4][3];
    MakeQuadEdges(c, e);

    auto norm3 = [](double v[3]) {
        const double l = LenD(v);
        if (l < kVerictDblMin) { return false; }
        for (int k = 0; k < 3; ++k) { v[k] /= l; }
        return true;
    };

    double n[4][3];
    CrossD(e[3], e[0], n[0]);
    CrossD(e[0], e[1], n[1]);
    CrossD(e[1], e[2], n[2]);
    CrossD(e[2], e[3], n[3]);

    for (int i = 0; i < 4; ++i) {
        if (!norm3(n[i])) { return kVerictDblMin; }
    }

    const double d02 = DotD(n[0], n[2]);
    const double d13 = DotD(n[1], n[3]);
    double warpage = (d02 < d13) ? d02 : d13;
    warpage = warpage * warpage * warpage;

    return (warpage > 0.0) ? std::min(warpage, kVerictDblMax) : std::max(warpage, -kVerictDblMax);
}

double CellQualityMath::ComputeQuadRelativeSizeSquared(const std::vector<Point>& points,
                                                                double averageArea) {
    if (averageArea <= kVerictDblMin) { return 0.0; }
    const double area = ComputeQuadArea(points); // 已对齐 Verdict quad_area
    double rel = area / averageArea;
    if (rel > kVerictDblMin) {
        rel = std::min(rel, 1.0 / rel);
        rel = rel * rel;
        return rel;
    }
    return 0.0;
}

double CellQualityMath::ComputeQuadShapeAndSize(const std::vector<Point>& points,
                                                         double averageArea) {
    const double shape = ComputeQuadShape(points);
    const double size = ComputeQuadRelativeSizeSquared(points, averageArea);
    const double v = shape * size;
    return (v > 0.0) ? std::min(v, kVerictDblMax) : std::max(v, -kVerictDblMax);
}

double CellQualityMath::ComputeQuadShearAndSize(const std::vector<Point>& points,
                                                         double averageArea) {
    const double shear = ComputeQuadShear(points);
    const double size = ComputeQuadRelativeSizeSquared(points, averageArea);
    const double v = shear * size;
    return (v > 0.0) ? std::min(v, kVerictDblMax) : std::max(v, -kVerictDblMax);
}


// ==================== 四面体指标 ====================
double CellQualityMath::ComputeTetEdgeRatio(const std::vector<Point>& points) {
    std::vector<double> result = GetMinAndMaxLenOfCell(points);
    return result[1] / result[0];
}

double CellQualityMath::ComputeTetVolume(const std::vector<Point>& points) {
    std::vector<Point> L(3);
    L[0] = points[1] - points[0];
    L[1] = points[2] - points[0];
    L[2] = points[3] - points[0];
    double volume = std::abs(L[0].cross(L[1]).dot(L[2]) / 6.0);

    return volume;
}

double CellQualityMath::GetAspectRatioOfCell(const std::vector<Point>& points) {
    // 对齐 Verdict tet_aspect_ratio（V_TetMetric.cpp:760-811）：
    //   hm = 最长棱；A/B/C/D = 4 个面的面积（叉积模长的一半，但 Verdict 未除 2）
    //   aspect_ratio = (sqrt(6)/12) * hm * (A+B+C+D) / |detTet|
    // Verdict 会先做 elem_scaling（包围盒对角线归一化），但这是纯比值运算，
    // char_size 在 hm、A/B/C/D、detTet 中按 1、1、2、3 次幂出现，
    // 归一化后整体不变，因此这里直接在原始尺度上计算。
    // 旧实现用 maxLength/(2*sqrt(6)*inradius)，是另一个量，与 VTK 差异达 601/2706。
    // 全程 double：侧边差与叉积都在 double 下完成。
    // 若在 Vector3f 下做叉积，百万级长宽比的极端退化单元会偏差 ~1%
    // （实测 sphere_ug 上 26/2706 个单元不符，最大绝对偏差 9.4e5）。
    auto dbl = [&](int i, int k) { return static_cast<double>(points[i][k]); };
    auto sub = [&](int a, int b, double out[3]) {
        out[0] = dbl(a, 0) - dbl(b, 0);
        out[1] = dbl(a, 1) - dbl(b, 1);
        out[2] = dbl(a, 2) - dbl(b, 2);
    };
    auto crossLen = [](const double u[3], const double v[3]) {
        const double x = u[1] * v[2] - u[2] * v[1];
        const double y = u[2] * v[0] - u[0] * v[2];
        const double z = u[0] * v[1] - u[1] * v[0];
        return std::sqrt(x * x + y * y + z * z);
    };
    auto sq = [](const double v[3]) { return v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; };

    double ab[3], ac[3], ad[3], bc[3], cd[3];
    sub(1, 0, ab);
    sub(2, 0, ac);
    sub(3, 0, ad);
    sub(2, 1, bc);
    sub(3, 2, cd);

    const double ab2 = sq(ab), ac2 = sq(ac), ad2 = sq(ad);
    const double bc2 = sq(bc), bd2 = 0, cd2 = sq(cd);
    (void) bd2;

    double A = (ab2 > bc2) ? ab2 : bc2;
    double B = (ac2 > ad2) ? ac2 : ad2;
    double C = (ad2 > cd2) ? ad2 : cd2; // 这里与 Verdict 的 bd2/cd2 取舍保持一致见下
    double D = (A > B) ? A : B;
    // Verdict 原文：C = max(bd2, cd2)。bd2 未在上方算出，补算以严格对齐。
    double bd[3];
    sub(3, 1, bd);
    C = (sq(bd) > cd2) ? sq(bd) : cd2;
    const double hm = (D > C) ? std::sqrt(D) : std::sqrt(C);

    const double detTet = ab[0] * (ac[1] * ad[2] - ac[2] * ad[1])
                        - ab[1] * (ac[0] * ad[2] - ac[2] * ad[0])
                        + ab[2] * (ac[0] * ad[1] - ac[1] * ad[0]);
    if (std::fabs(detTet) < 1e-300) { return 1e30; } // Verdict 的 VERDICT_DBL_MAX 哨兵

    const double sA = crossLen(ab, bc);
    const double sB = crossLen(ab, ad);
    const double sC = crossLen(ac, ad);
    const double sD = crossLen(bc, cd);

    const double aspectRatio = (std::sqrt(6.0) / 12.0) * hm * (sA + sB + sC + sD) / std::fabs(detTet);
    return (aspectRatio < 1e30) ? aspectRatio : 1e30;
}

double CellQualityMath::GetJacobianOfCell(const std::vector<Point>& points) {
    const double s0x = static_cast<double>(points[1][0]) - points[0][0];
    const double s0y = static_cast<double>(points[1][1]) - points[0][1];
    const double s0z = static_cast<double>(points[1][2]) - points[0][2];

    const double s2x = static_cast<double>(points[0][0]) - points[2][0];
    const double s2y = static_cast<double>(points[0][1]) - points[2][1];
    const double s2z = static_cast<double>(points[0][2]) - points[2][2];

    const double s3x = static_cast<double>(points[3][0]) - points[0][0];
    const double s3y = static_cast<double>(points[3][1]) - points[0][1];
    const double s3z = static_cast<double>(points[3][2]) - points[0][2];

    // side2 × side0
    const double cx = s2y * s0z - s2z * s0y;
    const double cy = s2z * s0x - s2x * s0z;
    const double cz = s2x * s0y - s2y * s0x;

    return s3x * cx + s3y * cy + s3z * cz;
}

double CellQualityMath::GetCollapseRatioOfCell(const std::vector<Point>& points) {
    double volume = ComputeTetVolume(points);

    double CollapseRatio = GetCollapseRatioOfVertex(points[1], points[2], points[3], volume);

    CollapseRatio = std::min(CollapseRatio, GetCollapseRatioOfVertex(points[0], points[2], points[3], volume));
    CollapseRatio = std::min(CollapseRatio, GetCollapseRatioOfVertex(points[1], points[0], points[3], volume));
    CollapseRatio = std::min(CollapseRatio, GetCollapseRatioOfVertex(points[1], points[2], points[0], volume));
    return CollapseRatio;
}

double CellQualityMath::GetMinDihedralAngleOfTet(const std::vector<Point>& points) {
    // 全程用 double：侧边差、叉积、点积都在 double 下完成。
    // 若在 Vector3f 下做叉积，极小角（<0.001°）会因抵消误差被放大到几十倍
    // （实测 sphere_ug 上 435/2706 个单元不符，最大绝对偏差 0.0235）。
    auto dbl = [&](int i, int k) { return static_cast<double>(points[i][k]); };

    const double abx = dbl(1, 0) - dbl(0, 0), aby = dbl(1, 1) - dbl(0, 1), abz = dbl(1, 2) - dbl(0, 2);
    const double adx = dbl(3, 0) - dbl(0, 0), ady = dbl(3, 1) - dbl(0, 1), adz = dbl(3, 2) - dbl(0, 2);
    const double bcx = dbl(2, 0) - dbl(1, 0), bcy = dbl(2, 1) - dbl(1, 1), bcz = dbl(2, 2) - dbl(1, 2);
    const double cdx = dbl(3, 0) - dbl(2, 0), cdy = dbl(3, 1) - dbl(2, 1), cdz = dbl(3, 2) - dbl(2, 2);

    // 4 个面的法向（未归一化叉积）
    const double abc[3] = {aby * bcz - abz * bcy, abz * bcx - abx * bcz, abx * bcy - aby * bcx};
    const double abd[3] = {aby * adz - abz * ady, abz * adx - abx * adz, abx * ady - aby * adx};
    const double acd[3] = {ady * cdz - adz * cdy, adz * cdx - adx * cdz, adx * cdy - ady * cdx};
    const double bcd[3] = {bcy * cdz - bcz * cdy, bcz * cdx - bcx * cdz, bcx * cdy - bcy * cdx};

    auto len = [](const double v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };
    auto dot = [](const double a[3], const double b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };

    const double nabc = len(abc), nabd = len(abd), nacd = len(acd), nbcd = len(bcd);

    // 严格复现 Verdict 的数值行为（V_TetMetric.cpp:957-969）：
    //   1) 立刻 acos，【不做 clamp】。退化单元上 (u·v)/(|u||v|) 会略微越出 [-1,1]，
    //      此时 acos 返回 NaN；Verdict 的 min 链是 `a = a < b ? a : b` 形式，
    //      遇到 NaN 会走 else 分支把它丢掉。
    //   2) 不做法向模长为 0 的提前返回：薄片四面体（体积 ~1e-12）的基准值
    //      是 179.9999991（由非 NaN 的那个二面角给出），提前返回 0 会错。
    // 早期实现同时做了 clamp 和提前返回，在 holdout_sphere 上 2/368 个单元不符。
    double alpha = std::acos(dot(abc, abd) / (nabc * nabd));
    double beta = std::acos(dot(abc, acd) / (nabc * nacd));
    double gamma = std::acos(dot(abc, bcd) / (nabc * nbcd));
    double delta = std::acos(dot(abd, acd) / (nabd * nacd));
    double epsilon = std::acos(dot(abd, bcd) / (nabd * nbcd));
    double zeta = std::acos(dot(acd, bcd) / (nacd * nbcd));

    double mn = alpha;
    mn = mn < beta ? mn : beta;
    mn = mn < gamma ? mn : gamma;
    mn = mn < delta ? mn : delta;
    mn = mn < epsilon ? mn : epsilon;
    mn = mn < zeta ? mn : zeta;
    mn *= 180.0 / PI;

    // Verdict 的 fix_range：NaN → DBL_MAX，其余原样返回
    if (std::isnan(mn)) { return 1e30; }
    return mn;
}

double CellQualityMath::ComputeTetCondition(const std::vector<Point>& points) {
    double c[4][3];
    ToD8(points, 4, c);
    const double cs = ScaleOf<4>(c);

    double side0[3], side2[3], side3[3];
    SubV(c[1], c[0], side0);
    // 注意方向：Verdict 写的是 VerdictVector side2{coordinates[2], coordinates[0]}，
    // 即从 P2 指向 P0，结果是 P0 - P2（不是 P2 - P0）。写反会让 condition 差 ~11%。
    SubV(c[0], c[2], side2);
    SubV(c[3], c[0], side3);
    for (int k = 0; k < 3; ++k) { side0[k] /= cs; side2[k] /= cs; side3[k] /= cs; }

    const double s3 = std::sqrt(3.0), s6 = std::sqrt(6.0);
    double c1[3], c2[3], c3[3];
    for (int k = 0; k < 3; ++k) {
        c1[k] = side0[k];
        c2[k] = (-2.0 * side2[k] - side0[k]) / s3;
        c3[k] = (3.0 * side3[k] + side2[k] - side0[k]) / s6;
    }

    const double term1 = DotV(c1, c1) + DotV(c2, c2) + DotV(c3, c3);
    double x12[3], x23[3], x13[3];
    CrossV(c1, c2, x12);
    CrossV(c2, c3, x23);
    CrossV(c1, c3, x13);
    const double term2 = DotV(x12, x12) + DotV(x23, x23) + DotV(x13, x13);

    double cr[3];
    CrossV(c2, c3, cr);
    const double det = DotV(c1, cr);
    if (std::fabs(det) <= kVdMin) { return kVdMax; }

    return std::sqrt(term1) * std::sqrt(term2) / (3.0 * det);
}

double CellQualityMath::ComputeTetDistortion(const std::vector<Point>& points) {
    (void) points;
    return 1.0; // Verdict：直边四面体即目标形状
}

double CellQualityMath::ComputeTetEquiangleSkew(const std::vector<Point>& points) {
    double c[4][3];
    ToD8(points, 4, c);

    auto unit = [&](int from, int to, double out[3]) {
        SubV(c[to], c[from], out);
        const double l = LenV(out);
        if (l > kVdMin) { for (int k = 0; k < 3; ++k) { out[k] /= l; } }
    };

    double ab[3], ac[3], ad[3], bc[3], bd[3], cd[3];
    unit(0, 1, ab);
    unit(0, 2, ac);
    unit(0, 3, ad);
    unit(1, 2, bc);
    unit(1, 3, bd);
    unit(2, 3, cd);

    auto unitCross = [](const double u[3], const double v[3], double out[3]) {
        CrossV(u, v, out);
        const double l = LenV(out);
        if (l > kVdMin) { for (int k = 0; k < 3; ++k) { out[k] /= l; } }
    };

    double abc[3], abd[3], acd[3], bcd[3];
    unitCross(bc, ab, abc);
    unitCross(ab, ad, abd);
    unitCross(cd, ad, acd);
    unitCross(bc, cd, bcd);

    const double kRad2Deg = 180.0 / 3.14159265358979323846;
    auto acosClamped = [&](double x) { return std::acos(std::clamp(x, -1.0, 1.0)); };

    // A) 6 个二面角
    const double alpha = acosClamped(-DotV(abc, abd));
    const double beta = acosClamped(-DotV(abc, acd));
    const double gamma = acosClamped(-DotV(abc, bcd));
    const double delta = acosClamped(-DotV(abd, acd));
    const double epsilon = acosClamped(-DotV(abd, bcd));
    const double zeta = acosClamped(-DotV(acd, bcd));

    double dihMin = alpha;
    dihMin = std::min(dihMin, beta);
    dihMin = std::min(dihMin, gamma);
    dihMin = std::min(dihMin, delta);
    dihMin = std::min(dihMin, epsilon);
    dihMin = std::min(dihMin, zeta);
    double dihMax = alpha;
    dihMax = std::max(dihMax, beta);
    dihMax = std::max(dihMax, gamma);
    dihMax = std::max(dihMax, delta);
    dihMax = std::max(dihMax, epsilon);
    dihMax = std::max(dihMax, zeta);

    dihMin *= kRad2Deg;
    dihMax *= kRad2Deg;
    const double theta = std::acos(1.0 / 3.0) * kRad2Deg; // 70.5288

    const double dihedralSkewMax = (dihMax - theta) / (180.0 - theta);
    const double dihedralSkewMin = (theta - dihMin) / theta;

    // B) 12 个面内角（符号模式照抄 Verdict V_TetMetric.cpp:129-140）
    const double angles[12] = {
            acosClamped(-DotV(ab, bc)), acosClamped(DotV(bc, ac)), acosClamped(DotV(ac, ab)),
            acosClamped(-DotV(ab, bd)), acosClamped(DotV(bd, ad)), acosClamped(DotV(ad, ab)),
            acosClamped(-DotV(bc, cd)), acosClamped(DotV(cd, bd)), acosClamped(DotV(bd, bc)),
            acosClamped(DotV(ad, cd)), acosClamped(-DotV(cd, ac)), acosClamped(DotV(ac, ad))};

    double faceMin = 360.0, faceMax = 0.0;
    for (int i = 0; i < 12; ++i) {
        if (angles[i] < faceMin) { faceMin = angles[i]; }
        if (angles[i] > faceMax) { faceMax = angles[i]; }
    }
    faceMin *= kRad2Deg;
    faceMax *= kRad2Deg;

    const double skewMax = (faceMax - 60.0) / 120.0;
    const double skewMin = (60.0 - faceMin) / 60.0;

    double maxSkew = dihedralSkewMin;
    maxSkew = std::max(maxSkew, dihedralSkewMax);
    maxSkew = std::max(maxSkew, skewMin);
    maxSkew = std::max(maxSkew, skewMax);
    return maxSkew;
}

double CellQualityMath::ComputeTetRadiusRatio(const std::vector<Point>& points) {
    double c[4][3];
    ToD8(points, 4, c);
    const double cs = ScaleOf<4>(c);

    double s[6][3];
    SubV(c[1], c[0], s[0]);
    SubV(c[2], c[1], s[1]);
    SubV(c[0], c[2], s[2]);
    SubV(c[3], c[0], s[3]);
    SubV(c[3], c[1], s[4]);
    SubV(c[3], c[2], s[5]);
    for (int i = 0; i < 6; ++i) { for (int k = 0; k < 3; ++k) { s[i][k] /= cs; } }

    double t1[3], t2[3], t3[3];
    CrossV(s[2], s[0], t1);
    CrossV(s[3], s[0], t2);
    CrossV(s[3], s[2], t3);

    double numer[3];
    const double l3sq = LenSqV(s[3]), l2sq = LenSqV(s[2]), l0sq = LenSqV(s[0]);
    for (int k = 0; k < 3; ++k) {
        numer[k] = l3sq * t1[k] + l2sq * t2[k] + l0sq * t3[k];
    }

    double cr1[3], cr2[3], cr3[3], cr4[3];
    CrossV(s[2], s[0], cr1);
    CrossV(s[3], s[0], cr2);
    CrossV(s[4], s[1], cr3);
    CrossV(s[3], s[2], cr4);
    const double areaSum = (LenV(cr1) + LenV(cr2) + LenV(cr3) + LenV(cr4)) * 0.5;

    // 体积（已归一化）
    const double vol = std::fabs(s[3][0] * (s[2][1] * s[0][2] - s[2][2] * s[0][1])
                               - s[3][1] * (s[2][0] * s[0][2] - s[2][2] * s[0][0])
                               + s[3][2] * (s[2][0] * s[0][1] - s[2][1] * s[0][0])) / 6.0;
    if (vol < kVdMin) { return kVdMax; }

    const double rr = LenV(numer) * areaSum / (108.0 * vol * vol);
    return (rr > 0.0) ? std::min(rr, kVdMax) : std::max(rr, -kVdMax);
}

double CellQualityMath::ComputeTetScaledJacobian(const std::vector<Point>& points) {
    double c[4][3];
    ToD8(points, 4, c);
    const double cs = ScaleOf<4>(c);

    double s[6][3];
    SubV(c[1], c[0], s[0]); // side0 = P1-P0
    SubV(c[2], c[1], s[1]); // side1 = P2-P1
    SubV(c[0], c[2], s[2]); // side2 = P0-P2
    SubV(c[3], c[0], s[3]); // side3 = P3-P0
    SubV(c[3], c[1], s[4]); // side4 = P3-P1
    SubV(c[3], c[2], s[5]); // side5 = P3-P2
    for (int i = 0; i < 6; ++i) { for (int k = 0; k < 3; ++k) { s[i][k] /= cs; } }

    double cr[3];
    CrossV(s[2], s[0], cr);
    const double jacobi = DotV(s[3], cr);

    const double l0 = LenSqV(s[0]), l1 = LenSqV(s[1]), l2 = LenSqV(s[2]);
    const double l3 = LenSqV(s[3]), l4 = LenSqV(s[4]), l5 = LenSqV(s[5]);

    const double ls[4] = {l0 * l2 * l3, l0 * l1 * l4, l1 * l2 * l5, l3 * l4 * l5};
    int which = 0;
    if (ls[1] > ls[which]) { which = 1; }
    if (ls[2] > ls[which]) { which = 2; }
    if (ls[3] > ls[which]) { which = 3; }

    double lengthProduct = std::sqrt(ls[which]);
    if (lengthProduct < std::fabs(jacobi)) { lengthProduct = std::fabs(jacobi); }

    if (lengthProduct < kVdMin) { return kVdMax; }

    return std::sqrt(2.0) * jacobi / lengthProduct;
}

double CellQualityMath::ComputeTetShape(const std::vector<Point>& points) {
    double c[4][3];
    ToD8(points, 4, c);
    const double cs = ScaleOf<4>(c);

    double e[3][3];
    SubV(c[1], c[0], e[0]);
    SubV(c[0], c[2], e[1]);
    SubV(c[3], c[0], e[2]);
    for (int i = 0; i < 3; ++i) { for (int k = 0; k < 3; ++k) { e[i][k] /= cs; } }

    double cr[3];
    CrossV(e[1], e[0], cr);
    const double jac = DotV(e[2], cr);
    if (jac < kVdMin) { return 0.0; }

    const double num = 3.0 * std::pow(std::sqrt(2.0) * jac, 2.0 / 3.0);
    // Verdict V_TetMetric.cpp:915-916 原文：
    //   1.5*(e0·e0 + e1·e1 + e2·e2) - (e0·(-e1) + (-e1)·e2 + e2·e0)
    const double den = 1.5 * (DotV(e[0], e[0]) + DotV(e[1], e[1]) + DotV(e[2], e[2]))
                       - (-DotV(e[0], e[1]) - DotV(e[1], e[2]) + DotV(e[2], e[0]));
    if (den < kVdMin) { return 0.0; }
    double shape = num / den;
    if (shape < 0.0) { shape = 0.0; }
    return std::min(shape, kVdMax);
}

double CellQualityMath::ComputeTetRelativeSizeSquared(const std::vector<Point>& points,
                                                              double averageVolume) {
    if (averageVolume < kVdMin) { return 0.0; }

    double c[4][3];
    ToD8(points, 4, c);

    double side2[3], side0[3], side3[3];
    SubV(c[1], c[0], side2); // P1-P0
    SubV(c[2], c[0], side0); // P2-P0
    SubV(c[3], c[0], side3); // P3-P0

    double cr[3];
    CrossV(side2, side0, cr);
    const double volume = DotV(side3, cr) / 6.0; // 有符号，与 Verdict 一致

    double size = volume / averageVolume;
    if (size <= kVdMin) { return 0.0; }
    if (size > 1.0) { size = 1.0 / size; }
    return size * size;
}

double CellQualityMath::ComputeTetShapeAndSize(const std::vector<Point>& points,
                                                       double averageVolume) {
    const double shape = ComputeTetShape(points);
    const double size = ComputeTetRelativeSizeSquared(points, averageVolume);
    const double v = shape * size;
    return (v > 0.0) ? std::min(v, kVdMax) : std::max(v, -kVdMax);
}


// ==================== 六面体指标 ====================
double CellQualityMath::ComputeHexVolume(const std::vector<Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    auto X_1 = (P_1 - P_0) + (P_2 - P_3) + (P_5 - P_4) + (P_6 - P_7);
    auto X_2 = (P_3 - P_0) + (P_2 - P_1) + (P_7 - P_4) + (P_6 - P_5);
    auto X_3 = (P_4 - P_0) + (P_5 - P_1) + (P_6 - P_2) + (P_7 - P_3);

    return X_1.dot(X_2.cross(X_3)) / 64.0;
}

double CellQualityMath::ComputeHexEdgeRatio(const std::vector<Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    // 严格对齐 Verdict 官方 hex_edge_ratio（V_HexMetric.cpp:714-827）。
    //
    // 12 条棱按 Verdict 的顺序排列：
    //   edges[0..3]  = 底面 (0-1, 1-2, 2-3, 3-0)
    //   edges[4..7]  = 顶面 (4-5, 5-6, 6-7, 7-4)
    //   edges[8..11] = 竖向 (0-4, 1-5, 2-6, 3-7)
    // Verdict 内部用"长度平方"比较，最后开根号。
    const double e2[12] = {
            (P_1 - P_0).squaredNorm(), (P_2 - P_1).squaredNorm(), (P_3 - P_2).squaredNorm(), (P_3 - P_0).squaredNorm(),
            (P_5 - P_4).squaredNorm(), (P_6 - P_5).squaredNorm(), (P_7 - P_6).squaredNorm(), (P_7 - P_4).squaredNorm(),
            (P_4 - P_0).squaredNorm(), (P_5 - P_1).squaredNorm(), (P_6 - P_2).squaredNorm(), (P_7 - P_3).squaredNorm()};

    // 6 组"对边"：每组取 min / max
    const double mab = (e2[0] < e2[1]) ? e2[0] : e2[1];
    const double Mab = (e2[0] < e2[1]) ? e2[1] : e2[0];
    const double mcd = (e2[2] < e2[3]) ? e2[2] : e2[3];
    const double Mcd = (e2[2] < e2[3]) ? e2[3] : e2[2];
    const double mef = (e2[4] < e2[5]) ? e2[4] : e2[5];
    const double Mef = (e2[4] < e2[5]) ? e2[5] : e2[4];
    const double mgh = (e2[6] < e2[7]) ? e2[6] : e2[7];
    const double Mgh = (e2[6] < e2[7]) ? e2[7] : e2[6];
    const double mij = (e2[8] < e2[9]) ? e2[8] : e2[9];
    const double Mij = (e2[8] < e2[9]) ? e2[9] : e2[8];
    const double mkl = (e2[10] < e2[11]) ? e2[10] : e2[11];
    const double Mkl = (e2[10] < e2[11]) ? e2[11] : e2[10];

    // 分母：全部 12 条棱的最小值（平方）
    double m2 = (mab < mcd) ? mab : mcd;
    m2 = (m2 < mef) ? m2 : mef;
    m2 = (m2 < mgh) ? m2 : mgh;
    m2 = (m2 < mij) ? m2 : mij;
    m2 = (m2 < mkl) ? m2 : mkl;
    if (m2 < 1e-300) { return 0.0; }

    // 分子：★ Verdict 的真实行为 —— 虽然第 810-814 行先算了全部 6 组的 max，
    // 但第 815-818 行随即用"只含前 3 组"的结果覆盖了它。
    // 必须复现这个覆盖，否则与 VTK 结果不一致（实测差异面达 6.26%）。
    double M2 = (Mab > Mcd) ? Mab : Mcd;
    M2 = (M2 > Mef) ? M2 : Mef;

    return std::sqrt(M2 / m2);
}

double CellQualityMath::ComputeHexJacobian(const std::vector<Point>& points) {
    //be caution about order

    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    float jacobian = (std::numeric_limits<float>::max)();
    float current_jacobian = 0.0;

    Vector3f L_0, L_2, L_3;

    L_0 = P_1 - P_0;
    L_2 = P_3 - P_0;
    L_3 = P_4 - P_0;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_2 - P_1;
    L_2 = P_0 - P_1;
    L_3 = P_5 - P_1;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_3 - P_2;
    L_2 = P_1 - P_2;
    L_3 = P_6 - P_2;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_0 - P_3;
    L_2 = P_2 - P_3;
    L_3 = P_7 - P_3;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_7 - P_4;
    L_2 = P_5 - P_4;
    L_3 = P_0 - P_4;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_4 - P_5;
    L_2 = P_6 - P_5;
    L_3 = P_1 - P_5;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_5 - P_6;
    L_2 = P_7 - P_6;
    L_3 = P_2 - P_6;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_6 - P_7;
    L_2 = P_4 - P_7;
    L_3 = P_3 - P_7;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    return jacobian;
}

double CellQualityMath::ComputeHexSkew(const std::vector<Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    auto X_1 = (P_1 - P_0) + (P_2 - P_3) + (P_5 - P_4) + (P_6 - P_7);
    auto X_2 = (P_3 - P_0) + (P_2 - P_1) + (P_7 - P_4) + (P_6 - P_5);
    auto X_3 = (P_4 - P_0) + (P_5 - P_1) + (P_6 - P_2) + (P_7 - P_3);

    auto X_1_hat = X_1.normalized();
    auto X_2_hat = X_2.normalized();
    auto X_3_hat = X_3.normalized();

    auto skew_12 = std::abs(X_1_hat.dot(X_2_hat));
    auto skew_13 = std::abs(X_1_hat.dot(X_3_hat));
    auto skew_23 = std::abs(X_2_hat.dot(X_3_hat));

    return (std::max) ({skew_12, skew_13, skew_23});
}

double CellQualityMath::ComputeHexTaper(const std::vector<Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    auto X_1 = (P_1 - P_0) + (P_2 - P_3) + (P_5 - P_4) + (P_6 - P_7);
    auto X_2 = (P_3 - P_0) + (P_2 - P_1) + (P_7 - P_4) + (P_6 - P_5);
    auto X_3 = (P_4 - P_0) + (P_5 - P_1) + (P_6 - P_2) + (P_7 - P_3);

    auto X_12 = (P_2 - P_3) - (P_1 - P_0) + (P_6 - P_7) - (P_5 - P_4);
    auto X_13 = (P_5 - P_1) - (P_4 - P_0) + (P_6 - P_2) - (P_7 - P_3);
    auto X_23 = (P_7 - P_4) - (P_3 - P_0) + (P_6 - P_5) - (P_2 - P_1);

    auto T_12 = X_12.norm() / ((std::min) (X_1.norm(), X_2.norm()));
    auto T_13 = X_13.norm() / ((std::min) (X_1.norm(), X_3.norm()));
    auto T_23 = X_23.norm() / ((std::min) (X_2.norm(), X_3.norm()));

    return (std::max) ({T_12, T_13, T_23});
}

double CellQualityMath::ComputeHexStretch(const std::vector<Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    std::vector<float> edges(12, 0);
    edges[0] = (P_1 - P_0).norm();
    edges[1] = (P_2 - P_1).norm();
    edges[2] = (P_3 - P_2).norm();
    edges[3] = (P_3 - P_0).norm();
    edges[4] = (P_4 - P_0).norm();
    edges[5] = (P_5 - P_1).norm();
    edges[6] = (P_6 - P_2).norm();
    edges[7] = (P_7 - P_3).norm();
    edges[8] = (P_5 - P_4).norm();
    edges[9] = (P_6 - P_5).norm();
    edges[10] = (P_7 - P_6).norm();
    edges[11] = (P_7 - P_4).norm();

    std::vector<float> diagonals(4, 0);

    diagonals[0] = (P_6 - P_0).norm();
    diagonals[1] = (P_7 - P_1).norm();
    diagonals[2] = (P_4 - P_2).norm();
    diagonals[3] = (P_5 - P_3).norm();

    float L_min = *(std::min_element(edges.begin(), edges.end()));
    float D_max = *(std::max_element(diagonals.begin(), diagonals.end()));

    return std::sqrt(3.0f) * L_min / D_max;
}

double CellQualityMath::ComputeHexDiagonal(const std::vector<Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    std::vector<float> diagonals(4, 0);

    diagonals[0] = (P_6 - P_0).norm();
    diagonals[1] = (P_7 - P_1).norm();
    diagonals[2] = (P_4 - P_2).norm();
    diagonals[3] = (P_5 - P_3).norm();

    auto max_ele = std::max_element(diagonals.begin(), diagonals.end());
    auto min_ele = std::min_element(diagonals.begin(), diagonals.end());

    return *min_ele / *max_ele;
}

double CellQualityMath::ComputeHexMaxEdgeRatio(const std::vector<Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    auto X_1 = (P_1 - P_0) + (P_2 - P_3) + (P_5 - P_4) + (P_6 - P_7);
    auto X_2 = (P_3 - P_0) + (P_2 - P_1) + (P_7 - P_4) + (P_6 - P_5);
    auto X_3 = (P_4 - P_0) + (P_5 - P_1) + (P_6 - P_2) + (P_7 - P_3);

    auto L_1 = X_1.norm();
    auto L_2 = X_2.norm();
    auto L_3 = X_3.norm();

    auto A_12 = (std::max) (L_1 / L_2, L_2 / L_1);
    auto A_13 = (std::max) (L_1 / L_3, L_3 / L_1);
    auto A_23 = (std::max) (L_2 / L_3, L_3 / L_2);

    return (std::max) ({A_12, A_13, A_23});
}

double CellQualityMath::ComputeHexCondition(const std::vector<Point>& points) {
    double c[8][3];
    ToD8(points, 8, c);
    const double cs = ScaleOf<8>(c);
    for (int i = 0; i < 8; ++i) { for (int k = 0; k < 3; ++k) { c[i][k] /= cs; } }

    double cond = 0.0;
    for (int i = 0; i < 8; ++i) {
        double e0[3], e1[3], e2[3];
        HexCornerEdges(c, i, e0, e1, e2);
        const double cur = ConditionComp(e0, e1, e2);
        if (i == 0 || cur > cond) { cond = cur; }
    }
    if (cond >= kVdMax) { return kVdMax; }
    if (cond <= -kVdMax) { return -kVdMax; }
    return cond / 3.0; // Verdict V_HexMetric.cpp:1812
}

double CellQualityMath::ComputeHexDistortion(const std::vector<Point>& points) {
    double c[8][3];
    ToD8(points, 8, c);

    const double g = 1.0 / std::sqrt(3.0);
    double minimumJacobian = kVdMax;
    double elementVolume = 0.0;

    // 1) 高斯点
    for (int gi = 0; gi < 2; ++gi) {
        for (int gj = 0; gj < 2; ++gj) {
            for (int gk = 0; gk < 2; ++gk) {
                double dxi[3], det_[3], dze[3];
                HexDerivD(c, (gi ? g : -g), (gj ? g : -g), (gk ? g : -g), dxi, det_, dze);
                double cr[3];
                CrossV(det_, dze, cr);
                const double jac = DotV(dxi, cr); // 有符号
                if (minimumJacobian > jac) { minimumJacobian = jac; }
                elementVolume += jac; // 权重均为 1
            }
        }
    }

    // 2) 节点
    for (int i = 0; i < 8; ++i) {
        double dxi[3], det_[3], dze[3];
        HexDerivD(c, kHexPt[i][0], kHexPt[i][1], kHexPt[i][2], dxi, det_, dze);
        double cr[3];
        CrossV(det_, dze, cr);
        const double jac = DotV(dxi, cr);
        if (minimumJacobian > jac) { minimumJacobian = jac; }
    }

    // 3) 归一化
    double distortion = kVdMax;
    if (std::fabs(elementVolume) > 0.0) {
        distortion = minimumJacobian / elementVolume * 8.0;
    }
    if (distortion > kVdMax) { return kVdMax; }
    if (distortion < -kVdMax) { return -kVdMax; }
    return distortion;
}

double CellQualityMath::ComputeHexEquiangleSkew(const std::vector<Point>& points) {
    double c[8][3];
    ToD8(points, 8, c);
    const double cs = ScaleOf<8>(c);

    // Verdict 的 6 个面（节点序）
    const int F[6][4] = {{0, 3, 2, 1}, {0, 1, 5, 4}, {1, 2, 6, 5},
                         {2, 3, 7, 6}, {3, 0, 4, 7}, {4, 5, 6, 7}};

    double mn = 360.0, mx = 0.0;
    for (int fi = 0; fi < 6; ++fi) {
        double q[4][3];
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 3; ++k) { q[j][k] = c[F[fi][j]][k] / cs; }
        }
        double e[4][3];
        for (int j = 0; j < 4; ++j) { SubV(q[(j + 1) % 4], q[j], e[j]); }
        for (int j = 0; j < 4; ++j) {
            const int nx = (j + 1) % 4;
            const double li = LenV(e[j]), lj = LenV(e[nx]);
            if (li <= kVdMin || lj <= kVdMin) { continue; }
            const double a = std::acos(std::clamp(-DotV(e[j], e[nx]) / (li * lj), -1.0, 1.0)) * 180.0 / 3.14159265358979323846;
            if (a < mn) { mn = a; }
            if (a > mx) { mx = a; }
        }
    }
    const double skewMax = (mx - 90.0) / 90.0;
    const double skewMin = (90.0 - mn) / 90.0;
    return (skewMax > skewMin) ? skewMax : skewMin;
}

double CellQualityMath::ComputeHexOddy(const std::vector<Point>& points) {
    double c[8][3];
    ToD8(points, 8, c);
    const double cs = ScaleOf<8>(c);
    for (int i = 0; i < 8; ++i) { for (int k = 0; k < 3; ++k) { c[i][k] /= cs; } }

    double mx = 0.0;
    for (int i = 0; i < 8; ++i) {
        double e0[3], e1[3], e2[3];
        HexCornerEdges(c, i, e0, e1, e2);
        const double cur = OddyCompHex(e0, e1, e2);
        if (i == 0 || cur > mx) { mx = cur; }
    }
    return (mx > 0.0) ? std::min(mx, kVdMax) : std::max(mx, -kVdMax);
}

double CellQualityMath::ComputeHexScaledJacobian(const std::vector<Point>& points) {
    double c[8][3];
    ToD8(points, 8, c);
    const double cs = ScaleOf<8>(c);
    for (int i = 0; i < 8; ++i) { for (int k = 0; k < 3; ++k) { c[i][k] /= cs; } }

    double minSj = kVdMax;
    for (int i = 0; i < 8; ++i) {
        double dxi[3], det_[3], dze[3];
        HexDerivD(c, kHexPt[i][0], kHexPt[i][1], kHexPt[i][2], dxi, det_, dze);
        double cr[3];
        CrossV(det_, dze, cr);
        const double jac = DotV(dxi, cr);
        const double den = LenV(dxi) * LenV(det_) * LenV(dze);
        if (den < kVdMin) { return 0.0; }
        const double sj = jac / den;
        if (sj < minSj) { minSj = sj; }
    }
    return (minSj > 0.0) ? std::min(minSj, kVdMax) : std::max(minSj, -kVdMax);
}

double CellQualityMath::ComputeHexShape(const std::vector<Point>& points) {
    double c[8][3];
    ToD8(points, 8, c);
    const double cs = ScaleOf<8>(c);
    for (int i = 0; i < 8; ++i) { for (int k = 0; k < 3; ++k) { c[i][k] /= cs; } }

    double minShape = 1.0;
    for (int i = 0; i < 8; ++i) {
        double e0[3], e1[3], e2[3];
        HexCornerEdges(c, i, e0, e1, e2);

        double cr[3];
        CrossV(e1, e2, cr);
        const double det = DotV(e0, cr);
        if (det <= kVdMin) { return 0.0; }

        const double shape = 3.0 * std::pow(det, 2.0 / 3.0) /
                             (DotV(e0, e0) + DotV(e1, e1) + DotV(e2, e2));
        if (shape < minShape) { minShape = shape; }
    }
    return minShape;
}

double CellQualityMath::ComputeHexShear(const std::vector<Point>& points) {
    double c[8][3];
    ToD8(points, 8, c);
    const double cs = ScaleOf<8>(c);
    for (int i = 0; i < 8; ++i) { for (int k = 0; k < 3; ++k) { c[i][k] /= cs; } }

    double minShear = 1.0;
    for (int i = 0; i < 8; ++i) {
        double e0[3], e1[3], e2[3];
        HexCornerEdges(c, i, e0, e1, e2);

        const double l1 = LenSqV(e0), l2 = LenSqV(e1), l3 = LenSqV(e2);
        if (l1 <= kVdMin || l2 <= kVdMin || l3 <= kVdMin) { return 0.0; }

        double cr[3];
        CrossV(e1, e2, cr);
        const double det = DotV(e0, cr);
        if (det < kVdMin) { return 0.0; }

        const double shear = det / std::sqrt(l1 * l2 * l3);
        if (shear < minShear) { minShear = shear; }
    }
    return minShear;
}

double CellQualityMath::ComputeHexRelativeSizeSquared(const std::vector<Point>& points,
                                                      double averageVolume) {
    double c[8][3];
    ToD8(points, 8, c);

    const HexWeight w = HexGetWeight(averageVolume);
    if (!w.valid || w.detw < kVdMin) { return 0.0; }

    double detSum = 0.0;
    for (int i = 0; i < 8; ++i) {
        double e0[3], e1[3], e2[3];
        HexCornerEdges(c, i, e0, e1, e2);
        double cr[3];
        CrossV(e1, e2, cr);
        detSum += DotV(e0, cr);
    }

    double size = 0.0;
    if (detSum > kVdMin) {
        double tau = detSum / (8.0 * w.detw);
        tau = std::min(tau, 1.0 / tau);
        size = tau * tau;
    }

    return (size > 0.0) ? std::min(size, kVdMax) : std::max(size, -kVdMax);
}

double CellQualityMath::ComputeHexShapeAndSize(const std::vector<Point>& points,
                                                       double averageVolume) {
    const double shape = ComputeHexShape(points);
    const double size = ComputeHexRelativeSizeSquared(points, averageVolume);
    const double v = shape * size;
    return (v > 0.0) ? std::min(v, kVdMax) : std::max(v, -kVdMax);
}

double CellQualityMath::ComputeHexShearAndSize(const std::vector<Point>& points,
                                                       double averageVolume) {
    const double shear = ComputeHexShear(points);
    const double size = ComputeHexRelativeSizeSquared(points, averageVolume);
    const double v = shear * size;
    return (v > 0.0) ? std::min(v, kVdMax) : std::max(v, -kVdMax);
}

IGAME_NAMESPACE_END
