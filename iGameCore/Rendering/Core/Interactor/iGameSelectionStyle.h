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
    void SetSelectOrUnSelect(bool select = true);
    double GetSelectRadius() const;

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
    bool m_Select_OR_UnSelect{true};

    SmartPointer<Points> m_Points;
    SmartPointer<CellArray> m_Cells;
    SmartPointer<Model> m_Model;
    SmartPointer<Selection> m_Selection;
};
IGAME_NAMESPACE_END
#endif