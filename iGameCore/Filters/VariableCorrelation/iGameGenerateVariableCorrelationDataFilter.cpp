#include "iGameGenerateVariableCorrelationDataFilter.h"

IGAME_NAMESPACE_BEGIN

bool GenerateVariableCorrelationDataFilter::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_DataType != IG_POINT && m_DataType != IG_CELL) return false;
    Run();
    SetOutput(0, m_Data);
    return true;
}

void GenerateVariableCorrelationDataFilter::Run() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    m_Data = VariableCorrelationData::New(attrs, m_DataType);
}

GenerateVariableCorrelationDataFilter::GenerateVariableCorrelationDataFilter(IGenum dataType) {
    m_DataType = dataType;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END
