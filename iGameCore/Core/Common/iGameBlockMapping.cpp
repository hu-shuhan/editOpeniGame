#include "iGameBlockMapping.h"
#include <iGamePointFinder.h>
#include <iGameThreadPool.h>
#include <iGameBoundingBox.h>
#include <cmath>
#include <mutex>
using namespace std;
IGAME_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
// 体素网格：预计算每个体素对应的 part_id，查询 O(1)
// ---------------------------------------------------------------------------
namespace {

struct VoxelGrid {
    Vector3d origin;
    double rcpDx, rcpDy, rcpDz;
    int nx, ny, nz;
    std::vector<int> data;  // [iz*ny*nx + iy*nx + ix]

    int query(const Vector3d& p) const {
        int ix = static_cast<int>((p[0] - origin[0]) * rcpDx);
        int iy = static_cast<int>((p[1] - origin[1]) * rcpDy);
        int iz = static_cast<int>((p[2] - origin[2]) * rcpDz);
        ix = std::max(0, std::min(nx - 1, ix));
        iy = std::max(0, std::min(ny - 1, iy));
        iz = std::max(0, std::min(nz - 1, iz));
        return data[static_cast<size_t>(iz) * ny * nx + iy * nx + ix];
    }
};

// 从 partedMesh 的质心和 part_id 构建体素网格
VoxelGrid buildVoxelGrid(UnstructuredMesh::Pointer partedMesh,
                          ArrayObject::Pointer partIdArray) {
    const int M = static_cast<int>(partedMesh->GetNumberOfCells());

    // 1. 并行收集 partedMesh 各 cell 的质心和 part_id
    //    使用 GetCellPointIds（只读指针版本）保证线程安全，避免 GetCell() 内部的共享缓存写入
    auto pts = Points::New();
    pts->SetNumberOfPoints(M);
    std::vector<int> seedPartIds(M);

    std::mutex bboxMutex;
    BoundingBox bbox;
    bbox.reset();

    ThreadPool::parallelFor(0, M, [&](int s, int e) {
        BoundingBox localBbox;
        localBbox.reset();
        for (int i = s; i < e; i++) {
            const igIndex* ptIds = nullptr;
            int n = partedMesh->GetCellPointIds(i, ptIds);
            Vector3d c(0.0, 0.0, 0.0);
            for (int pi = 0; pi < n; pi++) {
                auto& p = partedMesh->GetPoint(ptIds[pi]);
                c[0] += p[0]; c[1] += p[1]; c[2] += p[2];
            }
            if (n > 0) { c[0] /= n; c[1] /= n; c[2] /= n; }
            pts->SetPoint(i, c);
            localBbox.add(c);
            seedPartIds[i] = static_cast<int>(partIdArray->GetElementValue(i, 0));
        }
        std::lock_guard<std::mutex> lock(bboxMutex);
        bbox.add(localBbox);
    });

    // 2. 包围盒加 padding
    double pad = bbox.diagVector().maxCoeff() * 0.01;
    bbox.min -= pad;
    bbox.max += pad;

    // 3. 确定体素分辨率：目标体素数 ≈ 4×M，按包围盒比例分配各轴
    Vector3d diag = bbox.diagVector();
    double bboxVol = diag[0] * diag[1] * diag[2];
    double targetVoxels = std::max(static_cast<double>(M) * 4.0, 32768.0); // 至少 32^3
    double voxelSide = std::cbrt(bboxVol / targetVoxels);

    int nx = std::max(1, std::min(256, static_cast<int>(std::ceil(diag[0] / voxelSide))));
    int ny = std::max(1, std::min(256, static_cast<int>(std::ceil(diag[1] / voxelSide))));
    int nz = std::max(1, std::min(256, static_cast<int>(std::ceil(diag[2] / voxelSide))));

    double dx = diag[0] / nx;
    double dy = diag[1] / ny;
    double dz = diag[2] / nz;

    // 4. 用 PointFinder 为每个体素找最近质心并存 part_id（并行，FindClosestPoint 只读线程安全）
    auto finder = PointFinder::New();
    finder->SetPoints(pts);
    finder->Initialize();

    VoxelGrid grid;
    grid.origin = bbox.min;
    grid.rcpDx = 1.0 / dx;
    grid.rcpDy = 1.0 / dy;
    grid.rcpDz = 1.0 / dz;
    grid.nx = nx; grid.ny = ny; grid.nz = nz;
    const int totalVoxels = nx * ny * nz;
    grid.data.resize(static_cast<size_t>(totalVoxels));

    ThreadPool::parallelFor(0, totalVoxels, [&](int s, int e) {
        for (int idx = s; idx < e; idx++) {
            int iz = idx / (ny * nx);
            int iy = (idx % (ny * nx)) / nx;
            int ix = idx % nx;
            Vector3d center;
            center[0] = bbox.min[0] + (ix + 0.5) * dx;
            center[1] = bbox.min[1] + (iy + 0.5) * dy;
            center[2] = bbox.min[2] + (iz + 0.5) * dz;
            igIndex nearest = finder->FindClosestPoint(center);
            grid.data[idx] = (nearest >= 0) ? seedPartIds[nearest] : 0;
        }
    });

    return grid;
}

} // namespace

// ---------------------------------------------------------------------------
// 公共辅助
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// SurfaceMesh
// ---------------------------------------------------------------------------
std::vector<int> BlockMapping::GetMappingBlockCells(SurfaceMesh::Pointer oriMesh,
                                                    UnstructuredMesh::Pointer partedMesh) {
    auto partIdArray = GetPartId(partedMesh);
    auto grid = buildVoxelGrid(partedMesh, partIdArray);

    auto oriCellNum = oriMesh->GetNumberOfFaces();
    std::vector<int> result(oriCellNum, 0);
    ThreadPool::parallelFor(0, static_cast<int>(oriCellNum), [&](int s, int e) {
        igIndex ptIds[64];
        for (int cellId = s; cellId < e; cellId++) {
            int nPts = oriMesh->GetFacePointIds(cellId, ptIds);
            if (nPts == 0) continue;
            Vector3d c(0.0, 0.0, 0.0);
            for (int pi = 0; pi < nPts; pi++) {
                auto& p = oriMesh->GetPoint(ptIds[pi]);
                c[0] += p[0]; c[1] += p[1]; c[2] += p[2];
            }
            c[0] /= nPts; c[1] /= nPts; c[2] /= nPts;
            result[cellId] = grid.query(c);
        }
    });
    return result;
}

IntArray::Pointer BlockMapping::GetMappingBlockCellsArray(SurfaceMesh::Pointer oriMesh,
                                                          UnstructuredMesh::Pointer partedMesh) {
    auto vec = GetMappingBlockCells(oriMesh, partedMesh);
    auto result = IntArray::New();
    result->SetDimension(1);
    result->Reserve(static_cast<IGsize>(vec.size()));
    for (int v : vec) result->AddValue(v);
    return result;
}

// ---------------------------------------------------------------------------
// UnstructuredMesh
// ---------------------------------------------------------------------------
std::vector<int> BlockMapping::GetMappingBlockCells(UnstructuredMesh::Pointer oriMesh,
                                                    UnstructuredMesh::Pointer partedMesh) {
    auto partIdArray = GetPartId(partedMesh);
    auto grid = buildVoxelGrid(partedMesh, partIdArray);

    auto oriCellNum = oriMesh->GetNumberOfCells();
    std::vector<int> result(oriCellNum, 0);
    ThreadPool::parallelFor(0, static_cast<int>(oriCellNum), [&](int s, int e) {
        for (int cellId = s; cellId < e; cellId++) {
            const igIndex* ptIds = nullptr;
            int nPts = oriMesh->GetCellPointIds(cellId, ptIds);
            if (nPts == 0) continue;
            Vector3d c(0.0, 0.0, 0.0);
            for (int pi = 0; pi < nPts; pi++) {
                auto& p = oriMesh->GetPoint(ptIds[pi]);
                c[0] += p[0]; c[1] += p[1]; c[2] += p[2];
            }
            c[0] /= nPts; c[1] /= nPts; c[2] /= nPts;
            result[cellId] = grid.query(c);
        }
    });
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

// ---------------------------------------------------------------------------
// VolumeMesh
// ---------------------------------------------------------------------------
std::vector<int> BlockMapping::GetMappingBlockCells(VolumeMesh::Pointer oriMesh,
                                                    UnstructuredMesh::Pointer partedMesh) {
    auto partIdArray = GetPartId(partedMesh);
    auto grid = buildVoxelGrid(partedMesh, partIdArray);

    auto oriCellNum = oriMesh->GetNumberOfVolumes();
    std::vector<int> result(oriCellNum, 0);
    ThreadPool::parallelFor(0, static_cast<int>(oriCellNum), [&](int s, int e) {
        igIndex ptIds[64];
        for (int cellId = s; cellId < e; cellId++) {
            int nPts = oriMesh->GetVolumePointIds(cellId, ptIds);
            if (nPts == 0) continue;
            Vector3d c(0.0, 0.0, 0.0);
            for (int pi = 0; pi < nPts; pi++) {
                auto& p = oriMesh->GetPoint(ptIds[pi]);
                c[0] += p[0]; c[1] += p[1]; c[2] += p[2];
            }
            c[0] /= nPts; c[1] /= nPts; c[2] /= nPts;
            result[cellId] = grid.query(c);
        }
    });
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
