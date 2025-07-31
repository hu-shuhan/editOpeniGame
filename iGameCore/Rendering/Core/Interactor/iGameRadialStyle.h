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
    void SetShow(bool show);
    bool GetShow() const;
    void SetPoint(const Point& startPoint, const Point& endPoint);
    const Point& GetStartPoint();
    const Point& GetEndPoint();
    void DrawRadial();
    void ClearDrawRadial();
    void _SetPointMoveCallBack(const std::string& name,
                               std::function<void()> callBack);
#define SetPointMoveCallBack(callBack)                                         \
    _SetPointMoveCallBack(std::string(__FILE__) + std::to_string(__LINE__),    \
                          callBack)

protected:
    float CalculatePickTolerance();
    void CallBack();

protected:
    RadialStyle() = default;
    ~RadialStyle() = default;

    bool m_Show{};

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