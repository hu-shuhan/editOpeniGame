#pragma once
#include <iGameMacro.h>
IGAME_NAMESPACE_BEGIN
class SelectionParameter {
private:
    SelectionParameter() = default;

public:
    static SelectionParameter& Instance();

    void SetSelectionRadius(double selectionRadius);
    double GetSelectionRadius() const;

    void SetSelectIgnoreUnSeeAbleCells(bool selectIgnoreUnSeeAbleCells);
    bool GetSelectIgnoreUnSeeAbleCells() const;

    void SetSelectOrUnSelect(bool selectOrUnSelect);
    bool GetSelectOrUnSelect() const;

    void SetSelectVariableIndex(int selectVariableIndex);
    int GetSelectVariableIndex() const;

    void SetSelectVariableRange(double selectVariableRange);
    double GetSelectVariableRange() const;

private:
    //When selecting points or faces, select the radius at one time.
    double m_SelectRadius{};
    bool m_SelectIgnoreUnSeeAbleCells{false};
    //true means select. false means unselect
    bool m_SelectOrUnSelect{true};
    //In context selection, the subscript of the variable that is based on.
    //A value of -1 indicates that no context selection is made.
    int m_SelectVariableIndex{-1};
    //In context selection, the range of selected variables.
    //When it is 1, it means all the values from the minimum value to the maximum value.
    //Under normal circumstances, the minimum is 0.
    //When it is -1, it means automatic judgment.
    double m_SelectVariableRange{1};
};

IGAME_NAMESPACE_END