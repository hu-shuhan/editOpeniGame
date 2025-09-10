#include "iGameGetClosestPointsInLine.h"
#include <iGamePointPicker.h>
IGAME_NAMESPACE_BEGIN
bool iGameGetClosestPointsInLine::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_Radius < 0) return false;
    Run();
    return true;
}

const std::vector<int>& iGameGetClosestPointsInLine::GetResult() { return m_Ids; }

void iGameGetClosestPointsInLine::Run() {
    m_Ids.clear();
    SmartPointer<PointPicker> picker = PointPicker::New();
    picker->SetDataObject(m_Mesh);
    Point p;
    auto id = picker->PickClosetPointOnLine(m_StartPoint, (m_EndPoint - m_StartPoint), p);
    if (m_Radius == 0) {
        m_Ids.push_back(id);
        return;
    }
    for (int pointId = 0; pointId < m_Mesh->GetNumberOfPoints(); pointId++) {
        auto& thisPoint = m_Mesh->GetPoint(pointId);
        if ((thisPoint - p).length() <= m_Radius) m_Ids.push_back(pointId);
    }
}

iGameGetClosestPointsInLine::iGameGetClosestPointsInLine(const Point& startPoint, const Point& endPoint,
                                                         double radius) {
    m_StartPoint = startPoint;
    m_EndPoint = endPoint;
    m_Radius = radius;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(0);
}
IGAME_NAMESPACE_END