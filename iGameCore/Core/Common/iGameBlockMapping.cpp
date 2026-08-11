#include "iGameBlockMapping.h"
#include <iGamePointFinder.h>
#include <unordered_map>
using namespace std;
IGAME_NAMESPACE_BEGIN

PointFinder::Pointer BlockMapping::BuildCentroidFinder(UnstructuredMesh::Pointer partedMesh) {
    auto partedCellNum = partedMesh->GetNumberOfCells();
    auto centroids = Points::New();
    centroids->Reserve(partedCellNum);
    for (int cellId = 0; cellId < partedCellNum; cellId++) {
        auto cell = partedMesh->GetCell(cellId);
        int pointSize = cell->GetNumberOfPoints();
        Vector3d centroid(0.0, 0.0, 0.0);
        for (int pi = 0; pi < pointSize; pi++) {
            auto& p = cell->GetPoint(pi);
            centroid[0] += p[0];
            centroid[1] += p[1];
            centroid[2] += p[2];
        }
        if (pointSize > 0) {
            centroid[0] /= pointSize;
            centroid[1] /= pointSize;
            centroid[2] /= pointSize;
        }
        centroids->AddPoint(centroid);
    }
    auto finder = PointFinder::New();
    finder->SetPoints(centroids);
    finder->Initialize();
    return finder;
}

std::vector<int> BlockMapping::GetMappingBlockCells(SurfaceMesh::Pointer oriMesh,
                                                    UnstructuredMesh::Pointer partedMesh) {
    auto partIdArray = GetPartId(partedMesh);
    auto finder = BuildCentroidFinder(partedMesh);

    auto oriCellNum = oriMesh->GetNumberOfFaces();
    std::vector<int> result(oriCellNum, 0);
    for (int cellId = 0; cellId < oriCellNum; cellId++) {
        auto cell = oriMesh->GetFace(cellId);
        int pointSize = cell->GetNumberOfPoints();
        std::unordered_map<int, int> votes;

        Vector3d centroid(0.0, 0.0, 0.0);
        for (int pi = 0; pi < pointSize; pi++) {
            auto& p = cell->GetPoint(pi);
            centroid[0] += p[0];
            centroid[1] += p[1];
            centroid[2] += p[2];

            // 对每个顶点单独投票
            Vector3d vp(p[0], p[1], p[2]);
            igIndex nearest = finder->FindClosestPoint(vp);
            votes[static_cast<int>(partIdArray->GetElementValue(nearest, 0))]++;
        }

        // 质心也参与投票
        if (pointSize > 0) {
            centroid[0] /= pointSize;
            centroid[1] /= pointSize;
            centroid[2] /= pointSize;
            igIndex nearest = finder->FindClosestPoint(centroid);
            votes[static_cast<int>(partIdArray->GetElementValue(nearest, 0))]++;
        }

        // 取票数最多的 part_id
        int bestId = 0, bestCount = 0;
        for (auto& [id, count] : votes) {
            if (count > bestCount) {
                bestCount = count;
                bestId = id;
            }
        }
        result[cellId] = bestId;
    }

    return result;
}

IntArray::Pointer BlockMapping::GetMappingBlockCellsArray(SurfaceMesh::Pointer oriMesh,
                                                          UnstructuredMesh::Pointer partedMesh) {
    auto vec = GetMappingBlockCells(oriMesh, partedMesh);
    auto result = IntArray::New();
    result->SetDimension(1);
    result->Reserve(static_cast<IGsize>(vec.size()));
    for (int v : vec) {
        result->AddValue(v);
    }
    return result;
}

ArrayObject::Pointer BlockMapping::GetPartId(UnstructuredMesh::Pointer partedMesh) {
    auto attrs = partedMesh->GetAttributeSet()->GetAllAttributes();
    ArrayObject::Pointer result;
    for (int i = 0; i < attrs->Size(); i++) {
        auto& attr = attrs->GetElement(i);
        if (attr.pointer->GetName() != "part_id") continue;
        result = attr.pointer;
    }
    return result;
}

std::vector<int> BlockMapping::GetMappingBlockCells(UnstructuredMesh::Pointer oriMesh,
                                                    UnstructuredMesh::Pointer partedMesh) {
    auto partIdArray = GetPartId(partedMesh);
    auto finder = BuildCentroidFinder(partedMesh);

    auto oriCellNum = oriMesh->GetNumberOfCells();
    std::vector<int> result(oriCellNum, 0);
    for (IGsize cellId = 0; cellId < oriCellNum; cellId++) {
        const igIndex* ptIds = nullptr;
        int nPts = oriMesh->GetCellPointIds(cellId, ptIds);
        std::unordered_map<int, int> votes;

        Vector3d centroid(0.0, 0.0, 0.0);
        for (int pi = 0; pi < nPts; pi++) {
            auto& p = oriMesh->GetPoint(ptIds[pi]);
            centroid[0] += p[0];
            centroid[1] += p[1];
            centroid[2] += p[2];
            Vector3d vp(p[0], p[1], p[2]);
            votes[static_cast<int>(partIdArray->GetElementValue(finder->FindClosestPoint(vp), 0))]++;
        }
        if (nPts > 0) {
            centroid[0] /= nPts;
            centroid[1] /= nPts;
            centroid[2] /= nPts;
            votes[static_cast<int>(partIdArray->GetElementValue(finder->FindClosestPoint(centroid), 0))]++;
        }

        int bestId = 0, bestCount = 0;
        for (auto& [id, count] : votes) {
            if (count > bestCount) { bestCount = count; bestId = id; }
        }
        result[cellId] = bestId;
    }
    return result;
}

IntArray::Pointer BlockMapping::GetMappingBlockCellsArray(UnstructuredMesh::Pointer oriMesh,
                                                          UnstructuredMesh::Pointer partedMesh) {
    auto vec = GetMappingBlockCells(oriMesh, partedMesh);
    auto result = IntArray::New();
    result->SetDimension(1);
    result->Reserve(static_cast<IGsize>(vec.size()));
    for (int v : vec) result->AddValue(v);
    return result;
}

std::vector<int> BlockMapping::GetMappingBlockCells(VolumeMesh::Pointer oriMesh,
                                                    UnstructuredMesh::Pointer partedMesh) {
    auto partIdArray = GetPartId(partedMesh);
    auto finder = BuildCentroidFinder(partedMesh);

    auto oriCellNum = oriMesh->GetNumberOfVolumes();
    std::vector<int> result(oriCellNum, 0);
    igIndex ptIds[64];
    for (IGsize cellId = 0; cellId < oriCellNum; cellId++) {
        int nPts = oriMesh->GetVolumePointIds(cellId, ptIds);
        std::unordered_map<int, int> votes;

        Vector3d centroid(0.0, 0.0, 0.0);
        for (int pi = 0; pi < nPts; pi++) {
            auto& p = oriMesh->GetPoint(ptIds[pi]);
            centroid[0] += p[0];
            centroid[1] += p[1];
            centroid[2] += p[2];
            Vector3d vp(p[0], p[1], p[2]);
            votes[static_cast<int>(partIdArray->GetElementValue(finder->FindClosestPoint(vp), 0))]++;
        }
        if (nPts > 0) {
            centroid[0] /= nPts;
            centroid[1] /= nPts;
            centroid[2] /= nPts;
            votes[static_cast<int>(partIdArray->GetElementValue(finder->FindClosestPoint(centroid), 0))]++;
        }

        int bestId = 0, bestCount = 0;
        for (auto& [id, count] : votes) {
            if (count > bestCount) { bestCount = count; bestId = id; }
        }
        result[cellId] = bestId;
    }
    return result;
}

IntArray::Pointer BlockMapping::GetMappingBlockCellsArray(VolumeMesh::Pointer oriMesh,
                                                          UnstructuredMesh::Pointer partedMesh) {
    auto vec = GetMappingBlockCells(oriMesh, partedMesh);
    auto result = IntArray::New();
    result->SetDimension(1);
    result->Reserve(static_cast<IGsize>(vec.size()));
    for (int v : vec) result->AddValue(v);
    return result;
}

IGAME_NAMESPACE_END