#ifndef IGAMEVIS_INTERACTOR_STYLE_H
#define IGAMEVIS_INTERACTOR_STYLE_H

#include "iGameObject.h"
#include "igm/igm.h"
#include "igm/transform.h"
//#include <Eigen/Dense>

/* Temprary not suit in Linux platform. */
// #include <format>

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

    virtual void Initialize(SmartPointer<Interactor> interactor) = 0;
    virtual void FilterEvent(IEvent event) {
        switch (event.type) {
            case IEvent::MousePress:
                MousePressEvent(event);
                break;
            case IEvent::MouseMove:
                MouseMoveEvent(event);
                break;
            case IEvent::MouseRelease:
                MouseReleaseEvent(event);
                break;
            case IEvent::Wheel:
                WheelEvent(event);
                break;
            default:
                break;
        }
    };
    virtual void MousePressEvent(IEvent event) {
        // std::cout << std::format(
        //         "Mouse press event at ({}, {}) with button {}\n", event.pos.x,
        //         event.pos.y, static_cast<int>(event.button));
    };
    virtual void MouseMoveEvent(IEvent event) {
        // std::cout << std::format("Mouse move event at ({}, {})\n", event.pos.x,
        //                          event.pos.y);
    };
    virtual void MouseReleaseEvent(IEvent event) {
        // std::cout << std::format("Mouse release event at ({}, {})\n",
        //                          event.pos.x, event.pos.y);
    };
    virtual void WheelEvent(IEvent event) {
        // std::cout << std::format("Mouse wheel event with delta {}\n",
        //                          event.delta);
    };

    /**
     * @brief 交互开始回调（默认空实现）。
     *
     * 由交互风格在开始一次拖拽/旋转等连续交互时调用（对标 MiniPVServer 的交互 LOD：
     * 在交互过程中降低采样率/分辨率、结束后恢复），供并行体绘制等场景按需切换
     * maxSamples / pixelStride / screenROI 以提升交互帧率。
     */
    virtual void StartInteraction() {};
    /**
     * @brief 交互结束回调（默认空实现）。与 StartInteraction 成对，用于恢复高质量参数。
     */
    virtual void EndInteraction() {};

    

protected:
    InteractorStyle() = default;
    ~InteractorStyle() override = default;

    friend class Interactor;

    
};
IGAME_NAMESPACE_END
#endif