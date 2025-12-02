#pragma once
#include <iGameCell.h>
#include <iGameMacro.h>
#include <map>
#include <mutex>
#include <set>
#include <utility>
#include <vector>
IGAME_NAMESPACE_BEGIN
class UnstructuredMesh;
class VolumeMesh;
class SurfaceMesh;
class CellFaceExtracter {
public:
    void PreVisit(UnstructuredMesh* mesh);
    void Clear();
    std::set<std::pair<int, int>> GetExtractPointIdPairs(const std::set<igIndex>& choosedCellIds,
                                                         UnstructuredMesh* mesh);
    std::vector<std::pair<Point, Point>> GetExtractBoundingBoxs(const std::set<igIndex>& choosedCellIds,
                                                                UnstructuredMesh* mesh);
    std::pair<Point, Point> GetCellsBoundingBox(const std::vector<igIndex>& choosedCellIds, UnstructuredMesh* mesh);
    std::pair<Point, Point> GetPointsBoundingBox(const std::vector<igIndex>& choosedPointIds, UnstructuredMesh* mesh);
    std::vector<int> GetSurfaceCellIds(UnstructuredMesh* mesh);
    std::vector<int> GetSurfaceCellIds(VolumeMesh* mesh);
    std::vector<int> GetSurfaceCellIds(SurfaceMesh* mesh);

private:
    using PointId = int;
    using CellId = int;
    using FaceId = int;
    using Face = std::vector<PointId>;
    using Edge = std::pair<PointId, PointId>;
    class FaceMsg {
    public:
        std::vector<CellId> Cells;
        std::vector<Edge> Edges;
    };
    std::vector<FaceMsg> m_Faces;
    std::vector<std::vector<FaceId>> m_CellToFace;

private:
    void VisitMesh(UnstructuredMesh* mesh);

    void _VisitCell(int cellId, Cell* cell, std::vector<std::vector<std::pair<Face, Face>>>& cellToPFace);
    void _BuildFaceEdgeMsgs(FaceId id, std::vector<Face>& oriFaces);
};
IGAME_NAMESPACE_END