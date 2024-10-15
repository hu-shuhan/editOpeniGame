#ifndef OPENIGAME_SELECTION_STYLE_H
#define OPENIGAME_SELECTION_STYLE_H

#include "iGameBasicStyle.h"
#include "iGameCellArray.h"

IGAME_NAMESPACE_BEGIN
class SelectionStyle : public BasicStyle {
public:
    I_OBJECT(SelectionStyle);
    static Pointer New() { return new SelectionStyle; }

    enum SelectedType{ 
        None = -1,
        SelectPoint, 
        SelectCell
    };
    void SetSelectedType(SelectedType type);
    SelectedType GetSelectedType() const;

    void Initialize(Interactor* a, Selection* s);

    void MousePressEvent(IEvent _event) override;

    void LeftButtonMouseMove() override;
    void RightButtonMouseMove() override;
    void MiddleButtonMouseMove() override;

protected:
    SelectionStyle() = default;
    ~SelectionStyle() override = default;

    SelectedType m_Type{SelectedType::None};

    Points* m_Points{nullptr};
    CellArray* m_Cells{nullptr};
    Model* m_Model{nullptr};
    Selection* m_Selection{nullptr};
};
IGAME_NAMESPACE_END
#endif