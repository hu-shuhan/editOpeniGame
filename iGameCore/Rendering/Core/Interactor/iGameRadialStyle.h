#pragma once
#include "iGameBasicStyle.h"
#include "iGamePoints.h"
#include <vector>
#include <map>
#include <functional>
#include <string>
IGAME_NAMESPACE_BEGIN
class RadialStyle : public BasicStyle {
public:
    I_OBJECT(RadialStyle);
    static Pointer New() { return new RadialStyle; }

    void MousePressEvent(IEvent event) override;
    void MouseMoveEvent(IEvent event) override;
    void MouseReleaseEvent(IEvent event) override;
    void SetLeftButtonOnly(bool enabled) { m_LeftButtonOnly = enabled; }
    void WheelEvent(IEvent event) override { if (!m_LeftButtonOnly) BasicStyle::WheelEvent(event); }
    bool IsInteractionActive() const override {
        return m_SelectedPoint != nullptr &&
               (m_MouseMode == MouseButton::MiddleButton ||
                (m_ParaViewInteraction && m_UniformScaling && m_MouseMode == MouseButton::RightButton) ||
                ((m_AllowLeftHandleDrag || m_AllowRegionResize ||
                  (m_CenterTranslationMode && m_SelectedPoint == &m_StartPoint)) &&
                 m_MouseMode == MouseButton::LeftButton));
    }
    void SetShow(bool show);
    bool GetShow() const;
    void SetPoint(const Point& startPoint, const Point& endPoint);
    void SetMoveEndWithStart(bool enabled) { m_MoveEndWithStart = enabled; }
    void SetShowSphere(bool enabled) { m_ShowSphere = enabled; }
    void SetAllowLeftHandleDrag(bool enabled) { m_AllowLeftHandleDrag = enabled; }
    void SetCenterTranslationMode(bool enabled) { m_CenterTranslationMode = enabled; }
    void SetAllowRegionResize(bool enabled, double minimumRadius = 1.e-7) {
        m_AllowRegionResize = enabled;
        m_MinRegionRadius = minimumRadius;
    }
    void SetAllowBoundaryDrag(bool enabled) { m_AllowBoundaryDrag = enabled; }
    void SetParaViewInteraction(bool enabled) { m_ParaViewInteraction = enabled; }
    Point GetRadiusHandle();
    bool IsRadiusResizing() const { return m_AllowRegionResize && m_MouseMode == MouseButton::LeftButton && m_SelectedPoint == &m_EndPoint; }
    const Point& GetStartPoint();
    const Point& GetEndPoint();
    void DrawRadial();
    void ClearDrawRadial();
    void _SetPointMoveCallBack(const std::string& name,
                               std::function<void()> callBack);
    void RemovePointMoveCallBack(const std::string& name);
#define SetPointMoveCallBack(callBack)                                         \
    _SetPointMoveCallBack(std::string(__FILE__) + std::to_string(__LINE__),    \
                          callBack)

protected:
    bool MeetSphereOutline(IEvent event);
    bool MeetSphereSurface(IEvent event);
    float CalculatePickTolerance();
    void CallBack();

protected:
    RadialStyle() = default;
    ~RadialStyle() = default;

    bool m_Show{};
    bool m_LeftButtonOnly{};
    bool m_MoveEndWithStart{};
    bool m_ShowSphere{};
    bool m_AllowLeftHandleDrag{};
    bool m_CenterTranslationMode{};
    bool m_AllowRegionResize{};
    bool m_AllowBoundaryDrag{};
    bool m_ParaViewInteraction{};
    bool m_UniformScaling{};
    Point m_ScalePreviousPoint;
    float m_ScalePreviousY{};
    bool m_BoundaryTranslation{};
    bool m_BoundaryHovered{};
    Point m_TranslationOffset;
    double m_MinRegionRadius{1.e-7};
    Point m_RadiusDragOffset;
    Point m_RadiusDragDirection;
    Point m_LastRadiusHandle;
    Point* m_HoveredPoint{nullptr};

    float m_MaxDis{};

    Point m_StartPoint;
    Point m_EndPoint;
    
    Point* m_SelectedPoint{nullptr};

    float m_SelectedNDCZ{};
    igm::mat4 m_MVP{};
    igm::mat4 m_InvertedMVP{};

    std::vector<IGuint> m_DrawHandles;
    std::map<std::string, std::function<void()>> m_PointMoveCallBacks;
};

IGAME_NAMESPACE_END
