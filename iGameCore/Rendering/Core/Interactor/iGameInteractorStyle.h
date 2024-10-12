#ifndef OPENIGAME_INTERACTOR_STYLE_H
#define OPENIGAME_INTERACTOR_STYLE_H

#include "iGameObject.h"
#include "igm/igm.h"
#include "igm/transform.h"
//#include <Eigen/Dense>
#include <format>

IGAME_NAMESPACE_BEGIN
enum MouseButton {
    NoButton = 0x00000000,
    LeftButton = 0x00000001,
    RightButton = 0x00000002,
    MiddleButton = 0x00000004
};

class IEvent {
public:
    typedef enum {
        MousePress = 0,
        MouseMove,
        MouseRelease,
        Wheel,
    } Type;


    Type type;
    MouseButton button;
    igm::vec2 pos;
    double delta;
};

class Interactor;
class InteractorStyle : public Object {
public:
    I_OBJECT(InteractorStyle);

    virtual void Initialize(Interactor* a) = 0;
    virtual void FilterEvent(IEvent _event) {
        switch (_event.type) {
            case IEvent::MousePress:
                MousePressEvent(_event);
                break;
            case IEvent::MouseMove:
                MouseMoveEvent(_event);
                break;
            case IEvent::MouseRelease:
                MouseReleaseEvent(_event);
                break;
            case IEvent::Wheel:
                WheelEvent(_event);
                break;
            default:
                break;
        }
    };
    virtual void MousePressEvent(IEvent _event) {
        std::cout << std::format(
                "Mouse press event at ({}, {}) with button {}\n", _event.pos.x,
                _event.pos.y, static_cast<int>(_event.button));
    };
    virtual void MouseMoveEvent(IEvent _event) {
        std::cout << std::format("Mouse move event at ({}, {})\n", _event.pos.x,
                                 _event.pos.y);
    };
    virtual void MouseReleaseEvent(IEvent _event) {
        std::cout << std::format("Mouse release event at ({}, {})\n",
                                 _event.pos.x, _event.pos.y);
    };
    virtual void WheelEvent(IEvent _event) {
        std::cout << std::format("Mouse wheel event with delta {}\n",
                                 _event.delta);
    };

    enum class Event { Slicing };

protected:
    InteractorStyle() = default;
    ~InteractorStyle() override = default;

    void InvokeEvent(Event _event, void* callData) {
        if (m_CallBack) { m_CallBack(_event, callData); }
    }

    std::function<void(Event, void*)> m_CallBack;

    friend class Interactor;
};
IGAME_NAMESPACE_END
#endif