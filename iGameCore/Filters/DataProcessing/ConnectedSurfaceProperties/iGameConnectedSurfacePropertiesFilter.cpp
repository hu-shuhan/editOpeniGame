#include "iGameConnectedSurfacePropertiesFilter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

struct EdgeKey {
    igIndex first{};
    igIndex second{};

    bool operator==(const EdgeKey& other) const {
        return first == other.first && second == other.second;
    }
};

struct EdgeKeyHasher {
    std::size_t operator()(const EdgeKey& edge) const {
        return std::hash<igIndex>{}(edge.first) ^ (std::hash<igIndex>{}(edge.second) << 1);
    }
};

struct EdgeUse {
    igIndex faceId{};
    bool forward{};
};

using FacePointIds = std::vector<igIndex>;
using EdgeMap = std::unordered_map<EdgeKey, std::vector<EdgeUse>, EdgeKeyHasher>;

EdgeKey MakeEdgeKey(igIndex first, igIndex second) {
    return first < second ? EdgeKey{first, second} : EdgeKey{second, first};
}

bool IsForwardEdge(igIndex first, igIndex second) { return first < second; }

Vector3d ToDoublePoint(const Point& point) {
    return {static_cast<double>(point[0]), static_cast<double>(point[1]), static_cast<double>(point[2])};
}

double Determinant(const Vector3d& first, const Vector3d& second, const Vector3d& third) {
    return first[0] * (second[1] * third[2] - second[2] * third[1])
            - first[1] * (second[0] * third[2] - second[2] * third[0])
            + first[2] * (second[0] * third[1] - second[1] * third[0]);
}

Vector3d PolygonNormal(const SurfaceMesh::Pointer& mesh, const FacePointIds& pointIds) {
    Vector3d normal(0.0, 0.0, 0.0);
    Vector3d firstEdge;
    std::size_t commonPointId = 0;
    std::size_t pointId = 0;
    for (; pointId + 2 < pointIds.size(); ++pointId) {
        const Vector3d common = ToDoublePoint(mesh->GetPoint(pointIds[pointId]));
        firstEdge = ToDoublePoint(mesh->GetPoint(pointIds[pointId + 1])) - common;
        if (firstEdge.squaredNorm() > 0.0) {
            commonPointId = pointId;
            pointId += 2;
            break;
        }
    }
    if (pointId >= pointIds.size()) return normal;

    const Vector3d common = ToDoublePoint(mesh->GetPoint(pointIds[commonPointId]));
    for (; pointId < pointIds.size(); ++pointId) {
        const Vector3d nextEdge = ToDoublePoint(mesh->GetPoint(pointIds[pointId])) - common;
        const Vector3d cross = firstEdge.cross(nextEdge);
        normal += cross;
        firstEdge = nextEdge;
    }
    const double length = normal.length();
    return length == 0.0 ? normal : normal / length;
}

double PolygonArea(const SurfaceMesh::Pointer& mesh, const FacePointIds& pointIds) {
    const Vector3d normal = PolygonNormal(mesh, pointIds);
    const double absoluteNormal[3]{std::abs(normal[0]), std::abs(normal[1]), std::abs(normal[2])};
    const int projectionAxis = absoluteNormal[0] > absoluteNormal[1]
            ? (absoluteNormal[0] > absoluteNormal[2] ? 0 : 2)
            : (absoluteNormal[1] > absoluteNormal[2] ? 1 : 2);
    if (absoluteNormal[projectionAxis] == 0.0) return 0.0;

    double projectedArea = 0.0;
    for (std::size_t index = 0; index < pointIds.size(); ++index) {
        const Vector3d previous = ToDoublePoint(mesh->GetPoint(pointIds[index]));
        const Vector3d current = ToDoublePoint(mesh->GetPoint(
                pointIds[(index + 1) % pointIds.size()]));
        const Vector3d next = ToDoublePoint(mesh->GetPoint(
                pointIds[(index + 2) % pointIds.size()]));
        if (projectionAxis == 0) projectedArea += current[1] * (next[2] - previous[2]);
        else if (projectionAxis == 1) projectedArea += current[0] * (next[2] - previous[2]);
        else projectedArea += current[0] * (next[1] - previous[1]);
    }
    return std::abs(projectedArea / (2.0 * absoluteNormal[projectionAxis]));
}

bool QuadUsesFirstDiagonal(const SurfaceMesh::Pointer& mesh, const FacePointIds& pointIds) {
    const Vector3d point0 = ToDoublePoint(mesh->GetPoint(pointIds[0]));
    const Vector3d point1 = ToDoublePoint(mesh->GetPoint(pointIds[1]));
    const Vector3d point2 = ToDoublePoint(mesh->GetPoint(pointIds[2]));
    const Vector3d point3 = ToDoublePoint(mesh->GetPoint(pointIds[3]));
    const Vector3d firstDiagonal = point2 - point0;
    const Vector3d secondDiagonal = point3 - point1;
    bool useFirst = firstDiagonal.squaredNorm() < secondDiagonal.squaredNorm();

    Vector3d firstSide;
    Vector3d secondSide;
    Vector3d firstNormal;
    Vector3d secondNormal;
    if (useFirst) {
        firstSide = point1 - point0;
        secondSide = point3 - point0;
        firstNormal = firstSide.cross(firstDiagonal);
        secondNormal = firstDiagonal.cross(secondSide);
    } else {
        firstSide = point2 - point1;
        secondSide = point0 - point1;
        firstNormal = firstSide.cross(secondDiagonal);
        secondNormal = secondDiagonal.cross(secondSide);
    }
    Vector3d normal = firstNormal + secondNormal;
    const double normalLength = normal.length();
    if (normalLength == 0.0) return useFirst;
    normal /= normalLength;

    const bool concave = firstNormal.dot(normal) < 0.0 || secondNormal.dot(normal) < 0.0;
    if (!concave) return useFirst;

    useFirst = !useFirst;
    if (useFirst) {
        firstSide = point1 - point0;
        secondSide = point3 - point0;
        firstNormal = firstSide.cross(firstDiagonal);
        secondNormal = firstDiagonal.cross(secondSide);
    } else {
        firstSide = point2 - point1;
        secondSide = point0 - point1;
        firstNormal = firstSide.cross(secondDiagonal);
        secondNormal = secondDiagonal.cross(secondSide);
    }
    if (firstNormal.dot(normal) > 0.0 && secondNormal.dot(normal) > 0.0) return useFirst;

    // vtkPolygon falls back to its ear-cut/quad path for a poorly shaped quad.
    // That path retains the 0-2 diagonal for these non-planar cases.
    return true;
}

struct EarVertex {
    std::size_t id{};
    Vector3d point;
    double measure{};
    EarVertex* previous{};
    EarVertex* next{};
    bool active{true};
    unsigned int version{};
};

double PlaneValue(const Vector3d& normal, const Vector3d& origin, const Vector3d& point) {
    return normal.dot(point - origin);
}

bool SegmentsIntersect(const Vector3d& firstStart, const Vector3d& firstEnd,
                       const Vector3d& secondStart, const Vector3d& secondEnd,
                       double tolerance) {
    const Vector3d firstDirection = firstEnd - firstStart;
    const Vector3d secondDirection = secondEnd - secondStart;
    const Vector3d offset = firstStart - secondStart;
    const double firstLength2 = firstDirection.squaredNorm();
    const double secondLength2 = secondDirection.squaredNorm();
    const double directionDot = firstDirection.dot(secondDirection);
    const double denominator = firstLength2 * secondLength2 - directionDot * directionDot;
    if (std::abs(denominator) <= tolerance) return false;

    const double firstParameter = (directionDot * secondDirection.dot(offset)
                                   - secondLength2 * firstDirection.dot(offset)) / denominator;
    const double secondParameter = (firstLength2 * secondDirection.dot(offset)
                                    - directionDot * firstDirection.dot(offset)) / denominator;
    return firstParameter >= 0.0 && firstParameter <= 1.0
            && secondParameter >= 0.0 && secondParameter <= 1.0;
}

double EarMeasure(EarVertex* vertex, const Vector3d& normal) {
    const Vector3d first = vertex->point - vertex->previous->point;
    const Vector3d second = vertex->next->point - vertex->point;
    const Vector3d third = vertex->previous->point - vertex->next->point;
    const double area = first.cross(second).dot(normal);
    if (area <= 0.0) return vertex->measure = -1.0;
    const double perimeter = first.length() + second.length() + third.length();
    return vertex->measure = perimeter * perimeter / area;
}

bool CanRemoveEar(EarVertex* current, int vertexCount, const Vector3d& normal,
                  double tolerance) {
    if (vertexCount <= 3) return true;
    EarVertex* previous = current->previous;
    EarVertex* next = current->next;
    Vector3d splitNormal = (next->point - previous->point).cross(normal);
    const double splitLength = splitNormal.length();
    if (splitLength == 0.0) return false;
    splitNormal /= splitLength;

    int valueSign = 0;
    bool hasNegative = false;
    double value = PlaneValue(splitNormal, previous->point, next->next->point);
    valueSign = value > tolerance ? 1 : (value < -tolerance ? -1 : 0);
    hasNegative = valueSign < 0;
    for (EarVertex* vertex = next->next->next; vertex != previous; vertex = vertex->next) {
        value = PlaneValue(splitNormal, previous->point, vertex->point);
        const int nextSign = value > tolerance ? 1 : (value < -tolerance ? -1 : 0);
        if (nextSign != valueSign) {
            hasNegative = hasNegative || nextSign < 0;
            if (SegmentsIntersect(previous->point, next->point,
                                  vertex->point, vertex->previous->point, tolerance)) {
                return false;
            }
            valueSign = nextSign;
        }
    }
    return hasNegative;
}

bool TriangulatePolygon(const SurfaceMesh::Pointer& mesh, const FacePointIds& pointIds,
                        std::vector<std::array<std::size_t, 3>>& triangles) {
    std::vector<EarVertex> vertices(pointIds.size());
    Vector3d minimum(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                     std::numeric_limits<double>::max());
    Vector3d maximum(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
                     std::numeric_limits<double>::lowest());
    for (std::size_t index = 0; index < pointIds.size(); ++index) {
        auto& vertex = vertices[index];
        vertex.id = index;
        vertex.point = ToDoublePoint(mesh->GetPoint(pointIds[index]));
        vertex.previous = &vertices[index == 0 ? pointIds.size() - 1 : index - 1];
        vertex.next = &vertices[index + 1 == pointIds.size() ? 0 : index + 1];
        for (int axis = 0; axis < 3; ++axis) {
            minimum[axis] = std::min(minimum[axis], vertex.point[axis]);
            maximum[axis] = std::max(maximum[axis], vertex.point[axis]);
        }
    }
    const double tolerance = 1.0e-6 * (maximum - minimum).length();
    const double tolerance2 = tolerance * tolerance;
    EarVertex* head = vertices.data();
    int vertexCount = static_cast<int>(vertices.size());
    for (int checked = 0; checked < static_cast<int>(vertices.size()); ++checked) {
        EarVertex* next = head->next;
        if ((head->point - next->point).squaredNorm() < tolerance2) {
            next->active = false;
            head->next = next->next;
            next->next->previous = head;
            --vertexCount;
        } else {
            head = next;
        }
    }
    if (vertexCount < 3) return false;

    Vector3d normal(0.0, 0.0, 0.0);
    const Vector3d anchor = head->point;
    for (EarVertex* vertex = head->next; vertex->next != head; vertex = vertex->next)
        normal += (vertex->point - anchor).cross(vertex->next->point - anchor);
    const double normalLength = normal.length();
    if (normalLength == 0.0) return false;
    normal /= normalLength;

    struct QueueEntry {
        double measure;
        std::size_t id;
        unsigned int version;
        bool operator>(const QueueEntry& other) const {
            if (measure != other.measure) return measure > other.measure;
            return id > other.id;
        }
    };
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;
    const auto update = [&queue, &normal](EarVertex* vertex) {
        ++vertex->version;
        if (EarMeasure(vertex, normal) > 0.0)
            queue.push({vertex->measure, vertex->id, vertex->version});
    };
    EarVertex* vertex = head;
    do {
        update(vertex);
        vertex = vertex->next;
    } while (vertex != head);

    while (vertexCount > 2 && !queue.empty()) {
        const QueueEntry entry = queue.top();
        queue.pop();
        EarVertex* ear = &vertices[entry.id];
        if (!ear->active || ear->version != entry.version) continue;
        if (!CanRemoveEar(ear, vertexCount, normal, tolerance)) continue;
        triangles.push_back({ear->id, ear->next->id, ear->previous->id});
        ear->active = false;
        --vertexCount;
        if (vertexCount < 3) break;
        ear->previous->next = ear->next;
        ear->next->previous = ear->previous;
        update(ear->previous);
        update(ear->next);
    }
    return vertexCount == 2;
}

double PolygonVolumeAndCentroidContribution(const SurfaceMesh::Pointer& mesh,
                                            const FacePointIds& pointIds,
                                            const Vector3d& referencePoint,
                                            bool orientation,
                                            Vector3d& centroidContribution) {
    double polygonVolume = 0.0;
    centroidContribution.setZero();
    const double orientationSign = orientation ? 1.0 : -1.0;
    std::vector<std::array<std::size_t, 3>> triangles;
    if (pointIds.size() == 3) {
        triangles.push_back({0, 1, 2});
    } else if (pointIds.size() == 4 && !QuadUsesFirstDiagonal(mesh, pointIds)) {
        triangles.push_back({0, 1, 3});
        triangles.push_back({1, 2, 3});
    } else if (pointIds.size() > 4) {
        TriangulatePolygon(mesh, pointIds, triangles);
    } else {
        for (std::size_t index = 1; index + 1 < pointIds.size(); ++index)
            triangles.push_back({0, index, index + 1});
    }
    for (const auto& triangle : triangles) {
        const Vector3d first = ToDoublePoint(mesh->GetPoint(pointIds[triangle[0]]));
        const Vector3d second = ToDoublePoint(mesh->GetPoint(pointIds[triangle[1]]));
        const Vector3d third = ToDoublePoint(mesh->GetPoint(pointIds[triangle[2]]));
        const double tetrahedronVolume = orientationSign
                * Determinant(first - referencePoint, second - referencePoint, third - referencePoint) / 6.0;
        polygonVolume += tetrahedronVolume;
        centroidContribution += (first + second + third + referencePoint) * (tetrahedronVolume / 4.0);
    }
    return polygonVolume;
}

template<typename TArray>
bool ReadObjectIds(TArray* array, IGsize faceCount, std::vector<long long>& objectIds) {
    if (!array || array->GetDimension() != 1 || array->GetNumberOfElements() != faceCount) return false;
    objectIds.resize(faceCount);
    for (IGsize faceId = 0; faceId < faceCount; ++faceId) {
        const auto value = array->RawPointer(faceId)[0];
        if constexpr (std::is_unsigned_v<std::remove_cv_t<decltype(value)>>) {
            if (value > static_cast<decltype(value)>(std::numeric_limits<long long>::max())) return false;
        } else if (value < 0) {
            return false;
        }
        objectIds[faceId] = static_cast<long long>(value);
    }
    return true;
}

bool ReadSuppliedObjectIds(const SurfaceMesh::Pointer& mesh, const std::string& name,
                           std::vector<long long>& objectIds) {
    auto* attributes = mesh->GetAttributeSet();
    auto* array = attributes->GetArrayPointer(IG_SCALAR, IG_CELL, name);
    if (auto* typed = dynamic_cast<LongLongArray*>(array))
        return ReadObjectIds(typed, mesh->GetNumberOfFaces(), objectIds);
    if (auto* typed = dynamic_cast<UnsignedLongLongArray*>(array))
        return ReadObjectIds(typed, mesh->GetNumberOfFaces(), objectIds);
    return false;
}

void AddOrReplaceCellScalar(AttributeSet* attributes, const ArrayObject::Pointer& array) {
    for (IGsize index = 0; index < attributes->GetNumberOfAttributes(); ++index) {
        auto& attribute = attributes->GetAttribute(index);
        if (!attribute.IsNone() && !attribute.IsDeleted() && attribute.pointer
            && attribute.attachmentType == IG_CELL && attribute.pointer->GetName() == array->GetName()) {
            attribute.SetPointer(array);
            attribute.SetType(IG_SCALAR);
            attribute.SetDataRange(nullptr);
            return;
        }
    }
    attributes->AddScalar(IG_CELL, array);
}

void PreserveUncopiedAttributes(const SurfaceMesh::Pointer& input,
                                const SurfaceMesh::Pointer& output) {
    auto* source = input->GetAttributeSet();
    auto* target = output->GetAttributeSet();
    const IGsize count = std::min(source->GetNumberOfAttributes(),
                                  target->GetNumberOfAttributes());
    for (IGsize index = 0; index < count; ++index) {
        const auto& sourceAttribute = source->GetAttribute(index);
        auto& targetAttribute = target->GetAttribute(index);
        if (!sourceAttribute.IsNone() && targetAttribute.IsNone()) {
            targetAttribute = sourceAttribute;
        }
    }
}

} // namespace

ConnectedSurfacePropertiesFilter::ConnectedSurfacePropertiesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ConnectedSurfacePropertiesFilter::Execute() {
    m_UsedSuppliedObjectIds = false;
    m_NumberOfObjects = 0;
    m_AllValid = false;
    m_TotalArea = 0.0;
    m_TotalVolume = 0.0;
    m_LastError.clear();
    m_ObjectIds = nullptr;
    m_Areas = nullptr;
    m_Volumes = nullptr;
    m_ObjectValidity = nullptr;
    m_ObjectAreas = nullptr;
    m_ObjectVolumes = nullptr;
    m_ObjectCentroids = nullptr;

    const auto fail = [this](std::string message) {
        m_LastError = std::move(message);
        return false;
    };

    const auto input = DynamicCast<SurfaceMesh>(GetInput(0));
    if (input.IsNull()) {
        return fail("Connected Surface Properties 仅支持 SurfaceMesh（对应 ParaView PolyData 的多边形表面）。"
                    "体网格、结构网格、点集和复合数据请先提取表面。");
    }
    const IGsize faceCount = input->GetNumberOfFaces();
    const IGsize pointCount = input->GetNumberOfPoints();
    if (faceCount == 0 || pointCount == 0) return fail("输入表面不包含可计算的多边形面。");

    std::vector<FacePointIds> faces(faceCount);
    EdgeMap edgeUses;
    Vector3d boundsMinimum(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                           std::numeric_limits<double>::max());
    Vector3d boundsMaximum(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
                           std::numeric_limits<double>::lowest());
    for (IGsize pointId = 0; pointId < pointCount; ++pointId) {
        const Vector3d point = ToDoublePoint(input->GetPoint(pointId));
        for (int axis = 0; axis < 3; ++axis) {
            boundsMinimum[axis] = std::min(boundsMinimum[axis], point[axis]);
            boundsMaximum[axis] = std::max(boundsMaximum[axis], point[axis]);
        }
    }
    for (IGsize faceId = 0; faceId < faceCount; ++faceId) {
        const igIndex* pointIds = nullptr;
        const int numberOfPoints = input->GetFaces()->GetCellIds(faceId, pointIds);
        if (!pointIds || numberOfPoints < 3) {
            return fail("输入包含少于 3 个点的退化面，无法计算表面属性。");
        }
        faces[faceId].assign(pointIds, pointIds + numberOfPoints);
        for (int localPointId = 0; localPointId < numberOfPoints; ++localPointId) {
            const igIndex first = pointIds[localPointId];
            const igIndex second = pointIds[(localPointId + 1) % numberOfPoints];
            if (first < 0 || second < 0 || static_cast<IGsize>(first) >= pointCount
                || static_cast<IGsize>(second) >= pointCount || first == second) {
                return fail("输入包含无效的面连接关系，无法计算表面属性。");
            }
            edgeUses[MakeEdgeKey(first, second)].push_back({static_cast<igIndex>(faceId),
                                                            IsForwardEdge(first, second)});
        }
    }

    std::vector<long long> objectIds(faceCount, -1);
    std::vector<unsigned char> orientations(faceCount, 1);
    std::vector<int> objectValidity;
    if (m_SkipObjectIdentification
        && ReadSuppliedObjectIds(input, m_ObjectIdsArrayName, objectIds)) {
        const long long maximumId = *std::max_element(objectIds.begin(), objectIds.end());
        if (maximumId > static_cast<long long>(std::numeric_limits<int>::max() - 1)) {
            return fail("指定的 ObjectIds 包含过大的对象编号，无法分配结果数组。");
        }
        m_NumberOfObjects = static_cast<int>(maximumId + 1);
        objectValidity.assign(m_NumberOfObjects, 1);
        m_UsedSuppliedObjectIds = true;
    } else {
        std::queue<igIndex> pendingFaces;
        for (IGsize startFaceId = 0; startFaceId < faceCount; ++startFaceId) {
            if (objectIds[startFaceId] >= 0) continue;
            const int objectId = m_NumberOfObjects++;
            objectValidity.push_back(1);
            objectIds[startFaceId] = objectId;
            orientations[startFaceId] = 1;
            pendingFaces.push(static_cast<igIndex>(startFaceId));

            while (!pendingFaces.empty()) {
                const igIndex faceId = pendingFaces.front();
                pendingFaces.pop();
                const auto& face = faces[faceId];
                for (std::size_t localPointId = 0; localPointId < face.size(); ++localPointId) {
                    const igIndex first = face[localPointId];
                    const igIndex second = face[(localPointId + 1) % face.size()];
                    const bool currentForward = IsForwardEdge(first, second);
                    const auto& uses = edgeUses.at(MakeEdgeKey(first, second));
                    if (uses.size() != 2) objectValidity[objectId] = 0;
                    for (const auto& use : uses) {
                        if (use.faceId == faceId) continue;
                        const unsigned char expectedOrientation = static_cast<unsigned char>(
                                orientations[faceId] ^ (currentForward == use.forward));
                        if (objectIds[use.faceId] < 0) {
                            objectIds[use.faceId] = objectId;
                            orientations[use.faceId] = expectedOrientation;
                            pendingFaces.push(use.faceId);
                        } else if (objectIds[use.faceId] == objectId
                                   && orientations[use.faceId] != expectedOrientation) {
                            objectValidity[objectId] = 0;
                        }
                    }
                }
            }
        }
    }
    if (m_NumberOfObjects <= 0) return fail("未识别到可计算的连通表面对象。");

    const Vector3d referencePoint = (boundsMinimum + boundsMaximum) / 2.0;
    std::vector<double> faceAreas(faceCount, 0.0);
    std::vector<double> faceVolumes(faceCount, 0.0);
    std::vector<double> objectAreas(m_NumberOfObjects, 0.0);
    std::vector<double> objectVolumes(m_NumberOfObjects, 0.0);
    std::vector<Vector3d> objectCentroidSums(m_NumberOfObjects, Vector3d(0.0, 0.0, 0.0));

    for (IGsize faceId = 0; faceId < faceCount; ++faceId) {
        const int objectId = static_cast<int>(objectIds[faceId]);
        if (objectId < 0 || objectId >= m_NumberOfObjects) {
            return fail("ObjectIds 中存在不连续或越界的对象编号。");
        }
        faceAreas[faceId] = PolygonArea(input, faces[faceId]);
        Vector3d centroidContribution(0.0, 0.0, 0.0);
        faceVolumes[faceId] = PolygonVolumeAndCentroidContribution(
                input, faces[faceId], referencePoint, orientations[faceId] != 0, centroidContribution);
        objectAreas[objectId] += faceAreas[faceId];
        objectVolumes[objectId] += faceVolumes[faceId];
        objectCentroidSums[objectId] += centroidContribution;
    }

    std::vector<Vector3d> objectCentroids(m_NumberOfObjects);
    const double volumeEpsilon = std::numeric_limits<double>::epsilon();
    m_AllValid = true;
    for (int objectId = 0; objectId < m_NumberOfObjects; ++objectId) {
        m_TotalArea += objectAreas[objectId];
        m_AllValid = m_AllValid && objectValidity[objectId] != 0;
        if (std::abs(objectVolumes[objectId]) > volumeEpsilon) {
            objectCentroids[objectId] = objectCentroidSums[objectId] / objectVolumes[objectId];
        } else {
            const double nan = std::numeric_limits<double>::quiet_NaN();
            objectCentroids[objectId] = Vector3d(nan, nan, nan);
        }
        if (objectValidity[objectId]) {
            objectVolumes[objectId] = std::abs(objectVolumes[objectId]);
            m_TotalVolume += objectVolumes[objectId];
        }
    }

    m_ObjectIds = LongLongArray::New();
    m_ObjectIds->SetName(ObjectIdsArrayName());
    m_ObjectIds->Reserve(faceCount);
    m_Areas = DoubleArray::New();
    m_Areas->SetName(AreasArrayName());
    m_Areas->Reserve(faceCount);
    m_Volumes = DoubleArray::New();
    m_Volumes->SetName(VolumesArrayName());
    m_Volumes->Reserve(faceCount);
    for (IGsize faceId = 0; faceId < faceCount; ++faceId) {
        m_ObjectIds->AddValue(objectIds[faceId]);
        m_Areas->AddValue(faceAreas[faceId]);
        m_Volumes->AddValue(faceVolumes[faceId]);
    }

    m_ObjectValidity = IntArray::New();
    m_ObjectValidity->SetName(ObjectValidityArrayName());
    m_ObjectAreas = DoubleArray::New();
    m_ObjectAreas->SetName(ObjectAreasArrayName());
    m_ObjectVolumes = DoubleArray::New();
    m_ObjectVolumes->SetName(ObjectVolumesArrayName());
    m_ObjectCentroids = DoubleArray::New();
    m_ObjectCentroids->SetName(ObjectCentroidsArrayName());
    m_ObjectCentroids->SetDimension(3);
    for (int objectId = 0; objectId < m_NumberOfObjects; ++objectId) {
        m_ObjectValidity->AddValue(objectValidity[objectId]);
        m_ObjectAreas->AddValue(objectAreas[objectId]);
        m_ObjectVolumes->AddValue(objectVolumes[objectId]);
        double centroid[3]{objectCentroids[objectId][0], objectCentroids[objectId][1],
                           objectCentroids[objectId][2]};
        m_ObjectCentroids->AddElement(centroid);
    }

    auto output = SurfaceMesh::New();
    if (!output->DeepCopy(input)) return fail("无法复制输入表面。");
    PreserveUncopiedAttributes(input, output);
    output->SetName(input->GetName() + "_ConnectedSurfaceProperties");
    AddOrReplaceCellScalar(output->GetAttributeSet(), m_ObjectIds);
    AddOrReplaceCellScalar(output->GetAttributeSet(), m_Areas);
    AddOrReplaceCellScalar(output->GetAttributeSet(), m_Volumes);

    auto* metadata = output->GetMetadata();
    metadata->AddInt("NumberOfObjects", m_NumberOfObjects);
    metadata->AddInt("AllValid", m_AllValid ? 1 : 0);
    metadata->AddDouble("TotalArea", m_TotalArea);
    metadata->AddDouble("TotalVolume", m_TotalVolume);
    metadata->AddIntArray(ObjectValidityArrayName(), m_ObjectValidity);
    metadata->AddDoubleArray(ObjectAreasArrayName(), m_ObjectAreas);
    metadata->AddDoubleArray(ObjectVolumesArrayName(), m_ObjectVolumes);
    metadata->AddDoubleArray(ObjectCentroidsArrayName(), m_ObjectCentroids);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
