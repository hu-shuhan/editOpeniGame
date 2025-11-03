#include "iGameSetPointsSelect.h"
#include <iostream>
IGAME_NAMESPACE_BEGIN
bool iGameSetPointsSelect::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_Operate != Selection::Operate::Add && m_Operate != Selection::Operate::Remove) return false;
    Run();
    SetOutput(0, m_Mesh);
    return true;
}

void iGameSetPointsSelect::Run() {
    auto selection = m_Mesh->GetSelection();
    m_Model->GetSelection()->SelectionCallBackEvent(IG_POINT, m_Ids, m_Operate);
}

iGameSetPointsSelect::iGameSetPointsSelect(Selection::Operate ope, const std::vector<int>& ids,
                                           Painter3D* painter) {
    m_Operate = ope;
    m_Ids = ids;
    m_Painter = painter;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END