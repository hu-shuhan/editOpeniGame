#include "iGameGetClosestPointsInLineFilter.h"
#include <iGamePointPicker.h>
#include <iGameSingleSelectionStyle.h>
IGAME_NAMESPACE_BEGIN
bool GetClosestPointsInLineFilter::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_Radius < 0) return false;
    Run();
    return true;
}

const std::vector<int>& GetClosestPointsInLineFilter::GetResult() { return m_Ids; }

void GetClosestPointsInLineFilter::Run() {
    m_Ids = SingleSelectionStyle::GetPointsInCondition(m_StartPoint, m_EndPoint, m_Mesh, m_Radius, m_UseAutoValueRange,
                                                       m_VariableIndex, m_ExpdRate);
}

GetClosestPointsInLineFilter::GetClosestPointsInLineFilter(const Point& startPoint, const Point& endPoint, double radius,
                                                         bool useAutoValueRange, int variableIndex, double expdRate) {
    m_StartPoint = startPoint;
    m_EndPoint = endPoint;
    m_Radius = radius;
    m_UseAutoValueRange = useAutoValueRange;
    m_VariableIndex = variableIndex;
    m_ExpdRate = expdRate;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(0);
}
IGAME_NAMESPACE_END