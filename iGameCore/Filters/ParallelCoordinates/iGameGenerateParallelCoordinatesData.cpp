#include "iGameGenerateParallelCoordinatesData.h"
IGAME_NAMESPACE_BEGIN

bool GenerateParallelCoordinatesDataFilter::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_DataType != IG_POINT && m_DataType != IG_CELL) return false;
    Run();
    SetOutput(0, m_Data);
    return true;
}

void GenerateParallelCoordinatesDataFilter::Run() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    m_Data = ParallelCoordinatesData::New(attrs, m_DataType);
}

GenerateParallelCoordinatesDataFilter::GenerateParallelCoordinatesDataFilter(IGenum dataType) {
    m_DataType = dataType;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END
