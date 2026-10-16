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

    enum class ConstraintAxis { FreePlane = 0, X = 1, Y = 2, Z = 3 };
    void SetConstraintAxis(ConstraintAxis axis) { m_ConstraintAxis = axis; }

protected:
    SingleDragStyle();
    ~SingleDragStyle() override;

    bool GetAxisDragParameter(const igm::vec2& mousePosition,
                              const Point& axisOrigin,
                              int component,
                              float& parameter);

    igIndex m_SelectedPointId;

    float m_SelectedNDCZ;
    igm::mat4 m_MVP;
    igm::mat4 m_InvertedMVP;
    igm::vec2 m_LastMousePosition;
    igm::vec3 m_DragDepthDirection;
    ConstraintAxis m_ConstraintAxis;
    Point m_AxisDragStartPoint;
    float m_AxisDragStartParameter;
    bool m_AxisDragReady;
};
IGAME_NAMESPACE_END
#endif
