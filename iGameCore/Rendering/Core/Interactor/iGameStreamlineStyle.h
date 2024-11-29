#ifndef OPENIGAME_STREAMLINE_DRAG_STYLE_H
#define OPENIGAME_STREAMLINE_DRAG_STYLE_H

#include "iGamePointPicker.h"
#include "iGameSelectionStyle.h"

IGAME_NAMESPACE_BEGIN
class StreamDragStyle : public SelectionStyle {
public:
    I_OBJECT(StreamDragStyle);
    static Pointer New() { return new StreamDragStyle; }

    void MousePressEvent(IEvent _event) override;
    void MouseMoveEvent(IEvent _event) override;

protected:
    StreamDragStyle() = default;
    ~StreamDragStyle() override = default;

    igIndex m_SelectedPointId{-1};

    float Selected_NDC_Z{0};
    igm::mat4 MVP{};
    igm::mat4 InvertedMVP{};
};
IGAME_NAMESPACE_END
#endif