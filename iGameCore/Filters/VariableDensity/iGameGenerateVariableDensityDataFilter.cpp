#include "iGameGenerateVariableDensityDataFilter.h"
IGAME_NAMESPACE_BEGIN
bool GenerateVariableDensityDataFilter::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_DataType != IG_POINT && m_DataType != IG_CELL) return false;
    Run();
    SetOutput(0, m_Data);
    return true;
}

void GenerateVariableDensityDataFilter::Run() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    m_Data = VariableDensityData::New(attrs, m_DataType, m_BoxNum);
}

GenerateVariableDensityDataFilter::GenerateVariableDensityDataFilter(IGenum dataType, int boxNum) {
    m_DataType = dataType;
    m_BoxNum = boxNum;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END