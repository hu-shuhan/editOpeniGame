#include "iGameVolumeResampleFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellType.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameThreadPool.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

// ---------------------------------------------------------------------------
// 自包含的 3x3 线性代数（避免依赖向量库实现细节）
// ---------------------------------------------------------------------------

double Det3(const double a[9]) {
    return a[0] * (a[4] * a[8] - a[5] * a[7]) -
           a[1] * (a[3] * a[8] - a[5] * a[6]) +
           a[2] * (a[3] * a[7] - a[4] * a[6]);
}

// 解 A * x = b（A 行主序 3x3）。失败返回 false。
bool Solve3x3(const double a[9], const double b[3], double x[3]) {
    double det = Det3(a);
    if (std::abs(det) < 1e-14) { return false; }

    double a0[9] = {a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]};
    double a1[9] = {a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]};
    double a2[9] = {a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]};

    a0[0] = b[0]; a0[3] = b[1]; a0[6] = b[2];  // 替换第 0 列
    a1[1] = b[0]; a1[4] = b[1]; a1[7] = b[2];  // 替换第 1 列
    a2[2] = b[0]; a2[5] = b[1]; a2[8] = b[2];  // 替换第 2 列

    x[0] = Det3(a0) / det;
    x[1] = Det3(a1) / det;
    x[2] = Det3(a2) / det;
    return true;
}

// ---------------------------------------------------------------------------
// 插值与包含判定
// ---------------------------------------------------------------------------

// 四面体重心坐标（p[4] 为 4 个顶点）。q 在内部时返回 true，w 为权重（和为 1）。
bool TetraBarycentric(const double p[4][3], const double q[3], double w[4]) {
    double a0[3] = {p[1][0] - p[0][0], p[1][1] - p[0][1], p[1][2] - p[0][2]};
    double a1[3] = {p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2]};
    double a2[3] = {p[3][0] - p[0][0], p[3][1] - p[0][1], p[3][2] - p[0][2]};
    double d[3] = {q[0] - p[0][0], q[1] - p[0][1], q[2] - p[0][2]};

    double m[9] = {a0[0], a1[0], a2[0],
                   a0[1], a1[1], a2[1],
                   a0[2], a1[2], a2[2]};
    double x[3] = {0.0, 0.0, 0.0};
    if (!Solve3x3(m, d, x)) { return false; }

    double w0 = 1.0 - x[0] - x[1] - x[2];
    const double tol = 1e-9;
    if (w0 < -tol || x[0] < -tol || x[1] < -tol || x[2] < -tol) { return false; }

    w[0] = w0;
    w[1] = x[0];
    w[2] = x[1];
    w[3] = x[2];
    return true;
}

// VTK 八节点六面体参数坐标（-1..1）。
const double kHexR[8] = {-1, 1, 1, -1, -1, 1, 1, -1};
const double kHexS[8] = {-1, -1, 1, 1, -1, -1, 1, 1};
const double kHexT[8] = {-1, -1, -1, -1, 1, 1, 1, 1};

// 六面体三线性（Newton 反解参数坐标）。q 在内部时返回 true，w 为形函数值。
bool HexaTrilinear(const double p[8][3], const double q[3], double w[8]) {
    double r = 0.0, s = 0.0, t = 0.0;
    for (int iter = 0; iter < 20; ++iter) {
        double f[3] = {-q[0], -q[1], -q[2]};
        double jac[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

        for (int i = 0; i < 8; ++i) {
            const double pr = 1.0 + r * kHexR[i];
            const double ps = 1.0 + s * kHexS[i];
            const double pt = 1.0 + t * kHexT[i];
            const double n = 0.125 * pr * ps * pt;
            const double dndr = 0.125 * kHexR[i] * ps * pt;
            const double dnds = 0.125 * pr * kHexS[i] * pt;
            const double dndt = 0.125 * pr * ps * kHexT[i];

            for (int c = 0; c < 3; ++c) {
                f[c] += n * p[i][c];
                jac[c * 3 + 0] += dndr * p[i][c];
                jac[c * 3 + 1] += dnds * p[i][c];
                jac[c * 3 + 2] += dndt * p[i][c];
            }
        }

        double delta[3] = {0.0, 0.0, 0.0};
        if (!Solve3x3(jac, f, delta)) { return false; }

        r += delta[0];
        s += delta[1];
        t += delta[2];

        if (std::abs(delta[0]) + std::abs(delta[1]) + std::abs(delta[2]) < 1e-12) {
            break;
        }
    }

    const double tol = 1e-6;
    if (r < -1.0 - tol || r > 1.0 + tol ||
        s < -1.0 - tol || s > 1.0 + tol ||
        t < -1.0 - tol || t > 1.0 + tol) {
        return false;
    }

    for (int i = 0; i < 8; ++i) {
        w[i] = 0.125 * (1.0 + r * kHexR[i]) *
               (1.0 + s * kHexS[i]) * (1.0 + t * kHexT[i]);
    }
    return true;
}

// 取某点/单元的标量：dim<=1 取分量 0，否则取模长（与 GPU 后端口径一致）。
double PointScalar(ArrayObject* arr, int dim, igIndex pointId) {
    if (dim <= 1) { return arr->GetValue(pointId); }
    return arr->GetElementValue(pointId, -1);
}

double CellScalar(ArrayObject* arr, int dim, igIndex cellId) {
    if (dim <= 1) { return arr->GetValue(cellId); }
    return arr->GetElementValue(cellId, -1);
}

// 选择标量场（优先点名匹配 -> 激活属性 -> 点标量/向量 -> 单元标量/向量）。
bool FindScalarArray(DataObject* obj, const std::string& fieldName,
                     ArrayObject::Pointer& arr, bool& isCellData, int& dim) {
    AttributeSet* attrs = obj->GetAttributeSet();
    if (!attrs) { return false; }

    auto pick = [&](AttributeSet::Attribute& a) -> bool {
        if (a.IsNone() || !a.pointer) { return false; }
        if (a.type != IG_SCALAR && a.type != IG_VECTOR) { return false; }
        arr = a.pointer;
        isCellData = (a.attachmentType == IG_CELL);
        dim = a.pointer->GetDimension();
        return true;
    };

    if (!fieldName.empty()) {
        auto& named = attrs->GetAttribute(fieldName);
        if (!named.IsNone() && pick(named)) { return true; }
    }

    const int nAttr = static_cast<int>(attrs->GetNumberOfAttributes());
    const int ai = obj->GetAttributeIndex();
    if (ai >= 0 && ai < nAttr) {
        auto& a = attrs->GetAttribute(ai);
        if (pick(a)) { return true; }
    }

    for (int i = 0; i < nAttr; ++i) {
        auto& a = attrs->GetAttribute(i);
        if (a.attachmentType == IG_POINT && pick(a)) { return true; }
    }
    for (int i = 0; i < nAttr; ++i) {
        auto& a = attrs->GetAttribute(i);
        if (a.attachmentType == IG_CELL && pick(a)) { return true; }
    }
    return false;
}

// 递归收集叶子网格（含点集、无子对象）。
void CollectMeshes(DataObject::Pointer obj, std::vector<DataObject::Pointer>& out) {
    if (!obj) { return; }
    if (obj->HasSubDataObject()) {
        for (auto it = obj->SubDataObjectIteratorBegin();
             it != obj->SubDataObjectIteratorEnd(); ++it) {
            CollectMeshes(it->second, out);
        }
        return;
    }
    if (obj->GetPoints() == nullptr) { return; }
    out.push_back(obj);
}

} // namespace

// ---------------------------------------------------------------------------
// 采样器的内部结构（仅 Execute 内使用，避免污染头文件）
// ---------------------------------------------------------------------------

namespace {

struct StructPiece {
    StructuredMesh::Pointer mesh;
    ArrayObject::Pointer scalar;
    bool isCellData{false};
    int dim{1};
    igIndex dims[3]{0, 0, 0};
    double mn[3]{0.0, 0.0, 0.0};
    double spacing[3]{1.0, 1.0, 1.0};

    bool Valid() const {
        return dims[0] >= 2 && dims[1] >= 2 && dims[2] >= 2 &&
               spacing[0] > 0.0 && spacing[1] > 0.0 && spacing[2] > 0.0;
    }

    bool Sample(const double q[3], double& out) const {
        const double u = (q[0] - mn[0]) / spacing[0];
        const double v = (q[1] - mn[1]) / spacing[1];
        const double w = (q[2] - mn[2]) / spacing[2];
        const double tol = 1e-4;
        const int ni = static_cast<int>(dims[0]);
        const int nj = static_cast<int>(dims[1]);
        const int nk = static_cast<int>(dims[2]);

        if (u < -tol || v < -tol || w < -tol) { return false; }
        if (u > (ni - 1) + tol || v > (nj - 1) + tol || w > (nk - 1) + tol) {
            return false;
        }

        int i0 = std::max(0, std::min(ni - 2, static_cast<int>(std::floor(u))));
        int j0 = std::max(0, std::min(nj - 2, static_cast<int>(std::floor(v))));
        int k0 = std::max(0, std::min(nk - 2, static_cast<int>(std::floor(w))));
        int i1 = std::min(ni - 1, i0 + 1);
        int j1 = std::min(nj - 1, j0 + 1);
        int k1 = std::min(nk - 1, k0 + 1);
        const double fu = u - i0;
        const double fv = v - j0;
        const double fw = w - k0;

        auto nodeIdx = [&](int i, int j, int k) -> igIndex {
            return static_cast<igIndex>(i) +
                   static_cast<igIndex>(j) * ni +
                   static_cast<igIndex>(k) * ni * nj;
        };

        if (isCellData) {
            const int ci = std::max(0, std::min(ni - 2, i0));
            const int cj = std::max(0, std::min(nj - 2, j0));
            const int ck = std::max(0, std::min(nk - 2, k0));
            const igIndex cellId = static_cast<igIndex>(ci) +
                                    static_cast<igIndex>(cj) * (ni - 1) +
                                    static_cast<igIndex>(ck) * (ni - 1) * (nj - 1);
            out = CellScalar(scalar.get(), dim, cellId);
            return true;
        }

        const double v000 = PointScalar(scalar.get(), dim, nodeIdx(i0, j0, k0));
        const double v100 = PointScalar(scalar.get(), dim, nodeIdx(i1, j0, k0));
        const double v010 = PointScalar(scalar.get(), dim, nodeIdx(i0, j1, k0));
        const double v110 = PointScalar(scalar.get(), dim, nodeIdx(i1, j1, k0));
        const double v001 = PointScalar(scalar.get(), dim, nodeIdx(i0, j0, k1));
        const double v101 = PointScalar(scalar.get(), dim, nodeIdx(i1, j0, k1));
        const double v011 = PointScalar(scalar.get(), dim, nodeIdx(i0, j1, k1));
        const double v111 = PointScalar(scalar.get(), dim, nodeIdx(i1, j1, k1));

        out = v000 * (1.0 - fu) * (1.0 - fv) * (1.0 - fw) +
              v100 * fu * (1.0 - fv) * (1.0 - fw) +
              v010 * (1.0 - fu) * fv * (1.0 - fw) +
              v110 * fu * fv * (1.0 - fw) +
              v001 * (1.0 - fu) * (1.0 - fv) * fw +
              v101 * fu * (1.0 - fv) * fw +
              v011 * (1.0 - fu) * fv * fw +
              v111 * fu * fv * fw;
        return true;
    }
};

struct CellRec {
    int pieceIndex{-1};
    IGenum type{IG_EMPTY_CELL};
    int npts{0};
    bool isCellData{false};
    int dim{1};
    igIndex ptIds[8]{0, 0, 0, 0, 0, 0, 0, 0};
    double mn[3]{0.0, 0.0, 0.0};
    double mx[3]{0.0, 0.0, 0.0};
    double nodeScalar[8]{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double cellScalar{0.0};

    bool BBoxContains(const double q[3]) const {
        return q[0] >= mn[0] && q[0] <= mx[0] &&
               q[1] >= mn[1] && q[1] <= mx[1] &&
               q[2] >= mn[2] && q[2] <= mx[2];
    }
};

// 对单个非结构化单元采样；命中返回 true 并写 out。
bool SampleCell(const CellRec& c, const std::vector<PointSet*>& pointSets,
                const double q[3], double& out) {
    const PointSet* ps = pointSets[c.pieceIndex];
    double p[8][3];

    if (c.type == IG_TETRA && c.npts == 4) {
        for (int k = 0; k < 4; ++k) {
            const Point& pt = ps->GetPoint(c.ptIds[k]);
            p[k][0] = pt[0]; p[k][1] = pt[1]; p[k][2] = pt[2];
        }
        double w[4] = {0.0, 0.0, 0.0, 0.0};
        if (!TetraBarycentric(p, q, w)) { return false; }
        if (c.isCellData) {
            out = c.cellScalar;
        } else {
            out = w[0] * c.nodeScalar[0] + w[1] * c.nodeScalar[1] +
                  w[2] * c.nodeScalar[2] + w[3] * c.nodeScalar[3];
        }
        return true;
    }

    if (c.type == IG_HEXAHEDRON && c.npts == 8) {
        for (int k = 0; k < 8; ++k) {
            const Point& pt = ps->GetPoint(c.ptIds[k]);
            p[k][0] = pt[0]; p[k][1] = pt[1]; p[k][2] = pt[2];
        }
        double w[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        if (!HexaTrilinear(p, q, w)) { return false; }
        if (c.isCellData) {
            out = c.cellScalar;
        } else {
            out = 0.0;
            for (int k = 0; k < 8; ++k) { out += w[k] * c.nodeScalar[k]; }
        }
        return true;
    }

    // 三棱柱 / 金字塔：最近节点退化（v1 简化；见头文件说明）。
    double best = 0.0;
    double bestDist = std::numeric_limits<double>::max();
    for (int k = 0; k < c.npts; ++k) {
        const Point& pt = ps->GetPoint(c.ptIds[k]);
        const double dx = q[0] - pt[0];
        const double dy = q[1] - pt[1];
        const double dz = q[2] - pt[2];
        const double d = dx * dx + dy * dy + dz * dz;
        if (d < bestDist) {
            bestDist = d;
            best = c.isCellData ? c.cellScalar : c.nodeScalar[k];
        }
    }
    out = best;
    return true;
}

} // namespace

bool iGameVolumeResampleFilter::Execute() {
    m_OutputMesh = nullptr;
    m_ValidMask = nullptr;

    DataObject::Pointer root = GetInput(0);
    if (!root) { return false; }

    std::vector<DataObject::Pointer> meshes;
    CollectMeshes(root, meshes);
    if (meshes.empty()) { return false; }

    // 1) 收集结构化 piece 与非结构化单元，并求全局包围盒。
    std::vector<StructPiece> structs;
    std::vector<PointSet*> pointSets;
    std::vector<CellRec> cells;
    BoundingBox bounds;
    bounds.reset();
    bool anyScalar = false;

    for (auto& mesh : meshes) {
        bounds.add(mesh->GetBoundingBox());

        ArrayObject::Pointer scalar;
        bool isCellData = false;
        int dim = 1;
        if (!FindScalarArray(mesh.get(), m_FieldName, scalar, isCellData, dim)) {
            continue;
        }
        anyScalar = true;

        if (auto sm = DynamicCast<StructuredMesh>(mesh)) {
            StructPiece sp;
            sp.mesh = sm;
            sp.scalar = scalar;
            sp.isCellData = isCellData;
            sp.dim = dim;
            igIndex* d = sm->GetDimensionSize();
            sp.dims[0] = d[0];
            sp.dims[1] = d[1];
            sp.dims[2] = d[2];
            const BoundingBox& b = sm->GetBoundingBox();
            sp.mn[0] = b.min[0]; sp.mn[1] = b.min[1]; sp.mn[2] = b.min[2];
            for (int a = 0; a < 3; ++a) {
                const double denom = static_cast<double>(sp.dims[a] - 1);
                sp.spacing[a] = denom > 0.0 ? (b.max[a] - b.min[a]) / denom : 0.0;
            }
            if (sp.Valid()) { structs.push_back(sp); }
            continue;
        }

        // 非结构化：VolumeMesh / UnstructuredMesh。
        PointSet* ps = DynamicCast<PointSet>(mesh);
        if (!ps) { continue; }
        const int pieceIndex = static_cast<int>(pointSets.size());
        pointSets.push_back(ps);

        auto addCell = [&](int cellId, IGenum type, int npts, const igIndex* ids) {
            if (npts < 1 || npts > 8) { return; }  // 只支持 <= 8 节点的常见单元
            CellRec c;
            c.pieceIndex = pieceIndex;
            c.type = type;
            c.npts = npts;
            c.isCellData = isCellData;
            c.dim = dim;
            c.mn[0] = c.mn[1] = c.mn[2] = std::numeric_limits<double>::max();
            c.mx[0] = c.mx[1] = c.mx[2] = -std::numeric_limits<double>::max();
            for (int k = 0; k < npts; ++k) {
                c.ptIds[k] = ids[k];
                const Point& pt = ps->GetPoint(ids[k]);
                c.mn[0] = std::min(c.mn[0], static_cast<double>(pt[0]));
                c.mn[1] = std::min(c.mn[1], static_cast<double>(pt[1]));
                c.mn[2] = std::min(c.mn[2], static_cast<double>(pt[2]));
                c.mx[0] = std::max(c.mx[0], static_cast<double>(pt[0]));
                c.mx[1] = std::max(c.mx[1], static_cast<double>(pt[1]));
                c.mx[2] = std::max(c.mx[2], static_cast<double>(pt[2]));
                c.nodeScalar[k] = isCellData ? 0.0 : PointScalar(scalar.get(), dim, ids[k]);
            }
            c.cellScalar = isCellData ? CellScalar(scalar.get(), dim, cellId) : 0.0;
            cells.push_back(c);
        };

        if (auto vm = DynamicCast<VolumeMesh>(mesh)) {
            const IGsize nVol = vm->GetNumberOfVolumes();
            igIndex ids[256]{};
            for (IGsize v = 0; v < nVol; ++v) {
                const IGuint npts = vm->GetVolumes()->GetCellSize(v);
                if (npts > 8) { continue; }  // 只支持 <= 8 节点的常见单元
                const IGenum type = VolumeMesh::GetVolumeTypeWithPointNum(static_cast<int>(npts));
                if (type != IG_TETRA && type != IG_HEXAHEDRON &&
                    type != IG_PYRAMID && type != IG_PRISM) {
                    continue;
                }
                vm->GetVolumePointIds(v, ids);
                addCell(static_cast<int>(v), type, static_cast<int>(npts), ids);
            }
        } else if (auto um = DynamicCast<UnstructuredMesh>(mesh)) {
            const IGsize nCell = um->GetNumberOfCells();
            igIndex ids[256]{};
            for (IGsize c = 0; c < nCell; ++c) {
                const IGenum type = um->GetCellType(c);
                if (type != IG_TETRA && type != IG_HEXAHEDRON &&
                    type != IG_PYRAMID && type != IG_PRISM) {
                    continue;
                }
                const int npts = um->GetCellPointIds(c, ids);
                addCell(static_cast<int>(c), type, npts, ids);
            }
        }
    }

    if (!anyScalar) { return false; }
    if (structs.empty() && cells.empty()) { return false; }

    // 2) 目标网格：原点/步长。
    const int ni = m_Dims[0];
    const int nj = m_Dims[1];
    const int nk = m_Dims[2];
    const double origin[3] = {bounds.min[0], bounds.min[1], bounds.min[2]};
    double spacing[3] = {1.0, 1.0, 1.0};
    for (int a = 0; a < 3; ++a) {
        const double size = bounds.max[a] - bounds.min[a];
        spacing[a] = (size > 0.0) ? size / static_cast<double>(m_Dims[a] - 1) : 1.0;
    }

    const IGsize total = static_cast<IGsize>(ni) * nj * nk;
    if (total > static_cast<IGsize>(std::numeric_limits<int>::max())) { return false; }

    // 3) 输出 StructuredMesh。
    igIndex s[3] = {ni, nj, nk};
    m_OutputMesh = StructuredMesh::New();
    m_OutputMesh->SetDimensionSize(s);
    igIndex e[6] = {0, ni - 1, 0, nj - 1, 0, nk - 1};
    m_OutputMesh->SetExtent(e);

    Points::Pointer pts = Points::New();
    pts->SetNumberOfPoints(total);
    m_OutputMesh->SetPoints(pts);

    FloatArray::Pointer scalarArr = FloatArray::New();
    scalarArr->SetDimension(1);
    scalarArr->Resize(total);
    scalarArr->SetName(m_FieldName.empty() ? std::string("scalar") : m_FieldName);
    m_OutputMesh->GetAttributeSet()->AddScalar(IG_POINT, scalarArr);

    m_ValidMask = UnsignedCharArray::New();
    m_ValidMask->SetDimension(1);
    m_ValidMask->Resize(total);

    // 4) 并行逐体素采样（读共享只读结构，写各自不重叠的输出区间）。
    float* ptBuf = pts->RawPointer();
    float* scalarBuf = scalarArr->RawPointer();
    unsigned char* maskBuf = m_ValidMask->RawPointer();

    ThreadPool::parallelFor(0, static_cast<int>(total), [&](int start, int end) {
        for (int p = start; p < end; ++p) {
            const int i = p % ni;
            const int j = (p / ni) % nj;
            const int k = p / (ni * nj);

            const double q[3] = {origin[0] + i * spacing[0],
                                 origin[1] + j * spacing[1],
                                 origin[2] + k * spacing[2]};

            double val = 0.0;
            unsigned char valid = 0;

            for (const auto& sp : structs) {
                double v = 0.0;
                if (sp.Sample(q, v)) {
                    val = v;
                    valid = 1;
                    break;
                }
            }

            if (!valid) {
                for (const auto& c : cells) {
                    if (!c.BBoxContains(q)) { continue; }
                    double v = 0.0;
                    if (SampleCell(c, pointSets, q, v)) {
                        val = v;
                        valid = 1;
                        break;
                    }
                }
            }

            ptBuf[p * 3 + 0] = static_cast<float>(q[0]);
            ptBuf[p * 3 + 1] = static_cast<float>(q[1]);
            ptBuf[p * 3 + 2] = static_cast<float>(q[2]);
            scalarBuf[p] = static_cast<float>(val);
            maskBuf[p] = valid;
        }
    });

    m_OutputMesh->GenStructuredCellConnectivities();
    SetOutput(0, m_OutputMesh);
    return true;
}

IGAME_NAMESPACE_END
