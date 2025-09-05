#include "iGameSetCellsSelect.h"
IGAME_NAMESPACE_BEGIN
bool iGameSetCellsSelect::Execute() {
    if (m_Operate != Selection::Event::Add || m_Operate != Selection::Event::Remove) return false;
    if (m_Points == nullptr || m_CellArrays == nullptr) return false;
    RUN();
    return true;
}

const std::vector<Selection::Event>& iGameSetCellsSelect::GetResult() { return m_Events; }

void iGameSetCellsSelect::RUN() {
    m_Events = Selection::GenerateEvents(m_Ids, IG_CELL, m_Operate, m_Points, m_CellArrays, m_Painter);
    m_Selection->SelectionCallBackEvent(m_Events);
}

iGameSetCellsSelect::iGameSetCellsSelect(Selection::Pointer selection, Selection::Event::Operate ope,
                                         const std::vector<int>& ids, Points* points, CellArray* cellArrays,
                                         Painter3D* painter) {
    m_Selection = selection;
    m_Operate = ope;
    m_Ids = ids;
    m_Points = points;
    m_CellArrays = cellArrays;
    m_Painter = painter;
    SetNumberOfInputs(0);
    SetNumberOfOutputs(0);
}
IGAME_NAMESPACE_END