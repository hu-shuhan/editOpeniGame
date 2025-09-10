#include "iGameSetPointsSelect.h"
#include <iostream>
IGAME_NAMESPACE_BEGIN
bool iGameSetPointsSelect::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_Operate != Selection::Event::Operate::Add && m_Operate != Selection::Event::Operate::Remove) return false;
    Run();
    SetOutput(0, m_Mesh);
    return true;
}

void iGameSetPointsSelect::Run() {
    auto selection = m_Mesh->GetSelection();
    auto Events =
            Selection::GenerateEvents(m_Ids, IG_POINT, m_Operate, m_Mesh->GetPoints(), m_Mesh->GetCells(), m_Painter);
    selection->SelectionCallBackEvent(Events);
}

iGameSetPointsSelect::iGameSetPointsSelect(Selection::Event::Operate ope, const std::vector<int>& ids,
                                           Painter3D* painter) {
    m_Operate = ope;
    m_Ids = ids;
    m_Painter = painter;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END