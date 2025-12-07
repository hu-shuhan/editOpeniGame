#ifndef IGAME_DRAG_CENTER_STYLE_H
#define IGAME_DRAG_CENTER_STYLE_H

#include "iGameCamera.h"
#include "iGameCenterAxesModel.h"
#include "iGameInteractorStyle.h"
#include "iGameScene.h"
#include "iGameInteractor.h"
#include "iGameVector.h"
#include "igm/igm.h"

IGAME_NAMESPACE_BEGIN
class Scene;
class Interactor;

class DragCenterStyle : public InteractorStyle {
public:
    I_OBJECT(DragCenterStyle);
    static Pointer New() { return new DragCenterStyle(); }
    // 必须实现的纯虚函数
    void Initialize(SmartPointer<Interactor> interactor) override;

    void SetAxesModel(const SmartPointer<CenterAxesModel>& model) {
        m_AxesModel = model;
    }
    void MousePressEvent(IEvent event) override;
    void MouseMoveEvent(IEvent event) override;
    void MouseReleaseEvent(IEvent event) override;
    void WheelEvent(IEvent event) override {} // 不需要处理滚轮事件，但必须实现

    float CalculateAdaptiveSpeed();

    bool IsMouseOverAxes(const igm::vec2& mousePos);

protected:
    DragCenterStyle();
    ~DragCenterStyle() override;

private:
    SmartPointer<CenterAxesModel> m_AxesModel;
    SmartPointer<Interactor> m_Interactor;
    SmartPointer<Scene> m_Scene;
    SmartPointer<Camera> m_Camera;
    bool m_IsDragging = false;
    igm::vec2 m_LastMousePos;
    float m_CameraMoveSpeed = 0.005f; // 与BasicStyle保持一致
    bool m_IsMouseOverAxes = false;
    float m_PickTolerance = 80.0f;
};

IGAME_NAMESPACE_END
#endif // IGAME_DRAG_CENTER_STYLE_H