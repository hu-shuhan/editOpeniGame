#include "iGameSetCellsSelect.h"
IGAME_NAMESPACE_BEGIN
bool iGameSetCellsSelect::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_Operate != Selection::Event::Add || m_Operate != Selection::Event::Remove) return false;
    if (m_Points == nullptr || m_CellArrays == nullptr) return false;
    RUN();
    SetOutput(0, m_Mesh);
    return true;
}

void iGameSetCellsSelect::RUN() {
    auto selection = m_Mesh->GetSelection();
    auto Events = Selection::GenerateEvents(m_Ids, IG_CELL, m_Operate, m_Points, m_CellArrays, m_Painter);
    selection->SelectionCallBackEvent(Events);
}

iGameSetCellsSelect::iGameSetCellsSelect(Selection::Event::Operate ope, const std::vector<int>& ids, Points* points,
                                         CellArray* cellArrays, Painter3D* painter) {
    m_Operate = ope;
    m_Ids = ids;
    m_Points = points;
    m_CellArrays = cellArrays;
    m_Painter = painter;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END