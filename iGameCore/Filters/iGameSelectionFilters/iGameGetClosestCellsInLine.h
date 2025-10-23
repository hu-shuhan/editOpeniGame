#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGamePoints.h>
#include <iGameUnstructuredMesh.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class iGameGetClosestCellsInLine : public Filter {
public:
    I_OBJECT(iGameGetClosestCellsInLine);
    static Pointer New(const Point& startPoint, const Point& endPoint, double radius = 0.0,
                       bool useVariableCondition = false, int variableIndex = -1, bool useAutoValueRange = false,
                       double valueRange = 1.0) {
        return new iGameGetClosestCellsInLine(startPoint, endPoint, radius, useVariableCondition, variableIndex,
                                              useAutoValueRange, valueRange);
    }
    bool Execute() override;
    const std::vector<int>& GetResult();

private:
    void Run();

protected:
    iGameGetClosestCellsInLine(const Point& startPoint, const Point& endPoint, double radius = 0.0,
                               bool useVariableCondition = false, int variableIndex = -1,
                               bool useAutoValueRange = false, double valueRange = 1.0);
    ~iGameGetClosestCellsInLine() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    Point m_StartPoint, m_EndPoint;
    double m_Radius{};
    bool m_UseVariableCondition{false};
    int m_VariableIndex{-1};
    bool m_UseAutoValueRange{false};
    double m_ValueRange{1.0};

private:
    /* Output */
    std::vector<int> m_Ids;
};
IGAME_NAMESPACE_END