#include "iGameSelectionParameter.h"

IGAME_NAMESPACE_BEGIN
SelectionParameter& SelectionParameter::Instance() {
    static SelectionParameter I;
    return I;
}

void SelectionParameter::SetSelectionRadius(double selectionRadius) { m_SelectRadius = selectionRadius; }
double SelectionParameter::GetSelectionRadius() const { return m_SelectRadius; }

void SelectionParameter::SetSelectIgnoreUnSeeAbleCells(bool selectIgnoreUnSeeAbleCells) {
    m_SelectIgnoreUnSeeAbleCells = selectIgnoreUnSeeAbleCells;
}
bool SelectionParameter::GetSelectIgnoreUnSeeAbleCells() const { return m_SelectIgnoreUnSeeAbleCells; }

void SelectionParameter::SetSelectOnlySelectSeeAbleCells(bool selectOnlySelectSeeAbleCells) {
    m_SelectOnlySelectSeeAbleCells = selectOnlySelectSeeAbleCells;
}
bool SelectionParameter::GetSelectOnlySelectSeeAbleCells() { return m_SelectOnlySelectSeeAbleCells; }

void SelectionParameter::SetSelectOrUnSelect(bool selectOrUnSelect) { m_SelectOrUnSelect = selectOrUnSelect; }
bool SelectionParameter::GetSelectOrUnSelect() const { return m_SelectOrUnSelect; }

void SelectionParameter::SetSelectVariableIndex(int selectVariableIndex) {
    m_SelectVariableIndex = selectVariableIndex;
}
int SelectionParameter::GetSelectVariableIndex() const { return m_SelectVariableIndex; }

void SelectionParameter::SetSelectVariableRange(double selectVariableRange) {
    m_SelectVariableRange = selectVariableRange;
}
double SelectionParameter::GetSelectVariableRange() const { return m_SelectVariableRange; }
IGAME_NAMESPACE_END