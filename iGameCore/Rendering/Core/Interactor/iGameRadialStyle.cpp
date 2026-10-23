#include <algorithm>
#include "iGameRadialStyle.h"
#include "iGameInteractor.h"
#include "iGameLine.h"
#include "iGameScene.h"
#include <cmath>
#include <limits>
using namespace std;
IGAME_NAMESPACE_BEGIN
namespace {
// Shared by drawing and picking, so every visible meridian/parallel is draggable.
std::vector<std::pair<Point, Point>> SphereWireSegments(const Point& center, float radius, bool latLong) {
    constexpr double pi = 3.14159265358979323846;
    std::vector<std::pair<Point, Point>> lines;
    if (latLong) {
        // Match ParaView 6.2 / VTK 9.7.1 SphereRepresentation resolution.
        constexpr int longitudeCount = 16, latitudeResolution = 8;
        auto point = [&](double theta, double phi) {
            return center + Point(radius * std::sin(phi) * std::cos(theta),
                                  radius * std::sin(phi) * std::sin(theta),
                                  radius * std::cos(phi));
        };
        for (int i = 0; i < longitudeCount; ++i) {
            const double theta = 2. * pi * i / longitudeCount;
            for (int j = 0; j < latitudeResolution - 1; ++j)
                lines.emplace_back(point(theta, pi * j / (latitudeResolution - 1)),
                                   point(theta, pi * (j + 1) / (latitudeResolution - 1)));
        }
        for (int j = 1; j < latitudeResolution - 1; ++j) {
            const double phi = pi * j / (latitudeResolution - 1);
            for (int i = 0; i < longitudeCount; ++i)
                lines.emplace_back(point(2. * pi * i / longitudeCount, phi),
                                   point(2. * pi * (i + 1) / longitudeCount, phi));
        }
    } else {
        for (int plane = 0; plane < 3; ++plane) {
            auto point = [&](double angle) {
                Point p = center;
                const float a = radius * std::cos(angle), b = radius * std::sin(angle);
                if (plane == 0) { p[1] += a; p[2] += b; }
                else if (plane == 1) { p[0] += a; p[2] += b; }
                else { p[0] += a; p[1] += b; }
                return p;
            };
            for (int i = 0; i < 64; ++i)
                lines.emplace_back(point(2. * pi * i / 64), point(2. * pi * (i + 1) / 64));
        }
    }
    return lines;
}
} // namespace

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

Point RadialStyle::GetRadiusHandle() {
    const auto mvp = m_Interactor->GetMVP();
    const auto inverse = mvp.invert();
    auto clip = mvp * igm::vec4(m_StartPoint[0], m_StartPoint[1], m_StartPoint[2], 1.f);
    clip /= clip.w;
    auto right = inverse * igm::vec4(clip.x + 0.1f, clip.y, clip.z, 1.f);
    right /= right.w;
    Point direction = Point(right.x, right.y, right.z) - m_StartPoint;
    const float length = direction.length();
    if (length <= 0.f) return m_EndPoint;
    return m_StartPoint + direction / length * (m_EndPoint - m_StartPoint).length();
}

bool RadialStyle::MeetSphereOutline(IEvent event) {
    if (!m_AllowBoundaryDrag || !m_CenterTranslationMode || !m_ShowSphere) return false;
    const auto mvp = m_Interactor->GetMVP();
    const auto inverse = mvp.invert();
    auto clip = mvp * igm::vec4(m_StartPoint[0], m_StartPoint[1], m_StartPoint[2], 1.f);
    if (clip.w <= 0.f) return false;
    clip /= clip.w;
    auto up = inverse * igm::vec4(clip.x, clip.y + .1f, clip.z, 1.f);
    up /= up.w;
    const float radius = (m_EndPoint - m_StartPoint).length();
    Point v = Point(up.x, up.y, up.z) - m_StartPoint;
    if (v.length() <= 0.f) return false;
    v = v / v.length() * radius;
    const Point u = GetRadiusHandle() - m_StartPoint;
    auto segments = SphereWireSegments(m_StartPoint, radius, m_ParaViewInteraction);
    if (m_AllowRegionResize) {
        for (int i = 0; i < 64; ++i) {
            const double a = 6.283185307179586 * i / 64;
            const double b = 6.283185307179586 * (i + 1) / 64;
            segments.emplace_back(m_StartPoint + u * std::cos(a) + v * std::sin(a),
                                  m_StartPoint + u * std::cos(b) + v * std::sin(b));
        }
    }
    for (const auto& edge : segments) {
        auto project = [&](const Point& p) {
            const auto c = mvp * igm::vec4(p[0], p[1], p[2], 1.f);
            return igm::vec2((c.x / c.w + 1.f) * m_Interactor->GetWidth() * .5f,
                             (1.f - c.y / c.w) * m_Interactor->GetHeight() * .5f);
        };
        const auto start = project(edge.first), d = project(edge.second) - start, offset = event.pos - start;
        const float length2 = d.x * d.x + d.y * d.y;
        const float t = length2 > 0.f ? std::clamp((offset.x * d.x + offset.y * d.y) / length2, 0.f, 1.f) : 0.f;
        if ((offset - d * t).length() <= 7.f) return true;
    }
    return false;
}

bool RadialStyle::MeetSphereSurface(IEvent event) {
    const igm::vec2 ndc(2.f * event.pos.x / m_Interactor->GetWidth() - 1.f,
                        1.f - 2.f * event.pos.y / m_Interactor->GetHeight());
    auto a = m_InvertedMVP * igm::vec4(ndc, -1.f, 1.f);
    auto b = m_InvertedMVP * igm::vec4(ndc, 1.f, 1.f);
    a /= a.w; b /= b.w;
    const Point origin(a.x, a.y, a.z), d(b.x - a.x, b.y - a.y, b.z - a.z);
    const auto offset = origin - m_StartPoint;
    const double radius = (m_EndPoint - m_StartPoint).length();
    const double aa = d.dot(d), bb = offset.dot(d);
    const double cc = offset.dot(offset) - radius * radius;
    const double discriminant = bb * bb - aa * cc;
    return aa > 0. && discriminant >= 0. && (-bb + std::sqrt(discriminant)) / aa >= 0.;
}

void RadialStyle::DrawRadial() {
    if (!m_Show) return;
    auto painter = m_Scene->GetPainter3D();

    if (m_CenterTranslationMode) {
        painter->SetPen(m_HoveredPoint == &m_StartPoint ? 16 : 11);
        painter->SetPen(m_HoveredPoint == &m_StartPoint ? Color::Magenta : Color::Orange);
        m_DrawHandles.push_back(painter->DrawPoint(m_StartPoint));
        if (m_ShowSphere) {
            const float radius = (m_EndPoint - m_StartPoint).length();
            painter->SetPen(3);
            if (m_BoundaryHovered || m_BoundaryTranslation) painter->SetPen(Color::Yellow);
            else painter->SetPen(0, 125, 240);
            // DrawCircle can inherit a filled brush from the scene. Use lines
            // so the region always remains hollow over the input model.
            for (const auto& edge : SphereWireSegments(m_StartPoint, radius, m_ParaViewInteraction))
                m_DrawHandles.push_back(painter->DrawLine(edge.first, edge.second));
        }
        if (m_AllowRegionResize) {
            m_LastRadiusHandle = GetRadiusHandle();
            painter->SetPen(m_HoveredPoint == &m_EndPoint ? 16 : 11);
            painter->SetPen(m_HoveredPoint == &m_EndPoint ? Color::Magenta : Color::Red);
            m_DrawHandles.push_back(painter->DrawPoint(m_LastRadiusHandle));
            // A view-facing circle keeps the radius handle on a visible outline.
            const auto inverse = m_Interactor->GetMVP().invert();
            auto clip = m_Interactor->GetMVP() * igm::vec4(m_StartPoint[0], m_StartPoint[1], m_StartPoint[2], 1.f);
            clip /= clip.w;
            auto up = inverse * igm::vec4(clip.x, clip.y + 0.1f, clip.z, 1.f);
            up /= up.w;
            Point v = Point(up.x, up.y, up.z) - m_StartPoint;
            const float radius = (m_EndPoint - m_StartPoint).length();
            v = v / v.length() * radius;
            const Point u = m_LastRadiusHandle - m_StartPoint;
            painter->SetPen(3);
            if (m_BoundaryHovered || m_BoundaryTranslation) painter->SetPen(Color::Yellow);
            else painter->SetPen(0, 125, 240);
            for (int i = 0; i < 64; ++i) {
                const double a = 6.283185307179586 * i / 64;
                const double b = 6.283185307179586 * (i + 1) / 64;
                m_DrawHandles.push_back(painter->DrawLine(
                    m_StartPoint + u * std::cos(a) + v * std::sin(a),
                    m_StartPoint + u * std::cos(b) + v * std::sin(b)));
            }
        }
        return;
    }

    painter->SetPen(m_HoveredPoint == &m_StartPoint ? 13 : 9);
    painter->SetPen(Color::Blue);
    int startPointHandle = painter->DrawPoint(m_StartPoint);
    m_DrawHandles.push_back(startPointHandle);

    painter->SetPen(3);
    painter->SetPen(Color::White);
    int lineHandle = painter->DrawLine(m_StartPoint, m_EndPoint);
    m_DrawHandles.push_back(lineHandle);

    painter->SetPen(m_HoveredPoint == &m_EndPoint ? 13 : 9);
    painter->SetPen(Color::Red);
    int endPointHandle = painter->DrawPoint(m_EndPoint);
    m_DrawHandles.push_back(endPointHandle);
    if (m_ShowSphere) {
        const float radius = (m_EndPoint - m_StartPoint).length();
        painter->SetPen(2);
        painter->SetPen(Color::Yellow);
        m_DrawHandles.push_back(painter->DrawCircle(m_StartPoint, Vector3f(1, 0, 0), radius, 64));
        m_DrawHandles.push_back(painter->DrawCircle(m_StartPoint, Vector3f(0, 1, 0), radius, 64));
        m_DrawHandles.push_back(painter->DrawCircle(m_StartPoint, Vector3f(0, 0, 1), radius, 64));
    }
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
    m_BoundaryTranslation = false;
    m_UniformScaling = false;
    if (!m_Show) return;

    if (m_LeftButtonOnly && event.button != MouseButton::LeftButton) return;
    if (!(m_ParaViewInteraction && m_AllowRegionResize && event.button == MouseButton::RightButton) &&
        event.button != MouseButton::MiddleButton &&
        !((m_AllowLeftHandleDrag || m_CenterTranslationMode || m_AllowRegionResize) &&
          event.button == MouseButton::LeftButton)) return;

    m_MVP = m_Interactor->GetMVP();
    m_InvertedMVP = m_MVP.invert();

    auto& pos = event.pos;

    const auto screenDistance = [this, &pos](const Point& point) {
        const igm::vec4 clip = m_MVP * igm::vec4(point[0], point[1], point[2], 1.f);
        if (clip.w <= 0.f) return std::numeric_limits<float>::max();
        const float x = (clip.x / clip.w + 1.f) * m_Interactor->GetWidth() * 0.5f;
        const float y = (1.f - clip.y / clip.w) * m_Interactor->GetHeight() * 0.5f;
        const float dx = x - pos.x, dy = y - pos.y;
        return std::sqrt(dx * dx + dy * dy);
    };
    const float distToStart = screenDistance(m_StartPoint);
    const Point radiusHandle = m_AllowRegionResize ? GetRadiusHandle() : m_EndPoint;
    const float distToEnd = screenDistance(radiusHandle);
    const bool startSelected = distToStart <= (m_CenterTranslationMode ? 26.f : 12.f);
    const bool endSelected = (!m_CenterTranslationMode || m_AllowRegionResize) && distToEnd <= 12.f;

    if (startSelected && endSelected) {
        m_SelectedPoint =
                (distToStart <= distToEnd) ? &m_StartPoint : &m_EndPoint;
    } else if (startSelected) {
        m_SelectedPoint = &m_StartPoint;
    } else if (endSelected) {
        m_SelectedPoint = &m_EndPoint;
    }

    if (m_SelectedPoint == nullptr && MeetSphereOutline(event)) {
        m_SelectedPoint = &m_StartPoint;
        m_BoundaryTranslation = true;
    }
    if (m_ParaViewInteraction &&
        (event.button == MouseButton::MiddleButton || event.button == MouseButton::RightButton)) {
        if (m_SelectedPoint == nullptr && !MeetSphereSurface(event)) return;
        m_SelectedPoint = &m_StartPoint;
        m_BoundaryTranslation = true;
        m_UniformScaling = event.button == MouseButton::RightButton;
    }
    if (m_SelectedPoint == nullptr) return;

    const Point picked = IsRadiusResizing() ? m_StartPoint : *m_SelectedPoint;
    igm::vec4 p{picked[0], picked[1], picked[2], 1.f};
    p = m_MVP * p;
    m_SelectedNDCZ = p.z / p.w;
    if (m_BoundaryTranslation) {
        const igm::vec2 ndc(2.f * pos.x / m_Interactor->GetWidth() - 1.f,
                            1.f - 2.f * pos.y / m_Interactor->GetHeight());
        auto world = m_InvertedMVP * igm::vec4(ndc, m_SelectedNDCZ, 1.f);
        world /= world.w;
        m_TranslationOffset = m_StartPoint - Point(world.x, world.y, world.z);
        m_ScalePreviousPoint = Point(world.x, world.y, world.z);
        m_ScalePreviousY = pos.y;
    }
    if (IsRadiusResizing()) {
        const igm::vec2 ndc(2.f * pos.x / m_Interactor->GetWidth() - 1.f,
                              1.f - 2.f * pos.y / m_Interactor->GetHeight());
        auto world = m_InvertedMVP * igm::vec4(ndc, m_SelectedNDCZ, 1.f);
        world /= world.w;
        m_RadiusDragOffset = radiusHandle - Point(world.x, world.y, world.z);
        m_RadiusDragDirection = (radiusHandle - m_StartPoint) / (radiusHandle - m_StartPoint).length();
    }
}

void RadialStyle::MouseMoveEvent(IEvent event) {
    if (!m_Show) return;
    if (m_SelectedPoint == nullptr) {
        if (m_LeftButtonOnly && m_MouseMode != MouseButton::NoButton) return;
        m_MVP = m_Interactor->GetMVP();
        const auto distance = [this, &event](const Point& point) {
            const igm::vec4 clip = m_MVP * igm::vec4(point[0], point[1], point[2], 1.f);
            if (clip.w <= 0.f) return std::numeric_limits<float>::max();
            const float x = (clip.x / clip.w + 1.f) * m_Interactor->GetWidth() * 0.5f;
            const float y = (1.f - clip.y / clip.w) * m_Interactor->GetHeight() * 0.5f;
            const float dx = x - event.pos.x, dy = y - event.pos.y;
            return std::sqrt(dx * dx + dy * dy);
        };
        const Point handle = m_AllowRegionResize ? GetRadiusHandle() : m_EndPoint;
        const float a = distance(m_StartPoint), b = distance(handle);
        Point* hovered = nullptr;
        if (m_CenterTranslationMode) {
            if (m_AllowRegionResize && b <= 12.f && b < a) hovered = &m_EndPoint;
            else if (a <= 26.f) hovered = &m_StartPoint;
        } else if (a <= 12.f || b <= 12.f) {
            hovered = a <= b ? &m_StartPoint : &m_EndPoint;
        }
        const bool boundary = hovered == nullptr && MeetSphereOutline(event);
        if (boundary != m_BoundaryHovered || hovered != m_HoveredPoint || (m_AllowRegionResize && (handle - m_LastRadiusHandle).length() > 1.e-6f)) {
            m_HoveredPoint = hovered;
            m_BoundaryHovered = boundary;
            ClearDrawRadial();
            DrawRadial();
        }
        return;
    }
    if (!IsInteractionActive()) return;

    igm::vec2 pos = event.pos;

    igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                  1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

    igm::vec4 Point_NDC{NDC, m_SelectedNDCZ, 1.f};
    igm::vec4 newPoint_WorldCoord = m_InvertedMVP * Point_NDC;
    newPoint_WorldCoord /= newPoint_WorldCoord.w;

    const Point newPoint(newPoint_WorldCoord.x, newPoint_WorldCoord.y,
                         newPoint_WorldCoord.z);
    if (m_UniformScaling) {
        // VTK 9.7.1 vtkSphereRepresentation::Scale: motion length / radius,
        // with upward motion growing and downward motion shrinking (Qt Y down).
        const auto direction = m_EndPoint - m_StartPoint;
        const double radius = direction.length();
        const double delta = (newPoint - m_ScalePreviousPoint).length();
        const double next = radius + (pos.y < m_ScalePreviousY ? delta : -delta);
        if (radius > 0. && next >= m_MinRegionRadius)
            m_EndPoint = m_StartPoint + direction * (next / radius);
        m_ScalePreviousPoint = newPoint;
        m_ScalePreviousY = pos.y;
    } else if (IsRadiusResizing()) {
        const Point direction = newPoint + m_RadiusDragOffset - m_StartPoint;
        const double length = direction.dot(m_RadiusDragDirection);
        m_EndPoint = m_StartPoint + m_RadiusDragDirection * std::max(length, m_MinRegionRadius);
    } else {
        const Point translated = m_BoundaryTranslation ? newPoint + m_TranslationOffset : newPoint;
        if (m_MoveEndWithStart && m_SelectedPoint == &m_StartPoint)
            m_EndPoint = m_EndPoint + (translated - m_StartPoint);
        *m_SelectedPoint = translated;
    }
    ClearDrawRadial();
    DrawRadial();
    CallBack();
}

void RadialStyle::MouseReleaseEvent(IEvent event) {
    BasicStyle::MouseReleaseEvent(event);
    m_SelectedPoint = nullptr;
    m_BoundaryTranslation = false;
    m_UniformScaling = false;
    if (m_Show && (m_AllowRegionResize || m_AllowBoundaryDrag)) { ClearDrawRadial(); DrawRadial(); }
}

void RadialStyle::SetShow(bool show) {
    if (m_Show == show) return;
    m_Show = show;
    if (m_Show == false) {
        ClearDrawRadial();
        m_HoveredPoint = nullptr;
        m_SelectedPoint = nullptr;
        m_BoundaryHovered = false;
        m_BoundaryTranslation = false;
    }
    else
        DrawRadial();
}

bool RadialStyle::GetShow() const { return m_Show; }

void RadialStyle::SetPoint(const Point& startPoint, const Point& endPoint) {
    if (m_Show) ClearDrawRadial();
    m_StartPoint = startPoint;
    m_EndPoint = endPoint;
    m_MaxDis = (m_StartPoint - m_EndPoint).length() * 0.02;
    if (m_Show) DrawRadial();
}

const Point& RadialStyle::GetStartPoint() { return m_StartPoint; }

const Point& RadialStyle::GetEndPoint() { return m_EndPoint; }

void RadialStyle::_SetPointMoveCallBack(const std::string& name,
                                       std::function<void()> callBack) {
    m_PointMoveCallBacks[name] = callBack;
}

void RadialStyle::RemovePointMoveCallBack(const std::string& name) {
    m_PointMoveCallBacks.erase(name);
}
IGAME_NAMESPACE_END
