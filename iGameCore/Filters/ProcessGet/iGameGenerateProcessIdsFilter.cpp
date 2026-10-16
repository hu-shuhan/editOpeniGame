#include "iGameGenerateProcessIdsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

// 生成结果数组名（固定命名，便于 GUI 与测试直接引用）
const char* const PointProcessIdsName = "PointProcessIds";
const char* const CellProcessIdsName = "CellProcessIds";

// 返回输入网格的单元总数；网格类型没有单元（如纯点云 PointSet）时返回 false。
// 作为实现细节留在匿名命名空间，不进头文件，避免把「按网格类型分发」固化成对外接口。
bool GetCellCount(PointSet* mesh, IGsize& cellCount) {
    switch (mesh->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto surfaceMesh = DynamicCast<SurfaceMesh>(mesh);
            if (surfaceMesh == nullptr) return false;
            cellCount = surfaceMesh->GetNumberOfFaces();
            return true;
        }
        case IG_VOLUME_MESH: {
            auto volumeMesh = DynamicCast<VolumeMesh>(mesh);
            if (volumeMesh == nullptr) return false;
            cellCount = volumeMesh->GetNumberOfVolumes();
            return true;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto unstructuredMesh = DynamicCast<UnstructuredMesh>(mesh);
            if (unstructuredMesh == nullptr) return false;
            cellCount = unstructuredMesh->GetNumberOfCells();
            return true;
        }
        case IG_STRUCTURED_MESH: {
            auto structuredMesh = DynamicCast<StructuredMesh>(mesh);
            if (structuredMesh == nullptr) return false;
            cellCount = structuredMesh->GetNumberOfCells();
            return true;
        }
        case IG_LAGRANGE_UNSTRUCTURED_MESH: {
            auto lagrangeMesh = DynamicCast<LagrangeUnstructuredMesh>(mesh);
            if (lagrangeMesh == nullptr) return false;
            cellCount = lagrangeMesh->GetNumberOfCells();
            return true;
        }
        case IG_POINT_SET:
        default:
            return false;
    }
}

// 结果属性集 = 输入属性集的拷贝（Attribute 记录是新的，数组直接共享），
// 按「名字 + 挂载类型」精确跳过与输出同名的旧数组（覆盖语义，避免 isDeleted 残留，
// 也避免误删点 / 单元上的同名数组）。
// 数组只读共享：结果只新增自己的进程号数组，从不修改已有数组；
// 深拷贝会让大模型的内存翻倍并一直驻留到删除结果节点，这里不做。
void CopyInputAttributes(const AttributeSet::Pointer& source, const AttributeSet::Pointer& target) {
    if (source == nullptr || target == nullptr) { return; }
    auto allAttributes = source->GetAllAttributes();
    if (allAttributes == nullptr) { return; }
    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
        auto& attribute = allAttributes->GetElement(i);
        if (attribute.IsNone() || attribute.pointer == nullptr) { continue; }
        const std::string name = attribute.pointer->GetName();
        if (attribute.attachmentType == IG_POINT && name == PointProcessIdsName) { continue; }
        if (attribute.attachmentType == IG_CELL && name == CellProcessIdsName) { continue; }

        target->AddAttribute(attribute.type, attribute.attachmentType, attribute.pointer);
    }
}

// 结果对象持有独立的 Points（与输入共享底层缓冲，但时间戳互不影响）：
// 若直接把输入的 Points 交给 result->SetPoints()，PointSet::SetPoints() 会对该共享对象调
// Modified()，把输入模型的几何时间戳顶掉，导致输入模型也要重跑表面提取。
Points::Pointer CreateSharedPoints(const Points::Pointer& source) {
    if (source == nullptr) { return nullptr; }
    auto points = Points::New();
    points->ShallowCopy(source);
    return points;
}

// 依据输入网格类型创建结果对象，几何（点/单元）与输入共享，不复制几何数据。
DataObject::Pointer CreateResultObject(DataObject::Pointer input) {
    switch (input->GetDataObjectType()) {
        case IG_POINT_SET: {
            auto source = DynamicCast<PointSet>(input);
            if (source == nullptr) { return nullptr; }
            auto result = PointSet::New();
            result->SetPoints(CreateSharedPoints(source->GetPoints()));
            return result;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto source = DynamicCast<UnstructuredMesh>(input);
            if (source == nullptr) { return nullptr; }
            auto result = UnstructuredMesh::New();
            result->SetPoints(CreateSharedPoints(source->GetPoints()));
            if (source->GetCells() && source->GetCellTypes()) {
                result->SetCells(source->GetCells(), UnsignedIntArray::Pointer(source->GetCellTypes()));
            }
            return result;
        }
        case IG_SURFACE_MESH: {
            auto source = DynamicCast<SurfaceMesh>(input);
            if (source == nullptr) { return nullptr; }
            auto result = SurfaceMesh::New();
            result->SetPoints(CreateSharedPoints(source->GetPoints()));
            if (source->GetFaces()) { result->SetFaces(CellArray::Pointer(source->GetFaces())); }
            return result;
        }
        case IG_VOLUME_MESH: {
            auto source = DynamicCast<VolumeMesh>(input);
            if (source == nullptr) { return nullptr; }
            auto result = VolumeMesh::New();
            result->SetPoints(CreateSharedPoints(source->GetPoints()));
            if (source->GetFaces()) { result->SetFaces(CellArray::Pointer(source->GetFaces())); }
            if (source->GetVolumes()) { result->SetVolumes(CellArray::Pointer(source->GetVolumes())); }
            return result;
        }
        case IG_STRUCTURED_MESH: {
            auto source = DynamicCast<StructuredMesh>(input);
            if (source == nullptr) { return nullptr; }
            auto result = StructuredMesh::New();
            result->SetPoints(CreateSharedPoints(source->GetPoints()));
            result->SetDimensionSize(source->GetDimensionSize());
            result->SetExtent(source->GetExtent());
            result->GenStructuredCellConnectivities();
            return result;
        }
        case IG_LAGRANGE_UNSTRUCTURED_MESH: {
            auto source = DynamicCast<LagrangeUnstructuredMesh>(input);
            if (source == nullptr) { return nullptr; }
            auto result = LagrangeUnstructuredMesh::New();
            result->SetPoints(CreateSharedPoints(source->GetPoints()));
            const IGsize cellNum = source->GetNumberOfCells();
            for (IGsize i = 0; i < cellNum; ++i) {
                const igIndex* ids = nullptr;
                const int count = source->GetCellPointIds(i, ids);
                if (count <= 0 || ids == nullptr) { continue; }
                std::vector<igIndex> cellIds(ids, ids + count);
                result->AddCell(cellIds.data(), count, source->GetSpecificCellType(i), source->GetCellOrder(i));
            }
            return result;
        }
        default:
            return nullptr;
    }
}

// 按输入元素数量生成进程号数组（count 为 0 时同样产出空数组，对齐 VTK）。
template <typename ValueOf>
LongLongArray::Pointer CreateProcessIds(const char* const name, const IGsize count, ValueOf valueOf) {
    auto ids = LongLongArray::New();
    ids->SetName(name);
    ids->SetDimension(1);
    ids->Resize(count);
    for (IGsize i = 0; i < count; ++i) { ids->SetValue(i, valueOf(i)); }
    return ids;
}

}  // namespace

GenerateProcessIdsFilter::GenerateProcessIdsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

// 默认策略：所有点 / 单元都写当前进程号；派生类可重写这两个方法自定义分区。
long long GenerateProcessIdsFilter::GetPointProcessId(IGsize index) {
    (void)index;
    return m_ProcessId;
}

long long GenerateProcessIdsFilter::GetCellProcessId(IGsize index) {
    (void)index;
    return m_ProcessId;
}

bool GenerateProcessIdsFilter::Execute() {
    m_Message.clear();
    // 执行失败时不保留上一次的结果
    this->SetOutput(0, nullptr);

    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "GenerateProcessIdsFilter has no input DataObject.";
        return false;
    }

    auto mesh = DynamicCast<PointSet>(input);
    if (mesh == nullptr) {
        m_Message = "GenerateProcessIdsFilter requires a PointSet input.";
        return false;
    }

    auto attributeSet = input->GetAttributeSet();
    if (attributeSet == nullptr) {
        m_Message = "GenerateProcessIdsFilter input has no AttributeSet.";
        return false;
    }

    if (!m_GeneratePointData && !m_GenerateCellData) {
        m_Message = "Neither point nor cell process ids were requested.";
        return false;
    }

    const IGsize pointNum = mesh->GetNumberOfPoints();

    // 单元数据是可选项：网格类型没有单元（如纯点云 PointSet）时不整体失败，
    // 已经生成的点进程号照常保留，只跳过单元数据并记录原因。
    IGsize cellNum = 0;
    const bool generateCellData = m_GenerateCellData && GetCellCount(mesh, cellNum);
    if (m_GenerateCellData && !generateCellData) {
        m_Message = "GenerateProcessIdsFilter skipped cell data: this mesh type has no cells.";
    }

    // 结果属性集：输入属性拷贝 + 新生成的进程号数组
    auto resultAttributeSet = AttributeSet::New();
    CopyInputAttributes(attributeSet, resultAttributeSet);

    if (m_GeneratePointData) {
        auto pointIds = CreateProcessIds(PointProcessIdsName, pointNum,
                                         [this](IGsize index) { return this->GetPointProcessId(index); });
        resultAttributeSet->AddScalar(IG_POINT, pointIds);
    }
    if (generateCellData) {
        auto cellIds = CreateProcessIds(CellProcessIdsName, cellNum,
                                        [this](IGsize index) { return this->GetCellProcessId(index); });
        resultAttributeSet->AddScalar(IG_CELL, cellIds);
    }

    // 结果对象：几何与输入共享
    auto result = CreateResultObject(input);
    if (result == nullptr) {
        m_Message = "GenerateProcessIdsFilter does not support an independent output for this mesh type.";
        return false;
    }

    const std::string inputName = input->GetName();
    result->SetName(inputName.empty() ? std::string("ProcessIds") : inputName + "_ProcessIds");
    result->SetAttributeSet(resultAttributeSet);
    SetOutput(result);
    return true;
}

IGAME_NAMESPACE_END
