#include "iGameOverlappingCellsDetectorFilter.h"

#include <iGameStructuredMesh.h>
#include <iGameVolume.h>
#include <iGameSurfaceMesh.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

IGAME_NAMESPACE_BEGIN

namespace {

struct SpatialHashKey {
    int x{};
    int y{};
    int z{};

    bool operator==(const SpatialHashKey& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct SpatialHashKeyHasher {
    std::size_t operator()(const SpatialHashKey& key) const {
        return std::hash<int>{}(key.x) ^ (std::hash<int>{}(key.y) << 1) ^ (std::hash<int>{}(key.z) << 2);
    }
};

struct CellPairHasher {
    std::size_t operator()(const std::pair<igIndex, igIndex>& pair) const {
        return std::hash<igIndex>{}(pair.first) ^ (std::hash<igIndex>{}(pair.second) << 1);
    }
};

using Tetrahedron = std::array<Vector3d, 4>;

DataObject::Pointer CloneInputMesh(const DataObject::Pointer& input) {
    auto copyPoints = [](PointSet* source) {
        auto points = Points::New();
        points->DeepCopy(source->GetPoints());
        return points;
    };
    auto copyAttributes = [](DataObject* source) {
        auto attributes = AttributeSet::New();
        attributes->DeepCopy(source->GetAttributeSet());
        return attributes;
    };

    if (input->GetDataObjectType() == IG_STRUCTURED_MESH) {
        auto source = DynamicCast<StructuredMesh>(input);
        auto output = StructuredMesh::New();
        output->SetPoints(copyPoints(source));
        output->SetDimensionSize(source->GetDimensionSize());
        output->GenStructuredCellConnectivities();
        output->SetAttributeSet(copyAttributes(source));
        output->SetName(source->GetName());
        return output;
    }
    if (input->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
        auto source = DynamicCast<UnstructuredMesh>(input);
        auto output = UnstructuredMesh::New();
        output->SetPoints(copyPoints(source));
        auto cells = CellArray::New();
        cells->DeepCopy(source->GetCells());
        auto types = UnsignedIntArray::New();
        types->DeepCopy(source->GetCellTypes());
        output->SetCells(cells, types);
        output->SetAttributeSet(copyAttributes(source));
        output->SetName(source->GetName());
        return output;
    }
    if (input->GetDataObjectType() == IG_SURFACE_MESH) {
        auto source = DynamicCast<SurfaceMesh>(input);
        auto output = SurfaceMesh::New();
        output->SetPoints(copyPoints(source));
        auto faces = CellArray::New();
        faces->DeepCopy(source->GetFaces());
        output->SetFaces(faces);
        output->SetAttributeSet(copyAttributes(source));
        if (source->GetEdges()) {
            auto edges = CellArray::New(); edges->DeepCopy(source->GetEdges()); output->SetEdges(edges);
        }
        output->SetName(source->GetName());
        return output;
    }
    if (input->GetDataObjectType() == IG_VOLUME_MESH) {
        auto source = DynamicCast<VolumeMesh>(input);
        auto output = VolumeMesh::New();
        output->SetPoints(copyPoints(source));
        auto volumes = CellArray::New();
        volumes->DeepCopy(source->GetVolumes());
        output->SetVolumes(volumes);
        output->SetAttributeSet(copyAttributes(source));
        output->SetName(source->GetName());
        return output;
    }
    return nullptr;
}

} // namespace

OverlappingCellsDetectorFilter::OverlappingCellsDetectorFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool OverlappingCellsDetectorFilter::Execute() {
    m_OverlappingCellPairs.clear();
    m_CandidateCellPairs.clear();
    m_CellBounds.clear();
    m_NumberOfOverlapsPerCell.clear();
    m_LastError.clear();

    const auto fail = [this](std::string message) {
        m_LastError = std::move(message);
        return false;
    };

    m_UnstructuredMesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    m_VolumeMesh = DynamicCast<VolumeMesh>(GetInput(0));
    if (GetInput(0) && GetInput(0)->GetDataObjectType() == IG_SURFACE_MESH) {
        auto surface = DynamicCast<SurfaceMesh>(GetInput(0));
        m_UnstructuredMesh = UnstructuredMesh::New();
        auto types = UnsignedIntArray::New();
        for (IGsize i = 0; i < surface->GetNumberOfFaces(); ++i) {
            const auto size = surface->GetFaces()->GetCellSize(i);
            types->AddValue(size == 3 ? IG_TRIANGLE : size == 4 ? IG_QUAD : IG_POLYGON);
        }
        m_UnstructuredMesh->SetPoints(surface->GetPoints());
        m_UnstructuredMesh->SetCells(surface->GetFaces(), types);
    }
    if (m_UnstructuredMesh.IsNull() && m_VolumeMesh.IsNull()) {
        return fail("支持 SurfaceMesh 的三角形、平面凸四边形及 UnstructuredMesh / VolumeMesh 的线性单元。"
                    "点集、二维结构网格及复合数据暂不支持。");
    }
    if (!m_VolumeMesh.IsNull() && m_VolumeMesh->GetIsPolyhedronType()) {
        return fail("当前 iGame 实现暂不支持 Polyhedron 体单元，请先转换为线性四面体、六面体、三棱柱或金字塔单元。");
    }

    const IGsize cellCount = !m_UnstructuredMesh.IsNull() ? m_UnstructuredMesh->GetNumberOfCells()
                                                           : m_VolumeMesh->GetNumberOfVolumes();
    if (cellCount == 0) return fail("输入网格不包含单元，无法检测重叠。");
    m_NumberOfOverlapsPerCell.assign(cellCount, 0);
    m_CellBounds.reserve(cellCount);

    BoundingBox meshBound;
    bool hasSurfaceCells = false, hasVolumeCells = false;
    for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
        const IGenum cellType = !m_UnstructuredMesh.IsNull()
                ? m_UnstructuredMesh->GetCellType(cellId)
                : VolumeMesh::GetVolumeTypeWithPointNum(m_VolumeMesh->GetVolumes()->GetCellSize(cellId));
        if (cellType != IG_TRIANGLE && cellType != IG_QUAD && cellType != IG_TETRA && cellType != IG_HEXAHEDRON && cellType != IG_PRISM && cellType != IG_PYRAMID) {
            return fail(std::string("不支持单元 ") + std::to_string(cellId) + " 的类型：" +
                        GetCellTypeAsString(cellType) +
                        "。支持 Triangle、平面凸 Quad、Tetra、Hexahedron、Prism、Pyramid；其他类型尚未实现。");
        }
        const igIndex* pointIds = nullptr;
        hasSurfaceCells |= cellType == IG_TRIANGLE || cellType == IG_QUAD;
        hasVolumeCells |= cellType != IG_TRIANGLE && cellType != IG_QUAD;
        if (hasSurfaceCells && hasVolumeCells) return fail("暂不支持二维面单元和三维体单元混合检测，请分别提取后检测。");
        const int pointCount = !m_UnstructuredMesh.IsNull()
                ? m_UnstructuredMesh->GetCellPointIds(cellId, pointIds)
                : m_VolumeMesh->GetVolumes()->GetCellIds(cellId, pointIds);
        if (pointCount <= 0 || pointIds == nullptr) {
            return fail(std::string("无法读取单元 ") + std::to_string(cellId) + " 的点连接关系。");
        }

        BoundingBox cellBound;
        for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
            const auto points = !m_UnstructuredMesh.IsNull() ? m_UnstructuredMesh->GetPoints() : m_VolumeMesh->GetPoints();
            if (pointIds[pointIndex] < 0 || pointIds[pointIndex] >= points->GetNumberOfPoints())
                return fail("单元包含越界的顶点编号。");
            cellBound.add(!m_UnstructuredMesh.IsNull() ? m_UnstructuredMesh->GetPoint(pointIds[pointIndex])
                                                       : m_VolumeMesh->GetPoint(pointIds[pointIndex]));
        }
        meshBound.add(cellBound);
        m_CellBounds.push_back(cellBound);
    }

    if (cellCount > 1) {
        // 宽阶段：空间哈希只生成同桶中的候选对，避免直接进行 O(n^2) 的精确几何测试。
        const int bucketsPerAxis = std::max(1, static_cast<int>(std::cbrt(static_cast<double>(cellCount))));
        const Vector3d extent = meshBound.diagVector();
        std::array<double, 3> bucketSize{};
        for (int axis = 0; axis < 3; ++axis) {
            bucketSize[axis] = extent[axis] > 0.0 ? extent[axis] / bucketsPerAxis : 1.0;
        }
        auto toBucketIndex = [&](double coordinate, int axis) {
            if (extent[axis] <= 0.0) return 0;
            const int index = static_cast<int>(std::floor((coordinate - meshBound.min[axis]) / bucketSize[axis]));
            return std::clamp(index, 0, bucketsPerAxis - 1);
        };

        std::unordered_map<SpatialHashKey, std::vector<igIndex>, SpatialHashKeyHasher> buckets;
        for (igIndex cellId = 0; cellId < cellCount; ++cellId) {
            const auto& bound = m_CellBounds[cellId];
            for (int x = toBucketIndex(bound.min[0], 0); x <= toBucketIndex(bound.max[0], 0); ++x)
                for (int y = toBucketIndex(bound.min[1], 1); y <= toBucketIndex(bound.max[1], 1); ++y)
                    for (int z = toBucketIndex(bound.min[2], 2); z <= toBucketIndex(bound.max[2], 2); ++z)
                        buckets[{x, y, z}].push_back(cellId);
        }

        std::unordered_set<CellPair, CellPairHasher> uniqueCandidates;
        for (const auto& [key, bucketCells] : buckets) {
            (void)key;
            for (std::size_t first = 0; first < bucketCells.size(); ++first) {
                for (std::size_t second = first + 1; second < bucketCells.size(); ++second) {
                    const igIndex firstCellId = std::min(bucketCells[first], bucketCells[second]);
                    const igIndex secondCellId = std::max(bucketCells[first], bucketCells[second]);
                    const auto& a = m_CellBounds[firstCellId];
                    const auto& b = m_CellBounds[secondCellId];
                    // Surface AABBs can have zero thickness. The strict volume
                    // predicate would reject every XY/YZ/ZX planar face pair.
                    const bool boundsOverlap = hasSurfaceCells
                        ? (a.min[0] <= b.max[0] && b.min[0] <= a.max[0] &&
                           a.min[1] <= b.max[1] && b.min[1] <= a.max[1] &&
                           a.min[2] <= b.max[2] && b.min[2] <= a.max[2])
                        : a.collide(b);
                    if (boundsOverlap) {
                        uniqueCandidates.emplace(firstCellId, secondCellId);
                    }
                }
            }
        }
        m_CandidateCellPairs.assign(uniqueCandidates.begin(), uniqueCandidates.end());
    }

    double maxAbsCoordinate = 0.0;
    for (int axis = 0; axis < 3; ++axis) {
        maxAbsCoordinate = std::max({maxAbsCoordinate, std::abs(meshBound.min[axis]),
                                     std::abs(meshBound.max[axis])});
    }
    const double numericalTolerance = 100.0 * std::max(
            std::sqrt(std::numeric_limits<double>::min()),
            std::numeric_limits<double>::epsilon() * maxAbsCoordinate);
    // VTK 的 Tolerance 会使单元适度收缩；在本 Filter 的 SAT 判定中用最小共同投影长度表达同一语义。
    const double strictOverlapTolerance = std::max(numericalTolerance, m_Tolerance);

    struct SurfaceCell {
        std::vector<Vector3d> points;
        Vector3d normal;
    };
    std::vector<SurfaceCell> surfaceCells(hasSurfaceCells ? cellCount : 0);
    // Validate every face, including isolated ones. Use local coordinates to
    // avoid cancellation on translated geometry; reject warped/concave quads.
    for (IGsize id = 0; id < surfaceCells.size(); ++id) {
        auto& cell = surfaceCells[id];
        const igIndex* ids = nullptr;
        const int count = m_UnstructuredMesh->GetCellPointIds(id, ids);
        const int expected = m_UnstructuredMesh->GetCellType(id) == IG_TRIANGLE ? 3 : 4;
        if (count != expected) return fail("面单元的顶点数量与类型不一致。");
        for (int i = 0; i < count; ++i) {
            const auto& point = m_UnstructuredMesh->GetPoint(ids[i]);
            cell.points.emplace_back(point[0], point[1], point[2]);
        }
        cell.normal = (cell.points[1] - cell.points[0]).cross(cell.points[2] - cell.points[0]);
        const double length = cell.normal.length();
        const double scale = m_CellBounds[id].diagVector().length();
        if (!std::isfinite(length) || length <= numericalTolerance * scale)
            return fail("检测到退化或无效的面单元。");
        cell.normal /= length;
        for (int i = 0; i < count; ++i) {
            if (std::abs((cell.points[i] - cell.points[0]).dot(cell.normal)) > numericalTolerance)
                return fail("目前只支持平面凸四边形，不支持扭曲四边形。");
            const auto edge = cell.points[(i + 1) % count] - cell.points[i];
            const auto next = cell.points[(i + 2) % count] - cell.points[(i + 1) % count];
            if (edge.cross(next).dot(cell.normal) <= numericalTolerance * scale)
                return fail("目前只支持非退化三角形及平面凸四边形，不支持凹或自交四边形。");
        }
    }
    auto surfaceOverlap = [&](igIndex firstId, igIndex secondId) {
        const auto& first = surfaceCells[firstId];
        const auto& second = surfaceCells[secondId];
        const auto origin = first.points[0];
        auto direction = first.normal.cross(second.normal);
        const double directionLength = direction.length();
        if (directionLength <= 1e-12) {
            for (const auto& p : second.points)
                if (std::abs((p - origin).dot(first.normal)) > numericalTolerance) return false;
            // Planar convex SAT: strict interval overlap on every edge normal
            // excludes shared edges/vertices, even with distinct point IDs.
            for (const auto* polygon : {&first, &second}) {
                for (size_t i = 0; i < polygon->points.size(); ++i) {
                    auto axis = (polygon->points[(i + 1) % polygon->points.size()] - polygon->points[i]).cross(first.normal);
                    axis /= axis.length();
                    double aMin = std::numeric_limits<double>::max(), aMax = -aMin;
                    double bMin = aMin, bMax = aMax;
                    for (const auto& p : first.points) { const double v = (p-origin).dot(axis); aMin=std::min(aMin,v); aMax=std::max(aMax,v); }
                    for (const auto& p : second.points) { const double v = (p-origin).dot(axis); bMin=std::min(bMin,v); bMax=std::max(bMax,v); }
                    if (std::min(aMax,bMax) - std::max(aMin,bMin) <= strictOverlapTolerance) return false;
                }
            }
            return true;
        }
        // Non-coplanar faces: the common line must cross both interiors.
        // Merely touching a boundary (including a shared edge) is excluded.
        direction /= directionLength;
        auto slice = [&](const SurfaceCell& polygon, const SurfaceCell& plane, std::vector<Vector3d>& hits) {
            for (size_t i=0; i<polygon.points.size(); ++i) {
                const auto& a=polygon.points[i]; const auto& b=polygon.points[(i+1)%polygon.points.size()];
                const double da=(a-plane.points[0]).dot(plane.normal), db=(b-plane.points[0]).dot(plane.normal);
                if (std::abs(da)<=numericalTolerance) hits.push_back(a);
                if ((da < -numericalTolerance && db > numericalTolerance) || (db < -numericalTolerance && da > numericalTolerance))
                    hits.push_back(a+(b-a)*(da/(da-db)));
            }
        };
        std::vector<Vector3d> aHits,bHits;
        slice(first,second,aHits); slice(second,first,bHits);
        if(aHits.size()<2 || bHits.size()<2) return false;
        auto interval = [&](const std::vector<Vector3d>& hits) {
            double lo=std::numeric_limits<double>::max(), hi=-lo;
            for(const auto& p:hits) {double t=(p-origin).dot(direction);lo=std::min(lo,t);hi=std::max(hi,t);}
            return std::make_pair(lo,hi);
        };
        const auto a=interval(aHits), b=interval(bHits);
        const double lo=std::max(a.first,b.first), hi=std::min(a.second,b.second);
        if(hi-lo<=strictOverlapTolerance) return false;
        const auto midpoint=aHits[0]+direction*((lo+hi)/2-(aHits[0]-origin).dot(direction));
        for(const auto* polygon:{&first,&second}) {
            for(size_t i=0;i<polygon->points.size();++i) {
                const auto edge=polygon->points[(i+1)%polygon->points.size()]-polygon->points[i];
                if(edge.cross(midpoint-polygon->points[i]).dot(polygon->normal)<=strictOverlapTolerance*edge.length()) return false;
            }
        }
        return true;
    };

    auto getVolume = [&](igIndex cellId, Cell::Pointer& owner) -> Volume* {
        if (m_UnstructuredMesh.IsNull()) return m_VolumeMesh->GetVolume(cellId);
        if (!m_UnstructuredMesh->GetCell(cellId, owner)) return nullptr;
        return DynamicCast<Volume>(owner).get();
    };

    // Two conforming volume cells may share an entire (possibly warped) face.  Their
    // tetrahedralizations can choose different diagonals on that face and create a
    // false intersection.  ParaView avoids this by slightly shrinking cells before
    // testing.  Preserve genuinely overlapping neighbours by suppressing a pair only
    // when the two cell centres lie on opposite sides of the shared face.
    auto sharesSeparatingFace = [&](igIndex firstCellId, igIndex secondCellId) {
        struct CellGeometry {
            IGenum type{};
            std::vector<igIndex> pointIds;
            std::vector<Vector3d> points;
        };
        auto readGeometry = [&](igIndex cellId) {
            CellGeometry geometry;
            const igIndex* pointIds = nullptr;
            const int pointCount = !m_UnstructuredMesh.IsNull()
                    ? m_UnstructuredMesh->GetCellPointIds(cellId, pointIds)
                    : m_VolumeMesh->GetVolumes()->GetCellIds(cellId, pointIds);
            geometry.type = !m_UnstructuredMesh.IsNull()
                    ? m_UnstructuredMesh->GetCellType(cellId)
                    : VolumeMesh::GetVolumeTypeWithPointNum(pointCount);
            geometry.pointIds.assign(pointIds, pointIds + pointCount);
            geometry.points.reserve(pointCount);
            for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                const Point& point = !m_UnstructuredMesh.IsNull()
                        ? m_UnstructuredMesh->GetPoint(pointIds[pointIndex])
                        : m_VolumeMesh->GetPoint(pointIds[pointIndex]);
                geometry.points.emplace_back(point[0], point[1], point[2]);
            }
            return geometry;
        };

        const CellGeometry first = readGeometry(firstCellId);
        const CellGeometry second = readGeometry(secondCellId);

        Vector3d firstCenter(0.0, 0.0, 0.0);
        Vector3d secondCenter(0.0, 0.0, 0.0);
        for (const auto& point : first.points) firstCenter += point;
        for (const auto& point : second.points) secondCenter += point;
        firstCenter /= first.points.size();
        secondCenter /= second.points.size();

        auto geometryHasFace = [](const CellGeometry& geometry, const auto& faces,
                                  int faceCount, int faceSizeIndex,
                                  const std::array<igIndex, 4>& pointIds, int pointCount) {
            for (int faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
                if (faces[faceIndex][faceSizeIndex] != pointCount) continue;
                bool sameFace = true;
                for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                    const igIndex candidateId = geometry.pointIds[faces[faceIndex][pointIndex]];
                    if (std::find(pointIds.begin(), pointIds.begin() + pointCount, candidateId) ==
                        pointIds.begin() + pointCount) {
                        sameFace = false;
                        break;
                    }
                }
                if (sameFace) return true;
            }
            return false;
        };
        auto secondHasFace = [&](const std::array<igIndex, 4>& pointIds, int pointCount) {
            switch (second.type) {
            case IG_TETRA:
                return geometryHasFace(second, Tetra::faces, Tetra::NumberOfFaces,
                                       Tetra::MaxFaceSize, pointIds, pointCount);
            case IG_HEXAHEDRON:
                return geometryHasFace(second, Hexahedron::faces, Hexahedron::NumberOfFaces,
                                       Hexahedron::MaxFaceSize, pointIds, pointCount);
            case IG_PRISM:
                return geometryHasFace(second, Prism::faces, Prism::NumberOfFaces,
                                       Prism::MaxFaceSize, pointIds, pointCount);
            case IG_PYRAMID:
                return geometryHasFace(second, Pyramid::faces, Pyramid::NumberOfFaces,
                                       Pyramid::MaxFaceSize, pointIds, pointCount);
            default:
                return false;
            }
        };

        auto checkFaces = [&](const auto& faces, int faceCount, int faceSizeIndex) {
            for (int faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
                const int facePointCount = faces[faceIndex][faceSizeIndex];
                std::array<igIndex, 4> facePointIds{};
                for (int pointIndex = 0; pointIndex < facePointCount; ++pointIndex) {
                    facePointIds[pointIndex] = first.pointIds[faces[faceIndex][pointIndex]];
                }
                if (!secondHasFace(facePointIds, facePointCount)) continue;

                std::array<Vector3d, 4> facePoints{};
                Vector3d faceCenter(0.0, 0.0, 0.0);
                for (int pointIndex = 0; pointIndex < facePointCount; ++pointIndex) {
                    facePoints[pointIndex] = first.points[faces[faceIndex][pointIndex]];
                    faceCenter += facePoints[pointIndex];
                }
                faceCenter /= facePointCount;

                Vector3d normal;
                if (facePointCount == 3) {
                    normal = (facePoints[1] - facePoints[0]).cross(facePoints[2] - facePoints[0]);
                } else {
                    const Vector3d du = (facePoints[1] - facePoints[0]) +
                                        (facePoints[2] - facePoints[3]);
                    const Vector3d dv = (facePoints[3] - facePoints[0]) +
                                        (facePoints[2] - facePoints[1]);
                    normal = du.cross(dv);
                }
                if (normal.norm() <= numericalTolerance * numericalTolerance) continue;

                const double firstSide = normal.dot(firstCenter - faceCenter);
                const double secondSide = normal.dot(secondCenter - faceCenter);
                if (firstSide * secondSide < 0.0) return true;
            }
            return false;
        };

        switch (first.type) {
        case IG_TETRA:
            return checkFaces(Tetra::faces, Tetra::NumberOfFaces, Tetra::MaxFaceSize);
        case IG_HEXAHEDRON:
            return checkFaces(Hexahedron::faces, Hexahedron::NumberOfFaces,
                              Hexahedron::MaxFaceSize);
        case IG_PRISM:
            return checkFaces(Prism::faces, Prism::NumberOfFaces, Prism::MaxFaceSize);
        case IG_PYRAMID:
            return checkFaces(Pyramid::faces, Pyramid::NumberOfFaces, Pyramid::MaxFaceSize);
        default:
            return false;
        }
    };

    std::vector<std::vector<Tetrahedron>> tetrahedra(cellCount);
    std::vector<bool> tetrahedraPrepared(cellCount, false);
    auto prepareTetrahedra = [&](igIndex cellId) {
        if (tetrahedraPrepared[cellId]) return !tetrahedra[cellId].empty();
        tetrahedraPrepared[cellId] = true;

        Cell::Pointer owner;
        Volume* volume = getVolume(cellId, owner);
        if (volume == nullptr) {
            m_LastError = std::string("单元 ") + std::to_string(cellId) + " 不是可处理的体单元。";
            return false;
        }
        auto addTetrahedron = [&](int first, int second, int third, int fourth) {
            Tetrahedron tetra{};
            tetra[0] = volume->GetPoint(first);
            tetra[1] = volume->GetPoint(second);
            tetra[2] = volume->GetPoint(third);
            tetra[3] = volume->GetPoint(fourth);
            tetrahedra[cellId].push_back(tetra);
        };

        switch (volume->GetCellType()) {
        case IG_TETRA:
            addTetrahedron(0, 1, 2, 3);
            break;
        case IG_HEXAHEDRON:
            addTetrahedron(0, 1, 3, 4);
            addTetrahedron(1, 2, 3, 6);
            addTetrahedron(3, 4, 6, 7);
            addTetrahedron(1, 4, 5, 6);
            addTetrahedron(1, 3, 4, 6);
            break;
        case IG_PRISM:
            addTetrahedron(0, 1, 2, 5);
            addTetrahedron(0, 3, 4, 5);
            addTetrahedron(0, 1, 4, 5);
            break;
        case IG_PYRAMID:
            addTetrahedron(0, 1, 3, 4);
            addTetrahedron(1, 2, 3, 4);
            break;
        default:
            m_LastError = std::string("单元 ") + std::to_string(cellId) + " 的四面体分解失败。";
            return false;
        }
        if (tetrahedra[cellId].empty()) {
            m_LastError = std::string("单元 ") + std::to_string(cellId) + " 未能分解为四面体。";
            return false;
        }
        return true;
    };

    auto hasStrictProjectionOverlap = [&](const Tetrahedron& firstTetra, const Tetrahedron& secondTetra,
                                          const Vector3d& axis) {
        const double axisLength = axis.norm();
        if (axisLength <= numericalTolerance * numericalTolerance) return true;
        const Vector3d normalizedAxis = axis / axisLength;
        double firstMin = normalizedAxis.dot(firstTetra[0]);
        double firstMax = firstMin;
        double secondMin = normalizedAxis.dot(secondTetra[0]);
        double secondMax = secondMin;
        for (int pointIndex = 1; pointIndex < 4; ++pointIndex) {
            const double firstProjection = normalizedAxis.dot(firstTetra[pointIndex]);
            const double secondProjection = normalizedAxis.dot(secondTetra[pointIndex]);
            firstMin = std::min(firstMin, firstProjection);
            firstMax = std::max(firstMax, firstProjection);
            secondMin = std::min(secondMin, secondProjection);
            secondMax = std::max(secondMax, secondProjection);
        }
        return firstMax > secondMin + strictOverlapTolerance && secondMax > firstMin + strictOverlapTolerance;
    };

    auto tetrahedraOverlap = [&](const Tetrahedron& firstTetra, const Tetrahedron& secondTetra) {
        constexpr std::array<std::array<int, 3>, 4> faces{{{{0, 1, 2}}, {{0, 1, 3}}, {{0, 2, 3}}, {{1, 2, 3}}}};
        constexpr std::array<std::array<int, 2>, 6> edges{{{{0, 1}}, {{0, 2}}, {{0, 3}}, {{1, 2}}, {{1, 3}}, {{2, 3}}}};
        std::vector<Vector3d> axes;
        axes.reserve(faces.size() * 2 + edges.size() * edges.size());
        for (const auto& face : faces) {
            axes.push_back((firstTetra[face[1]] - firstTetra[face[0]]).cross(firstTetra[face[2]] - firstTetra[face[0]]));
            axes.push_back((secondTetra[face[1]] - secondTetra[face[0]]).cross(secondTetra[face[2]] - secondTetra[face[0]]));
        }
        for (const auto& firstEdge : edges)
            for (const auto& secondEdge : edges)
                axes.push_back((firstTetra[firstEdge[1]] - firstTetra[firstEdge[0]]).cross(
                        secondTetra[secondEdge[1]] - secondTetra[secondEdge[0]]));

        for (const auto& axis : axes) {
            if (!hasStrictProjectionOverlap(firstTetra, secondTetra, axis)) return false;
        }
        return true;
    };

    for (const auto& candidate : m_CandidateCellPairs) {
        if (hasSurfaceCells) {
            if (surfaceOverlap(candidate.first, candidate.second)) {
                m_OverlappingCellPairs.push_back(candidate);
                ++m_NumberOfOverlapsPerCell[candidate.first];
                ++m_NumberOfOverlapsPerCell[candidate.second];
            }
            continue;
        }
        if (sharesSeparatingFace(candidate.first, candidate.second)) continue;
        if (!prepareTetrahedra(candidate.first) || !prepareTetrahedra(candidate.second)) return false;
        bool overlapFound = false;
        for (const auto& firstTetra : tetrahedra[candidate.first]) {
            for (const auto& secondTetra : tetrahedra[candidate.second]) {
                if (tetrahedraOverlap(firstTetra, secondTetra)) {
                    overlapFound = true;
                    break;
                }
            }
            if (overlapFound) break;
        }
        if (overlapFound) {
            m_OverlappingCellPairs.push_back(candidate);
            ++m_NumberOfOverlapsPerCell[candidate.first];
            ++m_NumberOfOverlapsPerCell[candidate.second];
        }
    }

    auto overlapArray = LongLongArray::New();
    overlapArray->SetName(NumberOfOverlapsPerCellArrayName());
    overlapArray->SetDimension(1);
    overlapArray->Reserve(cellCount);
    for (const auto count : m_NumberOfOverlapsPerCell) {
        overlapArray->AddValue(static_cast<long long>(count));
    }

    auto output = CloneInputMesh(GetInput(0));
    if (output.IsNull()) return fail("创建独立输出网格失败。");
    output->SetName(GetInput(0)->GetName() + "_OverlappingCellsDetector");

    auto* attributes = output->GetAttributeSet();
    const int existingIndex = attributes->GetAttributeIndex(NumberOfOverlapsPerCellArrayName());
    if (existingIndex >= 0) {
        auto& attribute = attributes->GetAttribute(existingIndex);
        attribute.SetPointer(overlapArray);
        attribute.SetType(IG_SCALAR);
        attribute.SetAttachmentType(IG_CELL);
        attribute.UpdateAllDataRange();
    } else {
        attributes->AddAttribute(IG_SCALAR, IG_CELL, overlapArray);
    }

    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
