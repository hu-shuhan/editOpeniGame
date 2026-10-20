#include "iGameDecimatePolylineFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

struct InputPolyline {
    std::vector<igIndex> pointIds;
    IGsize sourceCellId{0};
};

struct VertexNode {
    igIndex pointId{-1};
    int previous{-1};
    int next{-1};
    bool removable{true};
    bool alive{true};
};

// vtkDecimatePolylineFilter 使用 vtkPriorityQueue。对于对称折线，相同优先级
// 元素的堆排序会直接影响最终结果，因此这里复现其排序方式，而不依赖
// std::priority_queue 对相同优先级元素未明确规定的顺序。
class VtkPriorityQueue {
public:
    // 将指定编号及其误差优先级插入最小堆。
    void Insert(double priority, int id) {
        if (id < static_cast<int>(m_Locations.size()) && m_Locations[id] != -1) return;
        if (id >= static_cast<int>(m_Locations.size())) m_Locations.resize(id + 1, -1);

        const int indexAtBottom = ++m_MaxId;
        if (m_MaxId >= static_cast<int>(m_Items.size())) m_Items.resize(m_MaxId + 1);
        m_Items[m_MaxId] = {priority, id};
        int index = indexAtBottom;
        m_Locations[id] = index;
        while (index > 0) {
            const int parent = (index - 1) / 2;
            if (m_Items[index].priority >= m_Items[parent].priority) break;
            Swap(index, parent);
            index = parent;
        }
    }

    // 删除并返回指定堆位置的元素，默认弹出误差最小的堆顶元素。
    int Pop(int location = 0) {
        if (m_MaxId < 0 || location < 0 || location > m_MaxId) return -1;

        const int id = m_Items[location].id;
        m_Items[location] = m_Items[m_MaxId];
        m_Locations[m_Items[location].id] = location;
        m_Locations[id] = -1;
        if (--m_MaxId <= 0) return id;

        const int lastParent = (m_MaxId - 1) / 2;
        for (int index = location; index <= lastParent;) {
            const int left = 2 * index + 1;
            const int child = left == m_MaxId ||
                            m_Items[left].priority < m_Items[left + 1].priority
                    ? left
                    : left + 1;
            if (m_Items[index].priority <= m_Items[child].priority) break;
            Swap(index, child);
            index = child;
        }

        // vtkPriorityQueue 的第二轮调整会重新从最初的删除位置开始，
        // 而不是从向下调整结束的位置开始。
        for (int index = location; index > 0;) {
            const int parent = (index - 1) / 2;
            if (m_Items[index].priority >= m_Items[parent].priority) break;
            Swap(index, parent);
            index = parent;
        }
        return id;
    }

    // 根据元素编号删除其当前位于队列中的记录。
    void DeleteId(int id) {
        if (id >= 0 && id < static_cast<int>(m_Locations.size()) && m_Locations[id] != -1) {
            Pop(m_Locations[id]);
        }
    }

private:
    struct Item {
        double priority;
        int id;
    };

    // 交换两个堆元素，并同步更新编号到堆位置的映射。
    void Swap(int first, int second) {
        std::swap(m_Items[first], m_Items[second]);
        m_Locations[m_Items[first].id] = first;
        m_Locations[m_Items[second].id] = second;
    }

    std::vector<Item> m_Items;
    std::vector<int> m_Locations;
    int m_MaxId{-1};
};

// 根据源数组的数据类型创建一个同类型的空数组。
ArrayObject::Pointer CreateArrayLike(const ArrayObject::Pointer& source) {
    if (!source) return nullptr;
    switch (source->GetArrayType()) {
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

template<typename TArray, typename TValue>
// 按来源下标复制指定类型数组中的完整元素。
void CopyArrayElements(ArrayObject::Pointer& destination,
                       const ArrayObject::Pointer& source,
                       const std::vector<IGsize>& sourceIndices) {
    auto sourceArray = DynamicCast<TArray>(source);
    auto destinationArray = DynamicCast<TArray>(destination);
    if (!sourceArray || !destinationArray) return;

    const int dimension = source->GetDimension();
    const TValue* sourceValues = sourceArray->RawPointer();
    TValue* destinationValues = destinationArray->RawPointer();
    for (IGsize outputId = 0; outputId < static_cast<IGsize>(sourceIndices.size()); ++outputId) {
        const IGsize sourceOffset = sourceIndices[outputId] * static_cast<IGsize>(dimension);
        const IGsize destinationOffset = outputId * static_cast<IGsize>(dimension);
        std::copy_n(sourceValues + sourceOffset, dimension, destinationValues + destinationOffset);
    }
}

// 根据数组类型分派到对应的元素复制实现。
void CopyArrayByIndex(ArrayObject::Pointer& destination,
                      const ArrayObject::Pointer& source,
                      const std::vector<IGsize>& sourceIndices) {
    switch (source->GetArrayType()) {
    case IG_FloatArray:
        CopyArrayElements<FloatArray, float>(destination, source, sourceIndices);
        break;
    case IG_DoubleArray:
        CopyArrayElements<DoubleArray, double>(destination, source, sourceIndices);
        break;
    case IG_IntArray:
        CopyArrayElements<IntArray, int>(destination, source, sourceIndices);
        break;
    case IG_UnsignedIntArray:
        CopyArrayElements<UnsignedIntArray, unsigned int>(destination, source, sourceIndices);
        break;
    case IG_CharArray:
        CopyArrayElements<CharArray, char>(destination, source, sourceIndices);
        break;
    case IG_UnsignedCharArray:
        CopyArrayElements<UnsignedCharArray, unsigned char>(destination, source, sourceIndices);
        break;
    case IG_ShortArray:
        CopyArrayElements<ShortArray, short>(destination, source, sourceIndices);
        break;
    case IG_UnsignedShortArray:
        CopyArrayElements<UnsignedShortArray, unsigned short>(destination, source, sourceIndices);
        break;
    case IG_LongLongArray:
        CopyArrayElements<LongLongArray, long long>(destination, source, sourceIndices);
        break;
    case IG_UnsignedLongLongArray:
        CopyArrayElements<UnsignedLongLongArray, unsigned long long>(destination, source, sourceIndices);
        break;
    default: break;
    }
}

// 检查所有来源下标是否都落在数组的有效元素范围内。
bool IndicesFitArray(const ArrayObject::Pointer& array, const std::vector<IGsize>& sourceIndices) {
    if (!array) return false;
    const IGsize tupleCount = array->GetNumberOfElements();
    for (IGsize sourceId : sourceIndices) {
        if (sourceId >= tupleCount) return false;
    }
    return true;
}

// 判断当前边集合是否完全由表面网格的面拓扑派生而来。
bool EdgesMatchFaceTopology(SurfaceMesh::Pointer surface, CellArray* edges) {
    auto faces = surface ? surface->GetFaces() : nullptr;
    if (!faces || faces->GetNumberOfCells() == 0 || !edges) return false;

    using EdgeKey = std::pair<igIndex, igIndex>;
    // 将边的两个端点按固定顺序排列，便于忽略方向比较边。
    auto makeEdgeKey = [](igIndex first, igIndex second) {
        return first < second ? EdgeKey{first, second} : EdgeKey{second, first};
    };

    std::vector<EdgeKey> faceTopology;
    for (IGsize faceId = 0; faceId < faces->GetNumberOfCells(); ++faceId) {
        const igIndex* ids = nullptr;
        const int size = faces->GetCellIds(faceId, ids);
        if (!ids || size < 3) continue;
        for (int i = 0; i < size; ++i) {
            faceTopology.push_back(makeEdgeKey(ids[i], ids[(i + 1) % size]));
        }
    }
    std::sort(faceTopology.begin(), faceTopology.end());
    faceTopology.erase(std::unique(faceTopology.begin(), faceTopology.end()), faceTopology.end());

    std::vector<EdgeKey> currentEdges;
    currentEdges.reserve(edges->GetNumberOfCells());
    for (IGsize edgeId = 0; edgeId < edges->GetNumberOfCells(); ++edgeId) {
        const igIndex* ids = nullptr;
        const int size = edges->GetCellIds(edgeId, ids);
        if (!ids || size != 2) return false;
        currentEdges.push_back(makeEdgeKey(ids[0], ids[1]));
    }
    std::sort(currentEdges.begin(), currentEdges.end());
    currentEdges.erase(std::unique(currentEdges.begin(), currentEdges.end()), currentEdges.end());
    return currentEdges == faceTopology;
}

// 按输出点和单元的来源下标复制输入网格中的点属性与单元属性。
void CopyAttributes(DataObject::Pointer input,
                    UnstructuredMesh::Pointer output,
                    const std::vector<IGsize>& pointSourceIds,
                    const std::vector<IGsize>& cellSourceIds) {
    auto outputAttributes = AttributeSet::New();
    auto inputAttributes = input ? input->GetAttributeSet() : nullptr;
    if (!inputAttributes) {
        output->SetAttributeSet(outputAttributes);
        return;
    }

    auto allAttributes = inputAttributes->GetAllAttributes();
    if (!allAttributes) {
        output->SetAttributeSet(outputAttributes);
        return;
    }

    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
        auto& attribute = allAttributes->GetElement(i);
        if (attribute.isDeleted || !attribute.pointer) continue;

        const std::vector<IGsize>* sourceIndices = nullptr;
        if (attribute.attachmentType == IG_POINT) {
            sourceIndices = &pointSourceIds;
        } else if (attribute.attachmentType == IG_CELL) {
            sourceIndices = &cellSourceIds;
        } else {
            continue;
        }

        if (!IndicesFitArray(attribute.pointer, *sourceIndices)) continue;
        auto outputArray = CreateArrayLike(attribute.pointer);
        if (!outputArray) continue;

        outputArray->SetName(attribute.pointer->GetName());
        outputArray->SetDimension(attribute.pointer->GetDimension());
        outputArray->Resize(static_cast<IGsize>(sourceIndices->size()));
        CopyArrayByIndex(outputArray, attribute.pointer, *sourceIndices);

        const IGsize attributeId = outputAttributes->AddAttribute(
                attribute.type, attribute.attachmentType, outputArray);
        if (attributeId != static_cast<IGsize>(-1)) {
            outputAttributes->GetAttribute(attributeId).UpdateAllDataRange();
        }
    }
    output->SetAttributeSet(outputAttributes);
}

// 从 SurfaceMesh 的显式折线或 UnstructuredMesh 的线单元中提取折线。
bool CollectInputPolylines(DataObject::Pointer input,
                           PointSet::Pointer& pointSet,
                           std::vector<InputPolyline>& polylines) {
    pointSet = DynamicCast<PointSet>(input);
    if (!pointSet || !pointSet->GetPoints() || pointSet->GetNumberOfPoints() == 0) return false;

    CellArray* cells = nullptr;
    UnstructuredMesh::Pointer mesh;
    if (auto surface = DynamicCast<SurfaceMesh>(input)) {
        cells = surface->GetEdges();
        if (!cells || cells == surface->GetFaces() || EdgesMatchFaceTopology(surface, cells)) {
            return false;
        }
    } else if ((mesh = DynamicCast<UnstructuredMesh>(input))) {
        cells = mesh->GetCells();
        if (!cells || !mesh->GetCellTypes() ||
            mesh->GetCellTypes()->GetNumberOfElements() < mesh->GetNumberOfCells()) {
            return false;
        }
    } else {
        return false;
    }

    polylines.reserve(cells->GetNumberOfCells());
    for (IGsize cellId = 0; cellId < cells->GetNumberOfCells(); ++cellId) {
        if (mesh) {
            const IGenum type = mesh->GetCellType(cellId);
            if (type != IG_LINE && type != IG_POLY_LINE) continue;
        }

        const igIndex* ids = nullptr;
        const int size = cells->GetCellIds(cellId, ids);
        if (ids && size >= 2) {
            polylines.push_back({std::vector<igIndex>(ids, ids + size), cellId});
        }
    }

    if (polylines.empty()) return false;
    const IGsize numberOfPoints = pointSet->GetNumberOfPoints();
    for (const auto& polyline : polylines) {
        for (igIndex pointId : polyline.pointIds) {
            if (pointId < 0 || static_cast<IGsize>(pointId) >= numberOfPoints) return false;
        }
    }
    return true;
}

// 计算候选点到其前后邻点所成无限直线的距离平方。
double ComputeDistanceError(const Points::Pointer& points,
                            const std::vector<VertexNode>& vertices,
                            int vertexIndex) {
    const VertexNode& current = vertices[vertexIndex];
    const Point& origin = points->GetPoint(current.pointId);
    const Point& p1 = points->GetPoint(vertices[current.previous].pointId);
    const Point& p2 = points->GetPoint(vertices[current.next].pointId);

    Vector3d neighbourLine = Vector3d(p1) - Vector3d(p2);
    const double lineLength = neighbourLine.norm();
    // vtkDecimatePolylineDistanceStrategy 将这种退化情况的误差定义为零。
    if (lineLength == 0.0) return 0.0;

    // 保持 vtkLine::DistanceToLine 的运算顺序。先对每个分量做除法再计算
    // 点积，可确保两个候选点误差几乎相同时仍具有一致的优先级顺序。
    neighbourLine /= lineLength;
    const Vector3d fromP1 = Vector3d(origin) - Vector3d(p1);
    const double projection = DotProduct(fromP1, neighbourLine);
    // vtkLine::DistanceToLine(x, p1, p2) 返回点到无限直线的距离平方，
    // 而不是到有限线段的距离平方。
    return fromP1.squaredNorm() - projection * projection;
}

// 计算候选点处两条相邻线段夹角对应的余弦误差。
double ComputeAngleError(const Points::Pointer& points,
                         const std::vector<VertexNode>& vertices,
                         int vertexIndex) {
    const VertexNode& current = vertices[vertexIndex];
    const Vector3d origin(points->GetPoint(current.pointId));
    const Vector3d p1(points->GetPoint(vertices[current.previous].pointId));
    const Vector3d p2(points->GetPoint(vertices[current.next].pointId));
    const Vector3d first = origin - p1;
    const Vector3d second = origin - p2;
    const double normProduct = first.norm() * second.norm();
    if (normProduct <= 0.0) return std::numeric_limits<double>::max();
    return DotProduct(first, second) / normProduct;
}

// 计算候选点及其前后邻点之间自定义字段的最大分量差值。
double ComputeCustomFieldError(const ArrayObject::Pointer& field,
                               const std::vector<VertexNode>& vertices,
                               int vertexIndex) {
    const VertexNode& current = vertices[vertexIndex];
    const igIndex originId = current.pointId;
    const igIndex firstId = vertices[current.previous].pointId;
    const igIndex secondId = vertices[current.next].pointId;
    double error = 0.0;
    for (int component = 0; component < field->GetDimension(); ++component) {
        const double origin = field->GetElementValue(originId, component);
        const double first = field->GetElementValue(firstId, component);
        const double second = field->GetElementValue(secondId, component);
        error = std::max(error, std::abs(origin - first));
        error = std::max(error, std::abs(origin - second));
        error = std::max(error, std::abs(first - second));
    }
    return error;
}

// 根据当前简化策略调用相应的局部误差计算方法。
double ComputeError(const Points::Pointer& points,
                    const std::vector<VertexNode>& vertices,
                    int vertexIndex,
                    DecimatePolylineFilter::DecimationStrategy strategy,
                    const ArrayObject::Pointer& customField) {
    switch (strategy) {
    case DecimatePolylineFilter::DecimationStrategy::Angle:
        return ComputeAngleError(points, vertices, vertexIndex);
    case DecimatePolylineFilter::DecimationStrategy::CustomField:
        return ComputeCustomFieldError(customField, vertices, vertexIndex);
    case DecimatePolylineFilter::DecimationStrategy::Distance:
        return ComputeDistanceError(points, vertices, vertexIndex);
    }
    return std::numeric_limits<double>::max();
}

// 按目标缩减率和最大误差逐步删除一条折线中的低误差点。
std::vector<igIndex> DecimateOnePolyline(const Points::Pointer& points,
                                         const std::vector<igIndex>& inputIds,
                                         double targetReduction,
                                         double maximumError,
                                         DecimatePolylineFilter::DecimationStrategy strategy,
                                         const ArrayObject::Pointer& customField) {
    const int originalSize = static_cast<int>(inputIds.size());
    std::vector<VertexNode> vertices(originalSize);
    for (int i = 0; i < originalSize; ++i) {
        vertices[i].pointId = inputIds[i];
        vertices[i].previous = i > 0 ? i - 1 : -1;
        vertices[i].next = i + 1 < originalSize ? i + 1 : -1;
    }
    vertices.front().removable = false;
    vertices.back().removable = false;

    const bool isLoop = inputIds.front() == inputIds.back();
    VtkPriorityQueue queue;

    // 重新计算指定顶点的误差，并更新它在优先队列中的记录。
    auto updateQueueEntry = [&](int vertexIndex) {
        VertexNode& vertex = vertices[vertexIndex];
        if (!vertex.alive || !vertex.removable || vertex.previous < 0 || vertex.next < 0) return;
        const double error = ComputeError(points, vertices, vertexIndex, strategy, customField);
        queue.DeleteId(vertexIndex);
        if (error <= maximumError) queue.Insert(error, vertexIndex);
    };

    if (originalSize > 2) {
        for (int i = 1; i + 1 < originalSize; ++i) updateQueueEntry(i);
    }

    int currentSize = originalSize;
    const int minimumSize = isLoop ? 3 : 2;
    while (1.0 - static_cast<double>(currentSize) / static_cast<double>(originalSize) < targetReduction &&
           currentSize > minimumSize) {
        const int candidate = queue.Pop();
        if (candidate < 0) break;

        VertexNode& removed = vertices[candidate];
        const int previous = removed.previous;
        const int next = removed.next;
        removed.alive = false;
        vertices[previous].next = next;
        vertices[next].previous = previous;
        --currentSize;

        if (vertices[previous].removable) updateQueueEntry(previous);
        if (vertices[next].removable) updateQueueEntry(next);
    }

    std::vector<igIndex> outputIds;
    outputIds.reserve(currentSize);
    for (int vertexIndex = 0; vertexIndex >= 0; vertexIndex = vertices[vertexIndex].next) {
        if (vertices[vertexIndex].alive) outputIds.push_back(vertices[vertexIndex].pointId);
    }
    return outputIds;
}

} // 匿名命名空间

// 初始化过滤器的输入端口和输出端口数量。
DecimatePolylineFilter::DecimatePolylineFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

// 设置目标缩减率，并将参数限制在零到一之间。
void DecimatePolylineFilter::SetTargetReduction(double value) {
    if (std::isnan(value)) value = 0.0;
    m_TargetReduction = std::max(0.0, std::min(1.0, value));
}

// 设置允许删除点的最大误差，并保证参数为非负有限范围值。
void DecimatePolylineFilter::SetMaximumError(double value) {
    if (std::isnan(value)) value = 0.0;
    m_MaximumError = std::max(0.0, std::min(std::numeric_limits<double>::max(), value));
}

// 检查输入数据中是否存在可供该过滤器处理的显式折线。
bool DecimatePolylineFilter::CanProcessInput(DataObject::Pointer input) {
    PointSet::Pointer pointSet;
    std::vector<InputPolyline> polylines;
    return CollectInputPolylines(input, pointSet, polylines);
}

// 执行折线简化，构造输出拓扑并复制保留点和折线的属性。
bool DecimatePolylineFilter::Execute() {
    // A failed execution must not expose data produced by an earlier run of
    // the same filter instance.
    SetOutput(nullptr);

    if (m_DecimationStrategy != DecimationStrategy::Angle && m_DecimationStrategy != DecimationStrategy::CustomField && m_DecimationStrategy != DecimationStrategy::Distance) {
        return false;
    }

    auto input = GetInput(0);
    PointSet::Pointer inputPointSet;
    std::vector<InputPolyline> inputPolylines;
    if (!CollectInputPolylines(input, inputPointSet, inputPolylines)) return false;

    ArrayObject::Pointer customField;
    if (m_DecimationStrategy == DecimationStrategy::CustomField) {
        auto attributes = input ? input->GetAttributeSet() : nullptr;
        if (!attributes || m_CustomFieldName.empty()) return false;

        // AttributeSet::GetAttribute(name) returns the first matching name and
        // does not distinguish Point Data from Cell Data. Search explicitly so
        // a same-named Cell Data array cannot shadow the requested point field.
        auto allAttributes = attributes->GetAllAttributes();
        if (!allAttributes) return false;
        for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
            const auto& candidate = allAttributes->GetElement(i);
            if (!candidate.isDeleted && candidate.pointer &&
                candidate.attachmentType == IG_POINT &&
                candidate.pointer->GetName() == m_CustomFieldName) {
                customField = candidate.pointer;
                break;
            }
        }
        if (!customField || !CreateArrayLike(customField) ||
            customField->GetNumberOfElements() < inputPointSet->GetNumberOfPoints()) {
            return false;
        }
    }

    auto output = UnstructuredMesh::New();
    auto outputPoints = Points::New();
    auto outputCells = CellArray::New();
    auto outputTypes = UnsignedIntArray::New();
    outputTypes->SetDimension(1);

    std::unordered_map<igIndex, igIndex> pointIdMap;
    std::vector<IGsize> pointSourceIds;
    std::vector<IGsize> cellSourceIds;
    pointIdMap.reserve(inputPointSet->GetNumberOfPoints());
    cellSourceIds.reserve(inputPolylines.size());

    for (const auto& inputPolyline : inputPolylines) {
        const auto retainedIds = DecimateOnePolyline(
                inputPointSet->GetPoints(), inputPolyline.pointIds, m_TargetReduction, m_MaximumError,
                m_DecimationStrategy, customField);
        if (retainedIds.size() < 2) continue;

        std::vector<igIndex> outputIds;
        outputIds.reserve(retainedIds.size());
        for (igIndex sourcePointId : retainedIds) {
            auto found = pointIdMap.find(sourcePointId);
            if (found == pointIdMap.end()) {
                const igIndex outputPointId = static_cast<igIndex>(outputPoints->GetNumberOfPoints());
                outputPoints->AddPoint(inputPointSet->GetPoint(sourcePointId));
                pointIdMap.emplace(sourcePointId, outputPointId);
                pointSourceIds.push_back(static_cast<IGsize>(sourcePointId));
                outputIds.push_back(outputPointId);
            } else {
                outputIds.push_back(found->second);
            }
        }

        outputCells->AddCellIds(outputIds.data(), static_cast<int>(outputIds.size()));
        outputTypes->AddValue(outputIds.size() == 2 ? IG_LINE : IG_POLY_LINE);
        cellSourceIds.push_back(inputPolyline.sourceCellId);
    }

    if (outputCells->GetNumberOfCells() == 0) return false;
    output->SetPoints(outputPoints);
    output->SetCells(outputCells, outputTypes);
    if (input && !input->GetName().empty()) output->SetName(input->GetName() + "_decimate_polyline");
    CopyAttributes(input, output, pointSourceIds, cellSourceIds);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
