#pragma once
#include "iGameBasicStyle.h"
#include "iGamePoints.h"
#include <vector>
#include <map>
#include <functional>
#include <string>
IGAME_NAMESPACE_BEGIN

class BoxStyle : public BasicStyle {
public:
    I_OBJECT(BoxStyle);
    static Pointer New() { return new BoxStyle; }

    void MousePressEvent(IEvent event) override;
    void MouseMoveEvent(IEvent event) override;

    

    void InitBox(const Point& p1, const Point& p2); 



    void _SetPointMoveCallBack(const std::string& name,
                               std::function<void()> callBack);
#define SetPointMoveCallBack(callBack)                                         \
    _SetPointMoveCallBack(std::string(__FILE__) + std::to_string(__LINE__),    \
                          callBack)

protected:
    BoxStyle() = default;
    ~BoxStyle() = default;

    

    float m_SelectedNDCZ{};
    igm::mat4 m_MVP{};
    igm::mat4 m_InvertedMVP{};

    std::vector<IGuint> m_DrawHandles;
    std::map<std::string, std::function<void()>> m_PointMoveCallBacks;
};

IGAME_NAMESPACE_END