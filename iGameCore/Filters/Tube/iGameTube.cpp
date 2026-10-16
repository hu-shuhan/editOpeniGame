#include "iGameTube.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <cmath>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

constexpr double kPi = 3.14159265358979323846;

// 路径节点：坐标 + 对应原始输入点 id（用于映射点属性）
using Node = std::pair<Vector3d, int>;

// 选择与切线 t 最不平行的坐标轴（自动初始法向用）
Vector3d PerpendicularAxis(const Vector3d& t) {
    const Vector3d axes[3] = {
        Vector3d(1.0, 0.0, 0.0),
        Vector3d(0.0, 1.0, 0.0),
        Vector3d(0.0, 0.0, 1.0)
    };
    int best = 0;
    double bestAbs = 1e30;
    for (int i = 0; i < 3; ++i) {
        double a = std::abs(t.dot(axes[i]));
        if (a < bestAbs) { bestAbs = a; best = i; }
    }
    return axes[best];
}

// 把种子方向 seed 投影到垂直于切线 t 的平面，返回单位法向。
// 若 seed 与 t 重合（投影为 0，例如管子一开始竖直而 seed 也竖直），
// 自动改用与 t 不平行的坐标轴。
Vector3d BuildNormal(const Vector3d& t, const Vector3d& seed) {
    Vector3d n = seed - t * seed.dot(t);
    if (n.norm() < 1e-9) {
        Vector3d ax = PerpendicularAxis(t);
        n = ax - t * ax.dot(t);
    }
    return n.normalized();
}

// 复制输入点属性到输出，输出点 j 对应原始点 srcOf[j]。
// 用动态缓冲（不使用固定栈数组），彻底避免属性维数过大时栈越界。
// 与 ParaView 一致：只映射 PointData，不复制 CellData。
void CopyPointAttributes(AttributeSet* src, AttributeSet* dst,
                         const std::vector<int>& srcOf, int count) {
    if (src == nullptr || dst == nullptr) return;
    auto pointAttrs = src->GetAllPointAttributes();
    if (pointAttrs == nullptr) return;

    for (int i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
        AttributeSet::Attribute& inAttr = pointAttrs->GetElement(i);
        if (inAttr.IsDeleted()) continue;

        ArrayObject::Pointer inData = inAttr.GetPointer();
        if (inData == nullptr) continue;

        const int dim = inData->GetDimension();
        DoubleArray::Pointer newData = DoubleArray::New();
        newData->SetName(inData->GetName());
        newData->SetDimension(dim);
        newData->Resize(count);

        std::vector<double> tmp(dim);
        for (int j = 0; j < count; ++j) {
            inData->GetElement(srcOf[j], tmp.data());
            newData->SetElement(j, tmp.data());
        }
        dst->AddAttribute(inAttr.GetType(), IG_POINT, newData, inAttr.GetDataRange());
    }
}

} // anonymous namespace

TubeFilter::TubeFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void TubeFilter::SetRadius(double radius) { m_Radius = radius; }
double TubeFilter::GetRadius() const { return m_Radius; }

void TubeFilter::SetNumberOfSides(int sides) {
    m_NumberOfSides = sides < 3 ? 3 : sides;
}
int TubeFilter::GetNumberOfSides() const { return m_NumberOfSides; }

void TubeFilter::SetCapping(bool capping) { m_Capping = capping; }
bool TubeFilter::IsCapping() const { return m_Capping; }

void TubeFilter::SetUseDefaultNormal(bool useDefault) { m_UseDefaultNormal = useDefault; }
bool TubeFilter::IsUsingDefaultNormal() const { return m_UseDefaultNormal; }

void TubeFilter::SetDefaultNormal(const Vector3d& normal) { m_DefaultNormal = normal; }
Vector3d TubeFilter::GetDefaultNormal() const { return m_DefaultNormal; }

bool TubeFilter::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) return false;

    // ------------------------------------------------------------
    // 1. 直接按线单元收集路径（不做坐标合并 / 拓扑重建）：
    //      IG_LINE      -> 2 点路径
    //      IG_POLY_LINE -> 多点路径
    //    统一只用内部指针（const ref），不向任何栈缓冲拷贝。
    // ------------------------------------------------------------
    std::vector<std::vector<Node>> paths;
    PointSet* ps = nullptr;

    // 由点 id 序列构造一条路径，剔除完全重合的连续点
    auto makePath = [](PointSet* ms, const igIndex* ids, int n) {
        std::vector<Node> path;
        for (int i = 0; i < n; ++i) {
            const int orig = static_cast<int>(ids[i]);
            const Point& p = ms->GetPoint(orig);
            Vector3d pos(p[0], p[1], p[2]);
            if (!path.empty() && (pos - path.back().first).norm() < 1e-12)
                continue;
            path.emplace_back(pos, orig);
        }
        return path;
    };

    if (auto um = DynamicCast<UnstructuredMesh>(input)) {
        ps = um;
        const int nCells = um->GetNumberOfCells();
        for (int c = 0; c < nCells; ++c) {
            const IGenum type = um->GetCellType(c);
            if (type != IG_LINE && type != IG_POLY_LINE) continue;
            const igIndex* ptr = nullptr;
            const int n = um->GetCellPointIds(c, ptr);
            if (ptr == nullptr || n < 2) continue;
            auto path = makePath(ps, ptr, n);
            if (path.size() >= 2) paths.push_back(std::move(path));
        }
    } else if (auto sm = DynamicCast<SurfaceMesh>(input)) {
        ps = sm;
        CellArray* edgeArr = sm->GetEdges();
        if (edgeArr != nullptr) {
            const int nEdges = edgeArr->GetNumberOfCells();
            for (int e = 0; e < nEdges; ++e) {
                const igIndex* ptr = nullptr;
                const int n = edgeArr->GetCellIds(e, ptr);
                if (ptr == nullptr || n < 2) continue;
                auto path = makePath(ps, ptr, n);
                if (path.size() >= 2) paths.push_back(std::move(path));
            }
        }
    } else {
        return false;
    }

    if (ps == nullptr || paths.empty()) return false;
    if (m_Radius <= 0.0) return false;
    const int sides = m_NumberOfSides;

    // ------------------------------------------------------------
    // 2. 逐路径生成圆管
    // ------------------------------------------------------------
    Points::Pointer outPoints = Points::New();
    CellArray::Pointer outFaces = CellArray::New();
    std::vector<int> outSrc; // 输出点 -> 原始点 id
    auto addOut = [&](const Vector3d& v, int orig) -> igIndex {
        igIndex id = static_cast<igIndex>(outPoints->AddPoint(v));
        outSrc.push_back(orig);
        return id;
    };

    for (const auto& pathSource : paths) {
        std::vector<Node> path = pathSource;
        // 闭合路径检测：首尾节点重合（如方形折线 0→1→2→3→0），
        // 要求去掉重复末尾点后至少剩 3 个节点，否则按开放路径处理。
        bool closed = path.size() >= 4
                      && (path.front().first - path.back().first).norm() < 1e-9;
        if (closed) path.pop_back(); // 末尾点与首点重合，丢弃后按环处理
        const int n = static_cast<int>(path.size());

        // 2.1 每段单位方向；闭合路径多一段「末节点→首节点」
        const int segCount = closed ? n : n - 1;
        std::vector<Vector3d> sd(segCount);
        for (int i = 0; i < n - 1; ++i) {
            Vector3d d = path[i + 1].first - path[i].first;
            sd[i] = d.normalized();
        }
        if (closed) {
            Vector3d d = path[0].first - path[n - 1].first;
            sd[n - 1] = d.normalized();
        }
        // 2.2 切线 T：
        //   开放路径：端点取相邻段方向，内部点为相邻两段方向之和再归一化（与 VTK 一致）
        //   闭合路径：所有点按环取前后两段方向之和
        std::vector<Vector3d> T(n);
        if (closed) {
            for (int i = 0; i < n; ++i) {
                Vector3d s = sd[(i - 1 + n) % n] + sd[i];
                T[i] = (s.norm() < 1e-12) ? sd[i] : s.normalized();
            }
        } else {
            T[0] = sd[0];
            T[n - 1] = sd[n - 2];
            for (int i = 1; i < n - 1; ++i) {
                Vector3d s = sd[i - 1] + sd[i];
                T[i] = (s.norm() < 1e-12) ? sd[i] : s.normalized();
            }
        }

        // 2.3 第一个截面的 N、B
        std::vector<Vector3d> N(n), B(n);
        Vector3d seed = m_UseDefaultNormal ? m_DefaultNormal
                                          : PerpendicularAxis(T[0]);
        N[0] = BuildNormal(T[0], seed);
        B[0] = T[0].cross(N[0]).normalized();

        // 2.4 平行传递：上一点 N 投影到垂直新切线的平面（最小转动），
        //     退化时重新选轴，保证不翻转。
        for (int i = 1; i < n; ++i) {
            Vector3d ni = N[i - 1] - T[i] * N[i - 1].dot(T[i]);
            if (ni.norm() < 1e-9) {
                Vector3d ax = PerpendicularAxis(T[i]);
                ni = ax - T[i] * ax.dot(T[i]);
            }
            N[i] = ni.normalized();
            B[i] = T[i].cross(N[i]).normalized();
        }

        // 2.5 每个路径点生成正多边形截面 ring
        std::vector<std::vector<igIndex>> ring(n, std::vector<igIndex>(sides));
        for (int i = 0; i < n; ++i) {
            for (int k = 0; k < sides; ++k) {
                double th = 2.0 * kPi * k / sides;
                Vector3d v = path[i].first
                           + (N[i] * std::cos(th) + B[i] * std::sin(th)) * m_Radius;
                ring[i][k] = addOut(v, path[i].second);
            }
        }

        // 2.6 相邻 ring 缝合：每个四边形拆 2 个三角形（法向朝外）；
        //     闭合路径最后一个 ring 接回 ring[0]
        const int stitchCount = closed ? n : n - 1;
        for (int i = 0; i < stitchCount; ++i) {
            const int j = (i + 1) % n;
            for (int k = 0; k < sides; ++k) {
                int k2 = (k + 1) % sides;
                igIndex A = ring[i][k];
                igIndex Bp = ring[i][k2];
                igIndex C = ring[j][k2];
                igIndex D = ring[j][k];
                outFaces->AddCellId3(A, Bp, C);
                outFaces->AddCellId3(A, C, D);
            }
        }

        // 2.7 封端（三角形扇，法向朝外）；闭合路径无端点，不封端
        if (m_Capping && !closed) {
            igIndex c0 = addOut(path[0].first, path[0].second);
            for (int k = 0; k < sides; ++k) {
                int k2 = (k + 1) % sides;
                outFaces->AddCellId3(c0, ring[0][k2], ring[0][k]);
            }
            igIndex cn = addOut(path[n - 1].first, path[n - 1].second);
            for (int k = 0; k < sides; ++k) {
                int k2 = (k + 1) % sides;
                outFaces->AddCellId3(cn, ring[n - 1][k], ring[n - 1][k2]);
            }
        }
    }

    // ------------------------------------------------------------
    // 3. 组装输出 SurfaceMesh
    // ------------------------------------------------------------
    SurfaceMesh::Pointer outMesh = SurfaceMesh::New();
    outMesh->SetName(input->GetName() + "_tube");
    outMesh->SetPoints(outPoints);
    outMesh->SetFaces(outFaces);

    auto dstAttrs = outMesh->GetAttributeSet();
    CopyPointAttributes(ps->GetAttributeSet(), dstAttrs, outSrc,
                        static_cast<int>(outSrc.size()));

    dstAttrs->ForceReConvertToDrawableData();
    dstAttrs->Modified();
    outMesh->Modified();

    SetOutput(0, outMesh);
    return true;
}

IGAME_NAMESPACE_END
