#include "iGameRadialStyle.h"
#include "iGameInteractor.h"
#include "iGameLine.h"
#include "iGameScene.h"
using namespace std;
IGAME_NAMESPACE_BEGIN

float RadialStyle::CalculatePickTolerance() {


    const float pixelTolerance = 5.0f;

    float ndcToleranceX = 2.0f * pixelTolerance / m_Interactor->GetWidth();
    float ndcToleranceY = 2.0f * pixelTolerance / m_Interactor->GetHeight();
    float ndcTolerance = std::max(ndcToleranceX, ndcToleranceY);

    igm::vec4 nearPlanePoint{0, 0, -1, 1};
    igm::vec4 farPlanePoint{0, 0, 1, 1};

    igm::vec4 nearWorld = m_InvertedMVP * nearPlanePoint;
    nearWorld /= nearWorld.w;

    igm::vec4 farWorld = m_InvertedMVP * farPlanePoint;
    farWorld /= farWorld.w;

    float depthScale = (farWorld - nearWorld).length() / 2.0f;

    return ndcTolerance * depthScale;
}

void RadialStyle::DrawRadial() {
    if (!m_Show) return;
    auto painter = m_Scene->GetPainter3D();

    painter->SetPen(7);
    painter->SetPen(Color::Blue);
    int startPointHandle = painter->DrawPoint(m_StartPoint);
    m_DrawHandles.push_back(startPointHandle);

    painter->SetPen(3);
    painter->SetPen(Color::White);
    int lineHandle = painter->DrawLine(m_StartPoint, m_EndPoint);
    m_DrawHandles.push_back(lineHandle);

    painter->SetPen(7);
    painter->SetPen(Color::Red);
    int endPointHandle = painter->DrawPoint(m_EndPoint);
    m_DrawHandles.push_back(endPointHandle);
}

void RadialStyle::ClearDrawRadial() {
    auto painter = m_Scene->GetPainter3D();
    for (auto& drawHandle: m_DrawHandles) painter->Delete(drawHandle);
    m_DrawHandles.clear();
}

void RadialStyle::CallBack() {
    for (auto& callBack: m_PointMoveCallBacks) callBack.second();
}

void RadialStyle::MousePressEvent(IEvent event) {
    BasicStyle::MousePressEvent(event);

    m_SelectedPoint = nullptr;
    if (!m_Show) return;

    if (event.button != MouseButton::MiddleButton) return;

    m_MVP = m_Interactor->GetMVP();
    m_InvertedMVP = m_MVP.invert();

    auto& pos = event.pos;

    igm::vec3 nearPoint = GetNearWorldCoord(pos, m_InvertedMVP);
    igm::vec3 farPoint = GetFarWorldCoord(pos, m_InvertedMVP);
    igm::vec3 rayDir = (farPoint - nearPoint).normalized();

    Point lineStartPoint = Point(nearPoint.x, nearPoint.y, nearPoint.z);
    Point lineDir = Point(rayDir.x, rayDir.y, rayDir.z);

    float tolerance = m_MaxDis;

    float distToStart =
            Line::ComputePointToLineDis(lineStartPoint, lineDir, m_StartPoint);
    bool startSelected = (distToStart <= tolerance);
    float distToEnd =
            Line::ComputePointToLineDis(lineStartPoint, lineDir, m_EndPoint);
    bool endSelected = (distToEnd <= tolerance);

    if (startSelected && endSelected) {
        m_SelectedPoint =
                (distToStart <= distToEnd) ? &m_StartPoint : &m_EndPoint;
    } else if (startSelected) {
        m_SelectedPoint = &m_StartPoint;
    } else if (endSelected) {
        m_SelectedPoint = &m_EndPoint;
    }

    if (m_SelectedPoint == nullptr) return;

    igm::vec4 p{m_SelectedPoint->operator[](0), m_SelectedPoint->operator[](1),
                m_SelectedPoint->operator[](2), 1.f};
    p = m_MVP * p;
    m_SelectedNDCZ = p.z / p.w;
}

void RadialStyle::MouseMoveEvent(IEvent event) {

    if (m_SelectedPoint == nullptr) return;
    if (!m_Show) return;
    if (m_MouseMode != MouseButton::MiddleButton) return;

    igm::vec2 pos = event.pos;

    igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                  1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

    igm::vec4 Point_NDC{NDC, m_SelectedNDCZ, 1.f};
    igm::vec4 newPoint_WorldCoord = m_InvertedMVP * Point_NDC;
    newPoint_WorldCoord /= newPoint_WorldCoord.w;

    *m_SelectedPoint = Point(newPoint_WorldCoord.x, newPoint_WorldCoord.y,
                             newPoint_WorldCoord.z);
    CallBack();
}

void RadialStyle::SetShow(bool show) {
    if (m_Show == show) return;
    m_Show = show;
    if (m_Show == false) ClearDrawRadial();
    else
        DrawRadial();
}

bool RadialStyle::GetShow() const { return m_Show; }

void RadialStyle::SetPoint(const Point& startPoint, const Point& endPoint) {
    m_StartPoint = startPoint;
    m_EndPoint = endPoint;
    m_MaxDis = (m_StartPoint - m_EndPoint).length() * 0.02;
}

const Point& RadialStyle::GetStartPoint() { return m_StartPoint; }

const Point& RadialStyle::GetEndPoint() { return m_EndPoint; }

void RadialStyle::_SetPointMoveCallBack(const std::string& name,
                                        std::function<void()> callBack) {
    m_PointMoveCallBacks[name] = callBack;
}
IGAME_NAMESPACE_END