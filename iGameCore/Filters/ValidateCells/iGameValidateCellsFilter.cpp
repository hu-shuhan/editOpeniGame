#include "iGameValidateCellsFilter.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <utility>
#include <vector>
IGAME_NAMESPACE_BEGIN
namespace {
using Vec = Vector3d;
// vtkCellValidator 默认公差 FLT_EPSILON（VTK 9.3 / ParaView）。
constexpr double kTolerance = 1.1920928955078125e-7;
// vtkLine::Intersection 默认相对公差。
constexpr double kLineRelTolerance = 1e-6;
// vtkTriangle::TrianglesIntersect 内部 epsilon。
constexpr double kTriEps = 256.0 * 2.2204460492503131e-16;
constexpr double kDblEps = 2.2204460492503131e-16;
constexpr double kDblMin = 2.2250738585072014e-308;
inline Vec ToVec(const Point& p) {
    return Vec(static_cast<double>(p[0]), static_cast<double>(p[1]),
               static_cast<double>(p[2]));
}
// ---------------------------------------------------------------------------
// vtkLine 对齐实现
// ---------------------------------------------------------------------------
double DistanceToLine(const Vec& x, const Vec& p1, const Vec& p2, double& t) {
    const Vec d = p2 - p1;
    const double dd = d.squaredNorm();
    t = dd > 0.0 ? (x - p1).dot(d) / dd : 0.0;
    return (x - (p1 + d * t)).norm();
}
// 2x2 线性系统（带部分主元），对应 vtkMath::SolveLinearSystem 的 2 阶情形。
int SolveLinearSystem2(double* A[2], double c[2]) {
    double* row[2] = {A[0], A[1]};
    if (std::fabs(row[1][0]) > std::fabs(row[0][0])) {
        std::swap(row[0], row[1]);
        // 行交换时右端项必须同步交换（与 VTK 的置换求解一致）。
        std::swap(c[0], c[1]);
    }
    if (std::fabs(row[0][0]) < kDblMin) {
        return 0;
    }
    const double factor = row[1][0] / row[0][0];
    row[1][1] -= factor * row[0][1];
    c[1] -= factor * c[0];
    if (std::fabs(row[1][1]) < kDblMin) {
        return 0;
    }
    c[1] /= row[1][1];
    c[0] = (c[0] - row[0][1] * c[1]) / row[0][0];
    return 1;
}
// 对应 vtkLine::Intersection。返回 0=NoIntersect, 2=Intersect, 3=OnLine。
int LineIntersection(const Vec& a1, const Vec& a2, const Vec& b1, const Vec& b2,
                     double& u, double& v, double tolerance, int tolType) {
    u = v = 0.0;
    const Vec a21 = a2 - a1;
    const Vec b21 = b2 - b1;
    const Vec b1a1 = b1 - a1;
    double row1[2] = {a21.dot(a21), -a21.dot(b21)};
    double row2[2] = {row1[1], b21.dot(b21)};
    double c[2] = {a21.dot(b1a1), -b21.dot(b1a1)};
    double* A[2] = {row1, row2};
    if (SolveLinearSystem2(A, c) == 0) {
        // 共线：四个端点中取最近点。
        double minDist = std::numeric_limits<double>::max();
        const Vec* p[4] = {&a1, &a2, &b1, &b2};
        const Vec* l1[4] = {&b1, &b1, &a1, &a1};
        const Vec* l2[4] = {&b2, &b2, &a2, &a2};
        double* uv1[4] = {&v, &v, &u, &u};
        double* uv2[4] = {&u, &u, &v, &v};
        double t = 0.0;
        for (int i = 0; i < 4; ++i) {
            const double dist = DistanceToLine(*p[i], *l1[i], *l2[i], t);
            if (dist < minDist) {
                minDist = dist;
                *uv1[i] = t;
                *uv2[i] = static_cast<double>(i % 2);
            }
        }
        return 3;  // OnLine
    }
    u = c[0];
    v = c[1];
    const Vec ptu = a1 + a21 * u;
    const Vec ptv = b1 + b21 * v;
    const double diff2 = (ptu - ptv).squaredNorm();
    double tol2 = 0.0;
    if (std::isfinite(tolerance)) {
        tol2 = (tolType == 1 || tolType == 3)
                   ? tolerance * tolerance
                   : tolerance * tolerance *
                         std::max(ptv.squaredNorm(), ptu.squaredNorm());
        if (diff2 > tol2) {
            return 0;
        }
    }
    if (0.0 <= u && u <= 1.0 && 0.0 <= v && v <= 1.0) {
        return 2;  // Intersect
    }
    if (tolType >= 2 && tol2 > 0.0) {
        const double uTol = std::sqrt(tol2 / a21.squaredNorm());
        const double vTol = std::sqrt(tol2 / b21.squaredNorm());
        if (-uTol <= u && u <= 1.0 + uTol && -vTol <= v &&
            v <= 1.0 + vTol) {
            return 2;
        }
    }
    return 0;
}
// 分量级重合判断（vtkCellValidator::PointsAreCoincident）。
bool PointsAreCoincident(const Vec& p, const Vec& q, double tolerance) {
    return std::fabs(p[0] - q[0]) < tolerance &&
           std::fabs(p[1] - q[1]) < tolerance &&
           std::fabs(p[2] - q[2]) < tolerance;
}
// vtkCellValidator::LineSegmentsIntersect：端点相接不算相交。
bool LineSegmentsIntersect(const Vec& p1, const Vec& p2, const Vec& q1,
                           const Vec& q2, double tolerance) {
    double u = 0.0, v = 0.0;
    if (LineIntersection(p1, p2, q1, q2, u, v, kLineRelTolerance, 0) == 2) {
        if ((std::fabs(u) > tolerance && std::fabs(u - 1.0) > tolerance) ||
            (std::fabs(v) > tolerance && std::fabs(v - 1.0) > tolerance)) {
            return true;
        }
    }
    return false;
}
// ---------------------------------------------------------------------------
// vtkTriangle 对齐实现
// ---------------------------------------------------------------------------
// 四点行列式：>0 表示 d 在平面 (a,b,c) 上方。
double CellDeterminant(const Vec& a, const Vec& b, const Vec& c,
                       const Vec& d) {
    const Vec A = a - d;
    const Vec B = b - d;
    const Vec C = c - d;
    return A[0] * (B[1] * C[2] - B[2] * C[1]) -
           A[1] * (B[0] * C[2] - B[2] * C[0]) +
           A[2] * (B[0] * C[1] - B[1] * C[0]);
}
enum OrientationState {
    Orient_Colinear = 1,
    Orient_Clockwise = 2,
    Orient_Counterclockwise = 4,
};
int Orientation2D(const std::array<double, 2>& p1,
                  const std::array<double, 2>& p2,
                  const std::array<double, 2>& p3) {
    const double v1x = p2[0] - p1[0], v1y = p2[1] - p1[1];
    const double v2x = p3[0] - p1[0], v2y = p3[1] - p1[1];
    const double signedArea = v1x * v2y - v1y * v2x;
    if (std::fabs(signedArea) < kTriEps) {
        return Orient_Colinear;
    }
    return signedArea > 0.0 ? Orient_Counterclockwise : Orient_Clockwise;
}
// Devillers-Guigue 共面三角形相交（2D）。
int CoplanarTrianglesIntersect(std::array<double, 2> p1,
                               std::array<double, 2> q1,
                               std::array<double, 2> r1,
                               std::array<double, 2> p2,
                               std::array<double, 2> q2,
                               std::array<double, 2> r2) {
    if (Orientation2D(p1, q1, r1) == Orient_Clockwise) std::swap(q1, r1);
    if (Orientation2D(p2, q2, r2) == Orient_Clockwise) std::swap(q2, r2);
    const int p1Orientation[3] = {Orientation2D(p2, q2, p1),
                                  Orientation2D(q2, r2, p1),
                                  Orientation2D(r2, p2, p1)};
    const int sumOfSigns = p1Orientation[0] + p1Orientation[1] +
                           p1Orientation[2];
    constexpr int Three_CCW = 3 * Orient_Counterclockwise;
    constexpr int TwoCol_OneCW = 2 * Orient_Colinear + Orient_Clockwise;
    constexpr int TwoCol_OneCCW = 2 * Orient_Colinear + Orient_Counterclockwise;
    constexpr int OneCol_TwoCCW = Orient_Colinear + 2 * Orient_Counterclockwise;
    if (sumOfSigns == Three_CCW || sumOfSigns == TwoCol_OneCW ||
        sumOfSigns == TwoCol_OneCCW || sumOfSigns == OneCol_TwoCCW) {
        return 1;
    }
    int index = 0;
    for (index = 0; index < 3; ++index) {
        if (p1Orientation[index] == Orient_Counterclockwise) {
            if (p1Orientation[(index + 1) % 3] == Orient_Counterclockwise &&
                p1Orientation[(index + 2) % 3] == Orient_Clockwise) {
                break;
            }
            if (p1Orientation[(index + 1) % 3] == Orient_Clockwise &&
                p1Orientation[(index + 2) % 3] == Orient_Clockwise) {
                break;
            }
            if (p1Orientation[(index + 1) % 3] == Orient_Colinear &&
                p1Orientation[(index + 2) % 3] == Orient_Clockwise) {
                break;
            }
            if (p1Orientation[(index + 1) % 3] == Orient_Clockwise &&
                p1Orientation[(index + 2) % 3] == Orient_Colinear) {
                break;
            }
        }
    }
    if (index == 3) {
        return 0;
    }
    const std::array<double, 2> T2[3] = {p2, q2, r2};
    p2 = T2[index];
    q2 = T2[(index + 1) % 3];
    r2 = T2[(index + 2) % 3];
    if (p1Orientation[(index + 1) % 3] == Orient_Counterclockwise) {
        if (Orientation2D(r2, p2, q1) != Orient_Clockwise) {
            if (Orientation2D(r2, p1, q1) != Orient_Clockwise) {
                if (Orientation2D(p1, p2, q1) != Orient_Clockwise) {
                    return 1;
                }
                if (Orientation2D(p1, p2, r1) != Orient_Clockwise) {
                    if (Orientation2D(q1, r1, p2) != Orient_Clockwise) {
                        return 1;
                    }
                }
            }
        } else {
            if (Orientation2D(r2, p2, r1) != Orient_Clockwise) {
                if (Orientation2D(q1, r1, r2) == Orient_Clockwise) {
                    return 0;
                }
                if (Orientation2D(p1, p2, r1) == Orient_Clockwise) {
                    return 0;
                }
                return 1;
            }
        }
        return 0;
    }
    if (Orientation2D(r2, p2, q1) != Orient_Clockwise) {
        if (Orientation2D(q2, r2, q1) != Orient_Clockwise) {
            if (Orientation2D(p1, p2, q1) != Orient_Clockwise) {
                if (Orientation2D(p1, q2, q1) == Orient_Counterclockwise) {
                    return 0;
                }
                return 1;
            }
            if (Orientation2D(p1, p2, r1) == Orient_Clockwise) {
                return 0;
            }
            if (Orientation2D(p2, q1, r1) == Orient_Clockwise) {
                return 0;
            }
            return 1;
        }
        if (Orientation2D(p1, q2, q1) != Orient_Counterclockwise) {
            if (Orientation2D(q2, r2, r1) == Orient_Clockwise) {
                return 0;
            }
            if (Orientation2D(q1, r1, q2) == Orient_Clockwise) {
                return 0;
            }
            return 1;
        }
        return 0;
    }
    if (Orientation2D(r2, p2, r1) == Orient_Clockwise) {
        return 0;
    }
    if (Orientation2D(q1, r1, r2) != Orient_Clockwise) {
        if (Orientation2D(r1, p1, p2) == Orient_Clockwise) {
            return 0;
        }
        return 1;
    }
    if (Orientation2D(q1, r1, q2) == Orient_Clockwise) {
        return 0;
    }
    if (Orientation2D(q2, r2, r1) == Orient_Clockwise) {
        return 0;
    }
    return 1;
}
// vtkTriangle::PointInTriangle，tol2 为距离平方阈值。
bool PointInTriangle(const Vec& x, const Vec& p1, const Vec& p2,
                     const Vec& p3, double tol2) {
    const Vec x1 = x - p1, x2 = x - p2, x3 = x - p3;
    const Vec v13 = p1 - p3, v21 = p2 - p1, v32 = p3 - p2;
    if (x1.squaredNorm() <= tol2 || x2.squaredNorm() <= tol2 ||
        x3.squaredNorm() <= tol2) {
        return true;
    }
    const Vec n1 = x1.cross(v13);
    const Vec n2 = x2.cross(v21);
    const Vec n3 = x3.cross(v32);
    return n1.dot(n2) >= 0.0 && n2.dot(n3) >= 0.0 && n1.dot(n3) >= 0.0;
}
// vtkTriangle::TrianglesIntersect：返回 1 相交 / 0 不相交。
int TrianglesIntersect(const Vec& p1, const Vec& q1, const Vec& r1,
                       const Vec& p2, const Vec& q2, const Vec& r2) {
    const double det1[3] = {CellDeterminant(p2, q2, r2, p1),
                            CellDeterminant(p2, q2, r2, q1),
                            CellDeterminant(p2, q2, r2, r1)};
    if (std::fabs(det1[0]) < kTriEps &&
        std::fabs(det1[1]) < kTriEps &&
        std::fabs(det1[2]) < kTriEps) {
        // 共面：投影到面积最大的主平面做 2D 判定。
        const Vec normal = (q1 - p1).cross(r1 - p1);
        int index = 0;
        for (int i = 1; i < 3; ++i) {
            if (std::fabs(normal[index]) < std::fabs(normal[i])) {
                index = i;
            }
        }
        auto proj = [&](const Vec& v) -> std::array<double, 2> {
            if (index == 0) return {v[1], v[2]};
            if (index == 1) return {v[0], v[2]};
            return {v[0], v[1]};
        };
        return CoplanarTrianglesIntersect(proj(p1), proj(q1), proj(r1),
                                         proj(p2), proj(q2), proj(r2));
    }
    bool degenerate = false;
    const Vec* points[3] = {&p1, &q1, &r1};
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(det1[i]) < kTriEps) {
            degenerate = true;
            if (PointInTriangle(*points[i], p2, q2, r2, kTriEps)) {
                return 1;
            }
        }
    }
    if (degenerate) {
        return 0;
    }
    {
        const int sumOfSigns = (det1[0] > 0.0) + (det1[1] > 0.0) +
                               (det1[2] > 0.0);
        if (sumOfSigns == 0 || sumOfSigns == 3) {
            return 0;
        }
    }
    const double det2[3] = {CellDeterminant(p1, q1, r1, p2),
                            CellDeterminant(p1, q1, r1, q2),
                            CellDeterminant(p1, q1, r1, r2)};
    {
        const int sumOfSigns = (det2[0] > 0.0) + (det2[1] > 0.0) +
                               (det2[2] > 0.0);
        if (sumOfSigns == 0 || sumOfSigns == 3) {
            return 0;
        }
    }
    int index1 = 0;
    for (index1 = 0; index1 < 3; ++index1) {
        const int sumOfSigns =
            (det1[(index1 + 1) % 3] > 0.0) +
            (det1[(index1 + 2) % 3] > 0.0);
        if (sumOfSigns != 1) break;
    }
    const Vec* T1[3] = {&p1, &q1, &r1};
    const Vec pp1 = *T1[index1];
    Vec qq1 = *T1[(index1 + 1) % 3];
    Vec rr1 = *T1[(index1 + 2) % 3];
    const bool swap1 = det1[index1] < -kTriEps;
    int index2 = 0;
    for (index2 = 0; index2 < 3; ++index2) {
        const int sumOfSigns =
            (det2[(index2 + 1) % 3] > 0.0) +
            (det2[(index2 + 2) % 3] > 0.0);
        if (sumOfSigns != 1) break;
    }
    const Vec* T2[3] = {&p2, &q2, &r2};
    const Vec pp2 = *T2[index2];
    Vec qq2 = *T2[(index2 + 1) % 3];
    Vec rr2 = *T2[(index2 + 2) % 3];
    const bool swap2 = det2[index2] < -kTriEps;
    if (swap1) std::swap(qq2, rr2);
    if (swap2) std::swap(qq1, rr1);
    if (CellDeterminant(pp1, qq1, pp2, qq2) <= 0.0 &&
        CellDeterminant(pp1, rr1, rr2, pp2) <= 0.0) {
        return 1;
    }
    return 0;
}
// vtkCellValidator 内部 TrianglesIntersect：允许相邻面共享 1~2 个点。
bool TrianglesOfFacesIntersect(const Vec* p, const Vec* q, double tolerance) {
    if (TrianglesIntersect(p[0], p[1], p[2], q[0], q[1], q[2]) == 1) {
        int nCoincidentPoints = 0;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (LineSegmentsIntersect(p[i], p[(i + 1) % 3], q[j],
                                          q[(j + 1) % 3], tolerance)) {
                    return true;
                }
                nCoincidentPoints +=
                    PointsAreCoincident(p[i], q[j], tolerance) ? 1 : 0;
            }
        }
        return nCoincidentPoints != 1 && nCoincidentPoints != 2;
    }
    return false;
}
// ---------------------------------------------------------------------------
// 二维单元检查（vtkPolygon / vtkQuad / vtkTriangle 对齐）
// ---------------------------------------------------------------------------
// 多边形法向：绕 p0 扇形叉积累加后归一化（vtkPolygon::ComputeNormal）。
Vec PolygonNormal(const std::vector<Vec>& p) {
    Vec n(0.0, 0.0, 0.0);
    const Vec p0 = p[0];
    Vec v1 = p[1] - p0;
    for (size_t i = 2; i < p.size(); ++i) {
        const Vec v2 = p[i] - p0;
        n = n + v1.cross(v2);
        v1 = v2;
    }
    return n.normalized();
}
// vtkPolygon::ComputeCentroid（VTK 9.4）：优先投影到 (x,y)，法线与 z 正交
// （投影退化为直线）时改投 (x,z)，再退化改投 (y,z)；投影面积为 0 时返回
// false 且质心置零（IsConvex 忽略返回值，直接使用零质心）。
bool PolygonCentroid(const std::vector<Vec>& p, Vec& centroid) {
    const int numPts = static_cast<int>(p.size());
    if (numPts < 2) return false;
    const Vec normal = PolygonNormal(p);
    int xOffset = 0;
    int yOffset = 0;
    {
        Vec c = normal.cross(Vec(0.0, 0.0, 1.0));
        if (std::fabs(c.dot(c) - 1.0) <= kDblEps) {
            yOffset = 1;
            c = normal.cross(Vec(0.0, 1.0, 0.0));
            if (std::fabs(c.dot(c) - 1.0) <= kDblEps) {
                xOffset = 1;
            }
        }
    }
    centroid = Vec(0.0, 0.0, 0.0);
    double a = 0.0;
    double maxabsdet = 0.0;
    for (int i = 0; i < numPts; ++i) {
        const Vec& pi = p[i];
        const Vec& pj = p[(i + 1) % numPts];
        const double ui = pi[xOffset];
        const double vi = pi[1 + yOffset];
        const double uj = pj[xOffset];
        const double vj = pj[1 + yOffset];
        const double det = ui * vj - uj * vi;
        centroid[xOffset] += (ui + uj) * det;
        centroid[1 + yOffset] += (vi + vj) * det;
        a += det;
        maxabsdet = std::fabs(det) > maxabsdet ? std::fabs(det) : maxabsdet;
    }
    if (std::fabs(a) < kDblEps * maxabsdet) {
        // 投影多边形退化。
        return false;
    }
    centroid[xOffset] /= 3.0 * a;
    centroid[1 + yOffset] /= 3.0 * a;
    const int w = 2 - xOffset - yOffset;
    centroid[w] = 1.0 / normal[w] *
                  (-normal[xOffset] * centroid[xOffset] -
                   normal[1 + yOffset] * centroid[1 + yOffset] +
                   normal.dot(p[0]));
    return true;
}
// vtkCellValidator::ContiguousEdges。
bool ContiguousEdges2D(const std::vector<Vec>& p, double tolerance) {
    const int n = static_cast<int>(p.size());
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        double u = 0.0, v = 0.0;
        LineIntersection(p[i], p[(i + 1) % n], p[j], p[(j + 1) % n], u, v,
                         std::numeric_limits<double>::infinity(), 0);
        if ((std::fabs(u) > tolerance &&
             std::fabs(1.0 - u) > tolerance) ||
            (std::fabs(v) > tolerance &&
             std::fabs(1.0 - v) > tolerance)) {
            return false;
        }
    }
    return true;
}
// vtkCellValidator::NoIntersectingEdges（二维）。
bool NoIntersectingEdges2D(const std::vector<Vec>& p, double tolerance) {
    const int n = static_cast<int>(p.size());
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (LineSegmentsIntersect(p[i], p[(i + 1) % n], p[j],
                                      p[(j + 1) % n], tolerance)) {
                return false;
            }
        }
    }
    return true;
}
// vtkPolygon::IsConvex。
bool IsConvex2D(const std::vector<Vec>& p) {
    const int numPts = static_cast<int>(p.size());
    if (numPts < 3) return false;
    if (numPts == 3) return true;
    Vec v0(0, 0, 0), v1 = p[0], v2 = p[1];
    Vec n(0, 0, 0);
    bool nComputed = false;
    for (int i = 0; i <= numPts; ++i) {
        const Vec tmp = v0;
        v0 = v1;
        v1 = v2;
        v2 = tmp;
        v2 = p[(i + 2) % numPts];
        const Vec a = v2 - v1;
        const Vec b = v0 - v1;
        if (!nComputed) {
            const double aMag = a.norm();
            const double bMag = b.norm();
            if (aMag > kDblEps && bMag > kDblEps) {
                n = a.cross(b);
                nComputed = n.norm() >
                            kDblEps * (aMag < bMag ? aMag : bMag);
            }
            continue;
        }
        const Vec ni = a.cross(b);
        if (ni.norm() > kDblEps && n.dot(ni) < 0.0) {
            return false;
        }
    }
    return true;
}
// ---------------------------------------------------------------------------
// 三维单元拓扑与检查（vtkHexahedron / vtkTetra / vtkWedge / vtkPyramid）
// ---------------------------------------------------------------------------
struct CellTopology {
    std::vector<std::pair<int, int>> edges;
    std::vector<std::vector<int>> faces;  // 外向顺序
};
const CellTopology& TetraTopology() {
    static const CellTopology t = {
        {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}},
        {{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {0, 3, 2}}};
    return t;
}
const CellTopology& HexTopology() {
    // 面顺序与循环起点严格对齐 vtkHexahedron 的 faces 表（VTK 9.4）。
    static const CellTopology t = {
        {{0, 1}, {1, 2}, {3, 2}, {0, 3}, {4, 5}, {5, 6},
         {7, 6}, {4, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}},
        {{0, 4, 7, 3}, {1, 2, 6, 5}, {0, 1, 5, 4},
         {3, 7, 6, 2}, {0, 3, 2, 1}, {4, 5, 6, 7}}};
    return t;
}
const CellTopology& PyramidTopology() {
    static const CellTopology t = {
        {{0, 1}, {1, 2}, {2, 3}, {3, 0},
         {0, 4}, {1, 4}, {2, 4}, {3, 4}},
        {{0, 3, 2,1}, {0, 1, 4}, {1, 2, 4},
         {2, 3, 4}, {3, 0, 4}}};
    return t;
}
const CellTopology& WedgeTopology() {
    static const CellTopology t = {
        {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3},
         {0, 3}, {1, 4}, {2, 5}},
        {{0, 2, 1}, {3, 4, 5}, {0, 1, 4, 3},
         {1, 2, 5, 4}, {2, 0, 3, 5}}};
    return t;
}
bool NoIntersectingEdges3D(const CellTopology& topo,
                           const std::vector<Vec>& p, double tolerance) {
    const int n = static_cast<int>(topo.edges.size());
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (LineSegmentsIntersect(p[topo.edges[i].first],
                                      p[topo.edges[i].second],
                                      p[topo.edges[j].first],
                                      p[topo.edges[j].second], tolerance)) {
                return false;
            }
        }
    }
    return true;
}
bool NoIntersectingFaces3D(const CellTopology& topo,
                           const std::vector<Vec>& p, double tolerance) {
    const int nFaces = static_cast<int>(topo.faces.size());
    // 每个面扇形三角化。
    std::vector<std::vector<std::array<int, 3>>> faceTris(nFaces);
    for (int f = 0; f < nFaces; ++f) {
        const auto& loop = topo.faces[f];
        for (size_t k = 1; k + 1 < loop.size(); ++k) {
            faceTris[f].push_back({loop[0], loop[k], loop[k + 1]});
        }
    }
    for (int fi = 0; fi < nFaces; ++fi) {
        for (int fj = fi + 1; fj < nFaces; ++fj) {
            for (const auto& ta : faceTris[fi]) {
                const Vec A[3] = {p[ta[0]], p[ta[1]], p[ta[2]]};
                for (const auto& tb : faceTris[fj]) {
                    const Vec B[3] = {p[tb[0]], p[tb[1]], p[tb[2]]};
                    if (TrianglesOfFacesIntersect(A, B, tolerance)) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}
// 建立边 -> 相邻两个面的映射，键为 lo*nPts+hi（lo<=hi）。
std::map<long long, std::array<int, 2>>
BuildEdgeFaceMap(const CellTopology& topo, int nPts) {
    std::map<long long, std::array<int, 2>> edgeFaces;
    for (int fi = 0; fi < static_cast<int>(topo.faces.size()); ++fi) {
        const auto& loop = topo.faces[fi];
        for (int k = 0; k < static_cast<int>(loop.size()); ++k) {
            const int a = loop[k];
            const int b = loop[(k + 1) % loop.size()];
            const int lo = std::min(a, b);
            const int hi = std::max(a, b);
            const long long key =
                static_cast<long long>(lo) * nPts + hi;
            auto it = edgeFaces.find(key);
            if (it == edgeFaces.end()) {
                edgeFaces.emplace(key, std::array<int, 2>{{fi, -1}});
            } else if (it->second[1] == -1) {
                it->second[1] = fi;
            } else {
                // More than two incident faces is invalid for these topologies.
                it->second[1] = -2;
            }
        }
    }
    return edgeFaces;
}
// 复刻 vtkEdgeTable 的遍历顺序：按较小顶点升序，同一顶点内按边首次插入
// （面按存储顺序、边按环顺序）的先后排列。
std::vector<std::array<int, 2>>
BuildEdgeTableOrder(const CellTopology& topo, int nPts) {
    std::vector<std::vector<int>> table(nPts);
    for (const auto& loop : topo.faces) {
        for (int k = 0; k < static_cast<int>(loop.size()); ++k) {
            const int a = loop[k];
            const int b = loop[(k + 1) % loop.size()];
            const int lo = std::min(a, b);
            const int hi = std::max(a, b);
            auto& list = table[lo];
            if (std::find(list.begin(), list.end(), hi) == list.end()) {
                list.push_back(hi);
            }
        }
    }
    std::vector<std::array<int, 2>> order;
    for (int lo = 0; lo < nPts; ++lo) {
        for (int hi : table[lo]) order.push_back({lo, hi});
    }
    return order;
}
// vtkPolyhedron::IsConvex（Devillers 局部凸性 + seam 全局检查）。
bool IsConvex3D(const CellTopology& topo, const std::vector<Vec>& p) {
    const int nFaces = static_cast<int>(topo.faces.size());
    const int nPts = static_cast<int>(p.size());
    std::vector<Vec> faceNorm(nFaces), faceCentroid(nFaces);
    for (int f = 0; f < nFaces; ++f) {
        std::vector<Vec> fp;
        fp.reserve(topo.faces[f].size());
        for (int idx : topo.faces[f]) {
            fp.push_back(p[idx]);
        }
        faceNorm[f] = PolygonNormal(fp);
        // 与 vtkPolyhedron::IsConvex 一致：使用 vtkPolygon::ComputeCentroid，
        // 忽略返回值（退化时质心为零）。
        PolygonCentroid(fp, faceCentroid[f]);
    }
    const auto edgeFaceMap = BuildEdgeFaceMap(topo, nPts);
    const auto edgeOrder = BuildEdgeTableOrder(topo, nPts);
    std::vector<int> d(nPts, 0);
    std::vector<double> px(nPts, 0.0);
    int r = 0;
    for (const auto& edge : edgeOrder) {
        const int w0 = edge[0];
        const int w1 = edge[1];
        const long long key =
            static_cast<long long>(w0) * nPts + w1;
        const auto mapIt = edgeFaceMap.find(key);
        if (mapIt == edgeFaceMap.end() || mapIt->second[0] < 0 ||
            mapIt->second[1] < 0) {
            return false;
        }
        const int f0 = mapIt->second[0];
        const int f1 = mapIt->second[1];
        // 局部凸性：两面质心中点必须位于两面内侧。
        const Vec cmid = (faceCentroid[f0] + faceCentroid[f1]) * 0.5;
        const double dot0 = faceNorm[f0].dot(cmid - faceCentroid[f0]);
        const double dot1 = faceNorm[f1].dot(cmid - faceCentroid[f1]);
        if (dot0 > 0.0 || dot1 > 0.0) {
            return false;
        }
        // seam edge 检查。
        const Vec ned = (p[w1] - p[w0]).normalized();
        if (std::fabs(ned[0]) < kTolerance &&
            std::fabs(ned[1]) < kTolerance) {
            continue;
        }
        const Vec np(ned[1], -ned[0], 0.0);
        const Vec emid = (p[w0] + p[w1]) * 0.5;
        const double tmp0 = np.dot(faceCentroid[f0] - emid);
        const double tmp1 = np.dot(faceCentroid[f1] - emid);
        if ((tmp0 < 0.0) != (tmp1 < 0.0)) {
            continue;
        }
        // VTK 9.4：取“面心相对边中点的向量在垂直于边方向上的投影”z 分量
        // 较大者为上面，再取其面法向的 z 分量。
        const Vec c0p = faceCentroid[f0] - emid;
        const Vec c1p = faceCentroid[f1] - emid;
        const double projZ0 = c0p[2] - c0p.dot(ned) * ned[2];
        const double projZ1 = c1p[2] - c1p.dot(ned) * ned[2];
        const double topFaceNz =
            projZ0 > projZ1 ? faceNorm[f0][2] : faceNorm[f1][2];
        if (std::fabs(topFaceNz) < kTolerance) {
            continue;
        }
        const int w[2] = {w0, w1};
        for (int i = 0; i < 2; ++i) {
            const int v = w[i];
            if (d[v] == 2) return false;
            if (d[v] == 0) {
                d[v]++;
                px[v] = p[w[(i + 1) % 2]][0];
            } else {
                d[v]++;
                if (p[w[i]][0] > p[w[(i + 1) % 2]][0] &&
                    p[w[i]][0] > px[v]) {
                    if (r == 0) r++;
                    else return false;
                }
            }
        }
    }
    return true;
}
// vtkCellValidator::FacesAreOrientedCorrectly（线性单元均为外向约定）。
bool FacesAreOrientedCorrectly3D(const CellTopology& topo,
                                 const std::vector<Vec>& p,
                                 double tolerance) {
    Vec cellCentroid(0, 0, 0);
    for (const Vec& v : p) cellCentroid = cellCentroid + v;
    cellCentroid = cellCentroid * (1.0 / static_cast<double>(p.size()));
    for (const auto& loop : topo.faces) {
        std::vector<Vec> fp;
        Vec fc(0, 0, 0);
        for (int idx : loop) {
            fp.push_back(p[idx]);
            fc = fc + p[idx];
        }
        fc = fc * (1.0 / static_cast<double>(fp.size()));
        // Match vtkCellValidator::Check(face): triangle and quad checks differ.
        bool faceValid = false;
        if (fp.size() == 3) {
            faceValid = ContiguousEdges2D(fp, tolerance);
        } else if (fp.size() == 4) {
            faceValid = NoIntersectingEdges2D(fp, tolerance) &&
                        ContiguousEdges2D(fp, tolerance) && IsConvex2D(fp);
        } else if (fp.size() >= 3) {
            faceValid = NoIntersectingEdges2D(fp, tolerance) &&
                        ContiguousEdges2D(fp, tolerance) && IsConvex2D(fp);
        }
        if (!faceValid) {
            return false;
        }
        const Vec fn = PolygonNormal(fp);
        const Vec dir = (fc - cellCentroid).normalized();
        // 外向约定：法向应与“中心 -> 面心”同向，点积不得为负。
        if (fn.dot(dir) < 0.0) {
            return false;
        }
    }
    return true;
}
// ---------------------------------------------------------------------------
// 单元校验入口
// ---------------------------------------------------------------------------
short ValidateCell(IGenum cellType, const std::vector<Vec>& p) {
    switch (cellType) {
        case IG_VERTEX:
            return p.size() == 1 ? Validity_Valid
                                 : Validity_WrongNumberOfPoints;
        case IG_LINE:
            return p.size() == 2 ? Validity_Valid
                                 : Validity_WrongNumberOfPoints;
        case IG_POLY_LINE:
            return p.size() >= 2 ? Validity_Valid
                                 : Validity_WrongNumberOfPoints;
        case IG_TRIANGLE: {
            if (p.size() != 3) return Validity_WrongNumberOfPoints;
            return ContiguousEdges2D(p, kTolerance)
                       ? Validity_Valid
                       : Validity_NoncontiguousEdges;
        }
        case IG_QUAD: {
            if (p.size() != 4) return Validity_WrongNumberOfPoints;
            short state = Validity_Valid;
            if (!NoIntersectingEdges2D(p, kTolerance))
                state |= Validity_IntersectingEdges;
            if (!ContiguousEdges2D(p, kTolerance))
                state |= Validity_NoncontiguousEdges;
            if (!IsConvex2D(p)) state |= Validity_Nonconvex;
            return state;
        }
        case IG_POLYGON:
        case IG_FACE: {
            if (p.size() < 3) return Validity_WrongNumberOfPoints;
            short state = Validity_Valid;
            if (!NoIntersectingEdges2D(p, kTolerance))
                state |= Validity_IntersectingEdges;
            if (!ContiguousEdges2D(p, kTolerance))
                state |= Validity_NoncontiguousEdges;
            if (!IsConvex2D(p)) state |= Validity_Nonconvex;
            return state;
        }
        case IG_TETRA: {
            if (p.size() != 4) return Validity_WrongNumberOfPoints;
            const CellTopology& t = TetraTopology();
            short state = Validity_Valid;
            if (!NoIntersectingEdges3D(t, p, kTolerance))
                state |= Validity_IntersectingEdges;
            if (!NoIntersectingFaces3D(t, p, kTolerance))
                state |= Validity_IntersectingFaces;
            return state;
        }
        case IG_HEXAHEDRON: {
            if (p.size() != 8) return Validity_WrongNumberOfPoints;
            const CellTopology& t = HexTopology();
            short state = Validity_Valid;
            if (!NoIntersectingEdges3D(t, p, kTolerance))
                state |= Validity_IntersectingEdges;
            if (!NoIntersectingFaces3D(t, p, kTolerance))
                state |= Validity_IntersectingFaces;
            if (!IsConvex3D(t, p)) state |= Validity_Nonconvex;
            if (!FacesAreOrientedCorrectly3D(t, p, kTolerance))
                state |= Validity_FacesAreOrientedIncorrectly;
            return state;
        }
        case IG_PYRAMID: {
            if (p.size() != 5) return Validity_WrongNumberOfPoints;
            const CellTopology& t = PyramidTopology();
            short state = Validity_Valid;
            if (!NoIntersectingEdges3D(t, p, kTolerance))
                state |= Validity_IntersectingEdges;
            if (!NoIntersectingFaces3D(t, p, kTolerance))
                state |= Validity_IntersectingFaces;
            if (!IsConvex3D(t, p)) state |= Validity_Nonconvex;
            if (!FacesAreOrientedCorrectly3D(t, p, kTolerance))
                state |= Validity_FacesAreOrientedIncorrectly;
            return state;
        }
        case IG_PRISM: {
            if (p.size() != 6) return Validity_WrongNumberOfPoints;
            const CellTopology& t = WedgeTopology();
            short state = Validity_Valid;
            if (!NoIntersectingEdges3D(t, p, kTolerance))
                state |= Validity_IntersectingEdges;
            if (!NoIntersectingFaces3D(t, p, kTolerance))
                state |= Validity_IntersectingFaces;
            if (!IsConvex3D(t, p)) state |= Validity_Nonconvex;
            if (!FacesAreOrientedCorrectly3D(t, p, kTolerance))
                state |= Validity_FacesAreOrientedIncorrectly;
            return state;
        }
        default:
            // 与 vtkCellValidator 一致：未知类型默认有效。
            return Validity_Valid;
    }
}
// ---------------------------------------------------------------------------
// 网格读取与复制
// ---------------------------------------------------------------------------
std::vector<Vec> GetCellPoints(const UnstructuredMesh::Pointer& mesh,
                               IGsize cellId, const IdArray::Pointer& ids) {
    ids->Reset();
    mesh->GetCellPointIds(cellId, ids);
    std::vector<Vec> pts;
    pts.reserve(static_cast<size_t>(ids->GetNumberOfIds()));
    for (int i = 0; i < ids->GetNumberOfIds(); ++i) {
        pts.push_back(ToVec(mesh->GetPoint(ids->GetId(i))));
    }
    return pts;
}
std::vector<Vec> GetFacePoints(const SurfaceMesh::Pointer& mesh,
                               IGsize faceId) {
    const int nPts = mesh->GetNumberOfPoints();
    if (nPts <= 0) return {};
    std::vector<igIndex> ids(static_cast<size_t>(nPts));
    const int nFacePts = mesh->GetFacePointIds(faceId, ids.data());
    if (nFacePts <= 0) return {};
    ids.resize(static_cast<size_t>(nFacePts));
    std::vector<Vec> pts;
    pts.reserve(ids.size());
    for (igIndex id : ids) pts.push_back(ToVec(mesh->GetPoint(id)));
    return pts;
}
DataObject::Pointer CloneMeshForValidation(DataObject::Pointer input) {
    if (input == nullptr) return nullptr;
    auto copyPoints = [](PointSet* src) -> Points::Pointer {
        auto points = Points::New();
        points->DeepCopy(src->GetPoints());
        return points;
    };
    auto copyAttributes = [](DataObject* src) -> AttributeSet::Pointer {
        auto attributes = AttributeSet::New();
        attributes->DeepCopy(src->GetAttributeSet());
        return attributes;
    };
    if (auto src = DynamicCast<UnstructuredMesh>(input)) {
        auto dst = UnstructuredMesh::New();
        dst->SetPoints(copyPoints(src.GetPointer()));
        auto cells = CellArray::New();
        cells->DeepCopy(src->GetCells());
        auto types = UnsignedIntArray::New();
        types->DeepCopy(src->GetCellTypes());
        dst->SetCells(cells, types);
        dst->SetAttributeSet(copyAttributes(src.GetPointer()));
        dst->SetName(src->GetName() + "_ValidateCells");
        return dst;
    }
    if (auto src = DynamicCast<SurfaceMesh>(input)) {
        auto dst = SurfaceMesh::New();
        dst->SetPoints(copyPoints(src.GetPointer()));
        if (src->GetFaces() != nullptr) {
            auto faces = CellArray::New();
            faces->DeepCopy(src->GetFaces());
            dst->SetFaces(faces);
        }
        dst->SetAttributeSet(copyAttributes(src.GetPointer()));
        dst->SetName(src->GetName() + "_ValidateCells");
        return dst;
    }
    return nullptr;
}
}  // namespace
// ---------------------------------------------------------------------------
// ValidateCellsFilter
// ---------------------------------------------------------------------------
ValidateCellsFilter::ValidateCellsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
void ValidateCellsFilter::ResetResults(IGsize count) {
    m_ValidityStates.assign(static_cast<size_t>(count), Validity_Valid);
    m_InvalidCellIds.clear();
    m_LastError.clear();
}
void ValidateCellsFilter::RecordCell(IGsize cellId, short state) {
    m_ValidityStates[static_cast<size_t>(cellId)] = state;
    if (state != Validity_Valid) {
        m_InvalidCellIds.push_back(static_cast<igIndex>(cellId));
    }
}
void ValidateCellsFilter::ApplyResultAttribute(DataObject* output) {
    if (output == nullptr) return;
    ShortArray::Pointer stateArray = ShortArray::New();
    stateArray->SetName(ValidityStateArrayName());
    stateArray->SetDimension(1);
    stateArray->Resize(static_cast<IGsize>(m_ValidityStates.size()));
    for (size_t i = 0; i < m_ValidityStates.size(); ++i) {
        stateArray->SetValue(static_cast<IGsize>(i), m_ValidityStates[i]);
    }
    auto* attributes = output->GetAttributeSet();
    if (attributes != nullptr) {
        while (true) {
            const int index =
                attributes->GetAttributeIndex(ValidityStateArrayName());
            if (index < 0) break;
            attributes->DeleteAttribute(index);
        }
        stateArray->Modified();
        attributes->AddAttribute(IG_SCALAR, IG_CELL, stateArray);
        attributes->Modified();
    }
}
std::string ValidateCellsFilter::GetValidityStateText(int state) {
    if (state == Validity_Valid) {
        return "有效";
    }
    const std::pair<int, const char*> flags[] = {
        {Validity_WrongNumberOfPoints, "点数量错误"},
        {Validity_IntersectingEdges, "边相交"},
        {Validity_IntersectingFaces, "面相交"},
        {Validity_NoncontiguousEdges, "边不连续"},
        {Validity_Nonconvex, "非凸"},
        {Validity_FacesAreOrientedIncorrectly, "面朝向错误"},
    };
    std::string text;
    for (const auto& flag : flags) {
        if ((state & flag.first) != 0) {
            if (!text.empty()) text += "、";
            text += flag.second;
        }
    }
    return text.empty() ? "无效" : text;
}
bool ValidateCellsFilter::Execute() {
    ResetResults(0);
    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) {
        m_LastError = "没有输入数据对象。";
        return false;
    }
    if (DynamicCast<UnstructuredMesh>(input).IsNull() &&
        DynamicCast<SurfaceMesh>(input).IsNull()) {
        m_LastError = "输入数据类型不支持，需要 UnstructuredMesh 或 SurfaceMesh。";
        return false;
    }
    DataObject::Pointer output = CloneMeshForValidation(input);
    if (output == nullptr) {
        m_LastError = "无法复制输入网格，无法生成独立输出节点。";
        return false;
    }
    if (auto mesh = DynamicCast<UnstructuredMesh>(output)) {
        const IGsize nCells = mesh->GetNumberOfCells();
        ResetResults(nCells);
        IdArray::Pointer ids = IdArray::New();
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            const IGenum cellType = mesh->GetCellType(cellId);
            const std::vector<Vec> pts = GetCellPoints(mesh, cellId, ids);
            const short state = ValidateCell(cellType, pts);
            RecordCell(cellId, state);
        }
        ApplyResultAttribute(mesh.GetPointer());
        mesh->Modified();
    } else if (auto mesh = DynamicCast<SurfaceMesh>(output)) {
        const IGsize nFaces = mesh->GetNumberOfFaces();
        ResetResults(nFaces);
        for (IGsize faceId = 0; faceId < nFaces; ++faceId) {
            const std::vector<Vec> pts = GetFacePoints(mesh, faceId);
            const short state = ValidateCell(IG_POLYGON, pts);
            RecordCell(faceId, state);
        }
        ApplyResultAttribute(mesh.GetPointer());
        mesh->Modified();
    } else {
        m_LastError = "内部错误：输出网格类型无效。";
        return false;
    }
    SetOutput(0, output);
    return true;
}
IGAME_NAMESPACE_END
