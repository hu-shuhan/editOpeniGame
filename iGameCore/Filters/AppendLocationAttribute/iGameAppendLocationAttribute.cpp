#include "iGameAppendLocationAttribute.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"

#include <algorithm>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {
constexpr const char* kLocationAttributeName = "LocationAttribute";
constexpr const char* kCellCenterAttributeName = "CellCenter";

/** 按源数组类型创建同类型数组（保留数据类型：float/double/int/uchar ...） */
ArrayObject::Pointer NewArrayLike(ArrayObject* src) {
    if (src == nullptr) { return nullptr; }
    switch (src->GetArrayType()) {
        case IG_FloatArray: return FloatArray::New();
        case IG_DoubleArray: return DoubleArray::New();
        case IG_IntArray: return IntArray::New();
        case IG_UnsignedIntArray: return UnsignedIntArray::New();
        case IG_CharArray: return CharArray::New();
        case IG_UnsignedCharArray: return UnsignedCharArray::New();
        case IG_ShortArray: return ShortArray::New();
        case IG_UnsignedShortArray: return UnsignedShortArray::New();
        case IG_LongLongArray: return LongLongArray::New();
        case IG_UnsignedLongLongArray: return UnsignedLongLongArray::New();
        default: return nullptr;
    }
}

/**
 * 深拷贝一份属性集。
 * 注意：AttributeSet::DeepCopy 内部只处理 FloatArray / DoubleArray，
 * 会丢掉整型 / 字符型数组（例如 validpointmask），因此这里自己按类型拷贝。
 */
AttributeSet::Pointer CopyAttributeSet(AttributeSet* src) {
    auto dst = AttributeSet::New();
    if (src == nullptr) { return dst; }

    auto all = src->GetAllAttributes();
    if (all == nullptr) { return dst; }

    std::vector<double> values;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }

        auto arr = NewArrayLike(attr.pointer);
        if (arr == nullptr) { continue; }

        const int dimension = attr.pointer->GetDimension();
        arr->SetName(attr.pointer->GetName());
        arr->SetDimension(dimension > 0 ? dimension : 1);

        const IGsize elementNum = attr.pointer->GetNumberOfElements();
        arr->Resize(elementNum);
        values.assign(static_cast<size_t>(dimension > 0 ? dimension : 1), 0.0);
        for (IGsize j = 0; j < elementNum; ++j) {
            attr.pointer->GetElement(j, values.data());
            arr->SetElement(j, values.data());
        }

        dst->AddAttribute(attr.type, attr.attachmentType, arr, attr.GetDataRange());
    }
    return dst;
}

Points::Pointer CopyPoints(Points* src) {
    auto dst = Points::New();
    if (src != nullptr) {
        Points::Pointer shared(src); // 引用计数临时 +1，拷贝完自动 -1
        dst->DeepCopy(shared);
    }
    return dst;
}

CellArray::Pointer CopyCellArray(CellArray* src) {
    auto dst = CellArray::New();
    if (src != nullptr) {
        CellArray::Pointer shared(src);
        dst->DeepCopy(shared);
    }
    return dst;
}

/** 拷贝 SurfaceMesh 层级的几何与属性（点 / 面 / 边 / 属性集 / 显示样式） */
void CopySurfaceMeshState(SurfaceMesh::Pointer dst, const SurfaceMesh::Pointer& src) {
    if (dst == nullptr || src == nullptr) { return; }
    dst->SetPoints(CopyPoints(src->GetPoints()));
    dst->SetFaces(CopyCellArray(src->GetFaces()));
    if (src->GetEdges() != nullptr) { dst->SetEdges(CopyCellArray(src->GetEdges())); }
    dst->SetAttributeSet(CopyAttributeSet(src->GetAttributeSet()));
    dst->SetViewStyle(src->GetViewStyle());
}
} // namespace

/* ------------------------------------------------------------------ */
/* 执行：生成「输入网格的副本 + 坐标属性」的新 DataObject              */
/* ------------------------------------------------------------------ */
bool AppendLocationAttribute::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "未选择输入数据";
        return false;
    }

    DataObject::Pointer output = nullptr;

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto src = DynamicCast<SurfaceMesh>(input);
            if (src == nullptr) {
                m_Message = "输入不是 SurfaceMesh";
                return false;
            }
            auto dst = SurfaceMesh::New();
            CopySurfaceMeshState(dst, src);
            output = dst;
        } break;

        case IG_VOLUME_MESH: {
            auto src = DynamicCast<VolumeMesh>(input);
            if (src == nullptr) {
                m_Message = "输入不是 VolumeMesh";
                return false;
            }
            auto dst = VolumeMesh::New();
            CopySurfaceMeshState(dst, src);
            dst->SetVolumes(CopyCellArray(src->GetVolumes()));
            output = dst;
        } break;

        case IG_STRUCTURED_MESH: {
            auto src = DynamicCast<StructuredMesh>(input);
            if (src == nullptr) {
                m_Message = "输入不是 StructuredMesh";
                return false;
            }
            StructuredMesh::Pointer dst = StructuredMesh::New();
            CopySurfaceMeshState(dst, src);
            dst->SetVolumes(CopyCellArray(src->GetVolumes()));
            dst->SetDimensionSize(src->GetDimensionSize());
            dst->SetExtent(src->GetExtent());
            output = dst;
        } break;

        case IG_UNSTRUCTURED_MESH: {
            auto src = DynamicCast<UnstructuredMesh>(input);
            if (src == nullptr) {
                m_Message = "输入不是 UnstructuredMesh";
                return false;
            }
            // 注意：UnstructuredMesh 派生自 PointSet（不是 SurfaceMesh），单独拷贝
            auto dst = UnstructuredMesh::New();
            dst->SetPoints(CopyPoints(src->GetPoints()));
            auto types = UnsignedIntArray::New();
            if (src->GetCellTypes() != nullptr) {
                UnsignedIntArray::Pointer shared(src->GetCellTypes());
                types->DeepCopy(shared);
            }
            dst->SetCells(CopyCellArray(src->GetCells()), types);
            dst->SetAttributeSet(CopyAttributeSet(src->GetAttributeSet()));
            dst->SetViewStyle(src->GetViewStyle());
            output = dst;
        } break;

        default:
            m_Message = "不支持的输入类型（仅支持 Surface / Volume / Structured / Unstructured Mesh）";
            return false;
    }

    if (output == nullptr) {
        m_Message = "创建输出网格失败";
        return false;
    }

    output->SetName(MakeOutputName(input->GetName()));

    // 把点坐标作为属性附加到「输出网格」上（输入对象保持不变）
    if (!AppendLocationToOutput(output)) { return false; }
    if (!AppendCellCenterToOutput(output)) { return false; }

    SetOutput(output);
    m_Message = "OK";
    return true;
}

std::string AppendLocationAttribute::MakeOutputName(const std::string& inputName) {
    if (inputName.empty()) { return std::string(kLocationAttributeName); }
    return inputName + "AddLocation";
}

/* ------------------------------------------------------------------ */
/* 附加坐标属性                                                        */
/* ------------------------------------------------------------------ */
bool AppendLocationAttribute::AppendLocationToOutput(DataObject::Pointer mesh) {
    if (mesh == nullptr) {
        m_Message = "输出网格为空";
        return false;
    }

    auto points = mesh->GetPoints();
    if (points == nullptr) {
        m_Message = "Points为空";
        return false;
    }

    const IGsize pointNum = points->GetNumberOfPoints();
    AttributePoint.clear();
    AttributePoint.reserve(static_cast<size_t>(pointNum));

    auto location = FloatArray::New();
    location->SetDimension(3);
    location->SetName(kLocationAttributeName);
    location->Resize(pointNum);

    const IGsize blockNum = std::max<IGsize>(1, pointNum / 100);
    IGsize progress = 1;
    for (IGsize i = 0; i < pointNum; ++i) {
        const Point& p = points->GetPoint(i);
        const double value[3] = {p[0], p[1], p[2]};
        location->SetElement(i, value);
        AttributePoint.emplace_back(p);

        if (i >= blockNum * progress && progress < 100) {
            UpdateProgress(static_cast<double>(progress) * 0.01);
            ++progress;
        }
    }
    ResetProgress();

    // 附加到输出的属性集（同名属性则原地替换，避免重复点击时叠加）
    auto attrSet = mesh->GetAttributeSet();
    if (attrSet == nullptr) {
        auto newSet = AttributeSet::New();
        mesh->SetAttributeSet(newSet);
        attrSet = mesh->GetAttributeSet();
    }
    if (attrSet == nullptr) {
        m_Message = "输出网格属性集为空";
        return false;
    }

    const int existing = attrSet->GetAttributeIndex(kLocationAttributeName);
    if (existing >= 0) {
        auto& attr = attrSet->GetAttribute(static_cast<IGsize>(existing));
        attr.SetPointer(location);
        attr.SetType(IG_VECTOR);
        attr.SetAttachmentType(IG_POINT);
    } else {
        attrSet->AddVector(IG_POINT, location);
    }

    attributeSet = attrSet;
    return true;
}

bool AppendLocationAttribute::AppendCellCenterToOutput(DataObject::Pointer mesh) {
    if (mesh == nullptr) {
        m_Message = "输出网格为空";
        return false;
    }
    auto cells = mesh->GetCellArray();
    if (cells == nullptr) {
        m_Message = "Cells为空";
        return false;
    }


    // 获取单元数与点数组
    auto cellNum = cells->GetNumberOfCells();
    auto points = mesh->GetPoints();

    //定义验证数组
    AttributeCenter.clear();
    AttributeCenter.reserve(static_cast<size_t>(cellNum));

    //定义属性数组
    auto location = FloatArray::New();
    location->SetDimension(3);
    location->SetName(kCellCenterAttributeName);
    location->Resize(cellNum);

    auto pic = IdArray::New();
    const IGsize blockNum = std::max<IGsize>(1, cellNum / 100);
    IGsize progress = 1;
    for (IGsize i = 0; i < cellNum; ++i) {
        //point in cell
        pic->Reset();
        cells->GetCellIds(i, pic);
        std::vector<Point> candidates;
        candidates.reserve(pic->GetNumberOfIds());
        for (IGsize j = 0; j < pic->GetNumberOfIds(); j++) {
            auto id = pic->GetId(j);
            auto point = points->GetPoint(id);
            candidates.emplace_back(point);
        }
        auto cellcenter = GetCellCenter(candidates);
        std::vector<float> value = {cellcenter[0], cellcenter[1], cellcenter[2]};
        location->SetElement(i, value);
        AttributeCenter.emplace_back(cellcenter);

        if (i >= blockNum * progress && progress < 100) {
            UpdateProgress(static_cast<double>(progress) * 0.01);
            ++progress;
        }
    }
    ResetProgress();

    //附加到输出的属性集
    auto attrSet = mesh->GetAttributeSet();
    if (attrSet == nullptr) {
        auto newSet = AttributeSet::New();
        mesh->SetAttributeSet(newSet);
        attrSet = mesh->GetAttributeSet();
    }
    if (attrSet == nullptr) {
        m_Message = "输出网格属性为空";
        return false;
    }

    const int existing = attrSet->GetAttributeIndex(kCellCenterAttributeName);
    if (existing >= 0) {
        auto& attr = attrSet->GetAttribute(static_cast<IGsize>(existing));
        attr.SetPointer(location);
        attr.SetType(IG_VECTOR);
        attr.SetAttachmentType(IG_CELL);
    }else {
        attrSet->AddVector(IG_CELL, location);
    }
    attributeSet = attrSet;
    return true;
}

Point AppendLocationAttribute::GetCellCenter(std::vector<Point> cell) {
    float sum_x = 0;
    float sum_y = 0;
    float sum_z = 0;
    for (auto & i : cell) {
        sum_x += i[0];
        sum_y += i[1];
        sum_z += i[2];
    }
    Point center;
    center[0] = sum_x / static_cast<float>(cell.size());
    center[1] = sum_y / static_cast<float>(cell.size());
    center[2] = sum_z / static_cast<float>(cell.size());
    return center;
}
IGAME_NAMESPACE_END
