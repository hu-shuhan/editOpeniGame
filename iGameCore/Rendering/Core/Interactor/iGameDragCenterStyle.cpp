#include "iGameDragCenterStyle.h"

IGAME_NAMESPACE_BEGIN
DragCenterStyle::DragCenterStyle() {
    m_Interactor = nullptr;
    m_Scene = nullptr;
    m_Camera = nullptr;
    m_AxesModel = nullptr;
    m_IsDragging = false;
    m_LastMousePos = igm::vec2{0.0f};
    bool m_IsMouseOverAxes = false;
    float m_PickTolerance = 80.0f;
}

DragCenterStyle::~DragCenterStyle() {}

void DragCenterStyle::Initialize(SmartPointer<Interactor> interactor) {
    m_Interactor = interactor;
    m_Scene = interactor->GetScene();
    m_Camera = interactor->GetCamera();
}

void DragCenterStyle::MousePressEvent(IEvent event) {
    if (event.button != LeftButton || !m_Camera || !m_AxesModel) return;

    // 检查鼠标是否在坐标轴附近
    if (!IsMouseOverAxes(event.pos)) {
        return;
    }
    m_IsDragging = true;
    m_LastMousePos = event.pos;
}

void DragCenterStyle::MouseMoveEvent(IEvent event) {
    //检测鼠标是否在坐标轴附近
    bool wasOverAxes = m_IsMouseOverAxes; // 保存之前的状态
    m_IsMouseOverAxes = IsMouseOverAxes(event.pos);

    //状态改变更新高亮
    if (wasOverAxes != m_IsMouseOverAxes && !m_IsDragging) {
        if (m_AxesModel) { m_AxesModel->SetHighlight(m_IsMouseOverAxes); }
    }
    // 没有在拖拽中，直接返回
    if (!m_IsDragging) {
        m_LastMousePos = event.pos;
        return;
    }


    if (!m_IsDragging || !m_Camera || !m_Scene || !m_AxesModel) return;

    // 计算屏幕偏移量
    igm::vec2 offset = event.pos - m_LastMousePos;

    // 获取每像素的世界距离
    float pixelWorldDistance = CalculateAdaptiveSpeed();

    // 计算世界空间位移
    float moveX = offset.x * pixelWorldDistance; // 注意负号
    float moveY = -offset.y * pixelWorldDistance;

    igm::vec3 moveOffset(moveX, moveY, 0.0f);

    // 转换为视图空间
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
    m_Scene->SetRotationBoundingSphere(
            igm::vec4{m_AxesModel->GetRotationCenter(),
                      m_Scene->GetRotationBoundingSphere().w});

    m_LastMousePos = event.pos;
}

void DragCenterStyle::MouseReleaseEvent(IEvent event) {
    //if (event.button == LeftButton) { m_IsDragging = false; }
    m_IsDragging = false;
    //std::cout << "releaseEvent  : " << m_IsDragging << std::endl;
}

float DragCenterStyle::CalculateAdaptiveSpeed() {
    if (!m_Camera || !m_Scene) return 0.005f; // 默认值

    auto viewport = m_Camera->GetViewPort();
    float viewportHeight = static_cast<float>(viewport.y);

    if (m_Camera->GetType() == Camera::Type::ORTHOGRAPHIC) {
        // 使用相同的正交投影计算
        float orthoHeight = m_Camera->GetLengthToFocal() * 0.5f;
        return orthoHeight / viewportHeight * m_Camera->GetDevicePixelRatio();
    } else { // PERSPECTIVE
        // 使用相同的透视投影计算，但针对旋转中心
        igm::vec3 center = m_AxesModel->GetRotationCenter();

        // 构建MVP矩阵
        igm::mat4 model = m_Scene->GetModelMatrix();
        igm::mat4 view = m_Camera->GetViewMatrix();
        igm::mat4 proj = m_Camera->GetProjectionMatrix();
        auto mvp = proj * view * model;

        // 将旋转中心转换到裁剪空间
        auto centerMvp = mvp * igm::vec4{center, 1.0f};
        centerMvp /= centerMvp.w;

        // 计算向上移动1个像素
        auto p = igm::vec3{centerMvp.x, centerMvp.y + 2.0f / viewportHeight,
                           centerMvp.z};

        // 反向变换回世界坐标
        auto pWorldCoord = mvp.invert() * igm::vec4{p, 1.0f};
        pWorldCoord /= pWorldCoord.w;

        // 计算1个像素对应的世界距离
        return (pWorldCoord.xyz() - center).length();
    }
}


bool DragCenterStyle::IsMouseOverAxes(const igm::vec2& mousePos) {
    if (!m_AxesModel || !m_Camera || !m_Scene) return false;

    // 方法1：基于距离的拾取（较简单）
    // 将坐标轴中心投影到屏幕空间，计算与鼠标的距离

    // 获取坐标轴的世界位置
    igm::vec3 axesCenter = m_AxesModel->GetRotationCenter();

    // 将世界坐标转换为屏幕坐标
    igm::mat4 model = m_Scene->GetModelMatrix();
    igm::mat4 view = m_Camera->GetViewMatrix();
    igm::mat4 proj = m_Camera->GetProjectionMatrix();
    auto viewport = m_Camera->GetViewPort();

    // 计算MVP矩阵
    auto mvp = proj * view * model;

    // 将中心点变换到裁剪空间
    auto clipPos = mvp * igm::vec4(axesCenter, 1.0f);

    // 透视除法
    clipPos /= clipPos.w;

    // 转换到屏幕坐标
    float screenX = (clipPos.x + 1.0f) * 0.5f * viewport.x;
    float screenY = (1.0f - clipPos.y) * 0.5f * viewport.y; // Y轴翻转

    // 计算鼠标与坐标轴中心的距离
    float distance = sqrt((mousePos.x - screenX) * (mousePos.x - screenX) +
                          (mousePos.y - screenY) * (mousePos.y - screenY));

    // 如果距离小于容差，则认为鼠标在坐标轴上
    return distance <= m_PickTolerance;
}


IGAME_NAMESPACE_END
