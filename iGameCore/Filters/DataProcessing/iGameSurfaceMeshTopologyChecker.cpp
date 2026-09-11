#include "iGameSurfaceMeshTopologyChecker.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>

IGAME_NAMESPACE_BEGIN

namespace {
struct TriangleKey {
    std::array<igIndex, 3> ids{};

    bool operator==(const TriangleKey& other) const { return ids == other.ids; }
};

struct TriangleKeyHash {
    size_t operator()(const TriangleKey& key) const {
        size_t hash = 0;
        for (const igIndex id : key.ids) {
            const size_t value = std::hash<igIndex>{}(id);
            hash ^= value + size_t{0x9e3779b9} + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

struct EdgeKey {
    igIndex first{-1};
    igIndex second{-1};

    bool operator==(const EdgeKey& other) const {
        return first == other.first && second == other.second;
    }
};

struct EdgeKeyHash {
    size_t operator()(const EdgeKey& key) const {
        size_t hash = std::hash<igIndex>{}(key.first);
        const size_t value = std::hash<igIndex>{}(key.second);
        hash ^= value + size_t{0x9e3779b9} + (hash << 6) + (hash >> 2);
        return hash;
    }
};

struct EdgeInfo {
    igIndex id{-1};
    int adjacentFaceCount{0};
};
} // namespace

bool SurfaceMeshTopologyReport::IsValid() const {
    return nonTriangleFaceIds.empty() && invalidIndexFaceIds.empty() &&
           degenerateFaceIds.empty() && zeroAreaFaceIds.empty() &&
           duplicateFaceIds.empty() && invalidEdgeIds.empty() &&
           isolatedEdgeIds.empty() && nonManifoldEdgeIds.empty() &&
           invalidAdjacencyEdgeIds.empty();
}

SurfaceMeshTopologyReport SurfaceMeshTopologyChecker::Check(const SurfaceMesh::Pointer& mesh) {
    SurfaceMeshTopologyReport report;
    if (mesh == nullptr) { return report; }

    report.pointCount = mesh->GetNumberOfPoints();
    report.faceCount = mesh->GetNumberOfFaces();

    const double diagonal = mesh->GetBoundingBox().diag();
    const double areaTolerance = std::max(1e-30, diagonal * diagonal * 1e-14);
    std::unordered_map<TriangleKey, igIndex, TriangleKeyHash> knownTriangles;
    std::unordered_map<EdgeKey, EdgeInfo, EdgeKeyHash> edges;

    for (igIndex faceId = 0; faceId < report.faceCount; ++faceId) {
        igIndex ids[IGAME_CELL_MAX_SIZE]{};
        const int size = mesh->GetFacePointIds(faceId, ids);
        if (size != 3) {
            report.nonTriangleFaceIds.push_back(faceId);
            continue;
        }

        bool valid = true;
        for (int i = 0; i < 3; ++i) {
            if (ids[i] < 0 || ids[i] >= report.pointCount) {
                valid = false;
                break;
            }
        }
        if (!valid) {
            report.invalidIndexFaceIds.push_back(faceId);
            continue;
        }

        if (ids[0] == ids[1] || ids[1] == ids[2] || ids[2] == ids[0]) {
            report.degenerateFaceIds.push_back(faceId);
            continue;
        }

        TriangleKey key{{ids[0], ids[1], ids[2]}};
        std::sort(key.ids.begin(), key.ids.end());
        if (!knownTriangles.emplace(key, faceId).second) {
            report.duplicateFaceIds.push_back(faceId);
        }

        const Point p0 = mesh->GetPoint(ids[0]);
        const Point p1 = mesh->GetPoint(ids[1]);
        const Point p2 = mesh->GetPoint(ids[2]);
        const double twiceArea = CrossProduct(p1 - p0, p2 - p0).norm();
        if (!std::isfinite(twiceArea) || twiceArea <= areaTolerance) {
            report.zeroAreaFaceIds.push_back(faceId);
        }

        for (int i = 0; i < 3; ++i) {
            const igIndex a = ids[i];
            const igIndex b = ids[(i + 1) % 3];
            const EdgeKey edge{std::min(a, b), std::max(a, b)};
            auto [iter, inserted] = edges.emplace(edge, EdgeInfo{});
            if (inserted) { iter->second.id = static_cast<igIndex>(edges.size() - 1); }
            ++iter->second.adjacentFaceCount;
        }
    }

    // Build adjacency independently from triangle point IDs. Do not call
    // RequestEditStatus(): constructing the mesh's own links is precisely
    // what may fail for malformed extracted surfaces.
    report.edgeCount = static_cast<IGsize>(edges.size());
    for (const auto& [edge, info] : edges) {
        if (edge.first == edge.second) {
            report.invalidEdgeIds.push_back(info.id);
        } else if (info.adjacentFaceCount > 2) {
            report.nonManifoldEdgeIds.push_back(info.id);
        }
    }

    return report;
}

IGAME_NAMESPACE_END
