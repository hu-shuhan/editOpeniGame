#include "iGameGenerateParallelCoordinatesData.h"
IGAME_NAMESPACE_BEGIN

bool iGameGenerateParallelCoordinatesData::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_DataType != IG_POINT && m_DataType != IG_CELL) return false;
    RUN();
    SetOutput(0, m_Data);
    return true;
}

void iGameGenerateParallelCoordinatesData::RUN() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    m_Data = ParallelCoordinatesData::New(attrs, m_DataType);
}

iGameGenerateParallelCoordinatesData::iGameGenerateParallelCoordinatesData(IGenum dataType) {
    m_DataType = dataType;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END
