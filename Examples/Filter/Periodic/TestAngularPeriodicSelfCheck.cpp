// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Periodic/TestAngularPeriodicSelfCheck.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
// 角度周期复制过滤器回归自检（无 GUI、无外部模型依赖）。
// 覆盖：多单元类型、PointData/CellData 属性、>16 点大单元、
//       VolumeMesh（不得按面类型错建）、StructuredMesh 显式拓扑。
#include <Periodic/iGameAngularPeriodicFilter.h>

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGamePoints.h>
#include <iGamePointSet.h>
#include <iGameStructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace iGame;

namespace {

int g_checks = 0;
int g_failures = 0;

void Expect(bool ok, const char* what) {
    ++g_checks;
    if (!ok) ++g_failures;
    std::printf("  -> %s : %s\n", ok ? "PASS" : "FAIL", what);
}

struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3(double _x = 0, double _y = 0, double _z = 0) : x(_x), y(_y), z(_z) {}
};

Vec3 Rodrigues(const Vec3& p, const Vec3& axis, const Vec3& origin, double angleRad) {
    const double cosA = std::cos(angleRad), sinA = std::sin(angleRad);
    const double vx = p.x - origin.x, vy = p.y - origin.y, vz = p.z - origin.z;
    const double cx = axis.y * vz - axis.z * vy;
    const double cy = axis.z * vx - axis.x * vz;
    const double cz = axis.x * vy - axis.y * vx;
    const double dot = axis.x * vx + axis.y * vy + axis.z * vz;
    const double k = dot * (1.0 - cosA);
    return Vec3(origin.x + vx * cosA + cx * sinA + axis.x * k,
                origin.y + vy * cosA + cy * sinA + axis.y * k,
                origin.z + vz * cosA + cz * sinA + axis.z * k);
}

// 与过滤器一致的张量旋转：out = R·T·Rᵀ（R 由 Rodrigues 构造）。
void RotateTensorMat(const Vec3& axisIn, double angleRad, const double T[3][3], double out[3][3]) {
    double R[3][3];
    for (int j = 0; j < 3; ++j) {
        Vec3 e(j == 0 ? 1.0 : 0.0, j == 1 ? 1.0 : 0.0, j == 2 ? 1.0 : 0.0);
        Vec3 col = Rodrigues(e, axisIn, Vec3(0, 0, 0), angleRad);
        R[0][j] = col.x;
        R[1][j] = col.y;
        R[2][j] = col.z;
    }
    double tmp[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double s = 0.0;
            for (int k = 0; k < 3; ++k) { s += R[i][k] * T[k][j]; }
            tmp[i][j] = s;
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double s = 0.0;
            for (int k = 0; k < 3; ++k) { s += tmp[i][k] * R[j][k]; }
            out[i][j] = s;
        }
    }
}

void CheckPointOnCopy(const Points* pts,
                      IGsize srcId, int copy, IGsize srcCount,
                      const Vec3& axisIn, const Vec3& origin, double angleStepRad) {
    double len = std::sqrt(axisIn.x * axisIn.x + axisIn.y * axisIn.y + axisIn.z * axisIn.z);
    const Vec3 axis = len > 1e-12 ? Vec3(axisIn.x / len, axisIn.y / len, axisIn.z / len) : axisIn;
    const auto& q0 = pts->GetPoint(srcId);
    const auto& qr = pts->GetPoint(srcId + copy * srcCount);
    const Vec3 p(q0[0], q0[1], q0[2]);
    const Vec3 q = Rodrigues(p, axis, origin, angleStepRad * copy);
    const double dx = qr[0] - q.x, dy = qr[1] - q.y, dz = qr[2] - q.z;
    Expect(dx * dx + dy * dy + dz * dz < 1e-5, "rotated coordinates match Rodrigues formula");
}

// ---------------- 输入网格构建 ----------------

FloatArray::Pointer FillPointAttr(const char* name, int dim, const std::vector<float>& values) {
    auto arr = FloatArray::New();
    arr->SetName(name);
    arr->SetDimension(dim);
    arr->Resize(values.size() / dim);
    for (size_t i = 0; i < values.size() / dim; ++i) {
        float* dst = arr->RawPointer(i);
        for (int d = 0; d < dim; ++d) { dst[d] = values[i * dim + d]; }
    }
    return arr;
}

UnstructuredMesh::Pointer MakeUnstructuredMixed() {
    auto mesh = UnstructuredMesh::New();
    auto pts = mesh->GetPoints();
    for (int i = 0; i < 12; ++i) {
        pts->AddPoint(Point(float(i % 3), float((i / 3) % 2), float(i / 6)));
    }
    struct C { IGenum type; std::vector<igIndex> ids; };
    std::vector<C> cs;
    cs.push_back({IG_TETRA, {0, 1, 2, 3}});
    cs.push_back({IG_HEXAHEDRON, {4, 5, 6, 7, 8, 9, 10, 11}});
    cs.push_back({IG_PYRAMID, {0, 1, 2, 3, 4}});
    cs.push_back({IG_PRISM, {1, 2, 3, 4, 5, 6}});
    for (const C& c : cs) {
        mesh->AddCell(const_cast<igIndex*>(c.ids.data()), static_cast<int>(c.ids.size()), c.type);
    }

    auto attributes = AttributeSet::New();
    std::vector<float> ps;
    for (int i = 0; i < 12; ++i) { ps.push_back(float(i)); }
    attributes->AddAttribute(IG_SCALAR, IG_POINT, FillPointAttr("P", 1, ps));
    std::vector<float> pv;
    for (int i = 0; i < 12; ++i) { pv.push_back(float(i)); pv.push_back(float(i * 2)); pv.push_back(float(-i)); }
    attributes->AddAttribute(IG_VECTOR, IG_POINT, FillPointAttr("PV", 3, pv));
    std::vector<float> csd;
    for (size_t i = 0; i < cs.size(); ++i) { csd.push_back(float(i * 10)); }
    attributes->AddAttribute(IG_SCALAR, IG_CELL, FillPointAttr("C", 1, csd));
    std::vector<float> cvd;
    for (size_t i = 0; i < cs.size(); ++i) { cvd.push_back(float(i)); cvd.push_back(float(i + 1)); cvd.push_back(float(i + 2)); }
    attributes->AddAttribute(IG_VECTOR, IG_CELL, FillPointAttr("CV", 3, cvd));
    std::vector<float> pt;
    for (int i = 0; i < 12; ++i) {
        for (int k = 0; k < 9; ++k) { pt.push_back(float(i * 9 + k)); }
    }
    attributes->AddAttribute(IG_TENSOR, IG_POINT, FillPointAttr("PT", 9, pt));
    // 法向量（IG_NORMAL，3 分量）：应随几何旋转
    std::vector<float> pn;
    for (int i = 0; i < 12; ++i) { pn.push_back(float(i + 1)); pn.push_back(float(-i)); pn.push_back(float(i * i)); }
    attributes->AddAttribute(IG_NORMAL, IG_POINT, FillPointAttr("PN", 3, pn));
    // 6 分量对称张量（Voigt: xx,yy,zz,xy,yz,xz）：应做 R·T·Rᵀ
    std::vector<float> pst;
    for (int i = 0; i < 12; ++i) {
        for (int k = 0; k < 6; ++k) { pst.push_back(float(i * 6 + k + 1)); }
    }
    attributes->AddAttribute(IG_TENSOR, IG_POINT, FillPointAttr("PS", 6, pst));
    mesh->SetAttributeSet(attributes);
    return mesh;
}

UnstructuredMesh::Pointer MakeUnstructuredWithBigPoly() {
    auto mesh = UnstructuredMesh::New();
    auto pts = mesh->GetPoints();
    const int bigN = 40; // >16 点的大单元
    for (int i = 0; i < bigN + 4; ++i) { pts->AddPoint(Point(float(i), 0.f, 0.f)); }
    std::vector<igIndex> big(bigN);
    for (int i = 0; i < bigN; ++i) { big[i] = igIndex(4 + i); }
    mesh->AddCell(big.data(), bigN, IG_POLYGON);
    igIndex quad[4] = {0, 1, 2, 3};
    mesh->AddCell(quad, 4, IG_QUAD);
    return mesh;
}

SurfaceMesh::Pointer MakeSurfaceMixed() {
    auto mesh = SurfaceMesh::New();
    auto pts = mesh->GetPoints();
    for (int i = 0; i < 8; ++i) { pts->AddPoint(Point(float(i), 0.f, 0.f)); }
    auto faces = CellArray::New();
    igIndex tri[3] = {0, 1, 2};
    igIndex quad[4] = {1, 2, 3, 4};
    std::vector<igIndex> ngon(8);
    for (int i = 0; i < 8; ++i) { ngon[i] = igIndex(i); }
    faces->AddCellIds(tri, 3);
    faces->AddCellIds(quad, 4);
    faces->AddCellIds(ngon.data(), 8);
    mesh->SetFaces(faces);
    return mesh;
}

VolumeMesh::Pointer MakeVolumeTetHex() {
    auto mesh = VolumeMesh::New();
    auto pts = Points::New();
    for (int i = 0; i < 12; ++i) {
        pts->AddPoint(Point(float(i % 3), float((i / 3) % 2), float(i / 6)));
    }
    mesh->SetPoints(pts);
    auto vols = CellArray::New();
    igIndex tet[4] = {0, 1, 2, 3};
    igIndex hex[8] = {4, 5, 6, 7, 8, 9, 10, 11};
    vols->AddCellIds(tet, 4);
    vols->AddCellIds(hex, 8);
    mesh->SetVolumes(vols);

    auto attributes = AttributeSet::New();
    std::vector<float> pd;
    for (int i = 0; i < 12; ++i) { pd.push_back(float(i)); }
    attributes->AddAttribute(IG_SCALAR, IG_POINT, FillPointAttr("P", 1, pd));
    std::vector<float> cd = {5.f, 9.f};
    attributes->AddAttribute(IG_SCALAR, IG_CELL, FillPointAttr("C", 1, cd));
    mesh->SetAttributeSet(attributes);
    return mesh;
}

StructuredMesh::Pointer MakeStructured3D() {
    auto mesh = StructuredMesh::New();
    auto pts = Points::New();
    const igIndex nx = 2, ny = 2, nz = 2;
    for (igIndex k = 0; k < nz; ++k)
        for (igIndex j = 0; j < ny; ++j)
            for (igIndex i = 0; i < nx; ++i) { pts->AddPoint(Point(float(i), float(j), float(k))); }
    mesh->SetPoints(pts);
    igIndex dims[3] = {nx, ny, nz};
    mesh->SetDimensionSize(dims);
    return mesh;
}

StructuredMesh::Pointer MakeStructured2D() {
    auto mesh = StructuredMesh::New();
    auto pts = Points::New();
    const igIndex nx = 2, ny = 2;
    for (igIndex j = 0; j < ny; ++j)
        for (igIndex i = 0; i < nx; ++i) { pts->AddPoint(Point(float(i), float(j), 0.f)); }
    mesh->SetPoints(pts);
    igIndex dims[3] = {nx, ny, 1};
    mesh->SetDimensionSize(dims);
    return mesh;
}

// ---------------- 运行与校验 ----------------

struct RunResult {
    UnstructuredMesh::Pointer out;
    bool ok = false;
};

RunResult RunFilter(PointSet::Pointer src, const Vec3& axis, const Vec3& origin,
                    int copies, float angleDeg) {
    RunResult r;
    auto filter = AngularPeriodicFilter::New();
    filter->SetInput(src);
    filter->SetRotationAxis(Point(float(origin.x), float(origin.y), float(origin.z)),
                            Vector3d(axis.x, axis.y, axis.z));
    filter->SetNumberOfCopies(copies);
    filter->SetAngle(angleDeg);
    if (!filter->Execute()) {
        std::printf("  filter Execute failed: %s\n", filter->GetMessage().c_str());
        return r;
    }
    r.out = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    r.ok = r.out != nullptr;
    return r;
}

IGsize GetCellSize(UnstructuredMesh* out, IGsize idx) {
    const igIndex* ids = nullptr;
    return out->GetCells()->GetCellIds(idx, ids);
}

IGenum GetCellTypeAt(UnstructuredMesh* out, IGsize idx) {
    auto types = out->GetCellTypes();
    return static_cast<IGenum>(types->GetValue(idx));
}

bool CheckCellCopies(UnstructuredMesh* out,
                     const std::vector<std::vector<igIndex>>& srcCells,
                     IGsize srcPointCount, int copies) {
    auto cellArray = out->GetCells();
    auto types = out->GetCellTypes();
    if (!cellArray || !types) return false;
    if (cellArray->GetNumberOfCells() != srcCells.size() * static_cast<size_t>(copies)) return false;
    for (int c = 0; c < copies; ++c) {
        for (size_t k = 0; k < srcCells.size(); ++k) {
            const IGsize idx = c * static_cast<IGsize>(srcCells.size()) + k;
            const igIndex* ids = nullptr;
            const int n = cellArray->GetCellIds(idx, ids);
            if (static_cast<size_t>(n) != srcCells[k].size()) return false;
            for (int j = 0; j < n; ++j) {
                if (ids[j] != srcCells[k][j] + c * srcPointCount) return false;
            }
        }
    }
    return true;
}

// ---------------- 用例 ----------------

void TestUnstructuredMixed() {
    std::printf("[case] unstructured mixed cell types + attributes\n");
    auto src = MakeUnstructuredMixed();
    const int copies = 3;
    const float angleDeg = 120.f; // 周期角度：相邻两份间隔 120°（0/120/240）
    const Vec3 axis(0, 0, 1), origin(0, 0, 0);
    auto r = RunFilter(src, axis, origin, copies, angleDeg);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    const IGsize srcNpts = 12, srcNc = 4;
    Expect(r.out->GetNumberOfPoints() == srcNpts * copies, "point count = src*copies");
    Expect(r.out->GetCells()->GetNumberOfCells() == srcNc * copies, "cell count = src*copies");

    const IGenum expect[4] = {IG_TETRA, IG_HEXAHEDRON, IG_PYRAMID, IG_PRISM};
    bool typeOk = true;
    for (int c = 0; c < copies; ++c)
        for (IGsize k = 0; k < srcNc; ++k)
            if (GetCellTypeAt(r.out.get(), c * srcNc + k) != expect[k]) typeOk = false;
    Expect(typeOk, "per-copy cell types preserved (no face-type rebuild)");

    std::vector<std::vector<igIndex>> cells;
    auto cellArray = src->GetCells();
    for (IGsize k = 0; k < srcNc; ++k) {
        const igIndex* ids = nullptr;
        int n = cellArray->GetCellIds(k, ids);
        cells.emplace_back(ids, ids + n);
    }
    Expect(CheckCellCopies(r.out.get(), cells, srcNpts, copies), "cell id offsets per copy correct");

    auto outAttrs = r.out->GetAttributeSet();
    auto ps = DynamicCast<FloatArray>(outAttrs->GetAttribute("P").pointer);
    auto pv = DynamicCast<FloatArray>(outAttrs->GetAttribute("PV").pointer);
    auto pt = DynamicCast<FloatArray>(outAttrs->GetAttribute("PT").pointer);
    auto pn = DynamicCast<FloatArray>(outAttrs->GetAttribute("PN").pointer);
    auto pst = DynamicCast<FloatArray>(outAttrs->GetAttribute("PS").pointer);
    auto csd = DynamicCast<FloatArray>(outAttrs->GetAttribute("C").pointer);
    auto cvd = DynamicCast<FloatArray>(outAttrs->GetAttribute("CV").pointer);
    Expect(ps && pv && pt && pn && pst && csd && cvd, "all point/cell attributes present");
    if (!(ps && pv && pt && pn && pst && csd && cvd)) return;

    const double step = static_cast<double>(angleDeg) * 3.141592653589793 / 180.0;

    bool scalarOk = true, vectorOk = true, tensorOk = true, normalOk = true, tensor6Ok = true;
    for (int c = 0; c < copies; ++c) {
        const double angleRad = step * c;
        for (IGsize i = 0; i < srcNpts; ++i) {
            const int ival = static_cast<int>(i);
            const float* pp = ps->RawPointer(c * srcNpts + i);
            if (pp[0] != float(ival)) scalarOk = false;

            // 向量随几何旋转：v' = R·v
            const float* pvp = pv->RawPointer(c * srcNpts + i);
            const Vec3 v = Rodrigues(Vec3(ival, ival * 2, -ival), axis, origin, angleRad);
            if (std::fabs(pvp[0] - v.x) > 1e-3 || std::fabs(pvp[1] - v.y) > 1e-3 ||
                std::fabs(pvp[2] - v.z) > 1e-3) {
                vectorOk = false;
            }

            // 法向量（IG_NORMAL）：同样随几何旋转
            const float* pnp = pn->RawPointer(c * srcNpts + i);
            const Vec3 n = Rodrigues(Vec3(float(ival + 1), float(-ival), float(ival * ival)), axis, origin, angleRad);
            if (std::fabs(pnp[0] - n.x) > 1e-3 || std::fabs(pnp[1] - n.y) > 1e-3 ||
                std::fabs(pnp[2] - n.z) > 1e-3) {
                normalOk = false;
            }

            // 9 分量张量按 R·T·Rᵀ 旋转
            const float* ptp = pt->RawPointer(c * srcNpts + i);
            double T[3][3], Tt[3][3];
            for (int m = 0; m < 3; ++m)
                for (int n2 = 0; n2 < 3; ++n2) T[m][n2] = static_cast<double>(ival * 9 + m * 3 + n2);
            RotateTensorMat(axis, angleRad, T, Tt);
            for (int m = 0; m < 3; ++m)
                for (int n2 = 0; n2 < 3; ++n2)
                    if (std::fabs(ptp[m * 3 + n2] - Tt[m][n2]) > 1e-3) tensorOk = false;

            // 6 分量对称张量（Voigt xx,yy,zz,xy,yz,xz）按 R·T·Rᵀ 旋转
            const float* psp = pst->RawPointer(c * srcNpts + i);
            const double s0 = ival * 6 + 1, s1 = ival * 6 + 2, s2 = ival * 6 + 3;
            const double s3 = ival * 6 + 4, s4 = ival * 6 + 5, s5 = ival * 6 + 6;
            double S[3][3] = {{s0, s3, s5}, {s3, s1, s4}, {s5, s4, s2}};
            double St[3][3];
            RotateTensorMat(axis, angleRad, S, St);
            const double exp6[6] = {St[0][0], St[1][1], St[2][2], St[0][1], St[1][2], St[0][2]};
            for (int k = 0; k < 6; ++k) {
                if (std::fabs(psp[k] - exp6[k]) > 1e-3) tensor6Ok = false;
            }
        }
        for (IGsize k = 0; k < srcNc; ++k) {
            const int kval = static_cast<int>(k);
            const float* cpp = csd->RawPointer(c * srcNc + k);
            if (cpp[0] != float(kval * 10)) scalarOk = false;

            const float* cvp = cvd->RawPointer(c * srcNc + k);
            const Vec3 v = Rodrigues(Vec3(kval, kval + 1, kval + 2), axis, origin, angleRad);
            if (std::fabs(cvp[0] - v.x) > 1e-3 || std::fabs(cvp[1] - v.y) > 1e-3 ||
                std::fabs(cvp[2] - v.z) > 1e-3) {
                vectorOk = false;
            }
        }
    }
    Expect(scalarOk, "scalar attributes replicated identically per copy");
    Expect(vectorOk, "point/cell vector attributes rotate with geometry");
    Expect(tensorOk, "point tensor attribute rotates as R*T*R^T");
    Expect(normalOk, "IG_NORMAL attribute rotates with geometry");
    Expect(tensor6Ok, "6-component symmetric tensor rotates as R*T*R^T");

    for (int c = 0; c < copies; ++c) {
        CheckPointOnCopy(r.out->GetPoints(), 0, c, srcNpts, axis, origin, step);
        CheckPointOnCopy(r.out->GetPoints(), 11, c, srcNpts, axis, origin, step);
    }
}

void TestUnstructuredBigPoly() {
    std::printf("[case] unstructured with >16-point polygon cell\n");
    auto src = MakeUnstructuredWithBigPoly();
    const int copies = 2;
    auto r = RunFilter(src, Vec3(0, 0, 1), Vec3(0, 0, 0), copies, 180.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    Expect(r.out->GetCells()->GetNumberOfCells() == 4, "2 cells x 2 copies (none dropped)");
    Expect(GetCellSize(r.out.get(), 0) == 40, "big polygon cell preserved (40 pts)");
    Expect(GetCellSize(r.out.get(), 2) == 40, "big polygon preserved in 2nd copy");
    Expect(GetCellTypeAt(r.out.get(), 0) == IG_POLYGON, "big polygon type = IG_POLYGON");

    std::vector<std::vector<igIndex>> cells;
    auto cellArray = src->GetCells();
    for (IGsize k = 0; k < cellArray->GetNumberOfCells(); ++k) {
        const igIndex* ids = nullptr;
        int n = cellArray->GetCellIds(k, ids);
        cells.emplace_back(ids, ids + n);
    }
    Expect(CheckCellCopies(r.out.get(), cells, src->GetNumberOfPoints(), copies),
           "cell id offsets of big cell correct");
}

void TestSurfaceMixed() {
    std::printf("[case] surface faces -> correct mapped types\n");
    auto src = MakeSurfaceMixed();
    auto r = RunFilter(src, Vec3(0, 0, 1), Vec3(0, 0, 0), 2, 180.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    Expect(r.out->GetCells()->GetNumberOfCells() == 6, "3 faces x 2 copies");
    Expect(GetCellTypeAt(r.out.get(), 0) == IG_TRIANGLE, "triangle");
    Expect(GetCellTypeAt(r.out.get(), 1) == IG_QUAD, "quad");
    Expect(GetCellTypeAt(r.out.get(), 2) == IG_POLYGON, "8-gon -> IG_POLYGON");
}

void TestVolumeMesh() {
    std::printf("[case] VolumeMesh must NOT be rebuilt by face point count\n");
    auto src = MakeVolumeTetHex();
    auto r = RunFilter(src, Vec3(0, 0, 1), Vec3(0, 0, 0), 2, 180.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    Expect(r.out->GetCells()->GetNumberOfCells() == 4, "2 volumes x 2 copies");
    Expect(GetCellTypeAt(r.out.get(), 0) == IG_TETRA, "4-pt volume is TETRA (not QUAD)");
    Expect(GetCellTypeAt(r.out.get(), 1) == IG_HEXAHEDRON, "8-pt volume is HEXAHEDRON");
    Expect(GetCellTypeAt(r.out.get(), 2) == IG_TETRA, "2nd copy TETRA");

    auto outAttrs = r.out->GetAttributeSet();
    auto ps = DynamicCast<FloatArray>(outAttrs->GetAttribute("P").pointer);
    auto ca = DynamicCast<FloatArray>(outAttrs->GetAttribute("C").pointer);
    Expect(ps != nullptr && ca != nullptr, "volume point/cell attributes present");
    Expect(ps && ps->RawPointer(0)[0] == 0.f && ps->RawPointer(12)[0] == 0.f,
           "point attr length = npts*copies");
    Expect(ca && ca->RawPointer(0)[0] == 5.f && ca->RawPointer(2)[0] == 5.f,
           "cell attr values replicated per copy");
}

void TestStructured() {
    std::printf("[case] StructuredMesh 3D (single hexa) + 2D (quad)\n");
    auto s3 = MakeStructured3D();
    auto r3 = RunFilter(s3, Vec3(1, 0, 0), Vec3(0, 0, 0), 2, 180.f);
    Expect(r3.ok, "structured 3D Execute returns true");
    if (r3.ok) {
        Expect(r3.out->GetCells()->GetNumberOfCells() == 2, "1 hexa x 2 copies");
        Expect(GetCellTypeAt(r3.out.get(), 0) == IG_HEXAHEDRON, "structured 3D cell = HEXAHEDRON");
    }

    auto s2 = MakeStructured2D();
    auto r2 = RunFilter(s2, Vec3(0, 0, 1), Vec3(0, 0, 0), 2, 180.f);
    Expect(r2.ok, "structured 2D Execute returns true");
    if (r2.ok) {
        Expect(r2.out->GetCells()->GetNumberOfCells() == 2, "1 quad x 2 copies");
        Expect(GetCellTypeAt(r2.out.get(), 0) == IG_QUAD, "structured 2D cell = QUAD");
    }
}

void TestPolyhedronCell() {
    std::printf("[case] unstructured IG_POLYHEDRON encoded cell (face/vertex offset split)\n");
    // 手工构造多面体：编码 = [faceCount, n0, 顶点..., n1, 顶点...]
    auto mesh = UnstructuredMesh::New();
    auto pts = mesh->GetPoints();
    for (int i = 0; i < 10; ++i) { pts->AddPoint(Point(float(i), float(i * i % 3), 0.f)); }
    std::vector<igIndex> poly = {2,                     // 2 个面
                                 3, 0, 1, 2,            // 面 0：三角形 0-1-2
                                 4, 3, 4, 5, 6};        // 面 1：四边形 3-4-5-6
    mesh->AddCell(poly.data(), static_cast<int>(poly.size()), IG_POLYHEDRON);

    auto r = RunFilter(mesh, Vec3(0, 0, 1), Vec3(0, 0, 0), 2, 180.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    Expect(r.out->GetCells()->GetNumberOfCells() == 2, "1 polyhedron x 2 copies");
    Expect(GetCellTypeAt(r.out.get(), 0) == IG_POLYHEDRON, "copy0 type = IG_POLYHEDRON");
    Expect(GetCellTypeAt(r.out.get(), 1) == IG_POLYHEDRON, "copy1 type = IG_POLYHEDRON");

    const igIndex* ids0 = nullptr;
    const igIndex* ids1 = nullptr;
    r.out->GetCells()->GetCellIds(0, ids0);
    r.out->GetCells()->GetCellIds(1, ids1);

    // copy0 应与输入编码一致；copy1 仅顶点号 +10，faceCount/每面顶点数不动
    const igIndex exp0[10] = {2, 3, 0, 1, 2, 4, 3, 4, 5, 6};
    const igIndex exp1[10] = {2, 3, 10, 11, 12, 4, 13, 14, 15, 16};
    bool ok0 = true, ok1 = true;
    for (int i = 0; i < 10; ++i) {
        if (ids0[i] != exp0[i]) ok0 = false;
        if (ids1[i] != exp1[i]) ok1 = false;
    }
    Expect(ok0, "copy0 polyhedron encoding unchanged");
    Expect(ok1, "copy1 polyhedron vertex ids offset, face counts preserved");
}

void TestNonAlignedAxis() {
    std::printf("[case] non-axis-aligned rotation axis\n");
    auto src = MakeUnstructuredMixed();
    auto r = RunFilter(src, Vec3(1, 1, 1), Vec3(0.2, -0.1, 0.4), 4, 90.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;
    const double step = 90.0 * 3.141592653589793 / 180.0;
    for (int c = 0; c < 4; ++c) {
        CheckPointOnCopy(r.out->GetPoints(), 0, c, 12, Vec3(1, 1, 1), Vec3(0.2, -0.1, 0.4), step);
    }
}

void TestIterationModeAndCoverage() {
    std::printf("[case] iteration mode (Direct/Max) + coverage info\n");
    auto src = MakeUnstructuredMixed();

    auto run = [&](int mode, int copies, float angle, bool requireFull) {
        struct R {
            UnstructuredMesh::Pointer out;
            bool ok = false;
            int effective = 0;
            std::string coverage;
        };
        R r;
        auto filter = AngularPeriodicFilter::New();
        filter->SetInput(src);
        filter->SetRotationAxis(Point(0, 0, 0), Vector3d(0, 0, 1));
        filter->SetIterationMode(mode);
        filter->SetNumberOfCopies(copies);
        filter->SetAngle(angle);
        filter->SetRequireFullPeriod(requireFull);
        r.ok = filter->Execute();
        r.effective = filter->GetEffectiveNumberOfCopies();
        r.coverage = filter->GetCoverageInfo();
        if (r.ok) r.out = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        return r;
    };

    // MAX 90° -> 4 份，整周闭合
    auto a = run(AngularPeriodicFilter::ITERATION_MODE_MAX, 0, 90.f, false);
    Expect(a.ok, "MAX 90 deg Execute returns true");
    if (a.ok) {
        Expect(a.effective == 4, "MAX 90 deg -> 4 periods");
        Expect(a.out->GetNumberOfPoints() == IGsize(12) * 4, "MAX 90 deg output has 4 copies");
        Expect(a.coverage.find("整周闭合") != std::string::npos, "MAX 90 deg coverage = full period");
        // 显式断言 ParaView 语义：90°/4 份 -> 0/90/180/270
        const double step90 = 90.0 * 3.141592653589793 / 180.0;
        for (int c = 0; c < 4; ++c) {
            CheckPointOnCopy(a.out->GetPoints(), 0, c, 12, Vec3(0, 0, 1), Vec3(0, 0, 0), step90);
        }
    }

    // MAX 100° -> floor(360/100)=3 份，缺口 60°
    auto b = run(AngularPeriodicFilter::ITERATION_MODE_MAX, 999, 100.f, false);
    Expect(b.ok, "MAX 100 deg Execute returns true");
    if (b.ok) {
        Expect(b.effective == 3, "MAX 100 deg -> 3 periods (floor)");
        Expect(b.out->GetNumberOfPoints() == IGsize(12) * 3, "MAX 100 deg output has 3 copies");
        Expect(b.coverage.find("缺口") != std::string::npos, "MAX 100 deg coverage = gap");
    }

    // DIRECT 3 x 100° -> 缺口
    auto c = run(AngularPeriodicFilter::ITERATION_MODE_DIRECT_NB, 3, 100.f, false);
    Expect(c.ok && c.effective == 3, "Direct 3x100 deg executed");
    Expect(c.coverage.find("缺口") != std::string::npos, "Direct 3x100 deg coverage = gap");

    // DIRECT 5 x 90° -> 重叠
    auto d = run(AngularPeriodicFilter::ITERATION_MODE_DIRECT_NB, 5, 90.f, false);
    Expect(d.ok && d.effective == 5, "Direct 5x90 deg executed");
    Expect(d.out->GetNumberOfPoints() == IGsize(12) * 5, "Direct 5x90 deg output has 5 copies");
    Expect(d.coverage.find("重叠") != std::string::npos, "Direct 5x90 deg coverage = overlap");

    // RequireFullPeriod: 3 x 100° -> 失败
    auto e = run(AngularPeriodicFilter::ITERATION_MODE_DIRECT_NB, 3, 100.f, true);
    Expect(!e.ok, "RequireFullPeriod rejects 3x100 deg");

    // RequireFullPeriod: 3 x 120° -> 成功
    auto f = run(AngularPeriodicFilter::ITERATION_MODE_DIRECT_NB, 3, 120.f, true);
    Expect(f.ok, "RequireFullPeriod accepts 3x120 deg");
    if (f.ok) {
        Expect(f.coverage.find("整周闭合") != std::string::npos, "3x120 deg coverage = full period");
    }
}

} // namespace

int main() {
    TestUnstructuredMixed();
    TestUnstructuredBigPoly();
    TestSurfaceMixed();
    TestVolumeMesh();
    TestStructured();
    TestPolyhedronCell();
    TestNonAlignedAxis();
    TestIterationModeAndCoverage();

    std::printf("total checks: %d, failures: %d\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
