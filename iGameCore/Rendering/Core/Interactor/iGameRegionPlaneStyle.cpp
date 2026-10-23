#include "iGameRegionPlaneStyle.h"
#include "iGameInteractor.h"
#include "iGameScene.h"
#include <algorithm>
#include <cmath>
#include <limits>

IGAME_NAMESPACE_BEGIN
namespace {
constexpr float pi = 3.14159265358979323846f;
float SegmentDistance(const igm::vec2& p, const igm::vec2& a, const igm::vec2& b) {
    const auto d = b - a, offset = p - a;
    const float l2 = d.x * d.x + d.y * d.y;
    const float t = l2 > 0.f ? std::clamp((offset.x * d.x + offset.y * d.y) / l2, 0.f, 1.f) : 0.f;
    return (offset - d * t).length();
}
}

void RegionPlaneStyle::SetBounds(const BoundingBox& bounds) {
    m_Bounds = bounds;
    const auto size = bounds.max - bounds.min;
    m_Scale = std::max({static_cast<float>(size[0]), static_cast<float>(size[1]),
                        static_cast<float>(size[2]), 1.e-6f});
    m_PositiveLength = m_NegativeLength = m_Scale * .5;
    // Give flat datasets a visible context box as well.
    for (int i = 0; i < 3; ++i) if (size[i] < m_Scale * .05f) {
        m_Bounds.min[i] -= m_Scale * .05f;
        m_Bounds.max[i] += m_Scale * .05f;
    }
}

void RegionPlaneStyle::SetPlane(const Point& origin, const Point& normal) {
    if (!std::isfinite(normal.length()) || normal.length() < 1.e-12) return;
    m_Origin = origin;
    m_Normal = normal / normal.length();
    if (m_Show) { Clear(); Draw(); }
}

void RegionPlaneStyle::SetShow(bool show) {
    Clear();
    m_Show = show;
    m_Hover = m_Active = Outside;
    m_MouseMode = NoButton;
    if (show) Draw();
}

void RegionPlaneStyle::Basis(Point& u, Point& v) const {
    const Point reference = std::abs(m_Normal[2]) < .9 ? Point(0., 0., 1.) : Point(0., 1., 0.);
    u = m_Normal.cross(reference);
    u = u / u.length();
    v = m_Normal.cross(u);
}

std::vector<Point> RegionPlaneStyle::Polygon() const {
    std::vector<Point> vertices;
    Point corners[8];
    for (int i = 0; i < 8; ++i)
        for (int axis = 0; axis < 3; ++axis)
            corners[i][axis] = (i & (1 << axis)) ? m_Bounds.max[axis] : m_Bounds.min[axis];
    auto add = [&](const Point& p) {
        for (const auto& q : vertices) if ((q - p).length() < m_Scale * 1.e-6f) return;
        vertices.push_back(p);
    };
    for (int i = 0; i < 8; ++i) for (int axis = 0; axis < 3; ++axis) {
        if (i & (1 << axis)) continue;
        const auto& a = corners[i];
        const auto& b = corners[i | (1 << axis)];
        const double da = (a - m_Origin).dot(m_Normal), db = (b - m_Origin).dot(m_Normal);
        if (std::abs(da) < m_Scale * 1.e-7) add(a);
        if (std::abs(db) < m_Scale * 1.e-7) add(b);
        if ((da < 0. && db > 0.) || (da > 0. && db < 0.)) add(a + (b - a) * (da / (da - db)));
    }
    Point u, v;
    Basis(u, v);
    // Keep the widget usable when its plane is outside the dataset.
    if (vertices.size() < 3) return {m_Origin - u * m_Scale * .4 - v * m_Scale * .4,
        m_Origin + u * m_Scale * .4 - v * m_Scale * .4,
        m_Origin + u * m_Scale * .4 + v * m_Scale * .4,
        m_Origin - u * m_Scale * .4 + v * m_Scale * .4};
    Point center(0., 0., 0.);
    for (const auto& p : vertices) center += p;
    center = center / vertices.size();
    std::sort(vertices.begin(), vertices.end(), [&](const Point& a, const Point& b) {
        return std::atan2((a - center).dot(v), (a - center).dot(u)) <
               std::atan2((b - center).dot(v), (b - center).dot(u));
    });
    return vertices;
}

igm::vec2 RegionPlaneStyle::Project(const Point& p) const {
    const auto c = m_Interactor->GetMVP() * igm::vec4(p[0], p[1], p[2], 1.f);
    if (c.w <= 0.f) return {1.e20f, 1.e20f};
    return {(c.x / c.w + 1.f) * m_Interactor->GetWidth() * .5f,
             (1.f - c.y / c.w) * m_Interactor->GetHeight() * .5f};
}

Point RegionPlaneStyle::Unproject(const igm::vec2& p, float depth, const igm::mat4& inverse) const {
    auto world = inverse * igm::vec4(2.f * p.x / m_Interactor->GetWidth() - 1.f,
        1.f - 2.f * p.y / m_Interactor->GetHeight(), depth, 1.f);
    world /= world.w;
    return Point(world.x, world.y, world.z);
}

RegionPlaneStyle::Part RegionPlaneStyle::Pick(const igm::vec2& pos) const {
    if (!m_Show) return Outside;
    Part handle = Outside;
    float closest = 12.f;
    for (Part part : {Origin, PositiveNormal, NegativeNormal}) {
        const Point p = part == Origin ? m_Origin : GetNormalHandle(part == PositiveNormal);
        const float distance = (Project(p) - pos).length();
        if (distance < closest) { closest = distance; handle = part; }
    }
    if (handle != Outside) return handle;
    Point u, v;
    Basis(u, v);
    for (int i = 0; i < 96; ++i) {
        auto at = [&](int j) { return m_Origin + (u * std::cos(2.f * pi * j / 96) + v * std::sin(2.f * pi * j / 96)) * m_Scale * .65; };
        if (SegmentDistance(pos, Project(at(i)), Project(at(i + 1))) <= 7.f) return Ring;
    }
    if (SegmentDistance(pos, Project(GetNormalHandle(false)), Project(GetNormalHandle(true))) <= 6.f)
        return Plane;
    const auto polygon = Polygon();
    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const auto a = Project(polygon[j]), b = Project(polygon[i]);
        if (SegmentDistance(pos, a, b) <= 7.f) return Plane;
        if ((a.y > pos.y) != (b.y > pos.y) && pos.x < (b.x - a.x) * (pos.y - a.y) / (b.y - a.y) + a.x)
            inside = !inside;
    }
    return inside ? Plane : Outside;
}

void RegionPlaneStyle::MousePressEvent(IEvent event) {
    if (!m_Show) return;
    m_MouseMode = event.button;
    if (event.button != LeftButton) return;
    m_Hover = m_Active = Pick(event.pos);
    m_PreviousScreen = event.pos;
    m_DragInverse = m_Interactor->GetMVP().invert();
    Point picked = (m_Active == PositiveNormal || m_Active == NegativeNormal)
        ? GetNormalHandle(m_Active == PositiveNormal) : m_Origin;
    if (m_Active == Ring) {
        Point u, v;
        Basis(u, v);
        float closest = std::numeric_limits<float>::max();
        for (int i = 0; i < 384; ++i) {
            const auto p = m_Origin + (u * std::cos(2.f * pi * i / 384) + v * std::sin(2.f * pi * i / 384)) * m_Scale * .65;
            const float distance = (Project(p) - event.pos).length();
            if (distance < closest) { closest = distance; picked = p; }
        }
        m_RingGrip = picked;
    }
    const auto clip = m_Interactor->GetMVP() * igm::vec4(picked[0], picked[1], picked[2], 1.f);
    m_Depth = clip.z / clip.w;
    m_DragOffset = picked - Unproject(event.pos, m_Depth, m_DragInverse);
    Clear(); Draw();
}

void RegionPlaneStyle::MouseMoveEvent(IEvent event) {
    if (!m_Show) return;
    if (m_Active == Outside) {
        // A camera drag that started outside must never turn into a widget drag.
        if (m_MouseMode != NoButton) return;
        const auto hover = Pick(event.pos);
        if (hover != m_Hover) { m_Hover = hover; Clear(); Draw(); }
        return;
    }
    const auto delta = Unproject(event.pos, m_Depth, m_DragInverse) -
                       Unproject(m_PreviousScreen, m_Depth, m_DragInverse);
    if (m_Active == Origin) m_Origin += delta;
    else if (m_Active == Plane) {
        const auto screenNormal = Project(m_Origin + m_Normal * m_Scale) - Project(m_Origin);
        const auto screenDelta = event.pos - m_PreviousScreen;
        const float l2 = screenNormal.x * screenNormal.x + screenNormal.y * screenNormal.y;
        // End-on normals still push via vertical dragging.
        const double distance = l2 > 16.f ?
            (screenDelta.x * screenNormal.x + screenDelta.y * screenNormal.y) * m_Scale / l2 :
            -screenDelta.y * m_Scale / std::max(m_Interactor->GetHeight(), 1.f);
        m_Origin += m_Normal * distance;
    } else if (m_Active == Ring) {
        const Point nextGrip = Unproject(event.pos, m_Depth, m_DragInverse) + m_DragOffset;
        Point before = m_RingGrip - m_Origin, after = nextGrip - m_Origin;
        if (before.length() > m_Scale * 1.e-5 && after.length() > m_Scale * 1.e-5) {
            before = before / before.length();
            after = after / after.length();
            Point axis = before.cross(after);
            const double sine = axis.length();
            const double cosine = std::clamp(static_cast<double>(before.dot(after)), -1., 1.);
            if (sine > 1.e-8) {
                axis = axis / sine;
                m_Normal = m_Normal * cosine + axis.cross(m_Normal) * sine +
                    axis * axis.dot(m_Normal) * (1. - cosine);
                m_Normal = m_Normal / m_Normal.length();
            }
        }
        m_RingGrip = nextGrip;
    } else {
        // Match SlicingStyle: unproject the dragged endpoint at its picked
        // depth. Both endpoints follow the cursor, including after view rotation;
        // no screen-delta-to-angle sign convention is needed.
        const Point endpoint = Unproject(event.pos, m_Depth, m_DragInverse) + m_DragOffset;
        const Point direction = endpoint - m_Origin;
        const double length = direction.length();
        if (length > m_Scale * 1.e-5) {
            const bool positive = m_Active == PositiveNormal;
            m_Normal = direction / (positive ? length : -length);
            (positive ? m_PositiveLength : m_NegativeLength) = length;
        }
    }
    m_PreviousScreen = event.pos;
    Clear(); Draw();
    if (m_Changed) m_Changed();
}

void RegionPlaneStyle::MouseReleaseEvent(IEvent event) {
    if (!m_Show) return;
    m_Active = Outside;
    m_MouseMode = NoButton;
    m_Hover = Pick(event.pos);
    Clear(); Draw();
}

void RegionPlaneStyle::Clear() {
    if (!m_Scene) return;
    auto painter = m_Scene->GetPainter3D();
    for (auto handle : m_Handles) painter->Delete(handle);
    m_Handles.clear();
}

void RegionPlaneStyle::Draw() {
    if (!m_Show) return;
    auto painter = m_Scene->GetPainter3D();
    painter->SetPen(1);
    painter->SetPen(Color::White);
    Point corners[8];
    for (int i = 0; i < 8; ++i) for (int axis = 0; axis < 3; ++axis)
        corners[i][axis] = (i & (1 << axis)) ? m_Bounds.max[axis] : m_Bounds.min[axis];
    for (int i = 0; i < 8; ++i) for (int axis = 0; axis < 3; ++axis)
        if (!(i & (1 << axis))) m_Handles.push_back(painter->DrawLine(corners[i], corners[i | (1 << axis)]));
    painter->SetPen(m_Hover == Plane ? Color::Yellow : Color::White);
    painter->SetPen(m_Hover == Plane ? 5 : 3);
    const auto polygon = Polygon();
    for (size_t i = 0; i < polygon.size(); ++i)
        m_Handles.push_back(painter->DrawLine(polygon[i], polygon[(i + 1) % polygon.size()]));
    painter->SetPen(m_Hover == Ring ? Color::Yellow : Color::Red);
    painter->SetPen(m_Hover == Ring ? 4 : 3);
    Point u, v;
    Basis(u, v);
    for (int i = 0; i < 96; ++i) {
        auto at = [&](int j) { return m_Origin + (u * std::cos(2.f * pi * j / 96) + v * std::sin(2.f * pi * j / 96)) * m_Scale * .65; };
        m_Handles.push_back(painter->DrawLine(at(i), at(i + 1)));
    }
    painter->SetPen(Color::Red);
    painter->SetPen(2);
    m_Handles.push_back(painter->DrawLine(GetNormalHandle(false), GetNormalHandle(true)));
    for (Part part : {Origin, PositiveNormal, NegativeNormal}) {
        painter->SetPen(m_Hover == part ? Color::Green : Color::Red);
        painter->SetPen(m_Hover == part ? 18 : 16);
        m_Handles.push_back(painter->DrawPoint(part == Origin ? m_Origin : GetNormalHandle(part == PositiveNormal)));
    }
}

IGAME_NAMESPACE_END
