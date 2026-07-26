#include "iGameBlockMapping.h"
#include <iGamePointFinder.h>
#include <unordered_map>
using namespace std;
IGAME_NAMESPACE_BEGIN
std::vector<int> BlockMapping::GetMappingBlockCells(SurfaceMesh::Pointer oriMesh,
                                                    UnstructuredMesh::Pointer partedMesh) {
    // 写给AI
    // 这个函数用于将简化后的含有分块信息的partedMesh的分块信息，映射回原mesh：oriMesh
    // 具体的，关于partedMesh的来源：oriMesh简化->发送给分块服务器->分块并返回
    // partedMesh中，有一个attribute，名为part_id，表示各个cell对应的块的序号
    // 这个函数返回的vector，就代表了原mesh中各个cell所对应的序号。因此，不要修改oriMesh和partedMesh
    // 有关的函数的调用方法：
    // part_id获取：auto partIdArray = GetPartId(partedMesh);
    // 获取某个cell的part_id：double cellPartId = partIdArray->GetElementValue(cellId, 0);
    // 获取cell数：auto cellNum = partedMesh->GetNumberOfCells();
    // 遍历cellId：for (int cellId = 0; cellId < cellNum; cellId++)
    // 获取cell对象：auto cell = partedMesh->GetCell(cellId);
    // 获取cell点数：int pointSize = cell->GetNumberOfPoints();
    // 遍历cell中的各点：
    // for (int cellPointIdx = 0; cellPointIdx < pointSize; cellPointIdx++) {
    //  auto& point = cell->GetPoint(cellPointIdx);
    // }
    // 获取点的坐标：
    // point[0];
    // point[1];
    // point[2];
    //
    //

    auto partIdArray = GetPartId(partedMesh);
    auto partedCellNum = partedMesh->GetNumberOfCells();

    // 为 partedMesh 的每个 cell 计算质心，建立空间索引
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

    // 对 oriMesh 每个 cell，用顶点+质心多点查询投票决定 part_id
    // 仅用质心时，贴近分区边界的 cell 可能因邻区 partedMesh cell 较小而误判；
    // 对所有顶点各自查询后多数投票，可大幅降低边界误判率。
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

IGAME_NAMESPACE_END