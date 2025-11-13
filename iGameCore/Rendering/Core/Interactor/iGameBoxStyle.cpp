#include "iGameBoxStyle.h"
IGAME_NAMESPACE_BEGIN
void BoxStyle::MousePressEvent(IEvent event) {}

void BoxStyle::MouseMoveEvent(IEvent event) {}
void BoxStyle::InitBox(const Point& p1, const Point& p2) {}

void BoxStyle::_SetPointMoveCallBack(const std::string& name,
                                        std::function<void()> callBack) {
    m_PointMoveCallBacks[name] = callBack;
}
IGAME_NAMESPACE_END