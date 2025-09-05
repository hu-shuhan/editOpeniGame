#include "iGameGenerateVariableCorrelationData.h"

IGAME_NAMESPACE_BEGIN

bool iGameGenerateVariableCorrelationData::Execute() {
    if (m_DataType != IG_POINT && m_DataType != IG_CELL) return false;
    RUN();
    SetOutput(0, m_Data);
    return true;
}

void iGameGenerateVariableCorrelationData::RUN() {
    m_Data = VariableCorrelationData::New(m_Attrs, m_DataType);
}

iGameGenerateVariableCorrelationData::iGameGenerateVariableCorrelationData(
        ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
    m_Attrs = attrs;
    m_DataType = dataType;
    SetNumberOfInputs(0);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END