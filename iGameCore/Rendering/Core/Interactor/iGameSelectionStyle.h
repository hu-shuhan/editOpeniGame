#ifndef IGAMEVIS_SELECTION_STYLE_H
#define IGAMEVIS_SELECTION_STYLE_H

#include "iGameBasicStyle.h"
#include "iGameCellArray.h"
#include "iGameSelection.h"

IGAME_NAMESPACE_BEGIN
class SelectionStyle : public BasicStyle {
public:
    I_OBJECT(SelectionStyle);
    static Pointer New() { return new SelectionStyle; }

    enum SelectedType { None = -1, SelectPoint, SelectCell };
    void SetSelectedType(SelectedType type);
    SelectedType GetSelectedType() const;
    void SetSelectRadius(double selectRadius);
    double GetSelectRadius() const;
    void SetSelectOrUnSelect(bool select = true);
    void SetSelectVairableIndex(int variableIndex = -1);
    int GetSelectVariableIndex() const;
    void SetSelectVariableRange(double variableRange = 1);
    double GetSelectVariableRange() const;

    void Initialize(SmartPointer<Interactor> interactor,
                    SmartPointer<Selection> selection);

    void MousePressEvent(IEvent event) override;

    void LeftButtonMouseMove() override;
    void RightButtonMouseMove() override;
    void MiddleButtonMouseMove() override;

protected:
    SelectionStyle();
    ~SelectionStyle() override;

    SelectedType m_Type;
    //When selecting points or faces, select the radius at one time.
    double m_SelectRadius{};
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

    SmartPointer<Points> m_Points;
    SmartPointer<CellArray> m_Cells;
    SmartPointer<Model> m_Model;
    SmartPointer<Selection> m_Selection;
};
IGAME_NAMESPACE_END
#endif