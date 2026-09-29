#include "iGameFeatureEdgeRegionFilter.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN
namespace {

constexpr double NORMAL_EPSILON = 1e-30;
constexpr double NORMALS_FEATURE_ANGLE = 30.0;

using FacePoints = std::vector<std::vector<igIndex>>;
using EdgeKey = std::pair<igIndex, igIndex>;
using EdgeFaces = std::map<EdgeKey, std::vector<igIndex>>;

EdgeKey makeEdgeKey(igIndex first, igIndex second) {
    return first < second ? EdgeKey(first, second) : EdgeKey(second, first);
}

// 与 vtkPolygon::ComputeNormal 相同，使用 Newell 方法计算任意多边形的单位法向。
Vector3f getNormal(const SurfaceMesh::Pointer& mesh, const std::vector<igIndex>& pointIds) {
    if (mesh == nullptr || pointIds.size() < 3) { return Vector3f(0.0f, 0.0f, 0.0f); }

    double nx = 0.0;
    double ny = 0.0;
    double nz = 0.0;
    for (IGsize i = 0; i < pointIds.size(); ++i) {
        const Point& current = mesh->GetPoint(pointIds[i]);
        const Point& next = mesh->GetPoint(pointIds[(i + 1) % pointIds.size()]);
        nx += static_cast<double>(current[1] - next[1]) *
                static_cast<double>(current[2] + next[2]);
        ny += static_cast<double>(current[2] - next[2]) *
                static_cast<double>(current[0] + next[0]);
        nz += static_cast<double>(current[0] - next[0]) *
                static_cast<double>(current[1] + next[1]);
    }

    const double length = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (length <= NORMAL_EPSILON) { return Vector3f(0.0f, 0.0f, 0.0f); }
    return Vector3f(static_cast<float>(nx / length), static_cast<float>(ny / length),
                    static_cast<float>(nz / length));
}

FacePoints copyFacePoints(const SurfaceMesh::Pointer& mesh) {
    FacePoints result(static_cast<size_t>(mesh->GetNumberOfFaces()));
    for (igIndex faceId = 0; faceId < mesh->GetNumberOfFaces(); ++faceId) {
        const igIndex* ids = nullptr;
        const int size = mesh->GetFaces()->GetCellIds(faceId, ids);
        result[faceId].assign(ids, ids + size);
    }
    return result;
}

EdgeFaces buildEdgeFaces(const FacePoints& faces) {
    EdgeFaces result;
    for (igIndex faceId = 0; faceId < static_cast<igIndex>(faces.size()); ++faceId) {
        const auto& face = faces[faceId];
        for (IGsize i = 0; i < face.size(); ++i) {
            result[makeEdgeKey(face[i], face[(i + 1) % face.size()])].push_back(faceId);
        }
    }
    return result;
}

std::vector<std::vector<igIndex>> buildPointFaces(const FacePoints& faces, IGsize numberOfPoints) {
    std::vector<std::vector<igIndex>> result(static_cast<size_t>(numberOfPoints));
    for (igIndex faceId = 0; faceId < static_cast<igIndex>(faces.size()); ++faceId) {
        for (const igIndex pointId : faces[faceId]) { result[pointId].push_back(faceId); }
    }
    return result;
}

// 复现 vtkOrientPolyData 的 Consistency=On、NonManifoldTraversal=On。
void orientFacesConsistently(FacePoints& faces, const EdgeFaces& edgeFaces) {
    std::vector<unsigned char> visited(faces.size(), 0);
    std::vector<igIndex> wave;
    std::vector<igIndex> nextWave;

    for (igIndex seed = 0; seed < static_cast<igIndex>(faces.size()); ++seed) {
        if (visited[seed]) { continue; }
        visited[seed] = 1;
        wave.push_back(seed);

        while (!wave.empty()) {
            nextWave.clear();
            for (const igIndex faceId : wave) {
                const auto current = faces[faceId];
                if (current.size() < 3) { continue; }

                for (IGsize edge = 0; edge < current.size(); ++edge) {
                    const igIndex first = current[edge];
                    const igIndex second = current[(edge + 1) % current.size()];
                    const auto edgeIt = edgeFaces.find(makeEdgeKey(first, second));
                    if (edgeIt == edgeFaces.end()) { continue; }

                    for (const igIndex neighborId : edgeIt->second) {
                        if (neighborId == faceId || visited[neighborId]) { continue; }
                        auto& neighbor = faces[neighborId];
                        const auto secondIt = std::find(neighbor.begin(), neighbor.end(), second);
                        if (secondIt == neighbor.end()) { continue; }
                        const IGsize secondPosition =
                                static_cast<IGsize>(std::distance(neighbor.begin(), secondIt));
                        if (neighbor[(secondPosition + 1) % neighbor.size()] != first) {
                            std::reverse(neighbor.begin(), neighbor.end());
                        }
                        visited[neighborId] = 1;
                        nextWave.push_back(neighborId);
                    }
                }
            }
            wave.swap(nextWave);
        }
    }
}

struct SplitResult {
    FacePoints Faces;
    std::vector<igIndex> NewToOldPoint;
};

// 逐点复现 vtkSplitSharpEdgesPolyData::MarkAndSplitFunctor。
SplitResult splitSharpEdges(const FacePoints& orientedFaces,
                            const EdgeFaces& originalEdgeFaces,
                            const std::vector<std::vector<igIndex>>& pointFaces,
                            const std::vector<Vector3f>& normals,
                            IGsize numberOfPoints) {
    struct Replacement {
        igIndex FaceId;
        int Region;
    };

    const double cosAngle = std::cos(NORMALS_FEATURE_ANGLE / 180.0 * M_PI);
    std::vector<std::vector<Replacement>> replacements(static_cast<size_t>(numberOfPoints));
    std::vector<int> visited(orientedFaces.size(), -1);

    for (igIndex pointId = 0; pointId < static_cast<igIndex>(numberOfPoints); ++pointId) {
        const auto& incidentFaces = pointFaces[pointId];
        if (incidentFaces.size() <= 1) { continue; }
        for (const igIndex faceId : incidentFaces) { visited[faceId] = -1; }

        int numberOfRegions = 0;
        for (const igIndex seedFaceId : incidentFaces) {
            if (visited[seedFaceId] >= 0) { continue; }
            visited[seedFaceId] = numberOfRegions;
            const auto& seedFace = orientedFaces[seedFaceId];
            if (seedFace.size() < 3) { continue; }

            const auto seedPointIt = std::find(seedFace.begin(), seedFace.end(), pointId);
            if (seedPointIt == seedFace.end()) { continue; }
            const IGsize spot = static_cast<IGsize>(std::distance(seedFace.begin(), seedPointIt));
            const igIndex neighboringPoints[2]{
                    seedFace[(spot + 1) % seedFace.size()],
                    seedFace[(spot + seedFace.size() - 1) % seedFace.size()]};

            for (const igIndex initialNeighborPoint : neighboringPoints) {
                igIndex cellId = seedFaceId;
                igIndex neighborPoint = initialNeighborPoint;
                while (cellId >= 0) {
                    const auto edgeIt = originalEdgeFaces.find(makeEdgeKey(pointId, neighborPoint));
                    igIndex neighborCellId = -1;
                    int neighborCount = 0;
                    if (edgeIt != originalEdgeFaces.end()) {
                        for (const igIndex candidate : edgeIt->second) {
                            if (candidate != cellId) {
                                neighborCellId = candidate;
                                ++neighborCount;
                            }
                        }
                    }

                    if (neighborCount != 1 || visited[neighborCellId] >= 0 ||
                        normals[cellId].dot(normals[neighborCellId]) <= cosAngle) {
                        break;
                    }

                    visited[neighborCellId] = numberOfRegions;
                    cellId = neighborCellId;
                    const auto& neighborFace = orientedFaces[cellId];
                    const auto pointIt = std::find(neighborFace.begin(), neighborFace.end(), pointId);
                    if (pointIt == neighborFace.end()) { break; }
                    const IGsize pointPosition =
                            static_cast<IGsize>(std::distance(neighborFace.begin(), pointIt));
                    const igIndex next = neighborFace[(pointPosition + 1) % neighborFace.size()];
                    const igIndex previous =
                            neighborFace[(pointPosition + neighborFace.size() - 1) % neighborFace.size()];
                    neighborPoint = next != neighborPoint ? next : previous;
                }
            }
            ++numberOfRegions;
        }

        if (numberOfRegions <= 1) { continue; }
        for (const igIndex faceId : incidentFaces) {
            if (visited[faceId] > 0) {
                replacements[pointId].push_back({faceId, visited[faceId]});
            }
        }
    }

    SplitResult result;
    result.Faces = orientedFaces;
    result.NewToOldPoint.resize(static_cast<size_t>(numberOfPoints));
    for (igIndex pointId = 0; pointId < static_cast<igIndex>(numberOfPoints); ++pointId) {
        result.NewToOldPoint[pointId] = pointId;
    }

    for (igIndex pointId = 0; pointId < static_cast<igIndex>(numberOfPoints); ++pointId) {
        int maximumRegion = 0;
        for (const auto& replacement : replacements[pointId]) {
            maximumRegion = std::max(maximumRegion, replacement.Region);
        }
        if (maximumRegion == 0) { continue; }

        const igIndex firstNewPointId = static_cast<igIndex>(result.NewToOldPoint.size());
        for (int region = 1; region <= maximumRegion; ++region) {
            result.NewToOldPoint.push_back(pointId);
        }
        for (const auto& replacement : replacements[pointId]) {
            const igIndex replacementPointId = firstNewPointId + replacement.Region - 1;
            auto& face = result.Faces[replacement.FaceId];
            const auto pointIt = std::find(face.begin(), face.end(), pointId);
            if (pointIt != face.end()) { *pointIt = replacementPointId; }
        }
    }
    return result;
}

ArrayObject::Pointer newArrayLike(const ArrayObject::Pointer& source) {
    if (source == nullptr) { return nullptr; }
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
        default: return DoubleArray::New();
    }
}

void copyAttributes(const AttributeSet::Pointer& source, const AttributeSet::Pointer& target,
                    IGenum attachmentType, const std::vector<igIndex>& newToOld) {
    if (source == nullptr || target == nullptr) { return; }
    auto attributes = attachmentType == IG_POINT ? source->GetAllPointAttributes()
                                                 : source->GetAllCellAttributes();
    if (attributes == nullptr) { return; }

    for (int attributeId = 0; attributeId < attributes->GetNumberOfElements(); ++attributeId) {
        auto& attribute = attributes->GetElement(attributeId);
        auto sourceArray = attribute.GetPointer();
        if (attribute.IsDeleted() || sourceArray == nullptr || sourceArray->GetName() == "Normals" ||
            sourceArray->GetName() == "Normals_Magnitude") {
            continue;
        }

        auto targetArray = newArrayLike(sourceArray);
        targetArray->SetName(sourceArray->GetName());
        targetArray->SetDimension(sourceArray->GetDimension());
        targetArray->Resize(newToOld.size());
        std::vector<double> tuple(static_cast<size_t>(sourceArray->GetDimension()));
        for (IGsize outputId = 0; outputId < newToOld.size(); ++outputId) {
            const igIndex inputId = newToOld[outputId];
            if (inputId < 0 || inputId >= sourceArray->GetNumberOfElements()) { continue; }
            sourceArray->GetElement(inputId, tuple.data());
            targetArray->SetElement(outputId, tuple.data());
        }
        target->AddAttribute(attribute.GetType(), attachmentType, targetArray);
    }
}

class UnionFind {
public:
    std::vector<int> parent;

    UnionFind(int numFaces) {
        parent = std::vector<int>(numFaces, -1);
        for (int faceId = 0; faceId < numFaces; faceId++) { parent[faceId] = faceId; }
    }

    void Union(int faceId1, int faceId2) {
        int root1 = FindParent(faceId1);
        int root2 = FindParent(faceId2);
        if (root1 != root2) { parent[root1] = root2; }
    }

    int FindParent(int id) {
        if (parent[id] != id) { parent[id] = FindParent(parent[id]); }
        return parent[id];
    }
};

} // namespace

bool FeatureEdgeRegionFilter::Execute() {
    auto obj = GetInput(0); // Get the original mesh
    if (obj == nullptr) {
        std::cerr << "Failed to get input mesh" << std::endl;
        return false;
    }
    auto inputMesh = DynamicCast<SurfaceMesh>(obj);
    if (inputMesh == nullptr) {
        std::cerr << "Failed to get input surface mesh" << std::endl;
        return false;
    }

    const int numPoints = inputMesh->GetNumberOfPoints();
    const int numFaces = inputMesh->GetNumberOfFaces();
    if (numPoints == 0 || numFaces == 0) {
        std::cerr << "FeatureEdgeRegionFilter requires a non-empty surface mesh" << std::endl;
        return false;
    }

    // vtkGenerateRegionIds 会先执行默认 vtkPolyDataNormals：先统一绕序，再以固定 30 度拆点。
    auto orientedFaces = copyFacePoints(inputMesh);
    const auto originalEdgeFaces = buildEdgeFaces(orientedFaces);
    orientFacesConsistently(orientedFaces, originalEdgeFaces);

    std::vector<Vector3f> normals;
    normals.reserve(static_cast<size_t>(numFaces));
    for (const auto& face : orientedFaces) { normals.push_back(getNormal(inputMesh, face)); }

    const auto originalPointFaces = buildPointFaces(orientedFaces, numPoints);
    auto split = splitSharpEdges(orientedFaces, originalEdgeFaces, originalPointFaces, normals,
                                 numPoints);

    auto mesh = SurfaceMesh::New();
    auto points = Points::New();
    points->Reserve(split.NewToOldPoint.size());
    for (const igIndex oldPointId : split.NewToOldPoint) {
        points->AddPoint(inputMesh->GetPoint(oldPointId));
    }
    auto faces = CellArray::New();
    for (const auto& face : split.Faces) {
        faces->AddCellIds(face.data(), static_cast<int>(face.size()));
    }
    mesh->SetPoints(points);
    mesh->SetFaces(faces);

    auto attributeSet = mesh->GetAttributeSet();
    copyAttributes(inputMesh->GetAttributeSet(), attributeSet, IG_POINT, split.NewToOldPoint);
    std::vector<igIndex> faceMap(static_cast<size_t>(numFaces));
    for (igIndex faceId = 0; faceId < numFaces; ++faceId) { faceMap[faceId] = faceId; }
    copyAttributes(inputMesh->GetAttributeSet(), attributeSet, IG_CELL, faceMap);

    auto cellNormals = FloatArray::New();
    cellNormals->SetName("Normals");
    cellNormals->SetDimension(3);
    cellNormals->Reserve(numFaces);
    for (const auto& normal : normals) {
        cellNormals->AddElement3(normal[0], normal[1], normal[2]);
    }
    attributeSet->AddAttribute(IG_NORMAL, IG_CELL, cellNormals);

    const auto splitPointFaces = buildPointFaces(split.Faces, split.NewToOldPoint.size());
    auto pointNormals = FloatArray::New();
    pointNormals->SetName("Normals");
    pointNormals->SetDimension(3);
    pointNormals->Reserve(split.NewToOldPoint.size());
    for (const auto& incidentFaces : splitPointFaces) {
        Vector3f normal(0.0f, 0.0f, 0.0f);
        for (const igIndex faceId : incidentFaces) { normal += normals[faceId]; }
        const double length = normal.norm();
        if (length > NORMAL_EPSILON) {
            normal = Vector3f(static_cast<float>(normal[0] / length),
                              static_cast<float>(normal[1] / length),
                              static_cast<float>(normal[2] / length));
        }
        pointNormals->AddElement3(normal[0], normal[1], normal[2]);
    }
    attributeSet->AddAttribute(IG_NORMAL, IG_POINT, pointNormals);

    // vtkGenerateRegionIds 以拆点后的共享点作为邻接，并用严格的 dot > cos(MaxAngle)。
    UnionFind myUnion(numFaces);
    const double cosMaximumAngle = std::cos(m_featureAngle / 180.0 * M_PI);
    for (const auto& incidentFaces : splitPointFaces) {
        for (IGsize first = 0; first < incidentFaces.size(); ++first) {
            for (IGsize second = first + 1; second < incidentFaces.size(); ++second) {
                const int firstFace = incidentFaces[first];
                const int secondFace = incidentFaces[second];
                if (myUnion.FindParent(firstFace) != myUnion.FindParent(secondFace) &&
                    normals[firstFace].dot(normals[secondFace]) > cosMaximumAngle) {
                    myUnion.Union(firstFace, secondFace);
                }
            }
        }
    }

    // realign the region IDs
    std::map<IGint, IGint> regionIDs;
    auto regionArray = IntArray::New(); // save region ids
    regionArray->SetName("Region Id");
    regionArray->SetDimension(1);
    regionArray->Reserve(numFaces);
    int p = 0;
    for (int i = 0; i < numFaces; i++) {
        int regionID = myUnion.FindParent(i);
        auto it = regionIDs.find(regionID);
        if (it == regionIDs.end()) {
            regionIDs[regionID] = p;
            p++;
            if (p >= std::numeric_limits<IGint>::max()) {
                std::cerr << "Too many regions for IGint RegionId." << std::endl;
                return false;
            }
        }
        regionArray->AddValue(regionIDs[regionID]);
    }

    auto oldAttributeId = attributeSet->GetAttributeIndex("Region Id");
    if (oldAttributeId >= 0) {
        auto oldArray = DynamicCast<IntArray>(attributeSet->GetAttribute("Region Id").pointer);
        if (oldArray == nullptr) {
            std::cerr << "Region Id already exists, but it is not an IntArray." << std::endl;
            return false;
        }
        oldArray->SetDimension(1);
        oldArray->Resize(numFaces);
        for (int i = 0; i < numFaces; ++i) {
            oldArray->SetValue(i, regionArray->GetValue(i));
        }
        oldArray->Modified();
        attributeSet->GetAttribute("Region Id").UpdateAllDataRange();
    } else {
        attributeSet->AddScalar(IG_CELL, regionArray);
    }
    attributeSet->ForceReConvertToDrawableData();

    mesh->SetName(inputMesh->GetName() + "_regionId");
    std::cout << "Number of regions:" << p << std::endl;
    SetOutput(mesh);
    return true;
}

IGAME_NAMESPACE_END
