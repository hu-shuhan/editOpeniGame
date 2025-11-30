#include "iGameCellFaceExtracter.h"
#include <algorithm>
#include <iGameThreadPool.h>
#include <iGameTimer.h>
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
    VisitMesh(mesh);
    std::set<std::pair<int, int>> re;
    for (auto& cellId: choosedCellIds) {
        auto& faceSet = m_CellToFace[cellId];
        for (auto& face: faceSet) {
            if (m_Faces[face].Cells.empty()) continue;
            int cellNum{};
            for (auto& cId: m_Faces[face].Cells) {
                if (choosedCellIds.count(cId) == 0) continue;
                cellNum++;
            }
            if (cellNum != 1) continue;
            auto& edges = m_Faces[face].Edges;
            re.insert(edges.begin(), edges.end());
        }
    }
    return re;
}

std::vector<std::pair<Point, Point>> CellFaceExtracter::GetExtractBoundingBoxs(const std::set<igIndex>& choosedCellIds,
                                                                               UnstructuredMesh* mesh) {
    if (choosedCellIds.empty() || mesh == nullptr) return {};
    VisitMesh(mesh);
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
                auto& cellSetOfFace = m_Faces[face].Cells;
                for (auto& cellIdInSetOfFace: cellSetOfFace) {
                    if (cellIdInSetOfFace == currentCellId) continue;                 //is currentCell
                    if (choosedCellIds.count(cellIdInSetOfFace) == 0) continue;       //not choosed
                    if (cellGroups[cellIdInSetOfFace] != cellIdInSetOfFace) continue; //already visited
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

std::pair<Point, Point> CellFaceExtracter::GetCellsBoundingBox(const std::vector<igIndex>& choosedCellIds,
                                                             UnstructuredMesh* mesh) {
    if (choosedCellIds.empty() || mesh == nullptr) return {};
    VisitMesh(mesh);
    auto pMinMax = MinMaxPoint();
    auto& [pMin, pMax] = pMinMax;
    for (auto& cellId: choosedCellIds) {
        auto cell = mesh->GetCell(cellId);
        auto [cellPMin, cellPMax] = CellMinMaxPoint(cell);
        MinPoint(pMin, cellPMin);
        MaxPoint(pMax, cellPMax);
    }
    return pMinMax;
}

std::pair<Point, Point> CellFaceExtracter::GetPointsBoundingBox(const std::vector<igIndex>& choosedPointIds,
                                                                UnstructuredMesh* mesh) {
    if (choosedPointIds.empty() || mesh == nullptr) return {};
    VisitMesh(mesh);
    auto pMinMax = MinMaxPoint();
    auto& [pMin, pMax] = pMinMax;
    for (auto& pointId: choosedPointIds) {
        auto& point = mesh->GetPoint(pointId);
        MinMaxPoint(pMin, pMax, point);
    }
    return pMinMax;
}

std::vector<int> CellFaceExtracter::GetSurfaceCellIds(UnstructuredMesh* mesh) {
    if (mesh == nullptr) return {};
    VisitMesh(mesh);
    std::set<int> reSet;
    for (auto& face_: m_Faces) {
        auto& faceMsg = face_;
        if (faceMsg.Cells.size() != 1) continue;
        reSet.insert(*faceMsg.Cells.begin());
    }
    return std::vector<int>(reSet.begin(), reSet.end());
}

void CellFaceExtracter::VisitMesh(UnstructuredMesh* mesh) {
    static constexpr int PAR_THREAD_NUM_BASE = 64;
    if (mesh == nullptr) return;
    int PAR_THREAD_NUM = std::min<int>(PAR_THREAD_NUM_BASE, (mesh->GetNumberOfCells() / 10000) + 1);
    if (!m_CellToFace.empty()) return;
    m_CellToFace = std::vector<std::vector<FaceId>>(mesh->GetNumberOfCells());
    //concurrency::concurrent_vector<Face> oriFaces;
    {
        //concurrency::concurrent_unordered_map<Face, FaceId, FaceHash> tempFace;
        {
            std::vector<std::vector<std::pair<Face, Face>>> cellToPFace(
                    std::vector<std::vector<std::pair<Face, Face>>>(mesh->GetNumberOfCells()));
            iGame::ThreadPool::parallelFor(
                    0, mesh->GetNumberOfCells(),
                    [&](int st, int ed) {
                        Cell::Pointer cell;
                        for (int cellId = st; cellId < ed; cellId++) {
                            mesh->GetCell(cellId, cell);
                            _VisitCell(cellId, cell, cellToPFace);
                        }
                    },
                    PAR_THREAD_NUM);
            //concurrency::concurrent_unordered_set<Face, FaceHash> tempFaceSet;
            std::map<Face, FaceId> tempFaceSet;
            std::vector<Face> oriFaces;
            for (int cellId = 0; cellId < mesh->GetNumberOfCells(); cellId++) {
                for (auto& pFace: cellToPFace[cellId]) {
                    auto& [sFace, oriFace] = pFace;
                    if (tempFaceSet.count(sFace) != 0) continue;
                    oriFaces.push_back(oriFace);
                    tempFaceSet[sFace] = tempFaceSet.size();
                }
            }
            //iGame::ThreadPool::parallelFor(
            //        0, mesh->GetNumberOfCells(),
            //        [&](int st, int ed) {
            //            for (int cellId = st; cellId < ed; cellId++) {
            //                for (auto& pFace: cellToPFace[cellId]) {
            //                    auto& [sFace, oriFace] = pFace;
            //                    tempFaceSet.insert(sFace);
            //                }
            //            }
            //        },
            //        PAR_THREAD_NUM);
            m_Faces = std::vector<FaceMsg>(tempFaceSet.size());
            for (int cellId = 0; cellId < mesh->GetNumberOfCells(); cellId++) {
                for (auto& pFace: cellToPFace[cellId]) {
                    auto& [sFace, oriFace] = pFace;
                    int faceId = tempFaceSet[sFace];
                    m_Faces[faceId].Cells.push_back(cellId);
                    m_CellToFace[cellId].push_back(faceId);
                }
            }
            //iGame::ThreadPool::parallelFor(
            //        0, mesh->GetNumberOfCells(),
            //        [&](int st, int ed) {
            //            for (int cellId = st; cellId < ed; cellId++) {
            //                for (auto& pFace: cellToPFace[cellId]) {
            //                    auto& [sFace, oriFace] = pFace;
            //                    int faceId = tempFaceSet[sFace];
            //                    m_Faces[faceId].Cells.push_back(cellId);
            //                    m_CellToFace[cellId].push_back(faceId);
            //                }
            //            }
            //        },
            //        PAR_THREAD_NUM);
            iGame::ThreadPool::parallelFor(
                    0, m_Faces.size(),
                    [&](int st, int ed) {
                        for (int faceId = st; faceId < ed; faceId++) { _BuildFaceEdgeMsgs(faceId, oriFaces); }
                    },
                    PAR_THREAD_NUM);
            //iGame::ThreadPool::parallelFor(
            //        0, mesh->GetNumberOfCells(),
            //        [&](int st, int ed) {
            //            Cell::Pointer cell;
            //            for (int cellId = st; cellId < ed; cellId++) {
            //                for (auto& pFace: cellToPFace[cellId]) {
            //                    auto& [sFace, oriFace] = pFace;
            //                    int faceId = std::distance(tempFaceSet.begin(), tempFaceSet.find(sFace));
            //                    auto pointSize = oriFace.size();
            //                    for (int pointI = 0; pointI < pointSize; pointI++) {
            //                        auto pointIA = pointI;
            //                        auto pointIB = (pointI + 1) % pointSize;
            //                        auto pointIdA = oriFace[pointIA];
            //                        auto pointIdB = oriFace[pointIB];
            //                        m_Faces[faceId].Edges.push_back(std::minmax(pointIdA, pointIdB));
            //                    }
            //                }
            //            }
            //        },
            //        PAR_THREAD_NUM);


            //for (int cellId = 0; cellId < mesh->GetNumberOfCells(); cellId++) {
            //    for (auto& pFace: cellToPFace[cellId]) {
            //        auto& [sFace, oriFace] = pFace;
            //        int faceId{};
            //        if (tempFace.count(sFace) == 0) {
            //            faceId = tempFace.size();
            //            tempFace[sFace] = faceId;
            //            m_Faces.push_back(FaceMsg());
            //            oriFaces.push_back(oriFace);
            //        } else {
            //            faceId = tempFace[sFace];
            //        }
            //        m_Faces[faceId].Cells.insert(cellId);
            //        m_CellToFace[cellId].push_back(faceId);
            //    }
            //}
        }
    }
}

void CellFaceExtracter::_VisitCell(int cellId, Cell* cell,
                                   std::vector<std::vector<std::pair<Face, Face>>>& cellToPFace) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 1) return;
        Face face;
        for (int pointI = 0; pointI < pointSize; pointI++) {
            auto pointId = cell->GetPointId(pointI);
            face.push_back(pointId);
        }
        Face oriFace = face;
        SortVector(face);
        cellToPFace[cellId].push_back({face, oriFace});
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            _VisitCell(cellId, face, cellToPFace);
        }
    }
}

void CellFaceExtracter::_BuildFaceEdgeMsgs(FaceId id, std::vector<Face>& oriFaces) {
    if (!m_Faces[id].Edges.empty()) return;
    auto pointSize = oriFaces[id].size();
    Face& face = oriFaces[id];
    for (int pointI = 0; pointI < pointSize; pointI++) {
        auto pointIA = pointI;
        auto pointIB = (pointI + 1) % pointSize;
        auto pointIdA = face[pointIA];
        auto pointIdB = face[pointIB];
        m_Faces[id].Edges.push_back(std::minmax(pointIdA, pointIdB));
    }
}

void CellFaceExtracter::Clear() {
    m_CellToFace.clear();
    m_Faces.clear();
}

IGAME_NAMESPACE_END