//
// Created by Sumzeek on 12/2/2024.
//

#include "iGameCamera.h"

IGAME_NAMESPACE_BEGIN

Viewport::Viewport() {
    m_Offset = igm::ivec2{0, 0};
    m_Size = igm::ivec2{800, 600};
    m_DevicePixelRatio = 1;
}

Viewport::~Viewport() {}

void Viewport::SetViewPort(uint32_t width, uint32_t height) {
    m_Size.x = width;
    m_Size.y = height;
}

void Viewport::SetDevicePixelRatio(int devicePixelRatio) {
    m_DevicePixelRatio = devicePixelRatio;
}

igm::ivec2 Viewport::GetViewPort() { return m_Size; }

int Viewport::GetDevicePixelRatio() { return m_DevicePixelRatio; }

igm::ivec2 Viewport::GetScaledViewPort() { return m_Size * m_DevicePixelRatio; }

template<typename FloatT>
FloatT Viewport::aspect() const {
    return FloatT(m_Size.x) / FloatT(m_Size.y);
}

Viewer::Viewer() {
    m_ClippingRange.x = 0.01f;
    m_ClippingRange.y = 100.01f;
    m_Fov = 45.0f;
}

Viewer::~Viewer() {}

void Viewer::SetClippngRange(float nearz, float farz) {
    if (nearz <= 0.0f) {
        igError("Near plane value must be greater than 0.0f.");
    }

    if (nearz > farz) {
        igError("Near plane value cannot be greater than the far plane value.");
    }

    if (farz - nearz < 1e-10f) {
        igError("The difference between the near and far planes is too small.");
    }

    m_ClippingRange.x = nearz;
    m_ClippingRange.y = farz;
}

igm::vec2 Viewer::GetClippingRange() { return m_ClippingRange; }

void Viewer::SetFov(float fov) {
    if (fov < 1.0f || fov > 179.0f) {
        igDebug("fov provided is out of range (1.0 - 179.0 degrees). "
                "Clamping to valid range.");
        m_Fov = std::clamp(fov, 1.0f, 179.0f);
    } else {
        m_Fov = fov;
    }
}

float Viewer::GetFov() const { return m_Fov; }


/** Depth Map Visualization:
    *          -far           -near              near            far
    *           |--------------|------->eye------->|--------------|
    *           1              2      INF/-INF     0              1
    */
//float LinearizeDepth(float z) {
//    float ndcZ = z * 2.0f - 1.0f; // back to NDC
//    float depth = 2.0f * nearPlane / (1.0f - ndcZ);
//    return depth;
//};

// depth range: 0.0(near plane) -> 1.0(far plane)
igm::mat4 Viewer::GetProjectionMatrix() {
    return igm::perspectiveRH_ZO(static_cast<float>(igm::radians(m_Fov)),
                                 aspect<float>(), m_ClippingRange.x,
                                 m_ClippingRange.y);
};

/** Depth Map Visualization:
    *          -far           -near              near            far
    *           |--------------|------->eye------->|--------------|
    *           0             -1     -INF/INF      1              0
    */
//float LinearizeDepthReverseZ(float z) const {
//    float depth = nearPlane / z;
//    return depth;
//}

Camera::Camera() {
    m_CameraType = Type::PERSPECTIVE;

    m_Position = igm::vec3(0.0f, 0.0f, 1.0f);
    m_Focal = igm::vec3(0.0f, 0.0f, 0.0f);
    m_Up = igm::vec3(0.0f, 1.0f, 0.0f);
    UpdateCameraVectors();
}

Camera::~Camera() {}

void Camera::ChangeCameraType(Type type) { m_CameraType = type; }

float Camera::GetLengthToFocal() { return (m_Focal - m_Position).length(); }

void Camera::SetCameraPos(igm::vec3 pos) {
    m_Position = pos;
    UpdateCameraVectors();
}
void Camera::SetCameraPos(float posX, float posY, float posZ) {
    m_Position = {posX, posY, posZ};
    UpdateCameraVectors();
}
igm::vec3 Camera::GetCameraPos() const { return m_Position; }

void Camera::SetCameraFocal(igm::vec3 focal) {
    m_Focal = focal;
    UpdateCameraVectors();
}
void Camera::SetCameraFocal(float focalX, float focalY, float focalZ) {
    m_Focal = {focalX, focalY, focalZ};
    UpdateCameraVectors();
}
igm::vec3 Camera::GetCameraFocal() const { return m_Focal; }

void Camera::SetCameraUp(igm::vec3 up) {
    m_Up = up;
    UpdateCameraVectors();
}
void Camera::SetCameraUp(float upX, float upY, float upZ) {
    m_Up = {upX, upY, upZ};
    UpdateCameraVectors();
}
igm::vec3 Camera::GetCameraUp() const { return m_Up; }

void Camera::SetCameraType(Type type) { m_CameraType = type; }
Camera::Type Camera::GetCameraType() const { return m_CameraType; }

igm::mat4 Camera::GetViewMatrix() {
    return igm::lookAtRH(m_Position, m_Focal, m_Up);
}

igm::mat4 Camera::GetProjectionMatrix() {
    if (m_CameraType == PERSPECTIVE) {
        return igm::perspectiveRH_OZ(static_cast<float>(igm::radians(m_Fov)),
                                     aspect<float>(), m_ClippingRange.x,
                                     m_ClippingRange.y);
    } else if (m_CameraType == ORTHOGRAPHIC) {
        float dist = GetLengthToFocal();
        float orthoHeight = dist / 3.0f;
        float orthoWidth = orthoHeight * aspect<float>();

        return igm::orthoRH_OZ(-orthoWidth, orthoWidth, -orthoHeight,
                               orthoHeight, -m_ClippingRange.y,
                               m_ClippingRange.y);
    }
    return igm::mat4(1.0f);
}

void Camera::UpdateCameraVectors() {
    // Calculate the new m_Front vector
    m_Front = (m_Focal - m_Position).normalized();
    // Also re-calculate the Right and Up vector
    m_Right = (igm::cross(m_Front, m_Up)).normalized();
}

IGAME_NAMESPACE_END