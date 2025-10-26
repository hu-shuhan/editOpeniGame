#include "iGameCellFaceExtracter.h"
#include <algorithm>
#include <iGameThreadPool.h>
#include <iGameUnstructuredMesh.h>
IGAME_NAMESPACE_BEGIN
template<class T>
static inline void SortVector(std::vector<T>& v) {
    std::sort(v.begin(), v.end());
}
template<class T>
static inline void SortPair(std::pair<T, T>& p) {
    p = std::minmax(p.first, p.second);
}

void CellFaceExtracter::AddCell(const std::vector<int>& ids, UnstructuredMesh* mesh) {
    if (ids.empty() || mesh == nullptr) return;
    for (int i = 0; i < ids.size(); i++) {
        auto& id = ids[i];
        Cell* cell= mesh->GetCell(id);
        AddCell(id, cell);
    }
    //std::mutex GetCellMutex;
    //ThreadPool::parallelFor(0, ids.size(), [&](int st, int ed) {
    //    for (int i = st; i < ed; i++) {
    //        auto& id = ids[i];
    //        Cell* cell;
    //        {
    //            std::lock_guard lg(GetCellMutex);
    //            cell = mesh->GetCell(id);
    //        }
    //        AddCell(id, cell, true);
    //    }
    //});
}

void CellFaceExtracter::AddCell(int id, Cell* cell, bool useMutex) { _AddCell(id, cell, useMutex); }

void CellFaceExtracter::_AddCell(int id, Cell* cell, bool useMutex) {
    if (cell == nullptr) return;
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 1) return;
        std::vector<int> facePointIds;
        std::vector<std::pair<int, int>> faceEdgePointIds;
        for (int pointI = 0; pointI < pointSize; pointI++) {
            auto pointIA = pointI;
            auto pointIB = (pointI + 1) % pointSize;
            auto pointId = cell->GetPointId(pointI);
            auto pointIdA = cell->GetPointId(pointIA);
            auto pointIdB = cell->GetPointId(pointIB);
            facePointIds.push_back(pointId);
            faceEdgePointIds.push_back(std::minmax(pointIdA, pointIdB));
        }
        SortVector(facePointIds);
        if (useMutex) {
            std::lock_guard lg(m_CellsMutex);
            m_Cells[facePointIds][id] = faceEdgePointIds;
        } else {
            m_Cells[facePointIds][id] = faceEdgePointIds;
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            _AddCell(id, face, useMutex);
        }
    }
}

void CellFaceExtracter::RemoveCell(const std::vector<int>& ids, UnstructuredMesh* mesh) {
    if (ids.empty() || mesh == nullptr) return;
    for (int i = 0; i < ids.size(); i++) {
        auto& id = ids[i];
        Cell* cell = mesh->GetCell(id);
        RemoveCell(id, cell);
    }
    //std::mutex GetCellMutex;
    //ThreadPool::parallelFor(0, ids.size(), [&](int st, int ed) {
    //    for (int i = st; i < ed; i++) {
    //        auto& id = ids[i];
    //        Cell* cell;
    //        {
    //            std::lock_guard lg(GetCellMutex);
    //            cell = mesh->GetCell(id);
    //        }
    //        RemoveCell(id, cell, true);
    //    }
    //});
}

void CellFaceExtracter::RemoveCell(int id, Cell* cell, bool useMutex) { _RemoveCell(id, cell, useMutex); }

void CellFaceExtracter::_RemoveCell(int id, Cell* cell, bool useMutex) {
    if (cell == nullptr) return;
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 1) return;
        std::vector<int> facePointIds;
        for (int pointI = 0; pointI < pointSize; pointI++) {
            auto pointId = cell->GetPointId(pointI);
            facePointIds.push_back(pointId);
        }
        SortVector(facePointIds);
        if (useMutex) {
            std::lock_guard lg(m_CellsMutex);
            if (m_Cells.count(facePointIds) == 0) return;
            m_Cells[facePointIds].erase(id);
            if (m_Cells[facePointIds].empty()) { m_Cells.erase(facePointIds); }
        } else {
            if (m_Cells.count(facePointIds) == 0) return;
            m_Cells[facePointIds].erase(id);
            if (m_Cells[facePointIds].empty()) { m_Cells.erase(facePointIds); }
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            _RemoveCell(id, face, useMutex);
        }
    }
}

void CellFaceExtracter::Clear() { m_Cells.clear(); }

std::set<std::pair<int, int>> CellFaceExtracter::GetExtractPointIdPairs() {
    std::set<std::pair<int, int>> reSet;
    for (auto& faceMsgVectorPair: m_Cells) {
        auto& faceMsgVector = faceMsgVectorPair.second;
        if (faceMsgVector.size() != 1) continue;
        reSet.insert(faceMsgVector.begin()->second.begin(), faceMsgVector.begin()->second.end());
    }
    return reSet;
}

IGAME_NAMESPACE_END