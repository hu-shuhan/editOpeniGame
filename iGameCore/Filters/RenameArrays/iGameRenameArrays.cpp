
// 实现要点：
//   1) 输出 = filter 内部深拷贝(输入)：同类型、几何/拓扑/属性都独立的新网格，输入不被修改；
//   2) 因为是独立副本，改名只需在输出的属性集上 SetName（不需要像 VTK 那样克隆数组对象——
//      我们的 FlatArray::ShallowCopy 返回 false，也没有"共享缓冲"的能力）；
//   3) 只处理 AttributeSet 的两种关联：IG_POINT / IG_CELL
//      （框架没有 vertex 关联；字段数据在 DataObject::Metadata 里，不属于 AttributeSet，本 filter 不涉及）；
//   4) 空新名拒绝（同 vtkArrayRename）；同组重名拒绝并记入 GetMessage()
//      （VTK 是警告后覆盖，这里避免产生重名数组）；
//   5) 复合/多块数据集不支持，直接失败并给出消息（VTK 由 executive 逐块展开）。
//

#include "iGameRenameArrays.h"

#include "iGameAttributeSet.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

namespace {

// 复制单元数组，避免 CellArray::DeepCopy 在混合单元尺寸下产生错误偏移
CellArray::Pointer DeepCopyCellArray(CellArray* source) {
    auto output = CellArray::New();
    if (!source) {
        return output;
    }

    for (IGsize i = 0; i < source->GetNumberOfCells(); ++i) {
        const igIndex* ids = nullptr;
        const int count = source->GetCellIds(i, ids);
        output->AddCellIds(ids, count);
    }

    return output;
}

// 复制 PointSet 共有的点坐标和属性
void DeepCopyPointSetBase(PointSet* output, PointSet* input, AttributeSet::Pointer attributes) {
    auto points = Points::New();
    if (input->GetPoints()) {
        points->DeepCopy(input->GetPoints());
    }
    output->SetPoints(points);

    if (attributes != nullptr) {
        // 调用方给定属性集：几何取自 input、属性来自别处
        output->SetAttributeSet(attributes);
    } else {
        auto copiedAttributes = AttributeSet::New();
        if (input->GetAttributeSet()) {
            copiedAttributes->DeepCopy(input->GetAttributeSet());
        }
        output->SetAttributeSet(copiedAttributes);
    }
    output->SetName(input->GetName());
}

// 按数据类型创建并深拷贝一个独立输出对象
DataObject::Pointer DeepCopyDataObject(DataObject::Pointer input, AttributeSet::Pointer attributes = nullptr) {
    if (input == nullptr) {
        return nullptr;
    }

    const IGenum type = input->GetDataObjectType();
    DataObject::Pointer output = DataObject::CreateDataObject(type);
    if (!output && type == IG_LAGRANGE_UNSTRUCTURED_MESH) {
        output = LagrangeUnstructuredMesh::New();
    }
    if (!output) {
        return nullptr;
    }

    auto inputPointSet = DynamicCast<PointSet>(input);
    auto outputPointSet = DynamicCast<PointSet>(output);
    if (!inputPointSet || !outputPointSet) {
        return nullptr;
    }
    DeepCopyPointSetBase(outputPointSet.get(), inputPointSet.get(), attributes);

    switch (type) {
        case IG_UNSTRUCTURED_MESH: {
            auto inputMesh = DynamicCast<UnstructuredMesh>(input);
            auto outputMesh = DynamicCast<UnstructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            auto cellTypes = UnsignedIntArray::New();
            if (inputMesh->GetCellTypes()) {
                cellTypes->DeepCopy(inputMesh->GetCellTypes());
            }
            outputMesh->SetCells(DeepCopyCellArray(inputMesh->GetCells()), cellTypes);
            break;
        }
        case IG_SURFACE_MESH: {
            auto inputMesh = DynamicCast<SurfaceMesh>(input);
            auto outputMesh = DynamicCast<SurfaceMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            break;
        }
        case IG_VOLUME_MESH: {
            auto inputMesh = DynamicCast<VolumeMesh>(input);
            auto outputMesh = DynamicCast<VolumeMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            if (inputMesh->GetIsPolyhedronType()) {
                // 多面体网格需要同时重建面片和体-面索引
                auto faces = DeepCopyCellArray(inputMesh->GetFaces());
                auto volumeFaces = CellArray::New();
                for (IGsize i = 0; i < inputMesh->GetNumberOfVolumes(); ++i) {
                    std::vector<igIndex> faceIds(IGAME_CELL_MAX_SIZE);
                    const int faceCount = inputMesh->GetVolumeFaceIds(i, faceIds.data());
                    volumeFaces->AddCellIds(faceIds.data(), faceCount);
                }
                outputMesh->InitVolumesWithPolyhedron(faces, volumeFaces);
                break;
            }

            if (inputMesh->GetVolumes()) {
                outputMesh->SetVolumes(DeepCopyCellArray(inputMesh->GetVolumes()));
            }
            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            outputMesh->SetIsPolyhedronType(false);
            break;
        }
        case IG_STRUCTURED_MESH: {
            auto inputMesh = DynamicCast<StructuredMesh>(input);
            auto outputMesh = DynamicCast<StructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            igIndex* dimensions = inputMesh->GetDimensionSize();
            if (dimensions) {
                igIndex copiedDimensions[3]{dimensions[0], dimensions[1], dimensions[2]};
                outputMesh->SetDimensionSize(copiedDimensions);
            }
            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            break;
        }
        case IG_LAGRANGE_UNSTRUCTURED_MESH: {
            auto inputMesh = DynamicCast<LagrangeUnstructuredMesh>(input);
            auto outputMesh = DynamicCast<LagrangeUnstructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            for (IGsize i = 0; i < inputMesh->GetNumberOfCells(); ++i) {
                const igIndex* ids = nullptr;
                const int count = inputMesh->GetCellPointIds(i, ids);
                std::vector<igIndex> cellIds;
                if (count > 0 && ids) {
                    cellIds.assign(ids, ids + count);
                }
                outputMesh->AddCell(cellIds.data(), count,
                                    inputMesh->GetSpecificCellType(i),
                                    inputMesh->GetCellOrder(i));
            }
            break;
        }
        case IG_POINT_SET:
        default:
            break;
    }

    return output;
}

/** 收集属性集里某一关联类型下的数组名（按属性集内部顺序，即面板看到的顺序） */
void CollectAttributeNames(DataObject* object, IGenum attachmentType, std::vector<std::string>& names) {
    names.clear();
    if (object == nullptr) { return; }

    AttributeSet* attributes = object->GetAttributeSet();
    if (attributes == nullptr) { return; }

    auto all = attributes->GetAllAttributes();
    if (all == nullptr) { return; }

    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachmentType) { continue; }
        names.push_back(attr.pointer->GetName());
    }
}

} // namespace

/* ------------------------------------------------------------------ */
/* 配置                                                                */
/* ------------------------------------------------------------------ */
void RenameArrays::SetArrayName(IGenum attachmentType, const std::string& inputName, const std::string& newName) {
    if (inputName.empty()) { return; }
    // 与 vtkArrayRename 一致：拒绝空名字
    if (newName.empty()) { return; }

    m_ArrayMapping[attachmentType][inputName] = newName;
    this->Modified();
}

void RenameArrays::SetArrayName(IGenum attachmentType, IGsize idx, const std::string& newName) {
    if (newName.empty()) { return; }

    IndexRename pending;
    pending.attachmentType = attachmentType;
    pending.index = idx;
    pending.newName = newName;
    m_PendingByIndex.push_back(pending);
    this->Modified();
}

void RenameArrays::ClearAll() {
    m_ArrayMapping.clear();
    m_PendingByIndex.clear();
    this->Modified();
}

void RenameArrays::ClearMapping(IGenum attachmentType) {
    m_ArrayMapping[attachmentType].clear();
    this->Modified();
}

int RenameArrays::GetNumberOfMappings() const {
    size_t total = m_PendingByIndex.size();
    for (const auto& group : m_ArrayMapping) { total += group.second.size(); }
    return static_cast<int>(total);
}

/* ------------------------------------------------------------------ */
/* 供面板填表（从输入读，与 vtkArrayRename 一致）                      */
/* ------------------------------------------------------------------ */
int RenameArrays::GetNumberOfArrays(IGenum attachmentType) {
    std::vector<std::string> names;
    CollectAttributeNames(GetInput(0).GetPointer(), attachmentType, names);
    return static_cast<int>(names.size());
}

std::string RenameArrays::GetArrayOriginalName(IGenum attachmentType, IGsize idx) {
    std::vector<std::string> names;
    CollectAttributeNames(GetInput(0).GetPointer(), attachmentType, names);
    if (static_cast<size_t>(idx) >= names.size()) { return std::string(); }
    return names[static_cast<size_t>(idx)];
}

std::string RenameArrays::GetArrayNewName(IGenum attachmentType, IGsize idx) {
    const std::string originalName = GetArrayOriginalName(attachmentType, idx);
    if (originalName.empty()) { return std::string(); }

    const auto group = m_ArrayMapping.find(attachmentType);
    if (group == m_ArrayMapping.end()) { return std::string(); }

    const auto mapping = group->second.find(originalName);
    if (mapping == group->second.end()) { return std::string(); }
    return mapping->second;
}

/* ------------------------------------------------------------------ */
/* 执行                                                                */
/* ------------------------------------------------------------------ */
void RenameArrays::ResolveIndexMappings(DataObject* input) {
    if (input == nullptr || m_PendingByIndex.empty()) { return; }

    for (const auto& pending : m_PendingByIndex) {
        if (pending.newName.empty()) { continue; }
        const std::string originalName = GetArrayOriginalName(pending.attachmentType, pending.index);
        if (!originalName.empty()) {
            m_ArrayMapping[pending.attachmentType][originalName] = pending.newName;
        }
    }
    m_PendingByIndex.clear();
}

bool RenameArrays::HasArrayNamed(AttributeSet* attributes, IGenum attachmentType, const std::string& name) {
    if (attributes == nullptr) { return false; }

    auto all = attributes->GetAllAttributes();
    if (all == nullptr) { return false; }

    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachmentType) { continue; }
        if (attr.pointer->GetName() == name) { return true; }
    }
    return false;
}

bool RenameArrays::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "未选择输入数据";
        return false;
    }

    // 按下标配置的条目在这里按输入解析成旧名
    ResolveIndexMappings(input.GetPointer());

    // 输出 = 全新的同类型网格（几何、拓扑、属性全部独立），输入对象不被修改
    auto output = DeepCopyDataObject(input);
    if (output == nullptr) {
        m_Message = "不支持的输入类型（复合/多块数据集请先 ExtractBlock 或 MergeBlocks）";
        return false;
    }

    int renamed = 0;
    std::vector<std::string> conflicts;

    AttributeSet* attributes = output->GetAttributeSet();
    if (attributes != nullptr) {
        auto all = attributes->GetAllAttributes();
        if (all != nullptr) {
            for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
                auto& attr = all->GetElement(i);
                if (attr.isDeleted || attr.pointer == nullptr) { continue; }

                const auto group = m_ArrayMapping.find(attr.attachmentType);
                if (group == m_ArrayMapping.end()) { continue; }

                const std::string originalName = attr.pointer->GetName();
                const auto mapping = group->second.find(originalName);
                if (mapping == group->second.end() || mapping->second.empty() ||
                    mapping->second == originalName) {
                    continue;
                }

                // 同组重名：不改名，避免 GetAttribute(name) 歧义
                if (HasArrayNamed(attributes, attr.attachmentType, mapping->second)) {
                    conflicts.push_back(originalName + " -> " + mapping->second);
                    continue;
                }

                attr.pointer->SetName(mapping->second);
                ++renamed;
            }
        }
    }

    SetOutput(0, output);

    std::string message = "RenameArrays: renamed=" + std::to_string(renamed);
    if (!conflicts.empty()) {
        message += ", skipped(conflict)=" + std::to_string(conflicts.size()) + " [";
        for (size_t i = 0; i < conflicts.size(); ++i) {
            if (i != 0) { message += "; "; }
            message += conflicts[i];
        }
        message += "]";
    }
    m_Message = message;

    return true;
}

IGAME_NAMESPACE_END
