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

void SelectionParameter::SetAutoSelect(bool autoSelect) { m_AutoSelect = autoSelect; }

bool SelectionParameter::GetAutoSelect() const { return m_AutoSelect; }

void SelectionParameter::SetAutoSelectExpdRate(double autoSelectExpdRate) { m_AutoSelectExpdRate = autoSelectExpdRate; }

double SelectionParameter::GetAutoSelectExpdRate() const { return m_AutoSelectExpdRate; }

IGAME_NAMESPACE_END