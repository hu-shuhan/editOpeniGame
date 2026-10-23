#include "iGameDeflectNormalsFilter.h"
#include <cmath>

IGAME_NAMESPACE_BEGIN

DeflectNormalsFilter::DeflectNormalsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
DeflectNormalsFilter::~DeflectNormalsFilter() = default;

// ---- 改进点7：输入校验 ----
bool DeflectNormalsFilter::ValidateInputs(std::string& why) const {
    // 检查向量场是否已指定
    if (_attrIdx < 0 && _attrName.empty()) {
        why = "请选择一个向量场属性";  // 请选择一个向量场属性
        return false;
    }
    // 检查自定义法向是否为零向量
    if (_useUserNormal) {
        float len = _userNormal[0] * _userNormal[0] + _userNormal[1] * _userNormal[1] + _userNormal[2] * _userNormal[2];
        if (len < 1e-12f) {
            why = "自定义法向不能为零向量";  // 自定义法向不能为零向量
            return false;
        }
    }
    // 检查偏转强度是否为非法数字
    if (std::isnan(_strength) || std::isinf(_strength)) {
        why = "偏转强度是非法数字";  // 偏转强度是非法数字
        return false;
    }
    return true;
}

// ---- 改进点3：查找已有点法向 ----
bool DeflectNormalsFilter::FindExistingPointNormals(AttributeSet* attrSet, int& outIdx, std::string& why) {
    auto N = attrSet->GetNumberOfAttributes();
    for (int i = 0; i < (int)N; ++i) {
        auto& ref = attrSet->GetAttribute(i);
        if (ref.isDeleted || !ref.pointer) continue;
        if (ref.attachmentType != IG_POINT) continue;
        if (ref.pointer->GetDimension() != 3) continue;
        std::string name = ref.pointer->GetName();
        // 仅精确匹配约定的法向名称，避免误用 NormalVelocity、DeflectedNormals 等向量。
        if (name == "Normals" || name == "PointNormals" || name == "normals") {
            outIdx = i;
            return true;
        }
    }
    outIdx = -1;
    return false;
}

// ---- 改进点4：创建独立输出 ----
// 务必要让输出与输入（及其可渲染表面）完全隔离：只生成独立的表面网格深拷贝。
// 这样偏转法向只会写到输出对象上，不会把属性/脏标记打进原始模型的可渲染表面，
// 从而避免原始模型在后续每一帧被反复触发外壳提取 (Extracted surface) 和重建三角化 (Get draw array) 导致卡顿。
DataObject::Pointer DeflectNormalsFilter::CreateIndependentOutput(DataObject::Pointer input) {
    if (!input) return nullptr;

    SurfaceMesh::Pointer src;
    switch (input->GetDataObjectType()) {
    case IG_SURFACE_MESH:
        src = DynamicCast<SurfaceMesh>(input);
        break;
    case IG_VOLUME_MESH: {
        auto vm = DynamicCast<VolumeMesh>(input);
        if (vm) src = DynamicCast<SurfaceMesh>(vm->GetRenderableObject());
        break;
    }
    case IG_STRUCTURED_MESH: {
        auto sm = DynamicCast<StructuredMesh>(input);
        if (sm) src = DynamicCast<SurfaceMesh>(sm->GetRenderableObject());
        break;
    }
    case IG_UNSTRUCTURED_MESH: {
        auto um = DynamicCast<UnstructuredMesh>(input);
        if (!um) break;
        auto s = um->TransferToSurfaceMesh();
        if (!s) s = um->ExtractSurfaceMesh();
        src = s;
        break;
    }
    default:
        return nullptr;
    }
    if (!src) return nullptr;

    // 独立深拷贝：Points / Faces / AttributeSet 均为新建对象，与输入无共享关系。
    // 注意必须用 SetAttributeSet 让属性集的 m_DataObject 指向输出网格：
    // 因为 AttributeSet::ForceReConvertToDrawableData() 内部直接解引用 m_DataObject，
    // 若为空会访问冲突（表现为写入 0xE1 之类的小地址而崩溃）。
    auto out = SurfaceMesh::New();

    auto pts = Points::New();
    if (src->GetPoints()) pts->DeepCopy(src->GetPoints());
    out->SetPoints(pts);

    auto faces = CellArray::New();
    if (src->GetFaces()) faces->DeepCopy(src->GetFaces());
    out->SetFaces(faces);

    if (src->GetAttributeSet()) {
        auto as = AttributeSet::New();
        as->DeepCopy(src->GetAttributeSet());
        out->SetAttributeSet(as);  // 内部会回写 as->m_DataObject = out
    }
    return out;
}

ArrayObject::Pointer DeflectNormalsFilter::AttributeCell2Point(
    CellArray::Pointer cells, ArrayObject::Pointer ori, size_t pointNum)
{
    const int dim = ori->GetDimension();
    auto newArr = FloatArray::New();
    newArr->SetName(ori->GetName());
    newArr->SetDimension(dim);
    if (pointNum == 0) return newArr;
    newArr->Reserve(pointNum);

    float s[16] = { 0 }, t[16] = { 0 };
    for (size_t i = 0; i < pointNum; ++i) newArr->AddElement(s);

    std::vector<int> adj(pointNum, 0);
    igIndex cell[IGAME_CELL_MAX_SIZE];
    const size_t numOri = ori->GetNumberOfElements();
    for (int i = 0; i < cells->GetNumberOfCells(); ++i) {
        // 防御：向量场元组数不足时提前停止，避免越界读
        if (static_cast<size_t>(i) >= numOri) break;
        const int size = cells->GetCellIds(i, cell);
        ori->GetElement(i, s);
        for (int j = 0; j < size; ++j) {
            if (static_cast<size_t>(cell[j]) >= pointNum) continue;
            adj[cell[j]]++;
            newArr->GetElement(cell[j], t);
            for (int d = 0; d < dim; ++d) t[d] += s[d];
            newArr->SetElement(cell[j], t);
        }
    }
    for (size_t i = 0; i < pointNum; ++i) {
        if (adj[i] > 0) {
            newArr->GetElement(i, t);
            for (int d = 0; d < dim; ++d) t[d] /= static_cast<float>(adj[i]);
            newArr->SetElement(i, t);
        }
    }
    return newArr;
}

bool DeflectNormalsFilter::ResolveVectorField(
    AttributeSet* attrSet,
    IGenum& outAttach, AttributeSet::Attribute*& outAttr,
    bool& outNeedCellToPoint, std::string& why)
{
    auto N = attrSet->GetNumberOfAttributes();
    auto MatchIndex = [&](IGenum attach, AttributeSet::Attribute*& a) -> bool {
        for (int i = 0; i < N; ++i) {
            auto& ref = attrSet->GetAttribute(i);
            if (ref.isDeleted || !ref.pointer) continue;
            if (ref.attachmentType != attach) continue;
            if (_attrIdx >= 0 && _attrIdx == i) { a = &ref; return true; }
        }
        return false;
    };
    auto MatchName = [&](IGenum attach, AttributeSet::Attribute*& a) -> bool {
        for (int i = 0; i < N; ++i) {
            auto& ref = attrSet->GetAttribute(i);
            if (ref.isDeleted || !ref.pointer) continue;
            if (ref.attachmentType != attach) continue;
            if (!_attrName.empty() && ref.pointer->GetName() == _attrName) { a = &ref; return true; }
        }
        return false;
    };
    auto CheckDim = [&](AttributeSet::Attribute* a, IGenum attach, bool& needC2P) -> bool {
        if (!a || !a->pointer) return false;
        if (a->pointer->GetDimension() != 3) {
            why = "向量场属性的组件数必须为 3";  // 向量场属性的组件数必须为 3
            return false;
        }
        // 改进点7：检查数组元组数
        if (a->pointer->GetNumberOfElements() == 0) {
            why = "向量场属性的元素数为零";  // 向量场属性的元素数为零
            return false;
        }
        outAttach = attach;
        outAttr   = a;
        outNeedCellToPoint = (attach == IG_CELL);
        return true;
    };
    auto Try = [&](IGenum attach) -> bool {
        AttributeSet::Attribute* a = nullptr;
        if (_attrIdx >= 0 ? MatchIndex(attach, a) : MatchName(attach, a)) {
            return CheckDim(a, attach, outNeedCellToPoint);
        }
        return false;
    };

    if (_vfAttach == IG_POINT) {
        if (Try(IG_POINT)) return true;
        why = "在点数据中未找到指定向量场";  // 在点数据中未找到指定向量场
        return false;
    }
    if (_vfAttach == IG_CELL) {
        if (Try(IG_CELL)) return true;
        why = "在单元数据中未找到指定向量场";  // 在单元数据中未找到指定向量场
        return false;
    }
    if (Try(IG_POINT)) return true;
    if (Try(IG_CELL))  return true;
    why = "未找到指定向量场";  // 未找到指定向量场
    return false;
}

bool DeflectNormalsFilter::Execute() {
    // 改进点7：输入校验
    std::string why;
    if (!ValidateInputs(why)) {
        _msg = why;
        return false;
    }

    auto input = GetInput(0);
    if (!input) { _msg = "输入为空"; return false; }  // 输入为空

    // ----- 改进点5：准备源表面网格（仅用于派生几何/拓扑，不做任何修改） -----
    // 注意：这里拿到的源表面可能是输入模型（或其可渲染表面）。绝不可把属性/脏标记写回它，
    // 否则原始模型会在每一帧被反复触发外壳提取/三角化重建导致卡顿。因此所有计算都在下方独立副本上进行。
    CellArray::Pointer surfaceFaces;

    {
        SurfaceMesh::Pointer src;
        switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            src = DynamicCast<SurfaceMesh>(input);
            _baseNormalSource = "曲面法向";  // 曲面法向
            break;
        case IG_VOLUME_MESH: {
            auto vm = DynamicCast<VolumeMesh>(input);
            if (vm) { src = DynamicCast<SurfaceMesh>(vm->GetRenderableObject()); _baseNormalSource = "边界面法向"; }
            break;
        }
        case IG_STRUCTURED_MESH: {
            auto sm = DynamicCast<StructuredMesh>(input);
            if (sm) { src = DynamicCast<SurfaceMesh>(sm->GetRenderableObject()); _baseNormalSource = "结构化边界面法向"; }
            break;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto um = DynamicCast<UnstructuredMesh>(input);
            if (!um) break;
            auto s = um->TransferToSurfaceMesh();
            if (!s) s = um->ExtractSurfaceMesh();
            src = s;
            _baseNormalSource = "提取面法向";  // 提取面法向
            break;
        }
        default:
            _msg = "不是表面网格";  // 不是表面网格
            return false;
        }
        if (!src) {
            _msg = "无法获取表面网格用于法向计算";  // 无法获取表面网格用于法向计算
            return false;
        }
        surfaceFaces = src->GetFaces();
    }

    // ----- 改进点4：创建独立输出（始终为与输入隔离的表面网格深拷贝） -----
    auto output = CreateIndependentOutput(input);
    if (!output) {
        _msg = "创建独立输出失败";  // 创建独立输出失败
        return false;
    }
    auto outMesh = DynamicCast<SurfaceMesh>(output);
    if (!outMesh) {
        _msg = "创建独立表面网格失败";  // 创建独立表面网格失败
        return false;
    }
    outMesh->BuildFaceLinks();

    const int numPts = static_cast<int>(outMesh->GetNumberOfPoints());
    if (numPts <= 0) { _msg = "网格没有点"; return false; }  // 网格没有点

    auto attrSet = outMesh->GetAttributeSet();
    if (!attrSet) { _msg = "网格没有属性集"; return false; }  // 网格没有属性集

    // ----- 改进点3：优先使用已有点法向 -----
    int existingNormalIdx = -1;
    bool hasExistingNormal = false;
    if (!_useUserNormal) {
        std::string normalWhy;
        hasExistingNormal = FindExistingPointNormals(attrSet, existingNormalIdx, normalWhy);
        if (hasExistingNormal) {
            _baseNormalSource = "已有点法向 (" + attrSet->GetAttribute(existingNormalIdx).pointer->GetName() + ")";  // 已有点法向 (name)
        }
    }

    // ----- 先建立基准法向 -----
    FloatArray::Pointer result = FloatArray::New();
    result->SetName("DeflectedNormals");
    result->SetDimension(3);
    result->Reserve(numPts);

    igIndex faceIds[256];
    for (int ptId = 0; ptId < numPts; ++ptId) {
        Vector3f base(0.f, 0.f, 0.f);
        if (_useUserNormal) {
            base = _userNormal;
        } else if (hasExistingNormal && existingNormalIdx >= 0) {
            // 改进点3：使用已有点法向
            auto existingArr = DynamicCast<ArrayObject>(attrSet->GetAttribute(existingNormalIdx).pointer);
            if (existingArr) {
                float buf[3] = {0, 0, 0};
                existingArr->GetElement(ptId, buf);
                base = Vector3f(buf[0], buf[1], buf[2]);
            }
        } else {
            const int n = outMesh->GetPointToNeighborFaces(ptId, faceIds);
            for (int j = 0; j < n; ++j) {
                auto f = outMesh->GetFace(faceIds[j]);
                if (f) base += f->GetNormal();
            }
        }
        // 防御：基准法向为零向量（退化面或法向抵消）时回退到 +Z，避免归一化产生 NaN
        float baseLenSq = base[0] * base[0] + base[1] * base[1] + base[2] * base[2];
        if (baseLenSq < 1e-12f) {
            base = Vector3f(0.f, 0.f, 1.f);
        } else {
            base.normalize();
        }
        const float out[3] = { base[0], base[1], base[2] };
        result->AddElement(out);
    }

    // ----- 解析向量场（改进点6） -----
    if (!attrSet) { _msg = "网格没有属性集"; return false; }

    IGenum vfAttach{ IG_NONE };
    AttributeSet::Attribute* attr = nullptr;
    bool needCellToPoint = false;
    if (!ResolveVectorField(attrSet, vfAttach, attr, needCellToPoint, why)) {
        _msg = why.empty() ? std::string("请选择有效的向量场") : why;  // 请选择有效的向量场
        return false;
    }
    if (!attr || !attr->pointer) { _msg = "向量场属性为空"; return false; }  // 向量场属性为空

    ArrayObject::Pointer vecField = attr->pointer;
    if (needCellToPoint) {
        // 数量校验：单元向量场的元组数必须不少于单元数，否则转换会越界读
        if (!surfaceFaces) { _msg = "单元向量场缺少面数据，无法转换到点"; return false; }  // 单元向量场缺少面数据，无法转换到点
        if (vecField->GetNumberOfElements() < static_cast<size_t>(surfaceFaces->GetNumberOfCells())) {
            _msg = "向量场属性的元组数与单元数不匹配";  // 向量场属性的元组数与单元数不匹配
            return false;
        }
        vecField = AttributeCell2Point(surfaceFaces, vecField, numPts);
    } else {
        // 数量校验：点向量场的元组数必须不少于顶点数，否则后续读取会越界
        if (vecField->GetNumberOfElements() < static_cast<size_t>(numPts)) {
            _msg = "向量场属性的元组数与顶点数不匹配";  // 向量场属性的元组数与顶点数不匹配
            return false;
        }
    }
    if (!vecField) { _msg = "向量场解析后为空"; return false; }  // 向量场解析后为空

    // ----- 偏转公式：改进点8：当 base + strength*V 为零时保留基准法向 -----
    float buf[3] = { 0, 0, 0 };
    for (int ptId = 0; ptId < numPts; ++ptId) {
        result->GetElement(ptId, buf);
        Vector3f base(buf[0], buf[1], buf[2]);

        vecField->GetElement(ptId, buf);
        Vector3f V(buf[0], buf[1], buf[2]);

        Vector3f ND = base + V * _strength;

        // 改进点8：如果结果为零向量，保留基准法向，避免归一化产生 NaN
        float lenSq = ND[0]*ND[0] + ND[1]*ND[1] + ND[2]*ND[2];
        if (lenSq < 1e-12f) {
            // 保留基准法向
            ND = base;
        } else {
            ND.normalize();
        }

        const float out[3] = { ND[0], ND[1], ND[2] };
        result->SetElement(ptId, out);
    }

    // 改进点2：将输出设置为渲染器实际使用的点着色法向量
    // 添加为 IG_POINT 的 Vector 属性
    attrSet->AddVector(IG_POINT, result);
    attrSet->ForceReConvertToDrawableData();

    // 改进点4：SetOutput 为独立输出，不修改输入
    SetOutput(outMesh);
    UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END
