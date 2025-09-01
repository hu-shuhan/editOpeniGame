#include "iGameGeneratePlotLineData.h"
static constexpr int MIN_H = 0, MAX_H = 360, MIN_S = 100, MAX_S = 255;
IGAME_NAMESPACE_BEGIN
bool iGameGeneratePlotLineData::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_DataType != IG_POINT && m_DataType != IG_CELL) return false;
    RUN();
    SetOutput(0, m_Data);
    return true;
}

void iGameGeneratePlotLineData::RUN() {
    m_Data = DataChangeData::New(m_Attrs, m_DataType, MIN_H, MAX_H, MIN_S, MAX_S);
    m_Data->SetRadialData(m_Attrs, m_StartPoint, m_EndPoint, m_Mesh);
}

iGameGeneratePlotLineData::iGameGeneratePlotLineData(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                     IGenum dataType, const Point& startPoint, const Point& endPoint) {
    m_Attrs = attrs;
    m_DataType = dataType;
    m_StartPoint = startPoint;
    m_EndPoint = endPoint;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END