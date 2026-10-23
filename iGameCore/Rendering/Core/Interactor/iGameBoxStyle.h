#pragma once
#include "iGameBasicStyle.h"
#include "iGamePoints.h"
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <iGameDynamicBox.h>
#include <memory>
IGAME_NAMESPACE_BEGIN

class BoxStyle : public BasicStyle {
public:
    I_OBJECT(BoxStyle);
    static Pointer New() { return new BoxStyle; }

    void MousePressEvent(IEvent event) override;
    void MouseMoveEvent(IEvent event) override;
    void MouseReleaseEvent(IEvent event) override;
    void SetLeftButtonOnly(bool enabled) { m_LeftButtonOnly = enabled; }
    void WheelEvent(IEvent event) override { if (!m_LeftButtonOnly) BasicStyle::WheelEvent(event); }
    bool IsInteractionActive() const override {
        if (m_LeftButtonOnly) return m_SelectedItem != IG_NONE && m_MouseMode == MouseButton::LeftButton;
        return m_SelectedItem != IG_NONE &&
               (m_MouseMode == MouseButton::MiddleButton ||
                (m_ParaViewInteraction && m_UniformScaling && m_MouseMode == MouseButton::RightButton) ||
                ((m_AllowLeftHandleDrag || m_AllowRegionResize || m_AllowBoundaryDrag) && m_MouseMode == MouseButton::LeftButton) ||
                (m_CenterTranslationMode && m_SelectedItem == IG_MID_POINT &&
                 m_MouseMode == MouseButton::LeftButton));
    }

    void InitBox(const Point& p1, const Point& p2);
    void SetAllowRotation(bool allow) { m_AllowRotation = allow; }
    void SetAllowLeftHandleDrag(bool allow) { m_AllowLeftHandleDrag = allow; }
    void SetCenterTranslationMode(bool enabled) { m_CenterTranslationMode = enabled; }
    void SetAllowRegionResize(bool enabled, double minimumSize = 1.e-7) {
        m_AllowRegionResize = enabled;
        m_MinRegionSize = minimumSize;
    }
    void SetAllowBoundaryDrag(bool enabled) { m_AllowBoundaryDrag = enabled; }
    void SetParaViewInteraction(bool enabled) { m_ParaViewInteraction = enabled; }
    void DeleteBox();

    void ToDraw();
    void ClearDraw();

    DynamicBox::Pointer GetBox();

    void _SetPointMoveCallBack(const std::string& name,
                               std::function<void()> callBack);
#define SetPointMoveCallBack(callBack)                                         \
    _SetPointMoveCallBack(std::string(__FILE__) + std::to_string(__LINE__),    \
                          callBack)

    void RemovePointMoveCallBack(const std::string& name);

    void SetUpdateWidgetFunc(std::function<void()> func);

    void RemoveUpdateWidgetFunc();

private:
    void LeftButtonPress(IEvent event);
    void MiddleButtonPress(IEvent event);
    void RightButtonPress(IEvent event);

    void LeftButtonRelease(IEvent event);

    void RightButtonRelease(IEvent event);

private:
    std::pair<Point, Point> GetRayStartAndDir(IEvent event);
    std::tuple<bool, int, Point>
    MeetOpePoint(IEvent event, const Point& rayStart, const Point& rayDir);
    std::tuple<bool, Point> MeetCenterPoint(IEvent event, const Point& rayStart,
                                            const Point& rayDir);
    std::tuple<bool, Point> MeetBoxEdge(IEvent event, const Point& rayStart,
                                        const Point& rayDir);
    std::tuple<bool, Point> MeetBoxRotate(IEvent event, const Point& rayStart,
                                          const Point& rayDir);

private:
    IGenum m_BoxDrawMode{IG_NONE};
    int m_BoxDrawModeOpePoint{};
    void ChangeBoxDrawMode(IEvent event);
    void OperateBox(IEvent event);

private:
    void PointMoveCallBack();

private:
    void SetNeedReSet();

protected:
    BoxStyle() = default;
    ~BoxStyle();

    int m_SelectedDirection{-1};
    IGenum m_SelectedItem{IG_NONE};
    Point m_PrePosition;
    DynamicBox::Pointer m_DynamicBox{nullptr};

    bool m_ChoosedStation{};

    float m_SelectedNDCZ{};
    igm::mat4 m_MVP{};
    igm::mat4 m_InvertedMVP{};

    igm::vec2 m_PressSite = igm::vec2{0.0f};
    bool m_MeetedBox{};
    bool m_AllowRotation{true};
    bool m_LeftButtonOnly{false};
    bool m_AllowLeftHandleDrag{false};
    bool m_CenterTranslationMode{false};
    bool m_AllowRegionResize{false};
    bool m_AllowBoundaryDrag{false};
    bool m_ParaViewInteraction{false};
    bool m_UniformScaling{false};
    Point m_ScaleStartPoint;
    double m_MinRegionSize{1.e-7};
    std::pair<Point, Point> m_ResizeBounds;
    igm::vec2 m_ResizeScreenAxis{0.f};
    MouseButton m_MousePressButton{MouseButton::NoButton};

    std::vector<IGuint> m_DrawHandles;
    std::map<std::string, std::function<void()>> m_PointMoveCallBacks;
    std::shared_ptr<std::function<void()>> m_CancelBoxStyleCallBack;

    std::shared_ptr<std::function<void()>> m_UpdateWidgetFunc;
};

IGAME_NAMESPACE_END
