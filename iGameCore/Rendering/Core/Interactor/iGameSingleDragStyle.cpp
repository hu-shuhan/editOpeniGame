#include "iGameSingleDragStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"

IGAME_NAMESPACE_BEGIN
void SingleDragStyle::MousePressEvent(IEvent _event) {
    SelectionStyle::MousePressEvent(_event);
    MVP = m_Interactor->GetMVP();
    InvertedMVP = MVP.invert();

    auto& pos = _event.pos;
    igm::vec3 point1 = GetNearWorldCoord(pos, InvertedMVP);
    igm::vec3 point2 = GetFarWorldCoord(pos, InvertedMVP);

    igm::vec3 dir = (point1 - point2).normalized();

    Point p;
    PointPicker::Pointer picker = PointPicker::New();
    picker->SetPoints(m_Points);
    m_SelectedPointId = picker->PickClosetPointOnLine(
            Vector3d(point1.x, point1.y, point1.z),
            Vector3d(dir.x, dir.y, dir.z), p);

    //m_Model->GetPointPainter()->Clear();
    if (m_SelectedPointId != -1) {
        //std::cout << "click point id: " << m_SelectedPointId << std::endl;
        auto& tp = m_Points->GetPoint(m_SelectedPointId);
        igm::vec4 p{tp[0], tp[1], tp[2], 1.f};
        p = MVP * p;
        Selected_NDC_Z = p.z / p.w;

        auto painter = m_Model->GetPainter3D();
        painter->Clear();
        painter->SetPen(10);
        painter->SetPen(Color::Red);
        painter->DrawPoint(tp);
    }
}
void SingleDragStyle::MouseMoveEvent(IEvent _event) {
    igm::vec2 pos = _event.pos;

    if (m_MouseMode == MouseButton::LeftButton) {
        if (m_SelectedPointId == -1) { return; }

        //std::cout << "drag point id: " << m_SelectedPointId << std::endl;

        igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                      1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

        igm::vec4 Point_NDC{NDC, Selected_NDC_Z, 1.f};
        igm::vec4 newPoint_WorldCoord = InvertedMVP * Point_NDC;
        newPoint_WorldCoord /= newPoint_WorldCoord.w;
        if (m_Selection) {
            Selection::Event e;
            e.type = Selection::Event::DragPoint;
            e.pickId = m_SelectedPointId;
            e.pos = Vector3f{newPoint_WorldCoord.x, newPoint_WorldCoord.y,
                             newPoint_WorldCoord.z};
            m_Selection->FilterEvent(e);

            m_Points->SetPoint(m_SelectedPointId, e.pos);

            // updating point coordinates requires a re-conversion
            auto drawObject = DynamicCast<DrawObject>(m_Model->GetDataObject());

            //m_Model->GetPointPainter()->Clear();
            auto painter = m_Model->GetPainter3D();
            painter->Clear();
            painter->SetPen(10);
            painter->SetPen(Color::Red);
            painter->DrawPoint(e.pos);
        }
    }
}
IGAME_NAMESPACE_END
