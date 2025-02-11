#ifndef IGAMEVIS_SINGLE_DRAG_STYLE_H
#define IGAMEVIS_SINGLE_DRAG_STYLE_H

#include "iGamePointPicker.h"
#include "iGameSelectionStyle.h"

IGAME_NAMESPACE_BEGIN
class SingleDragStyle : public SelectionStyle {
public:
    I_OBJECT(SingleDragStyle);
    static Pointer New() { return new SingleDragStyle; }

    void MousePressEvent(IEvent event) override;
    void MouseMoveEvent(IEvent event) override;

protected:
    SingleDragStyle();
    ~SingleDragStyle() override;

    igIndex m_SelectedPointId;

    float m_SelectedNDCZ;
    igm::mat4 m_MVP;
    igm::mat4 m_InvertedMVP;
};
IGAME_NAMESPACE_END
#endif