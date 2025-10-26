#ifndef IGAMEVIS_SINGLE_SELECTION_STYLE_H
#define IGAMEVIS_SINGLE_SELECTION_STYLE_H

#include "iGameSelectionStyle.h"
#include <vector>
#include <utility>

IGAME_NAMESPACE_BEGIN
class UnstructuredMesh;
class SingleSelectionStyle : public SelectionStyle {
public:
    I_OBJECT(SingleSelectionStyle);
    static Pointer New() { return new SingleSelectionStyle; }

    void MousePressEvent(IEvent _event) override;

    void SelectPoint(igm::vec2 pos);
    void SelectFace(igm::vec2 pos);

protected:
    SingleSelectionStyle();
    ~SingleSelectionStyle() override;

private:
    std::pair<Point, Point> GetStartPointAndEndPoint(igm::vec2 pos);

public:
    static std::vector<int>
    GetPointsInCondition(const Point& startPoint, const Point& endPoint,
                         UnstructuredMesh* mesh, double radius = 0.0,
                         bool useVariableCondition = false,
                         int variableIndex = -1, bool useAutoValueRange = false,
                         double valueRange = 1.0);
    static std::vector<int>
    GetCellsInCondition(const Point& startPoint, const Point& endPoint,
                        UnstructuredMesh* mesh, double radius = 0.0,
                        bool useVariableCondition = false,
                        int variableIndex = -1, bool useAutoValueRange = false,
                        double valueRange = 1.0);
    static std::vector<int>
    GetFiltedCellsOfUsingAutoValueRange(int keyCellId,
                                        const std::vector<int>& cellIds,
                                        UnstructuredMesh* mesh);
    static std::vector<int> GetPointsOfCells(const std::vector<int>& cellIds,
                                             UnstructuredMesh* mesh);
};
IGAME_NAMESPACE_END
#endif