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

    void SetSelectOnlySelectSeeAbleCells(bool selectOnlySelectSeeAbleCells);
    bool GetSelectOnlySelectSeeAbleCells();

    void SetSelectOrUnSelect(bool selectOrUnSelect);
    bool GetSelectOrUnSelect() const;

    void SetSelectVariableIndex(int selectVariableIndex);
    int GetSelectVariableIndex() const;

    void SetAutoSelect(bool autoSelect);
    bool GetAutoSelect() const;

    void SetAutoSelectExpdRate(double autoSelectExpdRate);
    double GetAutoSelectExpdRate() const;

private:
    //When selecting points or faces, select the radius at one time.
    double m_SelectRadius{};
    bool m_SelectIgnoreUnSeeAbleCells{false};
    bool m_SelectOnlySelectSeeAbleCells{false};
    //true means select. false means unselect
    bool m_SelectOrUnSelect{true};
    //Use Auto Select
    bool m_AutoSelect{false};
    //In context selection, the subscript of the variable that is based on.
    //A value of -1 indicates that no context selection is made.
    int m_SelectVariableIndex{-1};
    //Auto Select Expanding Rate
    double m_AutoSelectExpdRate{1.0};
};

IGAME_NAMESPACE_END