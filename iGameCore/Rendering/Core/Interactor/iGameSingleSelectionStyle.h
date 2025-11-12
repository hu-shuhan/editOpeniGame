#ifndef IGAMEVIS_SINGLE_SELECTION_STYLE_H
#define IGAMEVIS_SINGLE_SELECTION_STYLE_H

#include "iGameSelectionStyle.h"
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN
class UnstructuredMesh;
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
    GetCellsInCondition(const Point& startPoint, const Point& endPoint,
                        UnstructuredMesh* mesh, double radius = 0.0,
                        bool useAutoSelect = false, int variableIndex = -1,
                        double autoSelectExpdRate = 1.0,
                        bool selectIgnoreUnSeeAbleCells = false,
                        bool onlySelectSeeAbleCells = false);
    //########### MODE SELECT ###########
    static std::vector<int> GetPointsInRadiusMode(const Point& startPoint,
                                                  const Point& endPoint,
                                                  UnstructuredMesh* mesh,
                                                  double radius = 0.0);

    static std::vector<int>
    GetCellsInRadiusMode(const Point& startPoint, const Point& endPoint,
                         UnstructuredMesh* mesh, double radius = 0.0,
                         bool selectIgnoreUnSeeAbleCells = false,
                         bool onlySelectSeeAbleCells = false);

    static std::vector<int>
    GetPointsInCtMode(const Point& startPoint, const Point& endPoint,
                      UnstructuredMesh* mesh, double radius = 0.0,
                      int variableIndex = -1, double autoSelectExpdRate = 1.0);

    static std::vector<int>
    GetCellsInCtMode(const Point& startPoint, const Point& endPoint,
                     UnstructuredMesh* mesh, double radius = 0.0,
                     int variableIndex = -1, double autoSelectExpdRate = 1.0,
                     bool selectIgnoreUnSeeAbleCells = false,
                     bool onlySelectSeeAbleCells = false);




    
    //########### OTHER FUNCS ###########
    static std::vector<int>
    GetFiltedPointsOfUsingAutoValueRange(int keyPointId,
                                         const std::vector<int>& pointIds,
                                         UnstructuredMesh* mesh);
    static std::vector<int>
    GetFiltedCellsOfUsingAutoValueRange(int keyCellId,
                                        const std::vector<int>& cellIds,
                                        UnstructuredMesh* mesh);
    static std::vector<int> GetPointsOfCells(const std::vector<int>& cellIds,
                                             UnstructuredMesh* mesh);
};
IGAME_NAMESPACE_END
#endif