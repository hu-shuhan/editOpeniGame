// ============================================================================
// ProbeLocator — 见 iGameProbeLocator.h
//
// 三角形包含：投影到法向分量最大的坐标平面，用 2D 有向面积求重心坐标；
//             点需满足：到三角形所在平面距离 <= tolerance * bboxDiag，
//             且重心坐标位于 [-tolerance, 1 + tolerance]。
// 四面体包含：用有符号体积求重心坐标；重心坐标位于 [-tolerance, 1 + tolerance]。
// 四边形包含：双线性映射 P(r,s) 的最小二乘反解（牛顿迭代，默认凸四边形）；
//             点需满足：残差距离 <= tolerance * bboxDiag，且 r,s 位于容差范围内。
// 六面体包含：三线性映射 P(r,s,t) 的反解（牛顿迭代，默认凸六面体）；
//             点需满足：残差距离 <= tolerance * bboxDiag，且 r,s,t 位于容差范围内。
// ============================================================================
#include "iGameProbeLocator.h"

#include <algorithm>
#include <cmath>

IGAME_NAMESPACE_BEGIN

namespace {

// 四面体有符号体积（1/6 * 混合积）
double SignedTetraVolume(const Point& a, const Point& b, const Point& c,
                         const Point& d) {
    const Vector3f ab = b - a;
    const Vector3f ac = c - a;
    const Vector3f ad = d - a;
    return ab.cross(ac).dot(ad) / 6.0;
}

// 把 float 点转成 double，避免牛顿迭代中的浮点累积误差
Vector3d ToDouble(const Point& p) { return Vector3d(p[0], p[1], p[2]); }

// 重心坐标是否在容差允许范围内：lambda_i ∈ [-tol, 1 + tol]，且 Σ lambda ≈ 1
bool BarycentricInside(const double* lambda, int n, double tolerance) {
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        if (lambda[i] < -tolerance || lambda[i] > 1.0 + tolerance) {
            return false;
        }
        sum += lambda[i];
    }
    return std::fabs(sum - 1.0) <= tolerance;
}

}  // namespace

// 四边形包含：双线性映射 P(r,s) = Σ N_i(r,s) p_i 的最小二乘反解。
// 从参考单元中心 (0.5, 0.5) 出发做牛顿迭代（默认四边形凸，允许轻微翘曲）；
// 点需满足：残差距离 ||P(r,s) - q|| <= projectionLimit，且 r,s ∈ [-tol, 1 + tol]。
// 顶点顺序为 p0(0,0), p1(1,0), p2(1,1), p3(0,1)（逆时针）。
bool ProbePointInQuad(const Point& q, const Point& a, const Point& b,
                      const Point& c, const Point& d, double tolerance,
                      double projectionLimit, double lambda[4]) {
    lambda[0] = lambda[1] = lambda[2] = lambda[3] = 0.0;

    const Vector3d p0 = ToDouble(a);
    const Vector3d p1 = ToDouble(b);
    const Vector3d p2 = ToDouble(c);
    const Vector3d p3 = ToDouble(d);
    const Vector3d qd = ToDouble(q);

    double r = 0.5;
    double s = 0.5;
    bool converged = false;
    for (int iter = 0; iter < 20; ++iter) {
        const double w0 = (1.0 - r) * (1.0 - s);
        const double w1 = r * (1.0 - s);
        const double w2 = r * s;
        const double w3 = (1.0 - r) * s;
        const Vector3d P = p0 * w0 + p1 * w1 + p2 * w2 + p3 * w3;
        const Vector3d F = P - qd;

        // 雅可比列向量：dP/dr, dP/ds
        const Vector3d dr = (p1 - p0) * (1.0 - s) + (p2 - p3) * s;
        const Vector3d ds = (p3 - p0) * (1.0 - r) + (p2 - p1) * r;

        const double a00 = dr.dot(dr);
        const double a01 = dr.dot(ds);
        const double a11 = ds.dot(ds);
        // 相对退化判定：两列平行或为零时 det 相对量级过小
        const double det = a00 * a11 - a01 * a01;
        if (det <= kProbeNumericalEps * a00 * a11) return false;

        // 法方程 (J^T J) δ = -J^T F 的解析解
        const double fr = dr.dot(F);
        const double fs = ds.dot(F);
        const double stepR = (-fr * a11 + fs * a01) / det;
        const double stepS = (-fs * a00 + fr * a01) / det;

        r += stepR;
        s += stepS;
        if (std::max(std::fabs(stepR), std::fabs(stepS)) < 1e-10) {
            converged = true;
            break;
        }
    }
    if (!converged) return false;

    // 收敛点处重算残差与插值权重
    const double w0 = (1.0 - r) * (1.0 - s);
    const double w1 = r * (1.0 - s);
    const double w2 = r * s;
    const double w3 = (1.0 - r) * s;
    const Vector3d P = p0 * w0 + p1 * w1 + p2 * w2 + p3 * w3;
    if ((P - qd).length() > projectionLimit) return false;

    lambda[0] = w0;
    lambda[1] = w1;
    lambda[2] = w2;
    lambda[3] = w3;
    return r >= -tolerance && r <= 1.0 + tolerance &&
           s >= -tolerance && s <= 1.0 + tolerance;
}

// 六面体包含：三线性映射 P(r,s,t) = Σ N_i(r,s,t) p_i 的反解。
// 从参考单元中心 (0.5, 0.5, 0.5) 出发做牛顿迭代（默认六面体凸，此时
// 雅可比行列式恒正、反解唯一）；
// 点需满足：残差距离 ||P(r,s,t) - q|| <= projectionLimit，且 r,s,t ∈ [-tol, 1 + tol]。
// 顶点顺序为 VTK 约定：底面 p0..p3（r/s 方向），顶面 p4..p7（t 方向）。
bool ProbePointInHexahedron(const Point& q, const Point pts[8], double tolerance,
                            double projectionLimit, double lambda[8]) {
    for (int i = 0; i < 8; ++i) lambda[i] = 0.0;

    const Vector3d p[8] = {ToDouble(pts[0]), ToDouble(pts[1]), ToDouble(pts[2]),
                           ToDouble(pts[3]), ToDouble(pts[4]), ToDouble(pts[5]),
                           ToDouble(pts[6]), ToDouble(pts[7])};
    const Vector3d qd = ToDouble(q);

    double r = 0.5;
    double s = 0.5;
    double t = 0.5;
    bool converged = false;
    for (int iter = 0; iter < 20; ++iter) {
        const double w0 = (1.0 - r) * (1.0 - s) * (1.0 - t);
        const double w1 = r * (1.0 - s) * (1.0 - t);
        const double w2 = r * s * (1.0 - t);
        const double w3 = (1.0 - r) * s * (1.0 - t);
        const double w4 = (1.0 - r) * (1.0 - s) * t;
        const double w5 = r * (1.0 - s) * t;
        const double w6 = r * s * t;
        const double w7 = (1.0 - r) * s * t;
        const Vector3d P = p[0] * w0 + p[1] * w1 + p[2] * w2 + p[3] * w3 +
                           p[4] * w4 + p[5] * w5 + p[6] * w6 + p[7] * w7;
        const Vector3d F = P - qd;

        // 雅可比列向量：dP/dr, dP/ds, dP/dt
        const Vector3d c0 = (p[1] - p[0]) * ((1.0 - s) * (1.0 - t)) +
                            (p[2] - p[3]) * (s * (1.0 - t)) +
                            (p[5] - p[4]) * ((1.0 - s) * t) +
                            (p[6] - p[7]) * (s * t);
        const Vector3d c1 = (p[3] - p[0]) * ((1.0 - r) * (1.0 - t)) +
                            (p[2] - p[1]) * (r * (1.0 - t)) +
                            (p[7] - p[4]) * ((1.0 - r) * t) +
                            (p[6] - p[5]) * (r * t);
        const Vector3d c2 = (p[4] - p[0]) * ((1.0 - r) * (1.0 - s)) +
                            (p[5] - p[1]) * (r * (1.0 - s)) +
                            (p[6] - p[2]) * (r * s) +
                            (p[7] - p[3]) * ((1.0 - r) * s);

        // 相对退化判定：雅可比三列接近共面/为零时行列式相对量级过小
        const double det = c0.dot(c1.cross(c2));
        const double scale = c0.length() * c1.length() * c2.length();
        if (std::fabs(det) <= kProbeNumericalEps * scale) return false;

        // J δ = -F，J^{-1} 用列向量叉积构造
        const Vector3d b = F * (-1.0);
        const double stepR = c1.cross(c2).dot(b) / det;
        const double stepS = c2.cross(c0).dot(b) / det;
        const double stepT = c0.cross(c1).dot(b) / det;

        r += stepR;
        s += stepS;
        t += stepT;
        if (std::max({std::fabs(stepR), std::fabs(stepS), std::fabs(stepT)}) <
            1e-10) {
            converged = true;
            break;
        }
    }
    if (!converged) return false;

    // 收敛点处重算残差与插值权重
    const double w0 = (1.0 - r) * (1.0 - s) * (1.0 - t);
    const double w1 = r * (1.0 - s) * (1.0 - t);
    const double w2 = r * s * (1.0 - t);
    const double w3 = (1.0 - r) * s * (1.0 - t);
    const double w4 = (1.0 - r) * (1.0 - s) * t;
    const double w5 = r * (1.0 - s) * t;
    const double w6 = r * s * t;
    const double w7 = (1.0 - r) * s * t;
    const Vector3d P = p[0] * w0 + p[1] * w1 + p[2] * w2 + p[3] * w3 +
                       p[4] * w4 + p[5] * w5 + p[6] * w6 + p[7] * w7;
    if ((P - qd).length() > projectionLimit) return false;

    lambda[0] = w0;
    lambda[1] = w1;
    lambda[2] = w2;
    lambda[3] = w3;
    lambda[4] = w4;
    lambda[5] = w5;
    lambda[6] = w6;
    lambda[7] = w7;
    return r >= -tolerance && r <= 1.0 + tolerance &&
           s >= -tolerance && s <= 1.0 + tolerance &&
           t >= -tolerance && t <= 1.0 + tolerance;
}

bool ProbePointInTriangle(const Point& q, const Point& a, const Point& b,
                          const Point& c, double tolerance, double projectionLimit,
                          double lambda[3]) {
    lambda[0] = lambda[1] = lambda[2] = 0.0;

    const Vector3f ab = b - a;
    const Vector3f ac = c - a;
    const Vector3f normal = ab.cross(ac);
    const double normalLen2 = normal.squaredLength();
    if (normalLen2 <= kProbeNumericalEps) return false;  // 退化三角形
    const double normalLen = std::sqrt(normalLen2);

    // 投影距离检查：点 q 到三角形所在平面的距离
    const Vector3f aq = q - a;
    if (std::fabs(normal.dot(aq)) / normalLen > projectionLimit) return false;

    // 投影到法向分量最大的坐标平面（去掉该轴），用 2D 有向面积求重心坐标
    int axis = 0;
    if (std::fabs(normal[1]) > std::fabs(normal[axis])) axis = 1;
    if (std::fabs(normal[2]) > std::fabs(normal[axis])) axis = 2;
    const int u = (axis + 1) % 3;
    const int v = (axis + 2) % 3;

    const auto area2 = [&](const Point& p0, const Point& p1, const Point& p2) {
        const double x0 = p0[u], y0 = p0[v];
        const double x1 = p1[u], y1 = p1[v];
        const double x2 = p2[u], y2 = p2[v];
        return (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
    };

    const double denom = area2(a, b, c);
    if (std::fabs(denom) <= kProbeNumericalEps) return false;

    lambda[0] = area2(q, b, c) / denom;
    lambda[1] = area2(a, q, c) / denom;
    lambda[2] = area2(a, b, q) / denom;
    return BarycentricInside(lambda, 3, tolerance);
}

bool ProbePointInTetra(const Point& q, const Point& a, const Point& b,
                       const Point& c, const Point& d, double tolerance,
                       double lambda[4]) {
    lambda[0] = lambda[1] = lambda[2] = lambda[3] = 0.0;

    const double vol = SignedTetraVolume(a, b, c, d);
    if (std::fabs(vol) <= kProbeNumericalEps) return false;  // 退化四面体

    lambda[0] = SignedTetraVolume(q, b, c, d) / vol;
    lambda[1] = SignedTetraVolume(a, q, c, d) / vol;
    lambda[2] = SignedTetraVolume(a, b, q, d) / vol;
    lambda[3] = SignedTetraVolume(a, b, c, q) / vol;
    return BarycentricInside(lambda, 4, tolerance);
}

bool EvaluatePosition(const Cell* cell, const Point& q, double tolerance,
                      double bboxDiag, ProbeCellHit& hit) {
    hit = ProbeCellHit{};
    if (cell == nullptr) return false;

    const double tol = std::max(tolerance, 0.0);
    const double projectionLimit = tol * std::max(bboxDiag, 0.0);

    switch (cell->GetCellType()) {
    case IG_TRIANGLE: {
        double lambda[3] = {0.0, 0.0, 0.0};
        if (!ProbePointInTriangle(q, cell->GetPoint(0), cell->GetPoint(1),
                                  cell->GetPoint(2), tol, projectionLimit, lambda)) {
            return false;
        }
        hit.found = true;
        hit.numVertices = 3;
        hit.localVertIds[0] = 0;
        hit.localVertIds[1] = 1;
        hit.localVertIds[2] = 2;
        hit.weights[0] = lambda[0];
        hit.weights[1] = lambda[1];
        hit.weights[2] = lambda[2];
        return true;
    }
    case IG_QUAD: {
        double lambda[4] = {0.0, 0.0, 0.0, 0.0};
        if (!ProbePointInQuad(q, cell->GetPoint(0), cell->GetPoint(1),
                              cell->GetPoint(2), cell->GetPoint(3), tol,
                              projectionLimit, lambda)) {
            return false;
        }
        hit.found = true;
        hit.numVertices = 4;
        for (int i = 0; i < 4; ++i) {
            hit.localVertIds[i] = i;
            hit.weights[i] = lambda[i];
        }
        return true;
    }
    case IG_TETRA: {
        double lambda[4] = {0.0, 0.0, 0.0, 0.0};
        if (!ProbePointInTetra(q, cell->GetPoint(0), cell->GetPoint(1),
                               cell->GetPoint(2), cell->GetPoint(3), tol, lambda)) {
            return false;
        }
        hit.found = true;
        hit.numVertices = 4;
        hit.localVertIds[0] = 0;
        hit.localVertIds[1] = 1;
        hit.localVertIds[2] = 2;
        hit.localVertIds[3] = 3;
        hit.weights[0] = lambda[0];
        hit.weights[1] = lambda[1];
        hit.weights[2] = lambda[2];
        hit.weights[3] = lambda[3];
        return true;
    }
    case IG_HEXAHEDRON: {
        const Point pts[8] = {cell->GetPoint(0), cell->GetPoint(1),
                              cell->GetPoint(2), cell->GetPoint(3),
                              cell->GetPoint(4), cell->GetPoint(5),
                              cell->GetPoint(6), cell->GetPoint(7)};
        double lambda[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        if (!ProbePointInHexahedron(q, pts, tol, projectionLimit, lambda)) {
            return false;
        }
        hit.found = true;
        hit.numVertices = 8;
        for (int i = 0; i < 8; ++i) {
            hit.localVertIds[i] = i;
            hit.weights[i] = lambda[i];
        }
        return true;
    }
    default:
        // v2：未支持的单元类型直接返回未命中
        return false;
    }
}

IGAME_NAMESPACE_END
