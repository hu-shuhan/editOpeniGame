#include "iGameGetClosestCellsInLine.h"
#include <iGameCell.h>
#include <iGameSingleSelectionStyle.h>
IGAME_NAMESPACE_BEGIN
bool iGameGetClosestCellsInLine::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_Radius < 0) return false;
    Run();
    return true;
}

const std::vector<int>& iGameGetClosestCellsInLine::GetResult() { return m_Ids; }

void iGameGetClosestCellsInLine::Run() {
    m_Ids = SingleSelectionStyle::GetCellsInCondition(m_StartPoint, m_EndPoint, m_Mesh, m_Radius,
                                                      m_UseVariableCondition, m_VariableIndex, m_UseAutoValueRange,
                                                      m_ValueRange);
}

iGameGetClosestCellsInLine::iGameGetClosestCellsInLine(const Point& startPoint, const Point& endPoint, double radius,
                                                       bool useVariableCondition, int variableIndex,
                                                       bool useAutoValueRange, double valueRange) {
    m_StartPoint = startPoint;
    m_EndPoint = endPoint;
    m_Radius = radius;
    m_UseVariableCondition = useVariableCondition;
    m_VariableIndex = variableIndex;
    m_UseAutoValueRange = useAutoValueRange;
    m_ValueRange = valueRange;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(0);
}
IGAME_NAMESPACE_END