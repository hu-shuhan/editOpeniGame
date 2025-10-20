#include "iGameDragCenterStyle.h"

IGAME_NAMESPACE_BEGIN
DragCenterStyle::DragCenterStyle() {
    m_Interactor = nullptr;
    m_Scene = nullptr;
    m_Camera = nullptr;
    m_AxesModel = nullptr;

    m_LastMousePos = igm::vec2{0.0f};
}

DragCenterStyle::~DragCenterStyle() {}

void DragCenterStyle::Initialize(SmartPointer<Interactor> interactor) {
    m_Interactor = interactor;
    m_Scene = interactor->GetScene();
    m_Camera = interactor->GetCamera();
}

void DragCenterStyle::MousePressEvent(IEvent event) {
    if (event.button != LeftButton  || !m_Camera || !m_AxesModel) return;

    m_IsDragging = true;
    m_LastMousePos = event.pos;
}

void DragCenterStyle::MouseMoveEvent(IEvent event) {
    if (!m_IsDragging || !m_Camera || !m_Scene || !m_AxesModel) return;

    // 1. 计算屏幕空间偏移量
    igm::vec2 offset = event.pos - m_LastMousePos;
    igm::vec3 moveOffset(offset.x * m_CameraMoveSpeed,
                         -offset.y * m_CameraMoveSpeed,
                         0.0f // Z分量为0保持深度不变
    );

    // 2. 转换为世界空间位移
    igm::mat4 invViewMatrix = m_Camera->GetViewMatrix().invert();
    igm::vec3 worldOffset = (invViewMatrix * igm::vec4(moveOffset, 0.0f)).xyz();
    //考虑模型旋转对方向的影响
    igm::mat4 modelMatrix = m_Scene->GetModelMatrix();
    igm::mat4 invModelMatrix = modelMatrix.invert();
    igm::vec3 modelSpaceOffset =
            (invModelMatrix * igm::vec4(worldOffset, 0.0f)).xyz();

    worldOffset = modelSpaceOffset;

    // 3. 更新坐标轴模型位置
    m_AxesModel->HandleDrag(worldOffset);
    m_Scene->SetRotationCenter(m_AxesModel->GetRotationCenter());

    m_LastMousePos = event.pos;
}

void DragCenterStyle::MouseReleaseEvent(IEvent event) {
    //if (event.button == LeftButton) { m_IsDragging = false; }
    m_IsDragging = false;
    //std::cout << "releaseEvent  : " << m_IsDragging << std::endl;
}

IGAME_NAMESPACE_END