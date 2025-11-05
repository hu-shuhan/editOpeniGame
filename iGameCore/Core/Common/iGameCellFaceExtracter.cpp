#include "iGameCellFaceExtracter.h"
#include <algorithm>
#include <iGameThreadPool.h>
#include <iGameUnstructuredMesh.h>
#include <limits>
#include <queue>
IGAME_NAMESPACE_BEGIN
template<class T>
static inline void SortVector(std::vector<T>& v) {
    std::sort(v.begin(), v.end());
}

static inline std::pair<Point, Point> MinMaxPoint() {
    return {Point(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                  std::numeric_limits<float>::max()),
            Point(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                  std::numeric_limits<float>::lowest())};
}

static inline void MinMaxPoint(Point& pMin, Point& pMax, const Point& p) {
    for (int i = 0; i < 3; i++) {
        pMin[i] = std::min<float>(pMin[i], p[i]);
        pMax[i] = std::max<float>(pMax[i], p[i]);
    }
}

static inline void MinPoint(Point& pMin, const Point& p) {
    for (int i = 0; i < 3; i++) { pMin[i] = std::min<float>(pMin[i], p[i]); }
}

static inline void MaxPoint(Point& pMax, const Point& p) {
    for (int i = 0; i < 3; i++) { pMax[i] = std::max<float>(pMax[i], p[i]); }
}

static inline std::pair<Point, Point> CellMinMaxPoint(Cell* cell) {
    auto pMinMax = MinMaxPoint();
    auto& [pMin, pMax] = pMinMax;
    int pNum = cell->GetNumberOfPoints();
    for (int i = 0; i < pNum; i++) {
        auto& p = cell->GetPoint(i);
        MinMaxPoint(pMin, pMax, p);
    }
    return pMinMax;
}

std::set<std::pair<int, int>> CellFaceExtracter::GetExtractPointIdPairs(const std::set<igIndex>& choosedCellIds,
                                                                        UnstructuredMesh* mesh) {
    if (choosedCellIds.empty() || mesh == nullptr) return {};
    for (auto& cellId: choosedCellIds) {
        auto cell = mesh->GetCell(cellId);
        VisitCell(cellId, cell);
    }
    std::set<std::pair<int, int>> re;
    for (auto& cellId: choosedCellIds) {
        auto& faceSet = m_CellToFace[cellId];
        for (auto& face: faceSet) {
            if (m_FaceToCell[face].size() != 1) continue;
            auto& edges = m_FaceToEdge[face];
            re.insert(edges.begin(), edges.end());
        }
    }
    return re;
}

std::vector<std::pair<Point, Point>> CellFaceExtracter::GetExtractBoundingBoxs(const std::set<igIndex>& choosedCellIds,
                                                                               UnstructuredMesh* mesh) {
    if (choosedCellIds.empty() || mesh == nullptr) return {};
    for (auto& cellId: choosedCellIds) {
        auto cell = mesh->GetCell(cellId);
        VisitCell(cellId, cell);
    }
    std::vector<std::pair<Point, Point>> re;
    std::map<CellId, CellId> cellGroups;
    for (auto& cellId: choosedCellIds) { cellGroups[cellId] = cellId; }
    for (auto& cellG_: cellGroups) {
        if (cellG_.first != cellG_.second) continue;
        auto pMinMax = MinMaxPoint();
        auto& [pMin, pMax] = pMinMax;
        auto& currentCellId = cellG_.first;
        std::queue<CellId> cellIdQueue;
        cellIdQueue.push(currentCellId);
        while (!cellIdQueue.empty()) {
            auto cellId = cellIdQueue.front();
            cellIdQueue.pop();
            auto& faces = m_CellToFace[cellId];
            for (auto& face: faces) {
                auto& cellSetOfFace = m_FaceToCell[face];
                for (auto& cellIdInSetOfFace: cellSetOfFace) {
                    if (cellIdInSetOfFace == currentCellId) continue;//is currentCell
                    if (choosedCellIds.count(cellIdInSetOfFace) == 0) continue;//not choosed
                    if (cellGroups[cellIdInSetOfFace] != cellIdInSetOfFace) continue;//already visited
                    cellGroups[cellIdInSetOfFace] = currentCellId;
                    cellIdQueue.push(cellIdInSetOfFace);
                }
            }
            auto cell = mesh->GetCell(cellId);
            auto [cellPMin, cellPMax] = CellMinMaxPoint(cell);
            MinPoint(pMin, cellPMin);
            MaxPoint(pMax, cellPMax);
        }
        re.push_back(pMinMax);
    }
    return re;
}

void CellFaceExtracter::VisitCell(int cellId, Cell* cell) {
    if (cell == nullptr) return;
    if (m_CellToFace.count(cellId) != 0) return;
    _VisitCell(cellId, cell);
}

void CellFaceExtracter::_VisitCell(int cellId, Cell* cell) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 1) return;
        Face face;
        for (int pointI = 0; pointI < pointSize; pointI++) {
            auto pointId = cell->GetPointId(pointI);
            face.push_back(pointId);
        }
        SortVector(face);
        m_CellToFace[cellId].insert(face);
        m_FaceToCell[face].insert(cellId);
        if (m_FaceToEdge.count(face) == 0) {
            for (int pointI = 0; pointI < pointSize; pointI++) {
                auto pointIA = pointI;
                auto pointIB = (pointI + 1) % pointSize;
                auto pointIdA = cell->GetPointId(pointIA);
                auto pointIdB = cell->GetPointId(pointIB);
                m_FaceToEdge[face].push_back(std::minmax(pointIdA, pointIdB));
            }
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            _VisitCell(cellId, face);
        }
    }
}

void CellFaceExtracter::Clear() {
    m_CellToFace.clear();
    m_FaceToCell.clear();
    m_FaceToEdge.clear();
}

IGAME_NAMESPACE_END