#pragma once
#include "iGameBasicStyle.h"
#include "iGameBoundingBox.h"
#include <functional>
#include <vector>

IGAME_NAMESPACE_BEGIN

// Lightweight implicit-plane widget: no input cells are visited while dragging.
class RegionPlaneStyle : public BasicStyle {
public:
    I_OBJECT(RegionPlaneStyle);
    static Pointer New() { return new RegionPlaneStyle; }
    void SetBounds(const BoundingBox& bounds);
    void SetPlane(const Point& origin, const Point& normal);
    const Point& GetOrigin() const { return m_Origin; }
    const Point& GetNormal() const { return m_Normal; }
    Point GetNormalHandle(bool positive) const {
        return m_Origin + m_Normal * (positive ? m_PositiveLength : -m_NegativeLength);
    }
    void SetShow(bool show);
    void SetChangedCallback(std::function<void()> callback) { m_Changed = std::move(callback); }
    bool IsInteractionActive() const override { return m_Active != Outside; }
    void MousePressEvent(IEvent event) override;
    void MouseMoveEvent(IEvent event) override;
    void MouseReleaseEvent(IEvent event) override;
    void WheelEvent(IEvent) override {}

protected:
    RegionPlaneStyle() = default;
    ~RegionPlaneStyle() override = default;

private:
    enum Part { Outside, Origin, PositiveNormal, NegativeNormal, Ring, Plane };
    Part Pick(const igm::vec2& position) const;
    igm::vec2 Project(const Point& point) const;
    Point Unproject(const igm::vec2& position, float depth, const igm::mat4& inverse) const;
    void Basis(Point& u, Point& v) const;
    std::vector<Point> Polygon() const;
    void Draw();
    void Clear();
    BoundingBox m_Bounds;
    Point m_Origin{0., 0., 0.}, m_Normal{1., 0., 0.};
    float m_Scale{1.f}, m_Depth{};
    double m_PositiveLength{.5}, m_NegativeLength{.5};
    Point m_DragOffset{0., 0., 0.};
    Point m_RingGrip{0., 0., 0.};
    bool m_Show{};
    Part m_Hover{Outside}, m_Active{Outside};
    igm::vec2 m_PreviousScreen;
    igm::mat4 m_DragInverse;
    std::vector<IGuint> m_Handles;
    std::function<void()> m_Changed;
};

IGAME_NAMESPACE_END
