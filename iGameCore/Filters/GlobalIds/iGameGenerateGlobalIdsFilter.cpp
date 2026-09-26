#include "iGameGenerateGlobalIdsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCompositeDataObject.h"
#include "iGameDrawObject.h"
#include "iGameFlatArray.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{

struct EntityCounts {
    IGsize PointCount{0};
    IGsize CellCount{0};

    const Points* PointIdentity{nullptr};

    bool SupportsPointIds{false};
    bool SupportsCellIds{false};
};

struct LeafPlan {
    DataObject::Pointer Object;

    IGsize PointCount{0};
    IGsize CellCount{0};

    const Points* PointIdentity{nullptr};

    bool SupportsPointIds{false};
    bool SupportsCellIds{false};

    iguIndex64 PointStart{0};
    iguIndex64 CellStart{0};

    int ExistingPointAttribute{-1};
    int ExistingCellAttribute{-1};

    bool KeepPointAttribute{false};
    bool KeepCellAttribute{false};
};

struct PendingAttribute {
    DataObject::Pointer Object;
    AttributeSet* Attributes{nullptr};

    int ExistingAttributeIndex{-1};
    IGenum AttachmentType{IG_NONE};

    DoubleArray::Pointer Array;
};

constexpr iguIndex64 MaximumExactDoubleInteger = iguIndex64{1} << std::numeric_limits<double>::digits;

struct CloneContext {
    // 多个叶子共享 Points 时，输出中也必须继续共享同一份克隆，否则会改变点 ID 的去重语义。
    std::unordered_map<const Points*, Points::Pointer> PointCopies;
    std::unordered_set<const DataObject*> VisitedObjects;
};

Points::Pointer ClonePoints(const PointSet::Pointer& source, CloneContext& context,
                            std::string& errorMessage) {
    if (!source || !source->GetPoints()) {
        errorMessage = "A PointSet has no Points object.";
        return nullptr;
    }

    const auto* pointIdentity = source->GetPoints().get();
    const auto existing = context.PointCopies.find(pointIdentity);
    if (existing != context.PointCopies.end()) { return existing->second; }

    auto points = Points::New();
    if (!points->DeepCopy(source->GetPoints())) {
        errorMessage = "Failed to clone a Points object.";
        return nullptr;
    }

    context.PointCopies.emplace(pointIdentity, points);
    return points;
}

CellArray::Pointer CloneCells(CellArray* source, std::string& errorMessage) {
    if (!source) {
        errorMessage = "Cannot clone a null CellArray.";
        return nullptr;
    }

    auto cells = CellArray::New();
    for (IGsize cellId = 0; cellId < source->GetNumberOfCells(); ++cellId) {
        const igIndex* pointIds = nullptr;
        const int pointCount = source->GetCellIds(cellId, pointIds);
        if (pointCount < 0 || (pointCount > 0 && !pointIds)) {
            errorMessage = "Failed to read cell connectivity while cloning a CellArray.";
            return nullptr;
        }

        const IGsize copiedCellId = cells->AddCellIds(pointIds, pointCount);
        if (copiedCellId != cellId) {
            errorMessage = "Failed to preserve cell order while cloning a CellArray.";
            return nullptr;
        }
        if (source->IsDeleted(cellId)) { cells->DeleteCell(copiedCellId); }
    }
    return cells;
}

bool CloneAttributes(const DataObject::Pointer& source, const DataObject::Pointer& destination,
                     std::string& errorMessage) {
    if (!source || !source->GetAttributeSet()) {
        errorMessage = "A DataObject has no AttributeSet.";
        return false;
    }

    auto attributes = AttributeSet::New();
    if (!attributes->DeepCopy(source->GetAttributeSet())) {
        errorMessage = "Failed to clone a DataObject AttributeSet.";
        return false;
    }
    destination->SetAttributeSet(attributes);
    return true;
}

DataObject::Pointer CloneDataObjectTreeImpl(const DataObject::Pointer& source, CloneContext& context,
                                            std::string& errorMessage) {
    if (!source) {
        errorMessage = "GenerateGlobalIdsFilter encountered a null DataObject while cloning.";
        return nullptr;
    }

    if (!context.VisitedObjects.insert(source.get()).second) {
        errorMessage = "GenerateGlobalIdsFilter cannot clone a cyclic or multiply referenced DataObject hierarchy.";
        return nullptr;
    }

    DataObject::Pointer destination;

    switch (source->GetDataObjectType()) {
        case IG_DATA_OBJECT:
            destination = DataObject::New();
            break;

        case IG_COMPOSITE_DATA_OBJECT:
            destination = CompositeDataObject::New();
            break;

        case IG_DRAW_OBJECT:
            destination = DrawObject::New();
            break;

        case IG_POINT_SET: {
            auto sourceMesh = DynamicCast<PointSet>(source);
            auto destinationMesh = PointSet::New();
            auto points = ClonePoints(sourceMesh, context, errorMessage);
            if (!points) { return nullptr; }
            destinationMesh->SetPoints(points);
            destination = destinationMesh;
            break;
        }

        case IG_SURFACE_MESH: {
            auto sourceMesh = DynamicCast<SurfaceMesh>(source);
            auto destinationMesh = SurfaceMesh::New();
            auto points = ClonePoints(sourceMesh, context, errorMessage);
            if (!points) { return nullptr; }
            destinationMesh->SetPoints(points);
            if (sourceMesh->GetFaces()) {
                auto faces = CloneCells(sourceMesh->GetFaces(), errorMessage);
                if (!faces) { return nullptr; }
                destinationMesh->SetFaces(faces);
            }
            if (sourceMesh->GetEdges()) {
                auto edges = CloneCells(sourceMesh->GetEdges(), errorMessage);
                if (!edges) { return nullptr; }
                destinationMesh->SetEdges(edges);
            }
            destination = destinationMesh;
            break;
        }

        case IG_VOLUME_MESH: {
            auto sourceMesh = DynamicCast<VolumeMesh>(source);
            auto destinationMesh = VolumeMesh::New();
            auto points = ClonePoints(sourceMesh, context, errorMessage);
            if (!points) { return nullptr; }
            destinationMesh->SetPoints(points);
            if (sourceMesh->GetVolumes()) {
                auto volumes = CloneCells(sourceMesh->GetVolumes(), errorMessage);
                if (!volumes) { return nullptr; }
                destinationMesh->SetVolumes(volumes);
            }
            if (sourceMesh->GetFaces()) {
                auto faces = CloneCells(sourceMesh->GetFaces(), errorMessage);
                if (!faces) { return nullptr; }
                destinationMesh->SetFaces(faces);
            }
            if (sourceMesh->GetEdges()) {
                auto edges = CloneCells(sourceMesh->GetEdges(), errorMessage);
                if (!edges) { return nullptr; }
                destinationMesh->SetEdges(edges);
            }
            destination = destinationMesh;
            break;
        }

        case IG_STRUCTURED_MESH: {
            auto sourceMesh = DynamicCast<StructuredMesh>(source);
            auto destinationMesh = StructuredMesh::New();
            auto points = ClonePoints(sourceMesh, context, errorMessage);
            if (!points) { return nullptr; }
            destinationMesh->SetPoints(points);
            destinationMesh->SetDimensionSize(sourceMesh->GetDimensionSize());
            if (sourceMesh->GetVolumes()) {
                auto volumes = CloneCells(sourceMesh->GetVolumes(), errorMessage);
                if (!volumes) { return nullptr; }
                destinationMesh->SetVolumes(volumes);
            }
            if (sourceMesh->GetFaces()) {
                auto faces = CloneCells(sourceMesh->GetFaces(), errorMessage);
                if (!faces) { return nullptr; }
                destinationMesh->SetFaces(faces);
            }
            if (sourceMesh->GetEdges()) {
                auto edges = CloneCells(sourceMesh->GetEdges(), errorMessage);
                if (!edges) { return nullptr; }
                destinationMesh->SetEdges(edges);
            }
            destination = destinationMesh;
            break;
        }

        case IG_UNSTRUCTURED_MESH: {
            auto sourceMesh = DynamicCast<UnstructuredMesh>(source);
            auto destinationMesh = UnstructuredMesh::New();
            auto points = ClonePoints(sourceMesh, context, errorMessage);
            if (!points) { return nullptr; }
            destinationMesh->SetPoints(points);

            auto cells = CloneCells(sourceMesh->GetCells().get(), errorMessage);
            if (!cells) { return nullptr; }
            auto cellTypes = UnsignedIntArray::New();
            if (!cellTypes->DeepCopy(sourceMesh->GetCellTypes())) {
                errorMessage = "Failed to clone UnstructuredMesh cell types.";
                return nullptr;
            }
            destinationMesh->SetCells(cells, cellTypes);
            destination = destinationMesh;
            break;
        }

        case IG_LAGRANGE_UNSTRUCTURED_MESH: {
            auto sourceMesh = DynamicCast<LagrangeUnstructuredMesh>(source);
            auto destinationMesh = LagrangeUnstructuredMesh::New();
            auto points = ClonePoints(sourceMesh, context, errorMessage);
            if (!points) { return nullptr; }
            destinationMesh->SetPoints(points);

            for (IGsize cellId = 0; cellId < sourceMesh->GetNumberOfCells(); ++cellId) {
                const igIndex* sourceIds = nullptr;
                const int pointCount = sourceMesh->GetCellPointIds(cellId, sourceIds);
                if (pointCount <= 0 || !sourceIds) {
                    errorMessage = "Failed to read a LagrangeUnstructuredMesh cell.";
                    return nullptr;
                }
                std::vector<igIndex> cellIds(sourceIds, sourceIds + pointCount);
                destinationMesh->AddCell(cellIds.data(), pointCount, sourceMesh->GetSpecificCellType(cellId),
                                         sourceMesh->GetCellOrder(cellId));
            }
            destination = destinationMesh;
            break;
        }

        default:
            errorMessage = "Unsupported DataObject type while creating an independent output: " +
                           std::to_string(source->GetDataObjectType()) + ".";
            return nullptr;
    }

    if (!destination) {
        errorMessage = "Failed to create an independent output DataObject.";
        return nullptr;
    }

    destination->SetName(source->GetName());
    if (!CloneAttributes(source, destination, errorMessage)) { return nullptr; }

    if (source->HasSubDataObject()) {
        for (auto it = source->SubDataObjectIteratorBegin(); it != source->SubDataObjectIteratorEnd(); ++it) {
            auto child = CloneDataObjectTreeImpl(it->second, context, errorMessage);
            if (!child) { return nullptr; }
            destination->AddSubDataObject(child);
        }
    }

    return destination;
}

DataObject::Pointer CloneDataObjectTree(const DataObject::Pointer& source, std::string& errorMessage) {
    CloneContext context;
    return CloneDataObjectTreeImpl(source, context, errorMessage);
}

/**
 * 判断两个 IGsize 相加是否会发生溢出。
 */
bool CheckedAdd(IGsize& total, const IGsize value) {
    const IGsize maximum = std::numeric_limits<IGsize>::max();

    if (value > maximum - total) { return false; }

    total += value;
    return true;
}

/**
 * 判断 [offset, offset + count) 是否能够由 iguIndex64 表示。
 */
bool RangeFits(const iguIndex64 offset, const IGsize count) {
    if (count == 0) { return true; }

    const iguIndex64 maximum = std::numeric_limits<iguIndex64>::max();

    return offset <= maximum - static_cast<iguIndex64>(count - 1);
}

/**
 * 判断 [offset, offset + count) 内的每个整数是否都能由 DoubleArray 精确表示。
 */
bool RangeFitsExactlyInDouble(const iguIndex64 offset, const IGsize count) {
    if (count == 0) { return true; }

    const auto lastDelta = static_cast<iguIndex64>(count - 1);
    return lastDelta <= MaximumExactDoubleInteger && offset <= MaximumExactDoubleInteger - lastDelta;
}

/**
 * 递归收集叶子 DataObject。
 *
 * 有子对象时只继续遍历子对象，不把父容器本身加入叶子列表。
 */
bool CollectLeafObjectsImpl(const DataObject::Pointer& object, std::vector<DataObject::Pointer>& leaves,
                            std::unordered_set<const DataObject*>& visiting,
                            std::unordered_set<const DataObject*>& visited,
                            std::vector<DataObject::Pointer>* hierarchyContainers, std::string& errorMessage) {
    if (!object) {
        errorMessage = "GenerateGlobalIdsFilter encountered a null DataObject.";
        return false;
    }

    const auto* objectKey = object.get();

    if (visiting.contains(objectKey)) {
        errorMessage = "GenerateGlobalIdsFilter detected a cycle in the sub DataObject hierarchy.";
        return false;
    }

    if (!visited.insert(objectKey).second) {
        errorMessage = "GenerateGlobalIdsFilter encountered the same DataObject through multiple hierarchy paths.";
        return false;
    }

    visiting.insert(objectKey);

    if (!object->HasSubDataObject()) {
        leaves.push_back(object);
        visiting.erase(objectKey);
        return true;
    }

    if (hierarchyContainers) { hierarchyContainers->push_back(object); }

    for (auto it = object->SubDataObjectIteratorBegin(); it != object->SubDataObjectIteratorEnd(); ++it) {
        if (!it->second) {
            errorMessage = "GenerateGlobalIdsFilter encountered a null "
                           "sub DataObject.";
            return false;
        }

        if (!CollectLeafObjectsImpl(it->second, leaves, visiting, visited, hierarchyContainers, errorMessage)) {
            return false;
        }
    }

    visiting.erase(objectKey);
    return true;
}

bool CollectLeafObjects(const DataObject::Pointer& object, std::vector<DataObject::Pointer>& leaves,
                        std::string& errorMessage,
                        std::vector<DataObject::Pointer>* hierarchyContainers = nullptr) {
    std::unordered_set<const DataObject*> visiting;
    std::unordered_set<const DataObject*> visited;
    return CollectLeafObjectsImpl(object, leaves, visiting, visited, hierarchyContainers, errorMessage);
}

/**
 * 获取一个叶子网格的 Point/Cell 数量。
 */
bool GetLeafEntityCounts(const DataObject::Pointer& object, EntityCounts& counts, std::string& errorMessage) {
    counts = EntityCounts{};

    if (!object) {
        errorMessage = "Input leaf DataObject is null.";
        return false;
    }

    // 当前支持的网格必须继承自 PointSet。
    auto pointSet = DynamicCast<PointSet>(object);
    if (!pointSet) {
        errorMessage = "Unsupported leaf DataObject type: " + std::to_string(object->GetDataObjectType()) +
                       ". The object is not a PointSet.";
        return false;
    }

    counts.SupportsPointIds = true;
    counts.PointCount = pointSet->GetNumberOfPoints();
    counts.PointIdentity = pointSet->GetPoints().get();

    switch (object->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto mesh = DynamicCast<SurfaceMesh>(object);
            if (!mesh) {
                errorMessage = "Failed to cast DataObject to SurfaceMesh.";
                return false;
            }

            counts.SupportsCellIds = true;
            counts.CellCount = mesh->GetNumberOfFaces();
            break;
        }

        case IG_VOLUME_MESH: {
            auto mesh = DynamicCast<VolumeMesh>(object);
            if (!mesh) {
                errorMessage = "Failed to cast DataObject to VolumeMesh.";
                return false;
            }

            counts.SupportsCellIds = true;
            counts.CellCount = mesh->GetNumberOfVolumes();
            break;
        }

        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(object);
            if (!mesh) {
                errorMessage = "Failed to cast DataObject to UnstructuredMesh.";
                return false;
            }

            counts.SupportsCellIds = true;
            counts.CellCount = mesh->GetNumberOfCells();
            break;
        }

        case IG_STRUCTURED_MESH: {
            auto mesh = DynamicCast<StructuredMesh>(object);
            if (!mesh) {
                errorMessage = "Failed to cast DataObject to StructuredMesh.";
                return false;
            }

            /*
             * StructuredMesh::GetCellArray() 返回 nullptr，
             * 因此必须显式调用 GetNumberOfCells()。
             */
            counts.SupportsCellIds = true;
            counts.CellCount = mesh->GetNumberOfCells();
            break;
        }

        case IG_LAGRANGE_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<LagrangeUnstructuredMesh>(object);

            if (!mesh) {
                errorMessage = "Failed to cast DataObject to "
                               "LagrangeUnstructuredMesh.";
                return false;
            }

            /*
             * LagrangeUnstructuredMesh 没有覆盖 GetCellArray()，
             * 因此也必须显式调用 GetNumberOfCells()。
             */
            counts.SupportsCellIds = true;
            counts.CellCount = mesh->GetNumberOfCells();
            break;
        }

        /*
         * 普通 PointSet 及其他 PointSet 派生类没有顶层 Cell。
         */
        case IG_POINT_SET:
        default:
            counts.SupportsCellIds = false;
            counts.CellCount = 0;
            break;
    }

    return true;
}

/**
 * 查找指定名称和 attachmentType 的属性。
 *
 * 不直接使用 GetAttributeIndex(name)，因为属性系统允许
 * Point 属性和 Cell 属性具有相同名称。
 */
int FindAttributeIndex(AttributeSet* attributes, const std::string& name, const IGenum attachmentType) {
    if (!attributes) { return -1; }

    auto allAttributes = attributes->GetAllAttributes();
    if (!allAttributes) { return -1; }

    const IGsize attributeCount = allAttributes->GetNumberOfElements();

    int result = -1;

    for (IGsize i = 0; i < attributeCount; ++i) {
        const auto& attribute = allAttributes->GetElement(i);

        if (attribute.isDeleted || !attribute.pointer) { continue; }

        if (attribute.attachmentType != attachmentType) { continue; }

        if (attribute.pointer->GetName() != name) { continue; }

        // -2 表示同一 attachment 下存在多个同名属性，避免只替换其中一个。
        if (result >= 0) { return -2; }
        result = static_cast<int>(i);
    }

    return result;
}

bool IsSupportedExistingIdArray(const ArrayObject::Pointer& array) {
    if (!array) { return false; }
    return array->GetArrayType() == IG_DoubleArray;
}

bool ValidateExistingIdValues(const ArrayObject::Pointer& array, const IGsize expectedElementCount,
                              const iguIndex64 expectedStart, const std::string& name,
                              std::string& errorMessage) {
    if (auto ids = DynamicCast<DoubleArray>(array)) {
        const auto* values = ids->RawPointer();
        for (IGsize i = 0; i < expectedElementCount; ++i) {
            const auto expected = expectedStart + static_cast<iguIndex64>(i);
            if (values[i] == static_cast<double>(expected)) { continue; }

            errorMessage = "Existing attribute '" + name + "' is not the expected continuous Global ID range at " +
                           std::to_string(i) + ". Expected " + std::to_string(expected) + ", but received " +
                           std::to_string(values[i]) + ".";
            return false;
        }
        return true;
    }

    errorMessage = "Existing attribute '" + name + "' is not a DoubleArray.";
    return false;
}

/**
 * 根据 ExistingIdPolicy 处理已有属性。
 */
bool ResolveExistingAttribute(AttributeSet* attributes, const std::string& name, const IGenum attachmentType,
                              const IGsize expectedElementCount, const iguIndex64 expectedStart,
                              const GenerateGlobalIdsFilter::ExistingIdPolicy policy, int& existingIndex,
                              bool& keepExisting, std::string& errorMessage) {
    existingIndex = FindAttributeIndex(attributes, name, attachmentType);

    keepExisting = false;

    if (existingIndex == -2) {
        errorMessage = "Multiple attributes named '" + name + "' exist for attachment type " +
                       std::to_string(attachmentType) + ".";
        return false;
    }

    if (existingIndex < 0) { return true; }

    switch (policy) {
        case GenerateGlobalIdsFilter::ExistingIdPolicy::Error: {
            errorMessage = "Attribute '" + name + "' already exists for attachment type " +
                           std::to_string(attachmentType) + ".";
            return false;
        }

        case GenerateGlobalIdsFilter::ExistingIdPolicy::Replace:
            return true;

        case GenerateGlobalIdsFilter::ExistingIdPolicy::KeepExisting: {
            auto& attribute = attributes->GetAttribute(static_cast<IGsize>(existingIndex));

            if (!attribute.pointer) {
                errorMessage = "Existing attribute '" + name + "' has a null array.";
                return false;
            }

            if (attribute.type != IG_SCALAR) {
                errorMessage = "Existing attribute '" + name + "' is not a scalar attribute.";
                return false;
            }

            if (!IsSupportedExistingIdArray(attribute.pointer)) {
                errorMessage = "Existing attribute '" + name + "' is not a DoubleArray.";
                return false;
            }

            if (attribute.pointer->GetDimension() != 1) {
                errorMessage = "Existing attribute '" + name + "' must have dimension 1.";
                return false;
            }

            if (attribute.pointer->GetNumberOfElements() != expectedElementCount) {
                errorMessage = "Existing attribute '" + name + "' has an invalid element count. Expected " +
                               std::to_string(expectedElementCount) + ", but received " +
                               std::to_string(attribute.pointer->GetNumberOfElements()) + ".";
                return false;
            }

            if (!ValidateExistingIdValues(attribute.pointer, expectedElementCount, expectedStart, name,
                                          errorMessage)) {
                return false;
            }

            keepExisting = true;
            return true;
        }
    }

    errorMessage = "Unknown ExistingIdPolicy value.";
    return false;
}

DoubleArray::Pointer CreateIdArray(const std::string& name, const IGsize count, const iguIndex64 start) {
    auto ids = DoubleArray::New();

    ids->SetName(name);
    ids->SetDimension(1);
    ids->Resize(count);

    auto* values = ids->RawPointer();

    for (IGsize i = 0; i < count; ++i) {
        values[i] = static_cast<double>(start + static_cast<iguIndex64>(i));
    }

    ids->Modified();
    return ids;
}

void CommitAttribute(const PendingAttribute& pending) {
    if (!pending.Attributes || !pending.Array) { return; }

    if (pending.ExistingAttributeIndex >= 0) {
        auto& attribute = pending.Attributes->GetAttribute(static_cast<IGsize>(pending.ExistingAttributeIndex));

        attribute.pointer = pending.Array;
        attribute.type = IG_SCALAR;
        attribute.attachmentType = pending.AttachmentType;
        attribute.isDeleted = false;

        /*
         * 新数组需要重新计算范围。
         * Global IDs 通常不用于颜色映射，因此不主动触发渲染转换。
         */
        attribute.dataRange = nullptr;
    } else {
        pending.Attributes->AddAttribute(IG_SCALAR, pending.AttachmentType, pending.Array);
    }

    pending.Attributes->Modified();

    if (pending.Object) { pending.Object->Modified(); }
}

} // namespace

GenerateGlobalIdsFilter::GenerateGlobalIdsFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

bool GenerateGlobalIdsFilter::CountEntities(DataObject::Pointer input, IGsize& pointCount, IGsize& cellCount) {
    pointCount = 0;
    cellCount = 0;

    if (!input) { return false; }

    std::vector<DataObject::Pointer> leaves;
    std::string errorMessage;

    if (!CollectLeafObjects(input, leaves, errorMessage)) { return false; }

    if (leaves.empty()) { return false; }

    std::unordered_set<const Points*> countedPointSets;

    for (const auto& leaf: leaves) {
        EntityCounts counts;

        if (!GetLeafEntityCounts(leaf, counts, errorMessage)) { return false; }

        const bool isFirstPointSet = counts.PointIdentity == nullptr || countedPointSets.insert(counts.PointIdentity).second;
        if (counts.SupportsPointIds && isFirstPointSet && !CheckedAdd(pointCount, counts.PointCount)) {
            pointCount = 0;
            cellCount = 0;
            return false;
        }

        if (counts.SupportsCellIds && !CheckedAdd(cellCount, counts.CellCount)) {
            pointCount = 0;
            cellCount = 0;
            return false;
        }
    }

    return true;
}

bool GenerateGlobalIdsFilter::Execute() {
    /*
     * 避免本次执行失败时仍返回上一次执行结果。
     */
    this->SetOutput(0, nullptr);
    this->UpdateProgress(0.0);
    m_Message.clear();

    auto input = this->GetInput(0);

    if (!input) {
        m_Message = "GenerateGlobalIdsFilter has no input DataObject.";
        return false;
    }

    if (m_GeneratePointIds && m_PointArrayName.empty()) {
        m_Message = "The Point Global ID array name is empty.";
        return false;
    }

    if (m_GenerateCellIds && m_CellArrayName.empty()) {
        m_Message = "The Cell Global ID array name is empty.";
        return false;
    }

    DataObject::Pointer output;
    try {
        output = CloneDataObjectTree(input, m_Message);
    } catch (const std::bad_alloc&) {
        m_Message = "Memory allocation failed while cloning the input DataObject.";
        return false;
    } catch (const std::length_error&) {
        m_Message = "The input DataObject is too large to clone on the current platform.";
        return false;
    }

    if (!output) { return false; }

    output->SetName(input->GetName().empty() ? "GlobalIds" : input->GetName() + "_GlobalIds");

    if (!m_GeneratePointIds && !m_GenerateCellIds) {
        m_Message = "No Global ID array was requested; an independent copy was created.";
        this->SetOutput(0, output);
        this->UpdateProgress(1.0);
        return true;
    }

    std::vector<DataObject::Pointer> leaves;
    std::vector<DataObject::Pointer> hierarchyContainers;

    if (!CollectLeafObjects(output, leaves, m_Message, &hierarchyContainers)) { return false; }

    if (leaves.empty()) {
        m_Message = "No leaf mesh was found in the input DataObject.";
        return false;
    }

    std::vector<LeafPlan> plans;
    plans.reserve(leaves.size());

    IGsize totalPointCount = 0;
    IGsize totalCellCount = 0;
    std::unordered_set<const Points*> countedPointSets;

    /*
     * 第一阶段：收集所有叶子网格并统计实体数量。
     * 此阶段只读取独立输出，不修改输入。
     */
    for (const auto& leaf: leaves) {
        EntityCounts counts;

        if (!GetLeafEntityCounts(leaf, counts, m_Message)) { return false; }

        LeafPlan plan;
        plan.Object = leaf;
        plan.PointCount = counts.PointCount;
        plan.CellCount = counts.CellCount;
        plan.PointIdentity = counts.PointIdentity;
        plan.SupportsPointIds = counts.SupportsPointIds;
        plan.SupportsCellIds = counts.SupportsCellIds;

        if (m_GeneratePointIds && counts.SupportsPointIds) {
            const bool isFirstPointSet = counts.PointIdentity == nullptr ||
                                         countedPointSets.insert(counts.PointIdentity).second;
            if (isFirstPointSet && !CheckedAdd(totalPointCount, counts.PointCount)) {
                m_Message = "The total Point count overflowed IGsize.";
                return false;
            }
        }

        if (m_GenerateCellIds && counts.SupportsCellIds) {
            if (!CheckedAdd(totalCellCount, counts.CellCount)) {
                m_Message = "The total Cell count overflowed IGsize.";
                return false;
            }
        }

        plans.push_back(std::move(plan));
    }

    if (m_GeneratePointIds && !RangeFits(m_PointOffset, totalPointCount)) {
        m_Message = "Point Global ID range overflowed iguIndex64.";
        return false;
    }

    if (m_GenerateCellIds && !RangeFits(m_CellOffset, totalCellCount)) {
        m_Message = "Cell Global ID range overflowed iguIndex64.";
        return false;
    }

    if (m_GeneratePointIds && !RangeFitsExactlyInDouble(m_PointOffset, totalPointCount)) {
        m_Message = "Point Global ID range cannot be represented exactly by DoubleArray.";
        return false;
    }

    if (m_GenerateCellIds && !RangeFitsExactlyInDouble(m_CellOffset, totalCellCount)) {
        m_Message = "Cell Global ID range cannot be represented exactly by DoubleArray.";
        return false;
    }

    /*
     * 为一个进程内的多个叶子网格计算局部前缀偏移。
     */
    IGsize pointPrefix = 0;
    IGsize cellPrefix = 0;
    std::unordered_map<const Points*, iguIndex64> sharedPointStarts;

    for (auto& plan: plans) {
        if (m_GeneratePointIds && plan.SupportsPointIds) {
            if (plan.PointIdentity) {
                const auto existingStart = sharedPointStarts.find(plan.PointIdentity);
                if (existingStart != sharedPointStarts.end()) {
                    plan.PointStart = existingStart->second;
                } else {
                    plan.PointStart = m_PointOffset + static_cast<iguIndex64>(pointPrefix);
                    sharedPointStarts.emplace(plan.PointIdentity, plan.PointStart);
                    pointPrefix += plan.PointCount;
                }
            } else {
                plan.PointStart = m_PointOffset + static_cast<iguIndex64>(pointPrefix);
                pointPrefix += plan.PointCount;
            }
        }

        if (m_GenerateCellIds && plan.SupportsCellIds) {
            if (plan.CellCount > 0) {
                plan.CellStart = m_CellOffset + static_cast<iguIndex64>(cellPrefix);
            } else {
                plan.CellStart = m_CellOffset;
            }

            cellPrefix += plan.CellCount;
        }
    }

    /*
     * 第二阶段：检查已有属性。
     * 仍然不提交属性，确保错误不会造成部分输出。
     */
    for (auto& plan: plans) {
        auto* attributes = plan.Object->GetAttributeSet();

        if (!attributes) {
            m_Message = "A leaf DataObject has no AttributeSet.";
            return false;
        }

        if (m_GeneratePointIds && plan.SupportsPointIds) {
            if (!ResolveExistingAttribute(attributes, m_PointArrayName, IG_POINT, plan.PointCount, plan.PointStart,
                                          m_ExistingIdPolicy, plan.ExistingPointAttribute, plan.KeepPointAttribute,
                                          m_Message)) {
                return false;
            }
        }

        if (m_GenerateCellIds && plan.SupportsCellIds) {
            if (!ResolveExistingAttribute(attributes, m_CellArrayName, IG_CELL, plan.CellCount, plan.CellStart,
                                          m_ExistingIdPolicy, plan.ExistingCellAttribute, plan.KeepCellAttribute,
                                          m_Message)) {
                return false;
            }
        }
    }

    this->UpdateProgress(0.2);

    /*
     * 第三阶段：在临时数组中生成全部编号。
     * 数组成功创建前仍然不修改 AttributeSet。
     */
    std::vector<PendingAttribute> pendingAttributes;

    for (const auto& plan: plans) {
        if (m_GeneratePointIds && plan.SupportsPointIds && !plan.KeepPointAttribute) {
            pendingAttributes.push_back(PendingAttribute{plan.Object, plan.Object->GetAttributeSet(),
                                                         plan.ExistingPointAttribute, IG_POINT, nullptr});
        }

        if (m_GenerateCellIds && plan.SupportsCellIds && !plan.KeepCellAttribute) {
            pendingAttributes.push_back(PendingAttribute{plan.Object, plan.Object->GetAttributeSet(),
                                                         plan.ExistingCellAttribute, IG_CELL, nullptr});
        }
    }

    std::size_t pendingIndex = 0;

    try {
        for (const auto& plan: plans) {
            if (m_GeneratePointIds && plan.SupportsPointIds && !plan.KeepPointAttribute) {
                pendingAttributes[pendingIndex].Array =
                        CreateIdArray(m_PointArrayName, plan.PointCount, plan.PointStart);

                ++pendingIndex;

                if (!pendingAttributes.empty()) {
                    this->UpdateProgress(0.2 + 0.7 * static_cast<double>(pendingIndex) /
                                                       static_cast<double>(pendingAttributes.size()));
                }
            }

            if (m_GenerateCellIds && plan.SupportsCellIds && !plan.KeepCellAttribute) {
                pendingAttributes[pendingIndex].Array = CreateIdArray(m_CellArrayName, plan.CellCount, plan.CellStart);

                ++pendingIndex;

                if (!pendingAttributes.empty()) {
                    this->UpdateProgress(0.2 + 0.7 * static_cast<double>(pendingIndex) /
                                                       static_cast<double>(pendingAttributes.size()));
                }
            }
        }
    } catch (const std::bad_alloc&) {
        m_Message = "Memory allocation failed while creating "
                    "Global ID arrays.";
        return false;
    } catch (const std::length_error&) {
        m_Message = "A Global ID array is too large for the current platform.";
        return false;
    }

    /*
     * 第四阶段：所有检查和内存分配成功后，一次性提交属性。
     */
    for (const auto& pending: pendingAttributes) { CommitAttribute(pending); }

    /*
     * Global ID 数组只属于叶子 AttributeSet，不向父容器创建长度语义不明确的数组。
     * 但所有中间容器都需要更新 Modified Time，使依赖父节点时间戳的缓存失效。
     */
    if (!pendingAttributes.empty()) {
        /*
         * CollectLeafObjects() 按根到叶的顺序记录容器。这里必须反向更新，
         * 保证任一父对象的 Modified Time 都晚于其已经更新的子对象。
         */
        for (auto it = hierarchyContainers.rbegin(); it != hierarchyContainers.rend(); ++it) {
            if (*it) { (*it)->Modified(); }
        }
    }

    this->SetOutput(0, output);
    this->UpdateProgress(1.0);

    m_Message = "Global ID generation completed. Point count: " + std::to_string(totalPointCount) +
                ", Cell count: " + std::to_string(totalCellCount) + ".";

    return true;
}

IGAME_NAMESPACE_END
