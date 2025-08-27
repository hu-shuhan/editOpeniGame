#include "iGameSingleDragStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"

IGAME_NAMESPACE_BEGIN

SingleDragStyle::SingleDragStyle() {
    m_SelectedPointId = -1;

    m_SelectedNDCZ = 0;
    m_MVP = igm::mat4{};
    m_InvertedMVP = igm::mat4{};
}
SingleDragStyle::~SingleDragStyle() {}

void SingleDragStyle::MousePressEvent(IEvent event) {
    SelectionStyle::MousePressEvent(event);
    m_MVP = m_Interactor->GetMVP();
    m_InvertedMVP = m_MVP.invert();

    auto& pos = event.pos;
    igm::vec3 point1 = GetNearWorldCoord(pos, m_InvertedMVP);
    igm::vec3 point2 = GetFarWorldCoord(pos, m_InvertedMVP);

    igm::vec3 dir = (point1 - point2).normalized();

    Point p;
    SmartPointer<PointPicker> picker = PointPicker::New();
    picker->SetPoints(m_Points);
    m_SelectedPointId = picker->PickClosetPointOnLine(
            Vector3d(point1.x, point1.y, point1.z),
            Vector3d(dir.x, dir.y, dir.z), p);

    //m_Model->GetPointPainter()->Clear();
    if (m_SelectedPointId != -1) {
        //std::cout << "click point id: " << m_SelectedPointId << std::endl;
        auto& tp = m_Points->GetPoint(m_SelectedPointId);
        igm::vec4 p{tp[0], tp[1], tp[2], 1.f};
        p = m_MVP * p;
        m_SelectedNDCZ = p.z / p.w;

        auto painter = m_Model->GetPainter3D();
        painter->Clear();
        painter->SetPen(10);
        painter->SetPen(Color::Red);
        painter->DrawPoint(tp);
    }
}

void SingleDragStyle::MouseMoveEvent(IEvent event) {
    igm::vec2 pos = event.pos;

    if (m_MouseMode == MouseButton::LeftButton) {
        if (m_SelectedPointId == -1) { return; }

        //std::cout << "drag point id: " << m_SelectedPointId << std::endl;

        igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                      1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

        igm::vec4 Point_NDC{NDC, m_SelectedNDCZ, 1.f};
        igm::vec4 newPoint_WorldCoord = m_InvertedMVP * Point_NDC;
        newPoint_WorldCoord /= newPoint_WorldCoord.w;
        if (m_Selection) {
            Selection::Event e;
            e.type = Selection::Event::DragPoint;
            e.pickId = m_SelectedPointId;
            e.pos = Vector3f{newPoint_WorldCoord.x, newPoint_WorldCoord.y,
                             newPoint_WorldCoord.z};
            m_Selection->SelectionCallBackEvent(e);

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
