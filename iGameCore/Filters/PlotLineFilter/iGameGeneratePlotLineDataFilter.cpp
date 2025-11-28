#include "iGameGeneratePlotLineDataFilter.h"
static constexpr int MIN_H = 0, MAX_H = 360, MIN_S = 100, MAX_S = 255;
IGAME_NAMESPACE_BEGIN
bool GeneratePlotLineDataFilter::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_DataType != IG_POINT && m_DataType != IG_CELL) return false;
    Run();
    SetOutput(0, m_Data);
    return true;
}

void GeneratePlotLineDataFilter::Run() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    m_Data = PlotLineData::New(attrs, m_DataType, MIN_H, MAX_H, MIN_S, MAX_S);
    m_Data->SetRadialData(attrs, m_StartPoint, m_EndPoint, m_Mesh);
}

GeneratePlotLineDataFilter::GeneratePlotLineDataFilter(IGenum dataType, const Point& startPoint, const Point& endPoint) {
    m_DataType = dataType;
    m_StartPoint = startPoint;
    m_EndPoint = endPoint;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END