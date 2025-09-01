#include "iGameGenerateVariableDensityData.h"
IGAME_NAMESPACE_BEGIN
bool iGameGenerateVariableDensityData::Execute() {
    if (m_DataType != IG_POINT && m_DataType != IG_CELL) return false;
    RUN();
    SetOutput(0, m_Data);
    return true;
}

void iGameGenerateVariableDensityData::RUN() {
    m_Data = VariableDensityData::New(m_Attrs, m_DataType, m_BoxNum);
}

iGameGenerateVariableDensityData::iGameGenerateVariableDensityData(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                                   IGenum dataType, int boxNum) {
    m_Attrs = attrs;
    m_DataType = dataType;
    m_BoxNum = boxNum;
    SetNumberOfInputs(0);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END