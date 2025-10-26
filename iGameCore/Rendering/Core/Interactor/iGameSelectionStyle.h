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

    SmartPointer<Points> m_Points;
    SmartPointer<CellArray> m_Cells;
    SmartPointer<Model> m_Model;
    SmartPointer<Selection> m_Selection;
};
IGAME_NAMESPACE_END
#endif