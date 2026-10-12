#include "iGameResampleToImageFilter.h"

#include "iGameAttributeSet.h"
#include "iGameArrayObject.h"
#include "iGameCellArray.h"
#include "iGameCellType.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// 匿名命名空间：点定位与插值权重的几何计算。
// 等价于 VTK 中 vtkCell::EvaluatePosition（对常见线性单元），返回点是否在
// 单元内，并给出各节点的插值权重。
namespace {

constexpr double kInsideEps = 1.0e-7;    // 重心/局部坐标的「在单元内」容差
constexpr double kDegenerateEps = 1.0e-20; // 退化解的最小行列式
constexpr double kRelDistTol = 1.0e-6;   // 点到单元表面的距离容差（相对单元尺度）

//------------------------------------------------------------------------------
// 属性数组的「原类型保留」：按源数组的存储类型新建同类型输出数组。
// 等价 VTK 在探测输出上按源数组类型复制数组（int 保持 int、double 保持 double）。
ArrayObject::Pointer NewArrayLike(const ArrayObject::Pointer& src) {
    switch (src->GetArrayType()) {
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

// 是否为「离散」存储类型（整型/字符型）：线性插值对其语义不正确。
bool IsDiscreteArrayType(IGenum arrayType) {
    switch (arrayType) {
        case IG_IntArray:
        case IG_UnsignedIntArray:
        case IG_CharArray:
        case IG_UnsignedCharArray:
        case IG_ShortArray:
        case IG_UnsignedShortArray:
        case IG_LongLongArray:
        case IG_UnsignedLongLongArray:
            return true;
        default:
            return false;
    }
}

// 名称形如 ID 的数组（即使存储为浮点，线性插值同样会插出并不存在的 ID）。
bool IsIdLikeArrayName(const std::string& name) {
    static const char* kExactNames[] = {"vtkOriginalPointIds", "vtkOriginalCellIds", "vtkPointIds",
                                        "vtkCellIds",          "GlobalPointIds",     "GlobalCellIds",
                                        "ProcessIds",          "vtkProcessId"};
    for (const char* n : kExactNames) {
        if (name == n) return true;
    }
    // 以 Id / Ids 结尾（大小写敏感，避免把 fluid 之类的普通名字误判为 ID）
    if (name.size() >= 2 && name.compare(name.size() - 2, 2, "Id") == 0) return true;
    if (name.size() >= 3 && name.compare(name.size() - 3, 3, "Ids") == 0) return true;
    return false;
}

// 本过滤器当前支持的单元类型（与 EvaluateCell 的分支保持一致）。
bool IsSupportedCellType(IGenum cellType) {
    switch (static_cast<IGCellType>(cellType)) {
        case IG_VERTEX:
        case IG_LINE:
        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_TETRA:
        case IG_HEXAHEDRON:
            return true;
        default:
            return false;
    }
}

// 输出告警（核心库没有 igWarning 宏，直接走 logger 的 warn 级别）。
void ReportWarning(const std::string& msg) {
    ::iGame::Log::GetCoreLogger()->warn("{}", msg);
}

// 与 vtkDataSetAttributes 一致（保持 ghost 数值一一对应）
constexpr unsigned char kHiddenPoint = 2;  // vtkDataSetAttributes::HIDDENPOINT
constexpr unsigned char kHiddenCell = 32;  // vtkDataSetAttributes::HIDDENCELL

inline double Dot(const double a[3], const double b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline void Cross(const double a[3], const double b[3], double out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

// det([a b c]) = a . (b x c)
inline double Det3(const double a[3], const double b[3], const double c[3]) {
    return a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0]) +
           a[2] * (b[0] * c[1] - b[1] * c[0]);
}

inline bool Solve3(const double A[3][3], const double b[3], double x[3]) {
    double M[3][4];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) M[i][j] = A[i][j];
        M[i][3] = b[i];
    }
    for (int col = 0; col < 3; ++col) {
        int piv = col;
        for (int r = col + 1; r < 3; ++r) {
            if (std::fabs(M[r][col]) > std::fabs(M[piv][col])) piv = r;
        }
        if (std::fabs(M[piv][col]) < kDegenerateEps) return false;
        if (piv != col) {
            for (int c = 0; c < 4; ++c) std::swap(M[piv][c], M[col][c]);
        }
        for (int r = col + 1; r < 3; ++r) {
            const double f = M[r][col] / M[col][col];
            for (int c = col; c < 4; ++c) M[r][c] -= f * M[col][c];
        }
    }
    for (int r = 2; r >= 0; --r) {
        double s = M[r][3];
        for (int c = r + 1; c < 3; ++c) s -= M[r][c] * x[c];
        x[r] = s / M[r][r];
    }
    return true;
}

inline bool Solve2(const double A[2][2], const double b[2], double x[2]) {
    const double det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    if (std::fabs(det) < kDegenerateEps) return false;
    x[0] = (b[0] * A[1][1] - b[1] * A[0][1]) / det;
    x[1] = (A[0][0] * b[1] - A[1][0] * b[0]) / det;
    return true;
}

// 单元尺度（最长对角线长度），用作「点到单元距离」相对容差的基准。
inline double CellScale(const double pts[][3], int npts) {
    double maxSq = 0.0;
    for (int i = 0; i < npts; ++i) {
        for (int j = i + 1; j < npts; ++j) {
            double d[3] = {pts[i][0] - pts[j][0], pts[i][1] - pts[j][1], pts[i][2] - pts[j][2]};
            maxSq = std::max(maxSq, Dot(d, d));
        }
    }
    return std::sqrt(maxSq);
}

// 顶点单元
bool EvalVertex(const double pts[][3], const double x[3], double w[8]) {
    double d = 0.0;
    for (int i = 0; i < 3; ++i) d += (x[i] - pts[0][i]) * (x[i] - pts[0][i]);
    w[0] = 1.0;
    return d <= 1.0e-12;
}

// 线段单元（线性插值）。
// 判定必须同时满足：
//   (1) 参数 t 落在 [0,1]；
//   (2) 点到直线的**垂直距离**足够小（否则离线段很远的点只要轴向投影落在区间内
//       也会被误判为「在单元内」，从而产生错误的插值结果）。
bool EvalLine(const double pts[][3], const double x[3], double w[8], double lc[4]) {
    double dir[3] = {pts[1][0] - pts[0][0], pts[1][1] - pts[0][1], pts[1][2] - pts[0][2]};
    double rel[3] = {x[0] - pts[0][0], x[1] - pts[0][1], x[2] - pts[0][2]};
    const double denom = Dot(dir, dir);
    if (denom < kDegenerateEps) {
        lc[0] = 0.0;
        return EvalVertex(pts, x, w);
    }
    const double t = Dot(rel, dir) / denom;
    lc[0] = t;
    w[0] = 1.0 - t;
    w[1] = t;

    // 垂直距离：rel 去掉沿 dir 的分量后的剩余长度
    double perp[3] = {rel[0] - t * dir[0], rel[1] - t * dir[1], rel[2] - t * dir[2]};
    const double perp2 = Dot(perp, perp);
    const double distTol = kRelDistTol * std::sqrt(denom);
    if (perp2 > distTol * distTol) return false;

    return t >= -kInsideEps && t <= 1.0 + kInsideEps;
}

// 三角形单元（平面重心坐标）。
// 判定必须同时满足：
//   (1) 点到三角形**所在平面**的距离足够小——重心坐标只是点在该平面内投影的坐标，
//       离平面很远的点其投影仍可能落在三角形内部，只判断重心坐标会误判为「在单元内」；
//   (2) 三个重心坐标均非负（允许 kInsideEps 级别的微小外溢）。
bool EvalTriangle(const double pts[][3], const double x[3], double w[8], double lc[4]) {
    double v0[3] = {pts[1][0] - pts[0][0], pts[1][1] - pts[0][1], pts[1][2] - pts[0][2]};
    double v1[3] = {pts[2][0] - pts[0][0], pts[2][1] - pts[0][1], pts[2][2] - pts[0][2]};
    double v2[3] = {x[0] - pts[0][0], x[1] - pts[0][1], x[2] - pts[0][2]};
    const double d00 = Dot(v0, v0), d01 = Dot(v0, v1), d11 = Dot(v1, v1);
    const double d20 = Dot(v2, v0), d21 = Dot(v2, v1);
    const double denom = d00 * d11 - d01 * d01;
    if (std::fabs(denom) < kDegenerateEps) {
        lc[0] = 0.0;
        lc[1] = 0.0;
        w[0] = 1.0;
        w[1] = 0.0;
        w[2] = 0.0;
        return false;
    }
    const double v = (d11 * d20 - d01 * d21) / denom;
    const double u = (d00 * d21 - d01 * d20) / denom;
    const double t = 1.0 - v - u;
    lc[0] = v;
    lc[1] = u;
    w[0] = t;
    w[1] = v;
    w[2] = u;

    // 点到平面距离：|(x - p0)·n| / |n|，其中 n = v0 × v1
    double n[3];
    Cross(v0, v1, n);
    const double nlen = std::sqrt(Dot(n, n));
    if (nlen < kDegenerateEps) return false; // 三点共线，退化三角形
    const double dist = std::fabs(Dot(v2, n)) / nlen;
    if (dist > kRelDistTol * CellScale(pts, 3)) return false;

    return t >= -kInsideEps && v >= -kInsideEps && u >= -kInsideEps;
}

// 四面体单元（三维重心坐标，与节点顺序无关）
bool EvalTetra(const double pts[][3], const double x[3], double w[8], double lc[4]) {
    double e1[3] = {pts[1][0] - pts[0][0], pts[1][1] - pts[0][1], pts[1][2] - pts[0][2]};
    double e2[3] = {pts[2][0] - pts[0][0], pts[2][1] - pts[0][1], pts[2][2] - pts[0][2]};
    double e3[3] = {pts[3][0] - pts[0][0], pts[3][1] - pts[0][1], pts[3][2] - pts[0][2]};
    double rhs[3] = {x[0] - pts[0][0], x[1] - pts[0][1], x[2] - pts[0][2]};
    const double det = Det3(e1, e2, e3);
    if (std::fabs(det) < kDegenerateEps) {
        for (int i = 0; i < 4; ++i) lc[i] = 0.0;
        w[0] = 1.0;
        for (int i = 1; i < 4; ++i) w[i] = 0.0;
        return false;
    }
    const double w1 = Det3(rhs, e2, e3) / det;
    const double w2 = Det3(e1, rhs, e3) / det;
    const double w3 = Det3(e1, e2, rhs) / det;
    const double w0 = 1.0 - w1 - w2 - w3;
    w[0] = w0;
    w[1] = w1;
    w[2] = w2;
    w[3] = w3;
    lc[0] = w0;
    lc[1] = w1;
    lc[2] = w2;
    lc[3] = w3;
    return w0 >= -kInsideEps && w1 >= -kInsideEps && w2 >= -kInsideEps && w3 >= -kInsideEps;
}

// 四边形单元（双线性，**三维** Gauss-Newton 迭代求局部坐标 r,s）。
// 原实现只取雅可比的 x/y 两个分量（等价于把四边形投影到 XY 平面再求解），
// 对不平行于 XY 平面、或发生翘曲（warped / 非平面）的四边形会给出错误结果。
// 这里改为在三维中最小化 ||x - P(r,s)||²，用正规方程 (JᵀJ)δ = Jᵀ(x-P) 迭代，
// 对任意朝向的四边形都成立；同时用收敛后的残差判断点是否真的落在四边形表面上。
bool EvalQuad(const double pts[][3], const double x[3], double w[8], double lc[4]) {
    const double R[4] = {-1.0, 1.0, 1.0, -1.0};
    const double S[4] = {-1.0, -1.0, 1.0, 1.0};
    const double scale = CellScale(pts, 4);
    if (scale <= 0.0) { // 四点重合，退化
        lc[0] = 0.0;
        lc[1] = 0.0;
        w[0] = 1.0;
        for (int i = 1; i < 4; ++i) w[i] = 0.0;
        return false;
    }
    const double distTol = kRelDistTol * scale;

    double r = 0.0, s = 0.0;
    double residual2 = std::numeric_limits<double>::max();
    for (int iter = 0; iter < 100; ++iter) {
        double P[3] = {0.0, 0.0, 0.0};
        double dPdr[3] = {0.0, 0.0, 0.0};
        double dPds[3] = {0.0, 0.0, 0.0};
        for (int i = 0; i < 4; ++i) {
            const double N = 0.25 * (1.0 + r * R[i]) * (1.0 + s * S[i]);
            const double dNdr = 0.25 * R[i] * (1.0 + s * S[i]);
            const double dNds = 0.25 * S[i] * (1.0 + r * R[i]);
            for (int c = 0; c < 3; ++c) {
                P[c] += N * pts[i][c];
                dPdr[c] += dNdr * pts[i][c];
                dPds[c] += dNds * pts[i][c];
            }
        }
        const double f[3] = {x[0] - P[0], x[1] - P[1], x[2] - P[2]};
        residual2 = Dot(f, f);
        if (residual2 <= distTol * distTol) break; // 已落在四边形表面上

        double JtJ[2][2] = {{Dot(dPdr, dPdr), Dot(dPdr, dPds)},
                            {Dot(dPdr, dPds), Dot(dPds, dPds)}};
        double Jtf[2] = {Dot(dPdr, f), Dot(dPds, f)};
        double delta[2];
        if (!Solve2(JtJ, Jtf, delta)) { // 雅可比奇异，退化四边形
            lc[0] = 0.0;
            lc[1] = 0.0;
            w[0] = 1.0;
            for (int i = 1; i < 4; ++i) w[i] = 0.0;
            return false;
        }
        r += delta[0];
        s += delta[1];
        if (std::fabs(delta[0]) < 1.0e-14 && std::fabs(delta[1]) < 1.0e-14) break;
    }

    lc[0] = r;
    lc[1] = s;
    for (int i = 0; i < 4; ++i) w[i] = 0.25 * (1.0 + r * R[i]) * (1.0 + s * S[i]);

    // 迭代收敛后点仍不在四边形表面上 → 不在单元内
    if (residual2 > distTol * distTol) return false;
    if (std::fabs(r) > 1.0 + kInsideEps || std::fabs(s) > 1.0 + kInsideEps) return false;
    return true;
}

// 六面体单元（三线性，Newton 迭代求局部坐标 r,s,t）
bool EvalHex(const double pts[][3], const double x[3], double w[8], double lc[4]) {
    const double R[8] = {-1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0};
    const double S[8] = {-1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0};
    const double T[8] = {-1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0};
    double r = 0.0, s = 0.0, t = 0.0;
    for (int iter = 0; iter < 100; ++iter) {
        double P[3] = {0.0, 0.0, 0.0};
        double dP[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
        for (int i = 0; i < 8; ++i) {
            const double N = 0.125 * (1.0 + r * R[i]) * (1.0 + s * S[i]) * (1.0 + t * T[i]);
            const double dNdr = 0.125 * R[i] * (1.0 + s * S[i]) * (1.0 + t * T[i]);
            const double dNds = 0.125 * S[i] * (1.0 + r * R[i]) * (1.0 + t * T[i]);
            const double dNdt = 0.125 * T[i] * (1.0 + r * R[i]) * (1.0 + s * S[i]);
            for (int c = 0; c < 3; ++c) {
                P[c] += N * pts[i][c];
                dP[0][c] += dNdr * pts[i][c];
                dP[1][c] += dNds * pts[i][c];
                dP[2][c] += dNdt * pts[i][c];
            }
        }
        double f[3] = {x[0] - P[0], x[1] - P[1], x[2] - P[2]};
        double delta[3];
        if (!Solve3(dP, f, delta)) { // 雅可比奇异，退化六面体
            lc[0] = 0.0;
            lc[1] = 0.0;
            lc[2] = 0.0;
            w[0] = 1.0;
            for (int i = 1; i < 8; ++i) w[i] = 0.0;
            return false;
        }
        r += delta[0];
        s += delta[1];
        t += delta[2];
        if (std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]) < 1.0e-12) break;
    }
    lc[0] = r;
    lc[1] = s;
    lc[2] = t;
    for (int i = 0; i < 8; ++i) {
        w[i] = 0.125 * (1.0 + r * R[i]) * (1.0 + s * S[i]) * (1.0 + t * T[i]);
    }
    if (std::fabs(r) > 1.0 + 1.0e-6 || std::fabs(s) > 1.0 + 1.0e-6 || std::fabs(t) > 1.0 + 1.0e-6) {
        return false;
    }
    return true;
}

// 与 vtkProbeFilter::CELL_TOLERANCE_FACTOR_SQR 一致的单元尺度容差系数
constexpr double kCellToleranceFactorSqr = 1.0e-6;

// 包围盒对角线长度的平方（等价 vtkDataSet::GetLength2 的量纲）
double BoxLength2(const std::array<double, 6>& b) {
    const double dx = b[3] - b[0];
    const double dy = b[4] - b[1];
    const double dz = b[5] - b[2];
    return dx * dx + dy * dy + dz * dz;
}

// 把单纯形（三角形 / 四面体）的重心权重截断到非负并归一化，
// 等价 VTK 把 pcoords 截断到单元参数域后重新计算权重。
void ClampSimplexWeights(double* w, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        if (w[i] < 0.0) w[i] = 0.0;
        sum += w[i];
    }
    if (sum > kDegenerateEps) {
        for (int i = 0; i < n; ++i) w[i] /= sum;
    } else {
        for (int i = 0; i < n; ++i) w[i] = (i == 0) ? 1.0 : 0.0;
    }
}

// 等价 vtkCell::EvaluatePosition 的「最近点」语义：先解出单元局部坐标，再把局部坐标截断到
// 单元参数域内，用截断后的坐标计算插值权重，并给出点到单元的距离平方 dist2（点在外时 > 0）。
// 与上面的 EvaluateCell 的区别：EvaluateCell 只回答「点是否严格落在单元内」（布尔判定），
// 本函数额外给出点位于单元外时的最近点权重与距离，供 vtkProbeFilter 语义的容差判定使用。
bool EvaluateCellClosest(IGenum cellType, const double pts[][3], int npts, const double x[3],
                         double weights[8], double& dist2) {
    static const double RQ[4] = {-1.0, 1.0, 1.0, -1.0};
    static const double SQ[4] = {-1.0, -1.0, 1.0, 1.0};
    static const double RH[8] = {-1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0};
    static const double SH[8] = {-1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0};
    static const double TH[8] = {-1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0};

    double lc[4] = {0.0, 0.0, 0.0, 0.0};
    double w[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    switch (static_cast<IGCellType>(cellType)) {
        case IG_VERTEX:
            if (npts < 1) return false;
            w[0] = 1.0;
            break;
        case IG_LINE:
            if (npts < 2) return false;
            EvalLine(pts, x, w, lc);
            lc[0] = std::min(1.0, std::max(0.0, lc[0]));
            w[0] = 1.0 - lc[0];
            w[1] = lc[0];
            break;
        case IG_TRIANGLE:
            if (npts < 3) return false;
            EvalTriangle(pts, x, w, lc);
            w[0] = 1.0 - lc[0] - lc[1];
            w[1] = lc[0];
            w[2] = lc[1];
            ClampSimplexWeights(w, 3);
            break;
        case IG_QUAD:
            if (npts < 4) return false;
            EvalQuad(pts, x, w, lc);
            lc[0] = std::min(1.0, std::max(-1.0, lc[0]));
            lc[1] = std::min(1.0, std::max(-1.0, lc[1]));
            for (int i = 0; i < 4; ++i) {
                w[i] = 0.25 * (1.0 + lc[0] * RQ[i]) * (1.0 + lc[1] * SQ[i]);
            }
            break;
        case IG_TETRA:
            if (npts < 4) return false;
            EvalTetra(pts, x, w, lc);
            ClampSimplexWeights(w, 4);
            break;
        case IG_HEXAHEDRON:
            if (npts < 8) return false;
            EvalHex(pts, x, w, lc);
            lc[0] = std::min(1.0, std::max(-1.0, lc[0]));
            lc[1] = std::min(1.0, std::max(-1.0, lc[1]));
            lc[2] = std::min(1.0, std::max(-1.0, lc[2]));
            for (int i = 0; i < 8; ++i) {
                w[i] = 0.125 * (1.0 + lc[0] * RH[i]) * (1.0 + lc[1] * SH[i]) * (1.0 + lc[2] * TH[i]);
            }
            break;
        default:
            // IG_PRISM / IG_PYRAMID / IG_POLYGON / IG_POLYHEDRON / 二阶单元暂不支持
            return false;
    }

    // 最近点 = Σ w_i · p_i，dist2 = |x - 最近点|²
    double p[3] = {0.0, 0.0, 0.0};
    double wsum = 0.0;
    for (int v = 0; v < npts && v < 8; ++v) {
        wsum += w[v];
        p[0] += w[v] * pts[v][0];
        p[1] += w[v] * pts[v][1];
        p[2] += w[v] * pts[v][2];
    }
    if (std::fabs(wsum) < kDegenerateEps) {
        p[0] = pts[0][0];
        p[1] = pts[0][1];
        p[2] = pts[0][2];
    } else if (std::fabs(wsum - 1.0) > 1.0e-12) {
        p[0] /= wsum;
        p[1] /= wsum;
        p[2] /= wsum;
    }
    const double d[3] = {x[0] - p[0], x[1] - p[1], x[2] - p[2]};
    dist2 = Dot(d, d);
    for (int v = 0; v < npts && v < 8; ++v) weights[v] = w[v];
    return true;
}

// 单元定位与插值权重总入口。
// 返回值：true=点在单元内，weights[0..n-1] 为各节点权重。
// 注意：四边形/六面体假定节点顺序与 VTK 一致（0..3 / 0..7 规范顺序）。
bool EvaluateCell(IGenum cellType, const double pts[][3], int npts, const double x[3],
                  double weights[8]) {
    double lc[4] = {0.0, 0.0, 0.0, 0.0};
    switch (static_cast<IGCellType>(cellType)) {
        case IG_VERTEX:
            return npts >= 1 && EvalVertex(pts, x, weights);
        case IG_LINE:
            return npts >= 2 && EvalLine(pts, x, weights, lc);
        case IG_TRIANGLE:
            return npts >= 3 && EvalTriangle(pts, x, weights, lc);
        case IG_QUAD:
            return npts >= 4 && EvalQuad(pts, x, weights, lc);
        case IG_TETRA:
            return npts >= 4 && EvalTetra(pts, x, weights, lc);
        case IG_HEXAHEDRON:
            return npts >= 8 && EvalHex(pts, x, weights, lc);
        default:
            // IG_PRISM / IG_PYRAMID / IG_POLYGON / IG_POLYHEDRON / 二阶单元暂不支持，
            // 视作不在单元内。
            return false;
    }
}

} // namespace

//------------------------------------------------------------------------------
ResampleToImageFilter::ResampleToImageFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void ResampleToImageFilter::SetSamplingDimensions(int i, int j, int k) {
    SamplingDimensions[0] = i;
    SamplingDimensions[1] = j;
    SamplingDimensions[2] = k;
}

void ResampleToImageFilter::SetSamplingDimensions(int dims[3]) {
    SetSamplingDimensions(dims[0], dims[1], dims[2]);
}

void ResampleToImageFilter::GetSamplingDimensions(int dims[3]) const {
    dims[0] = SamplingDimensions[0];
    dims[1] = SamplingDimensions[1];
    dims[2] = SamplingDimensions[2];
}

void ResampleToImageFilter::SetSamplingBounds(const double bounds[6]) {
    for (int i = 0; i < 6; ++i) SamplingBounds[i] = bounds[i];
}

void ResampleToImageFilter::SetSamplingBounds(double x0, double x1, double y0, double y1, double z0,
                                              double z1) {
    SamplingBounds[0] = x0;
    SamplingBounds[1] = x1;
    SamplingBounds[2] = y0;
    SamplingBounds[3] = y1;
    SamplingBounds[4] = z0;
    SamplingBounds[5] = z1;
}

void ResampleToImageFilter::GetSamplingBounds(double bounds[6]) const {
    for (int i = 0; i < 6; ++i) bounds[i] = SamplingBounds[i];
}

//------------------------------------------------------------------------------
const char* ResampleToImageFilter::GetSupportedCellTypesText() {
    return "Vertex / Line / Triangle / Quad / Tetra / Hexahedron";
}

//------------------------------------------------------------------------------
bool ResampleToImageFilter::IsCellTypeSupported(IGenum cellType) {
    return IsSupportedCellType(cellType);
}

//------------------------------------------------------------------------------
bool ResampleToImageFilter::EstimateOutputSize(IGsize& gridPoints, IGsize& gridCells,
                                               double& memoryMB) {
    gridPoints = 0;
    gridCells = 0;
    memoryMB = 0.0;

    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) return false;

    int dims[3];
    GetSamplingDimensions(dims);
    if (dims[0] <= 0 || dims[1] <= 0 || dims[2] <= 0) return false;

    gridPoints = static_cast<IGsize>(dims[0]) * static_cast<IGsize>(dims[1]) *
                 static_cast<IGsize>(dims[2]);
    gridCells = static_cast<IGsize>(std::max(1, dims[0] - 1)) *
                static_cast<IGsize>(std::max(1, dims[1] - 1)) *
                static_cast<IGsize>(std::max(1, dims[2] - 1));

    // 粗略内存量级：点 ghost + 掩膜 + 约 4 条 float 数组（按分量数粗估）+ 单元 ghost
    const double bytes = static_cast<double>(gridPoints) * (1.0 + 1.0 + 4.0 * 4.0) +
                         static_cast<double>(gridCells) * 1.0;
    memoryMB = bytes / (1024.0 * 1024.0);
    return true;
}

//------------------------------------------------------------------------------
bool ResampleToImageFilter::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) return false;

    // 统一把输入当作 UnstructuredMesh 处理（等价 VTK 接受 vtkDataSet）。
    UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(input);
    if (mesh == nullptr) {
        mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    }
    if (mesh == nullptr) {
        igError("ResampleToImageFilter: input must be a mesh (PointSet subclass).");
        return false;
    }

    const IGsize numberOfPoints = mesh->GetNumberOfPoints();
    const IGsize numberOfCells = mesh->GetNumberOfCells();
    if (numberOfPoints == 0 || numberOfCells == 0) {
        igError("ResampleToImageFilter: empty input.");
        return false;
    }

    // ---- 单元类型预扫描 ----
    // 明确支持的单元类型；遇到不支持的必须提示，不能静默产出不完整的结果。
    m_Message.clear();
    std::map<IGenum, IGsize> cellTypeCounts;
    for (IGsize cid = 0; cid < numberOfCells; ++cid) {
        ++cellTypeCounts[mesh->GetCellType(cid)];
    }
    IGsize unsupportedCellCount = 0;
    std::string unsupportedDetail;
    for (const auto& kv : cellTypeCounts) {
        if (IsSupportedCellType(kv.first)) continue;
        const char* typeName = GetCellTypeAsString(kv.first);
        const std::string name = (typeName != nullptr && *typeName != '\0')
                                         ? std::string(typeName)
                                         : ("类型#" + std::to_string(static_cast<long long>(kv.first)));
        unsupportedCellCount += kv.second;
        if (!unsupportedDetail.empty()) unsupportedDetail += "、";
        unsupportedDetail += name + "×" + std::to_string(static_cast<long long>(kv.second));
    }
    if (unsupportedCellCount > 0) {
        const std::string head =
                "输入包含 " + std::to_string(static_cast<long long>(unsupportedCellCount)) +
                " 个不受支持的单元（" + unsupportedDetail + "）；本过滤器仅支持 " +
                GetSupportedCellTypesText() + "。";
        if (m_FailOnUnsupportedCells) {
            m_Message = head + " 为不产出可能不完整的结果，已终止执行"
                               "（如需强行继续可调用 SetFailOnUnsupportedCells(false)）。";
            igError("{}", m_Message);
            return false;
        }
        m_Message = head + " 这些单元覆盖的格点将保持无效（vtkValidPointMask = 0）。";
        ReportWarning("ResampleToImageFilter: " + m_Message);
    }

    // 采样区域（等价 VTK vtkResampleToImage::RequestData）
    double samplingBounds[6];
    if (UseInputBounds) {
        double b[6] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
                       std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
                       std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
        for (IGsize i = 0; i < numberOfPoints; ++i) {
            const Point& p = mesh->GetPoint(i);
            b[0] = std::min(b[0], static_cast<double>(p[0]));
            b[1] = std::max(b[1], static_cast<double>(p[0]));
            b[2] = std::min(b[2], static_cast<double>(p[1]));
            b[3] = std::max(b[3], static_cast<double>(p[1]));
            b[4] = std::min(b[4], static_cast<double>(p[2]));
            b[5] = std::max(b[5], static_cast<double>(p[2]));
        }
        // 向内收缩 epsilon，避免浮点舍入导致在数据集外采样。
        constexpr double epsilon = 1.0e-6;
        for (int i = 0; i < 3; ++i) {
            const double center = 0.5 * (b[2 * i] + b[2 * i + 1]);
            const double span = b[2 * i + 1] - b[2 * i];
            samplingBounds[2 * i] = center - 0.5 * span * (1.0 - epsilon);
            samplingBounds[2 * i + 1] = center + 0.5 * span * (1.0 - epsilon);
        }
    } else {
        for (int i = 0; i < 6; ++i) samplingBounds[i] = SamplingBounds[i];
    }

    int dims[3];
    GetSamplingDimensions(dims);
    if (dims[0] <= 0 || dims[1] <= 0 || dims[2] <= 0) {
        igError("ResampleToImageFilter: sampling dimensions must be positive.");
        return false;
    }

    const double origin[3] = {samplingBounds[0], samplingBounds[2], samplingBounds[4]};
    double spacing[3];
    for (int i = 0; i < 3; ++i) {
        spacing[i] = (dims[i] == 1) ? 0.0
                                    : (samplingBounds[2 * i + 1] - samplingBounds[2 * i]) /
                                          static_cast<double>(dims[i] - 1);
    }

    const IGsize numberOfGridPoints = static_cast<IGsize>(dims[0]) * dims[1] * dims[2];

    // 构建输出 StructuredMesh（等价 vtkImageData）
    StructuredMesh::Pointer output = StructuredMesh::New();
    output->SetName(input->GetName() + "_image");
    igIndex idims[3] = {static_cast<igIndex>(dims[0]), static_cast<igIndex>(dims[1]),
                        static_cast<igIndex>(dims[2])};
    output->SetDimensionSize(idims);

    Points::Pointer gridPoints = Points::New();
    gridPoints->Reserve(numberOfGridPoints);
    for (int k = 0; k < dims[2]; ++k) {
        for (int j = 0; j < dims[1]; ++j) {
            for (int i = 0; i < dims[0]; ++i) {
                gridPoints->AddPoint(static_cast<float>(origin[0] + i * spacing[0]),
                                     static_cast<float>(origin[1] + j * spacing[1]),
                                     static_cast<float>(origin[2] + k * spacing[2]));
            }
        }
    }
    output->SetPoints(gridPoints);
    output->GenStructuredCellConnectivities();

    // 收集待插值的输入点属性数组 + 待「快照」的输入单元属性数组。
    // 等价 vtkProbeFilter：源点数据插值到输出点数据；源单元数据在
    // (a) 与某点数组同名时被丢弃（点数据优先），否则作为输出点数组「快照」。
    struct SrcArray {
        ArrayObject::Pointer arr;
        IGenum type;
        int dim;
        bool nearestVertex; // true = 不做线性插值，改用最近顶点取值（离散/ID 类数组）
    };
    std::vector<SrcArray> srcPointArrays;
    std::vector<SrcArray> srcCellArrays;
    std::set<std::string> pointArrayNames;
    std::string discreteDetail;
    {
        auto all = mesh->GetAttributeSet()->GetAllAttributes();
        for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& a = all->GetElement(i);
            if (a.isDeleted || a.pointer == nullptr) continue;
            if (a.attachmentType == IG_POINT) {
                if (a.pointer->GetNumberOfElements() != numberOfPoints) continue;
                // 整型/字符型（离散）或名称形如 ID 的点数组不做线性插值：
                // 对 ID 做线性插值会得到并不存在的 ID，语义错误。
                const bool discrete = m_DisableInterpolationForDiscrete &&
                                      (IsDiscreteArrayType(a.pointer->GetArrayType()) ||
                                       IsIdLikeArrayName(a.pointer->GetName()));
                if (discrete) {
                    if (!discreteDetail.empty()) discreteDetail += "、";
                    discreteDetail += a.pointer->GetName();
                }
                srcPointArrays.push_back({a.pointer, a.type, a.pointer->GetDimension(), discrete});
                pointArrayNames.insert(a.pointer->GetName());
            } else if (a.attachmentType == IG_CELL) {
                if (a.pointer->GetNumberOfElements() != numberOfCells) continue;
                srcCellArrays.push_back({a.pointer, a.type, a.pointer->GetDimension(), false});
            }
        }
    }
    // 源单元数据 → 输出点数据（快照），跳过与点数组同名的。
    // 同名点/单元数组冲突时（等价 vtkProbeFilter）**点数据优先**；此处显式记录冲突，
    // 避免用户误以为单元数据也参与了输出。
    std::vector<size_t> snappedCellIndices;
    std::string conflictDetail;
    for (size_t c = 0; c < srcCellArrays.size(); ++c) {
        if (pointArrayNames.count(srcCellArrays[c].arr->GetName()) == 0) {
            snappedCellIndices.push_back(c);
        } else {
            if (!conflictDetail.empty()) conflictDetail += "、";
            conflictDetail += srcCellArrays[c].arr->GetName();
        }
    }

    // 掩膜数组（等价 vtkValidPointMask）
    CharArray::Pointer mask = CharArray::New();
    mask->SetName(GetMaskArrayName());
    mask->SetDimension(1);
    mask->Resize(numberOfGridPoints);

    // 每一条输入点属性对应一条输出插值数组（默认值 0）。
    // 输出数组**保持源数组的存储类型与分量数**（等价 VTK 在探测输出上按源数组类型复制）。
    std::vector<ArrayObject::Pointer> outPointArrays(srcPointArrays.size());
    for (size_t s = 0; s < srcPointArrays.size(); ++s) {
        outPointArrays[s] = NewArrayLike(srcPointArrays[s].arr);
        outPointArrays[s]->SetName(srcPointArrays[s].arr->GetName());
        outPointArrays[s]->SetDimension(srcPointArrays[s].dim);
        outPointArrays[s]->Resize(numberOfGridPoints);
    }
    // 每一条被快照的源单元属性 → 一条输出点数组（默认值 0），同样保持源数组类型。
    std::vector<ArrayObject::Pointer> outCellArrays(snappedCellIndices.size());
    for (size_t s = 0; s < snappedCellIndices.size(); ++s) {
        outCellArrays[s] = NewArrayLike(srcCellArrays[snappedCellIndices[s]].arr);
        outCellArrays[s]->SetName(srcCellArrays[snappedCellIndices[s]].arr->GetName());
        outCellArrays[s]->SetDimension(srcCellArrays[snappedCellIndices[s]].dim);
        outCellArrays[s]->Resize(numberOfGridPoints);
    }

    // 插值/快照缓冲区按属性最大分量数动态分配，避免固定 16 分量的越界风险。
    int maxDim = 1;
    for (size_t s = 0; s < srcPointArrays.size(); ++s) {
        maxDim = std::max(maxDim, srcPointArrays[s].dim);
    }
    for (size_t q = 0; q < snappedCellIndices.size(); ++q) {
        maxDim = std::max(maxDim, srcCellArrays[snappedCellIndices[q]].dim);
    }

    // 源单元 ghost 标记（若存在 "vtkGhostType" 单元属性），探测时跳过 ghost 单元。
    std::vector<unsigned char> srcGhostFlags(numberOfCells, 0);
    {
        auto all = mesh->GetAttributeSet()->GetAllAttributes();
        for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& a = all->GetElement(i);
            if (a.isDeleted || a.pointer == nullptr) continue;
            if (a.attachmentType == IG_CELL && a.pointer->GetName() == "vtkGhostType" &&
                a.pointer->GetNumberOfElements() == numberOfCells) {
                for (IGsize c = 0; c < numberOfCells; ++c) {
                    srcGhostFlags[c] = static_cast<unsigned char>(a.pointer->GetElementValue(c, 0));
                }
            }
        }
    }

    // 预计算每个输入单元的包围盒，用于快速剔除。
    std::vector<std::array<double, 6>> cellBox(numberOfCells);
    for (IGsize cid = 0; cid < numberOfCells; ++cid) {
        const igIndex* ids = nullptr;
        const int n = mesh->GetCellPointIds(cid, ids);
        double mn[3] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                        std::numeric_limits<double>::max()};
        double mx[3] = {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
                        std::numeric_limits<double>::lowest()};
        for (int v = 0; v < n; ++v) {
            const Point& p = mesh->GetPoint(ids[v]);
            mn[0] = std::min(mn[0], static_cast<double>(p[0]));
            mx[0] = std::max(mx[0], static_cast<double>(p[0]));
            mn[1] = std::min(mn[1], static_cast<double>(p[1]));
            mx[1] = std::max(mx[1], static_cast<double>(p[1]));
            mn[2] = std::min(mn[2], static_cast<double>(p[2]));
            mx[2] = std::max(mx[2], static_cast<double>(p[2]));
        }
        cellBox[cid] = {mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]};
    }

    // 对每个源单元，在其包围盒覆盖的格点范围内做探针插值
    // （等价 vtkProbeFilter::ProbeImagePointsInCell：单元定向遍历，而非格点定向）。
    // 与 vtkProbeFilter 一致的格点判定容差：
    //   ComputeTolerance = true（默认）→ tol2 = 最大单元长度² × 1e-6
    //   ComputeTolerance = false       → tol2 = Tolerance²（VTK 中 Tolerance 默认 1.0）
    double maxCellLength2 = 0.0;
    for (IGsize cid = 0; cid < numberOfCells; ++cid) {
        maxCellLength2 = std::max(maxCellLength2, BoxLength2(cellBox[cid]));
    }
    const double tol2 = m_ComputeTolerance ? maxCellLength2 * kCellToleranceFactorSqr
                                           : m_Tolerance * m_Tolerance;
    const double tolDist = std::sqrt(tol2);
    // 每个格点当前命中单元的距离平方：0 表示严格落在单元内；点在外时取「更近的单元」覆盖
    std::vector<double> bestDist2(numberOfGridPoints, std::numeric_limits<double>::max());

    std::vector<double> vals(static_cast<size_t>(maxDim));
    double pts[8][3];
    double weights[8];
    for (IGsize cid = 0; cid < numberOfCells; ++cid) {
        if (srcGhostFlags[cid] != 0) continue; // 跳过 ghost 源单元
        IGenum cellType = mesh->GetCellType(cid);
        if (static_cast<IGCellType>(cellType) != IG_VERTEX &&
            static_cast<IGCellType>(cellType) != IG_LINE &&
            static_cast<IGCellType>(cellType) != IG_TRIANGLE &&
            static_cast<IGCellType>(cellType) != IG_QUAD &&
            static_cast<IGCellType>(cellType) != IG_TETRA &&
            static_cast<IGCellType>(cellType) != IG_HEXAHEDRON) {
            continue; // 不支持的单元类型
        }
        const igIndex* ids = nullptr;
        const int n = mesh->GetCellPointIds(cid, ids);
        for (int v = 0; v < n; ++v) {
            const Point& p = mesh->GetPoint(ids[v]);
            pts[v][0] = static_cast<double>(p[0]);
            pts[v][1] = static_cast<double>(p[1]);
            pts[v][2] = static_cast<double>(p[2]);
        }

        // 单元包围盒覆盖的格点 ijk 范围（floor/ceil 覆盖，避免边界漏点）
        const std::array<double, 6>& bx = cellBox[cid];
        int lo[3], hi[3];
        bool overlap = true;
        for (int ax = 0; ax < 3; ++ax) {
            if (spacing[ax] == 0.0) {
                lo[ax] = hi[ax] = 0;
                continue;
            }
            // 容差判定允许「单元包围盒外、但距离在 tolDist 内」的格点参与，故外扩 tolDist
            int a = static_cast<int>(std::floor((bx[ax] - tolDist - origin[ax]) / spacing[ax]));
            int b = static_cast<int>(std::ceil((bx[ax + 3] + tolDist - origin[ax]) / spacing[ax]));
            if (a < 0) a = 0;
            if (b > dims[ax] - 1) b = dims[ax] - 1;
            if (a > b) {
                overlap = false;
                break;
            }
            lo[ax] = a;
            hi[ax] = b;
        }
        if (!overlap) continue;

        for (int k = lo[2]; k <= hi[2]; ++k) {
            for (int j = lo[1]; j <= hi[1]; ++j) {
                for (int i = lo[0]; i <= hi[0]; ++i) {
                    const IGsize ptId = static_cast<IGsize>(i + dims[0] * (j + dims[1] * k));
                    // 严格命中（dist2 == 0）的格点无需再判定；其余允许被「更近的单元」覆盖
                    if (mask->ValueAt(ptId) != 0 && bestDist2[ptId] <= 0.0) continue;
                    const double x[3] = {origin[0] + i * spacing[0], origin[1] + j * spacing[1],
                                         origin[2] + k * spacing[2]};
                    double dist2 = 0.0;
                    if (!EvaluateCellClosest(cellType, pts, n, x, weights, dist2)) continue;
                    if (dist2 > tol2) continue;
                    if (mask->ValueAt(ptId) != 0 && dist2 >= bestDist2[ptId]) continue;
                    bestDist2[ptId] = dist2;

                    mask->ValueAt(ptId) = 1;
                    // 插值源点属性。离散/ID 类数组不做线性插值，改用包含该格点的源单元中
                    // 权重最大的顶点取值（最近顶点采样）。
                    for (size_t s = 0; s < srcPointArrays.size(); ++s) {
                        const int dim = srcPointArrays[s].dim;
                        if (srcPointArrays[s].nearestVertex) {
                            int best = 0;
                            for (int v = 1; v < n; ++v) {
                                if (weights[v] > weights[best]) best = v;
                            }
                            for (int d = 0; d < dim; ++d) {
                                vals[d] = srcPointArrays[s].arr->GetElementValue(ids[best], d);
                            }
                        } else {
                            for (int d = 0; d < dim; ++d) vals[d] = 0.0;
                            for (int v = 0; v < n; ++v) {
                                const double wv = weights[v];
                                for (int d = 0; d < dim; ++d) {
                                    vals[d] += wv * srcPointArrays[s].arr->GetElementValue(ids[v], d);
                                }
                            }
                        }
                        outPointArrays[s]->SetElement(ptId, vals.data());
                    }
                    // 快照源单元属性（该点所在源单元的 cell data）
                    for (size_t q = 0; q < snappedCellIndices.size(); ++q) {
                        const SrcArray& ca = srcCellArrays[snappedCellIndices[q]];
                        for (int d = 0; d < ca.dim; ++d) {
                            vals[d] = ca.arr->GetElementValue(cid, d);
                        }
                        outCellArrays[q]->SetElement(ptId, vals.data());
                    }
                }
            }
        }
    }

    // ghost 标记（等价 vtkResampleToImage::SetBlankPointsAndCells）
    UnsignedCharArray::Pointer pointGhost = UnsignedCharArray::New();
    pointGhost->SetName("vtkGhostType");
    pointGhost->SetDimension(1);
    pointGhost->Resize(numberOfGridPoints);
    for (IGsize p = 0; p < numberOfGridPoints; ++p) {
        if (mask->ValueAt(p) == 0) {
            pointGhost->ValueAt(p) = kHiddenPoint;
        }
    }

    const int pointDim[3] = {dims[0], dims[1], dims[2]};
    const int cellDim[3] = {std::max(1, dims[0] - 1), std::max(1, dims[1] - 1),
                            std::max(1, dims[2] - 1)};
    const int span[3] = {(dims[0] > 1) ? 1 : 0, (dims[1] > 1) ? 1 : 0, (dims[2] > 1) ? 1 : 0};
    const IGsize pointSlice = static_cast<IGsize>(pointDim[0]) * pointDim[1];
    const IGsize cellSlice = static_cast<IGsize>(cellDim[0]) * cellDim[1];
    const IGsize numberOfOutCells = output->GetNumberOfCells();
    UnsignedCharArray::Pointer cellGhost = UnsignedCharArray::New();
    cellGhost->SetName("vtkGhostType");
    cellGhost->SetDimension(1);
    cellGhost->Resize(numberOfOutCells);
    for (IGsize cellId = 0; cellId < numberOfOutCells; ++cellId) {
        int ijk[3];
        ijk[2] = static_cast<int>(cellId / cellSlice);
        ijk[1] = static_cast<int>((cellId % cellSlice) / cellDim[0]);
        ijk[0] = static_cast<int>(cellId % cellDim[0]);
        const IGsize ptid = static_cast<IGsize>(ijk[0]) + pointDim[0] * ijk[1] + pointSlice * ijk[2];
        bool valid = true;
        for (int k = 0; k <= span[2]; ++k) {
            for (int j = 0; j <= span[1]; ++j) {
                for (int i = 0; i <= span[0]; ++i) {
                    valid = valid &&
                            (mask->ValueAt(ptid + static_cast<IGsize>(i) +
                                           static_cast<IGsize>(j) * pointDim[0] +
                                           static_cast<IGsize>(k) * pointSlice) != 0);
                }
            }
        }
        if (!valid) {
            cellGhost->ValueAt(cellId) = kHiddenCell;
        }
    }

    // 挂载输出属性（点数据）。
    AttributeSet* outAttrs = output->GetAttributeSet();
    for (size_t s = 0; s < srcPointArrays.size(); ++s) {
        outAttrs->AddAttribute(srcPointArrays[s].type, IG_POINT, outPointArrays[s]);
    }
    for (size_t k = 0; k < snappedCellIndices.size(); ++k) {
        outAttrs->AddAttribute(srcCellArrays[snappedCellIndices[k]].type, IG_POINT, outCellArrays[k]);
    }
    outAttrs->AddAttribute(IG_SCALAR, IG_POINT, mask);
    outAttrs->AddAttribute(IG_SCALAR, IG_POINT, pointGhost);
    outAttrs->AddAttribute(IG_SCALAR, IG_CELL, cellGhost);

    // ---- 诊断信息汇总（供界面提示，避免静默的不完整/可疑结果）----
    {
        IGsize validCount = 0;
        for (IGsize p = 0; p < numberOfGridPoints; ++p) {
            if (mask->ValueAt(p) != 0) ++validCount;
        }
        std::string info;
        info += "采样维度 " + std::to_string(dims[0]) + "×" + std::to_string(dims[1]) + "×" +
                std::to_string(dims[2]) + "（" +
                std::to_string(static_cast<long long>(numberOfGridPoints)) + " 格点 / " +
                std::to_string(static_cast<long long>(numberOfOutCells)) + " 单元）；";
        info += "有效格点 " + std::to_string(static_cast<long long>(validCount)) + "/" +
                std::to_string(static_cast<long long>(numberOfGridPoints)) + "。";
        if (!conflictDetail.empty()) {
            info += " 同名数组冲突：点数据优先，以下单元数组已被忽略（" + conflictDetail + "）。";
        }
        if (!discreteDetail.empty()) {
            info += " 以下离散/ID 类数组未做线性插值（改用最近顶点取值）：" + discreteDetail + "。";
        }
        if (!m_Message.empty()) m_Message += " ";
        m_Message += info;
    }

    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
