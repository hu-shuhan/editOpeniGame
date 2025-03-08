#include "iGameStreamLineStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"

IGAME_NAMESPACE_BEGIN

StreamLineStyle::StreamLineStyle() {

}

StreamLineStyle::~StreamLineStyle() {}

void StreamLineStyle::Initialize(SmartPointer<Interactor> interactor) {
    m_Painter3D = interactor->GetPainter3D();
    m_DataObject = interactor->GetDataObject();
    Vector3Tovec3 v;
    auto& bbox = m_DataObject->GetBoundingBox();
    Start = v(bbox.min);
    End = v(bbox.max);

    vec3ToVector3d f;

    m_Painter3D->SetPen(16);
    m_Painter3D->SetPen(Color::Green);
    StartHandle = m_Painter3D->DrawPoint(f(Start));
    EndHandle = m_Painter3D->DrawPoint(f(End));

    m_Painter3D->SetPen(4);
    m_Painter3D->SetPen(Color::Red);
    LineHandle = m_Painter3D->DrawLine(f(Start), f(End));
}

void StreamLineStyle::MousePressEvent(IEvent _event) {
    BasicStyle::MousePressEvent(_event);

    MVP = m_Interactor->GetMVP();
    InvertedMVP = MVP.invert();

    auto& pos = _event.pos;
    igm::vec3 point1 = GetNearWorldCoord(pos, InvertedMVP);
    igm::vec3 point2 = GetFarWorldCoord(pos, InvertedMVP);

    igm::vec3 dir = (point1 - point2).normalized();

    if (DistancePointToLine(Start, point1, point2) < 0.1) { 
        Selected = 0;
        auto afterMVP = MVP * igm::vec4(Start, 1.0f);
        NDC_Z = (afterMVP / afterMVP.w).z;
    } else if (DistancePointToLine(End, point1, point2) < 0.1) {
        Selected = 1;
        auto afterMVP = MVP * igm::vec4(End, 1.0f);
        NDC_Z = (afterMVP / afterMVP.w).z;
    } else {
        Selected = -1;
    }
}

void StreamLineStyle::MouseMoveEvent(IEvent _event) {
    igm::vec2 pos = _event.pos;
    vec3ToVector3d f;

    if (m_MouseMode == MouseButton::LeftButton) {
        if (Selected == -1) { return; }

        igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                      1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

        igm::vec4 Point_NDC{NDC, NDC_Z, 1.f};
        igm::vec4 newPoint_WorldCoord = InvertedMVP * Point_NDC;
        newPoint_WorldCoord /= newPoint_WorldCoord.w;

        m_Painter3D->SetPen(16);
        m_Painter3D->SetPen(Color::Green);
        if (Selected == 0) { 
            Start = newPoint_WorldCoord.xyz();
            if (StartHandle != 0) m_Painter3D->Delete(StartHandle);
            StartHandle = m_Painter3D->DrawPoint(f(Start));
        } else if (Selected == 1) {
            
            End = newPoint_WorldCoord.xyz();
            if (EndHandle != 0) m_Painter3D->Delete(EndHandle);
            EndHandle = m_Painter3D->DrawPoint(f(End));
        }
        
        m_Painter3D->SetPen(4);
        m_Painter3D->SetPen(Color::Red);
        LineHandle = m_Painter3D->DrawLine(f(Start), f(End));
    }
}
IGAME_NAMESPACE_END
