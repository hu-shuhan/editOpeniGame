#pragma once
#include <iGameMacro.h>
#include <vector>
#include <map>
#include <utility>
#include <set>
#include <iGameCell.h>
#include <mutex>
IGAME_NAMESPACE_BEGIN
class UnstructuredMesh;
class CellFaceExtracter {
public:
    void Clear();
    std::set<std::pair<int, int>> GetExtractPointIdPairs(const std::set<igIndex>& choosedCellIds,
                                                         UnstructuredMesh* mesh);
    std::vector<std::pair<Point, Point>> GetExtractBoundingBoxs(const std::set<igIndex>& choosedCellIds,
                                                                UnstructuredMesh* mesh);

private:
    using PointId=int;
    using CellId=int;
    using Face=std::vector<PointId>;
    using Edge=std::pair<PointId,PointId>;

    std::map<Face, std::set<CellId>> m_FaceToCell;
    std::map<Face, std::vector<Edge>> m_FaceToEdge;
    std::map<CellId, std::set<Face>> m_CellToFace;

private:
    void VisitCell(int cellId, Cell* cell);
    void _VisitCell(int cellId, Cell* cell);
};
IGAME_NAMESPACE_END