#ifndef IGAMEVIS_SINGLE_SELECTION_STYLE_H
#define IGAMEVIS_SINGLE_SELECTION_STYLE_H

#include "iGameSelectionStyle.h"
#include <array>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN
class UnstructuredMesh;
class VolumeMesh;
class SurfaceMesh;
class SingleSelectionStyle : public SelectionStyle {
public:
    I_OBJECT(SingleSelectionStyle);
    static Pointer New() { return new SingleSelectionStyle; }

    void MousePressEvent(IEvent _event) override;

    void SelectPoint(igm::vec2 pos);
    void SelectCell(igm::vec2 pos);

protected:
    SingleSelectionStyle();
    ~SingleSelectionStyle() override;

private:
    std::pair<Point, Point> GetStartPointAndEndPoint(igm::vec2 pos);

public:
    //########### NORMAL SELECT ###########
    static std::vector<int>
    GetPointsInCondition(const Point& startPoint, const Point& endPoint,
                         UnstructuredMesh* mesh, double radius = 0.0,
                         bool useAutoSelect = false, int variableIndex = -1,
                         double autoSelectExpdRate = 1.0);
    static std::vector<int>
    GetPointsInCondition(const Point& startPoint, const Point& endPoint,
                         VolumeMesh* mesh, double radius = 0.0,
                         bool useAutoSelect = false, int variableIndex = -1,
                         double autoSelectExpdRate = 1.0);
    static std::vector<int>
    GetPointsInCondition(const Point& startPoint, const Point& endPoint,
                         SurfaceMesh* mesh, double radius = 0.0,
                         bool useAutoSelect = false, int variableIndex = -1,
                         double autoSelectExpdRate = 1.0);

    static std::vector<int>
    GetCellsInCondition(const Point& startPoint, const Point& endPoint,
                        UnstructuredMesh* mesh, double radius = 0.0,
                        bool useAutoSelect = false, int variableIndex = -1,
                        double autoSelectExpdRate = 1.0,
                        bool selectIgnoreUnSeeAbleCells = false,
                        bool onlySelectSeeAbleCells = false);
    static std::vector<int>
    GetCellsInCondition(const Point& startPoint, const Point& endPoint,
                        VolumeMesh* mesh, double radius = 0.0,
                        bool useAutoSelect = false, int variableIndex = -1,
                        double autoSelectExpdRate = 1.0,
                        bool selectIgnoreUnSeeAbleCells = false,
                        bool onlySelectSeeAbleCells = false);
    static std::vector<int>
    GetCellsInCondition(const Point& startPoint, const Point& endPoint,
                        SurfaceMesh* mesh, double radius = 0.0,
                        bool useAutoSelect = false, int variableIndex = -1,
                        double autoSelectExpdRate = 1.0,
                        bool selectIgnoreUnSeeAbleCells = false,
                        bool onlySelectSeeAbleCells = false);
    //########### MODE SELECT ###########
    static std::vector<int> GetPointsInRadiusMode(const Point& startPoint,
                                                  const Point& endPoint,
                                                  PointSet* mesh,
                                                  double radius = 0.0);

    //static std::vector<int>
    //GetPointsInRadiusMode(const Point& startPoint, const Point& endPoint,
    //                     UnstructuredMesh* mesh, double radius = 0.0,
    //                     bool selectIgnoreUnSeeAbleCells = false,
    //                     bool onlySelectSeeAbleCells = false);
    //static std::vector<int>
    //GetPointsInRadiusMode(const Point& startPoint, const Point& endPoint,
    //                     VolumeMesh* mesh, double radius = 0.0,
    //                     bool selectIgnoreUnSeeAbleCells = false,
    //                     bool onlySelectSeeAbleCells = false); //NEED UPDATE
    //static std::vector<int>
    //GetPointsInRadiusMode(const Point& startPoint, const Point& endPoint,
    //                     SurfaceMesh* mesh, double radius = 0.0,
    //                     bool selectIgnoreUnSeeAbleCells = false,
    //                     bool onlySelectSeeAbleCells = false); //NEED UPDATE

    static std::vector<int>
    GetCellsInRadiusMode(const Point& startPoint, const Point& endPoint,
                         UnstructuredMesh* mesh, double radius = 0.0,
                         bool selectIgnoreUnSeeAbleCells = false,
                         bool onlySelectSeeAbleCells = false);
    static std::vector<int>
    GetCellsInRadiusMode(const Point& startPoint, const Point& endPoint,
                         VolumeMesh* mesh, double radius = 0.0,
                         bool selectIgnoreUnSeeAbleCells = false,
                         bool onlySelectSeeAbleCells = false);//NEED UPDATE
    static std::vector<int>
    GetCellsInRadiusMode(const Point& startPoint, const Point& endPoint,
                         SurfaceMesh* mesh, double radius = 0.0,
                         bool selectIgnoreUnSeeAbleCells = false,
                         bool onlySelectSeeAbleCells = false);//NEED UPDATE

    static std::vector<int>
    GetPointsInCtMode(const Point& startPoint, const Point& endPoint,
                      UnstructuredMesh* mesh, double radius = 0.0,
                      int variableIndex = -1, double autoSelectExpdRate = 1.0);
    static std::vector<int>
    GetPointsInCtMode(const Point& startPoint, const Point& endPoint,
                      VolumeMesh* mesh, double radius = 0.0,
                      int variableIndex = -1, double autoSelectExpdRate = 1.0);
    static std::vector<int>
    GetPointsInCtMode(const Point& startPoint, const Point& endPoint,
                      SurfaceMesh* mesh, double radius = 0.0,
                      int variableIndex = -1, double autoSelectExpdRate = 1.0);

    static std::vector<int>
    GetCellsInCtMode(const Point& startPoint, const Point& endPoint,
                     UnstructuredMesh* mesh, double radius = 0.0,
                     int variableIndex = -1, double autoSelectExpdRate = 1.0,
                     bool selectIgnoreUnSeeAbleCells = false,
                     bool onlySelectSeeAbleCells = false);
    static std::vector<int>
    GetCellsInCtMode(const Point& startPoint, const Point& endPoint,
                     VolumeMesh* mesh, double radius = 0.0,
                     int variableIndex = -1, double autoSelectExpdRate = 1.0,
                     bool selectIgnoreUnSeeAbleCells = false,
                     bool onlySelectSeeAbleCells = false);//NEED UPDATE
    static std::vector<int>
    GetCellsInCtMode(const Point& startPoint, const Point& endPoint,
                     SurfaceMesh* mesh, double radius = 0.0,
                     int variableIndex = -1, double autoSelectExpdRate = 1.0,
                     bool selectIgnoreUnSeeAbleCells = false,
                     bool onlySelectSeeAbleCells = false);//NEED UPDATE

    static std::vector<int>
    GetPointsInBox(const std::array<std::array<Point, 4>, 6>& allFaces,
                   PointSet* mesh);

    static std::vector<int>
    GetCellsInBox(const std::array<std::array<Point, 4>, 6>& allFaces,
                  UnstructuredMesh* mesh, bool onlySelectSeeAbleCells = false);
    static std::vector<int>
    GetCellsInBox(const std::array<std::array<Point, 4>, 6>& allFaces,
                  VolumeMesh* mesh, bool onlySelectSeeAbleCells = false);//NEED UPDATE
    static std::vector<int>
    GetCellsInBox(const std::array<std::array<Point, 4>, 6>& allFaces,
                  SurfaceMesh* mesh, bool onlySelectSeeAbleCells = false);//NEED UPDATE


    //########### OTHER FUNCS ###########
    static std::vector<int>
    GetFiltedPointsOfUsingAutoValueRange(int keyPointId,
                                         const std::vector<int>& pointIds,
                                         UnstructuredMesh* mesh);
    static std::vector<int>
    GetFiltedPointsOfUsingAutoValueRange(int keyPointId,
                                         const std::vector<int>& pointIds, VolumeMesh* mesh);//NEED UPDATE
    static std::vector<int>
    GetFiltedPointsOfUsingAutoValueRange(int keyPointId,
                                         const std::vector<int>& pointIds,
                                         SurfaceMesh* mesh);//NEED UPDATE

    static std::vector<int>
    GetFiltedCellsOfUsingAutoValueRange(int keyCellId,
                                        const std::vector<int>& cellIds,
                                        UnstructuredMesh* mesh);
    static std::vector<int>
    GetFiltedCellsOfUsingAutoValueRange(int keyCellId,
                                        const std::vector<int>& cellIds, VolumeMesh* mesh);//NEED UPDATE
    static std::vector<int>
    GetFiltedCellsOfUsingAutoValueRange(int keyCellId,
                                        const std::vector<int>& cellIds, SurfaceMesh* mesh);//NEED UPDATE

    static std::vector<int> GetPointsOfCells(const std::vector<int>& cellIds,
                                             UnstructuredMesh* mesh);
    static std::vector<int> GetPointsOfCells(const std::vector<int>& cellIds,
                                             VolumeMesh* mesh);
    static std::vector<int> GetPointsOfCells(const std::vector<int>& cellIds,
                                             SurfaceMesh* mesh);
};
IGAME_NAMESPACE_END
#endif