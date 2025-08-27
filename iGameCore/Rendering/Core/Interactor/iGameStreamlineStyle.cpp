#include "iGameStreamlineStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"

IGAME_NAMESPACE_BEGIN

StreamLineStyle::StreamLineStyle() {

}

StreamLineStyle::~StreamLineStyle() {
    if (StartHandle != 0) m_Painter3D->Delete(StartHandle);
    if (EndHandle != 0) m_Painter3D->Delete(EndHandle);
    if (LineHandle != 0) m_Painter3D->Delete(LineHandle);
}

void StreamLineStyle::Draw() {
    vec3ToVector3d f;

    m_Painter3D->SetPen(16);
    m_Painter3D->SetPen(Color::Green);

    if (StartHandle != 0) m_Painter3D->Delete(StartHandle);
    if (EndHandle != 0) m_Painter3D->Delete(EndHandle);
    if (LineHandle != 0) m_Painter3D->Delete(LineHandle);

    StartHandle = m_Painter3D->DrawPoint(f(Start));
    EndHandle = m_Painter3D->DrawPoint(f(End));

    m_Painter3D->SetPen(4);
    m_Painter3D->SetPen(Color::Red);
    LineHandle = m_Painter3D->DrawLine(f(Start), f(End));
}

void StreamLineStyle::Emit() {
    if (!m_Selection) return;
    vec3ToVector3d f;
    m_Selection->Start = f(Start);
    m_Selection->End = f(End);
    m_Selection->Selected = Selected;
    m_Selection->SelectionCallBackEvent(
            Selection::Event(Selection::Event::Change));
}

void StreamLineStyle::Initialize(SmartPointer<Interactor> interactor,
                                 SmartPointer<Selection> s) {
    BasicStyle::Initialize(interactor);
    m_Selection = DynamicCast<StreamLineSelection>(s);
    if (m_Selection == nullptr) return;

    m_Painter3D = interactor->GetPainter3D();
    m_DataObject = interactor->GetDataObject();

    Vector3Tovec3 v;
    Start = v(m_Selection->Start);
    End = v(m_Selection->End);

    Draw();
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
    } else if (TwoLineIntersection(Start, End, point1, point2, Intersection)) {
        Selected = 2;
        auto afterMVP = MVP * igm::vec4(Intersection, 1.0f);
        NDC_Z = (afterMVP / afterMVP.w).z;
        P1 = Start - Intersection;
        P2 = End - Intersection;
    } else {
        Selected = -1;
    }
}

void StreamLineStyle::MouseMoveEvent(IEvent _event) {
    igm::vec2 pos = _event.pos;
    vec3ToVector3d f;

    if (Selected == -1) { 
        BasicStyle::MouseMoveEvent(_event);
        return;
    }

    if (m_MouseMode == MouseButton::LeftButton) {
        igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                      1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

        igm::vec4 Point_NDC{NDC, NDC_Z, 1.f};
        igm::vec4 newPoint_WorldCoord = InvertedMVP * Point_NDC;
        newPoint_WorldCoord /= newPoint_WorldCoord.w;

        if (Selected == 0) 
        { 
            Start = newPoint_WorldCoord.xyz();
        } 
        else if (Selected == 1) 
        {
            End = newPoint_WorldCoord.xyz();
        } 
        else if (Selected == 2) 
        {
            Start = newPoint_WorldCoord.xyz() + P1;
            End = newPoint_WorldCoord.xyz() + P2;
        }

        Draw();
        Emit();
    }
}

IGAME_NAMESPACE_END
