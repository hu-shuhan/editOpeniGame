#include "iGameSubdivide.h"

#include "iGameAttributeSet.h"
#include "iGameCell.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"

#include <unordered_map>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

// ============================================================
// 点属性插值：输出点 = 旧点（取自身）+ 边中点（两端平均）
//   nOldPts            : 旧顶点数量（输出点 0..nOldPts-1 为旧点）
//   mids[t]            : 第 t 个中点对应的两个端点（当前轮点 id）
//                        该中点的输出点 id = nOldPts + t
// 旧点取自身、中点取两端平均。
// ============================================================
void InterpolatePointAttributes(AttributeSet* src, AttributeSet* dst,
                                int nOldPts,
                                const std::vector<std::pair<int, int>>& mids,
                                int nOutPts) {
    if (src == nullptr || dst == nullptr) return;
    auto pointAttrs = src->GetAllPointAttributes();
    if (pointAttrs == nullptr) return;

    for (int i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
        AttributeSet::Attribute& inAttr = pointAttrs->GetElement(i);
        if (inAttr.IsDeleted()) continue;

        ArrayObject::Pointer inData = inAttr.GetPointer();
        if (inData == nullptr) continue;

        const int dim = inData->GetDimension();
        DoubleArray::Pointer nd = DoubleArray::New();
        nd->SetName(inData->GetName());
        nd->SetDimension(dim);
        nd->Resize(nOutPts);

        std::vector<double> va(dim), vb(dim), vm(dim);

        // 旧顶点：原样取自身
        for (int j = 0; j < nOldPts; ++j) {
            inData->GetElement(j, va.data());
            nd->SetElement(j, va.data());
        }
        // 边中点：两端平均
        for (size_t t = 0; t < mids.size(); ++t) {
            int a = mids[t].first;
            int b = mids[t].second;
            inData->GetElement(a, va.data());
            inData->GetElement(b, vb.data());
            for (int d = 0; d < dim; ++d) vm[d] = (va[d] + vb[d]) * 0.5;
            nd->SetElement(nOldPts + static_cast<int>(t), vm.data());
        }

        dst->AddAttribute(inAttr.GetType(), IG_POINT, nd, inAttr.GetDataRange());
    }
}

// ============================================================
// 单元属性继承：一个面 1-to-4 后，4 个子三角形全部继承父面的
// CellData（非三角面先做三角扇拆分，所有子面同样继承原面）。
//   faceOfOutCell[k] : 第 k 个输出面来自哪个输入面
// 多代细分时每代都继承，CellData 不会丢失。
// ============================================================
void InheritCellAttributes(AttributeSet* src, AttributeSet* dst,
                           const std::vector<int>& faceOfOutCell,
                           int nOutCells) {
    if (src == nullptr || dst == nullptr) return;
    auto cellAttrs = src->GetAllCellAttributes();
    if (cellAttrs == nullptr) return;

    for (int i = 0; i < cellAttrs->GetNumberOfElements(); ++i) {
        AttributeSet::Attribute& inAttr = cellAttrs->GetElement(i);
        if (inAttr.IsDeleted()) continue;

        ArrayObject::Pointer inData = inAttr.GetPointer();
        if (inData == nullptr) continue;

        const int dim = inData->GetDimension();
        DoubleArray::Pointer nd = DoubleArray::New();
        nd->SetName(inData->GetName());
        nd->SetDimension(dim);
        nd->Resize(nOutCells);

        std::vector<double> vf(dim);
        for (int k = 0; k < nOutCells; ++k) {
            const int parent = faceOfOutCell[static_cast<size_t>(k)];
            inData->GetElement(parent, vf.data());
            nd->SetElement(k, vf.data());
        }

        dst->AddAttribute(inAttr.GetType(), IG_CELL, nd, inAttr.GetDataRange());
    }
}

// ============================================================
// 一轮线性 1-to-4 细分，返回新的 SurfaceMesh
// ============================================================
SurfaceMesh::Pointer SubdivideOnce(SurfaceMesh* mesh) {
    const int nFaces = mesh->GetNumberOfFaces();
    const int nOldPts = mesh->GetNumberOfPoints();

    // 输出点：先放全部旧顶点（id 0..nOldPts-1），位置原样保留
    Points::Pointer outPts = Points::New();
    outPts->Reserve(nOldPts);
    for (int i = 0; i < nOldPts; ++i)
        outPts->AddPoint(mesh->GetPoint(i));

    CellArray::Pointer outFaces = CellArray::New();

    // 边 -> 中点 id；中点端点记录（用于点属性插值）
    std::unordered_map<long long, int> edgeMid;
    std::vector<std::pair<int, int>> midEndpoints;
    // 每个输出面来自哪个输入面（用于 CellData 继承）
    std::vector<int> faceOfOutCell;

    auto edgeKey = [](int a, int b) -> long long {
        if (a > b) { int t = a; a = b; b = t; }
        constexpr long long BASE = 2100000000LL; // 大于任何点 id
        return static_cast<long long>(a) * BASE + b;
    };

    auto getMid = [&](int a, int b) -> int {
        long long k = edgeKey(a, b);
        auto it = edgeMid.find(k);
        if (it != edgeMid.end()) return it->second;   // 共享边复用同一中点

        const Point& pa = mesh->GetPoint(a);
        const Point& pb = mesh->GetPoint(b);
        Vector3d m((static_cast<double>(pa[0]) + pb[0]) * 0.5,
                   (static_cast<double>(pa[1]) + pb[1]) * 0.5,
                   (static_cast<double>(pa[2]) + pb[2]) * 0.5);

        const int id = nOldPts + static_cast<int>(midEndpoints.size());
        outPts->AddPoint(m);
        midEndpoints.push_back({a, b});
        edgeMid[k] = id;
        return id;
    };

    igIndex ptIds[IGAME_CELL_MAX_SIZE]{};

    // 记录输出面的同时登记其父面，供 CellData 继承
    auto addOutFace = [&](igIndex a, igIndex b, igIndex c, int parentFace) {
        outFaces->AddCellId3(a, b, c);
        faceOfOutCell.push_back(parentFace);
    };

    for (int f = 0; f < nFaces; ++f) {
        const int npts = mesh->GetFacePointIds(f, ptIds);
        if (npts < 3) continue;

        // 把（可能的）非三角面拆成三角形扇，再对每个三角形 1-to-4
        for (int s = 0; s < npts - 2; ++s) {
            const int A = static_cast<int>(ptIds[0]);
            const int B = static_cast<int>(ptIds[s + 1]);
            const int C = static_cast<int>(ptIds[s + 2]);

            // 必须按 VTK 的边处理顺序请求中点，输出点顺序才一致：
            //   edge0 = C-A, edge1 = A-B, edge2 = B-C
            // （getMid 内部会对端点排序，因此 getMid(C,A) 与 getMid(A,C)
            //   是同一条边、同一个中点，几何不变，只是首次创建的先后不同。）
            const int Mca = getMid(C, A);
            const int Mab = getMid(A, B);
            const int Mbc = getMid(B, C);
// 四个子三角形的顶点排列严格对齐 VTK GenerateSubdivisionCells：
//   edgePts[0]=Mca(C-A), edgePts[1]=Mab(A-B), edgePts[2]=Mbc(B-C)
            addOutFace(static_cast<igIndex>(A),
                       static_cast<igIndex>(Mab),
                       static_cast<igIndex>(Mca), f);   // 1: A, Mab, Mca
            addOutFace(static_cast<igIndex>(Mab),
                       static_cast<igIndex>(B),
                       static_cast<igIndex>(Mbc), f);   // 2: Mab, B, Mbc
            addOutFace(static_cast<igIndex>(Mbc),
                       static_cast<igIndex>(C),
                       static_cast<igIndex>(Mca), f);   // 3: Mbc, C, Mca
            addOutFace(static_cast<igIndex>(Mab),
                       static_cast<igIndex>(Mbc),
                       static_cast<igIndex>(Mca), f);   // 4: Mab, Mbc, Mca
        }


    }

    SurfaceMesh::Pointer out = SurfaceMesh::New();
    out->SetPoints(outPts);
    out->SetFaces(outFaces);

    InterpolatePointAttributes(mesh->GetAttributeSet(), out->GetAttributeSet(),
                               nOldPts, midEndpoints,
                               static_cast<int>(outPts->GetNumberOfPoints()));

    InheritCellAttributes(mesh->GetAttributeSet(), out->GetAttributeSet(),
                          faceOfOutCell,
                          static_cast<int>(outFaces->GetNumberOfCells()));

    out->GetAttributeSet()->Modified();
    out->Modified();
    return out;
}

} // anonymous namespace

SubdivideFilter::SubdivideFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void SubdivideFilter::SetNumberOfSubdivisions(int n) {
    if (n < 1) n = 1;
    if (n > 8) n = 8; // 防止三角形数量指数爆炸
    m_NumberOfSubdivisions = n;
}

int SubdivideFilter::GetNumberOfSubdivisions() const {
    return m_NumberOfSubdivisions;
}

bool SubdivideFilter::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) return false;

    auto mesh0 = DynamicCast<SurfaceMesh>(input);
    if (mesh0 == nullptr) return false;
    if (mesh0->GetNumberOfFaces() == 0 || mesh0->GetNumberOfPoints() == 0)
        return false;

    const int times = m_NumberOfSubdivisions;

    // 逐代细分：每轮输出作为下一轮输入
    DataObject::Pointer current = input;
    for (int i = 0; i < times; ++i) {
        auto inMesh = DynamicCast<SurfaceMesh>(current);
        if (inMesh == nullptr) return false;
        current = SubdivideOnce(inMesh);
    }

    auto finalMesh = DynamicCast<SurfaceMesh>(current);
    finalMesh->SetName(input->GetName() + "_subdivide");
    finalMesh->GetAttributeSet()->ForceReConvertToDrawableData();
    finalMesh->GetAttributeSet()->Modified();
    finalMesh->Modified();

    SetOutput(0, finalMesh);
    return true;
}

IGAME_NAMESPACE_END
