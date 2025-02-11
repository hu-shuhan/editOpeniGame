//
// Created by Sumzeek on 12/2/2024.
//

#include "iGameCamera.h"
#include "iGameRenderingLogger.h"
#include <algorithm>

IGAME_NAMESPACE_BEGIN

Viewport::Viewport() {
    m_Offset = igm::uvec2{0, 0};
    m_Size = igm::uvec2{800, 600};
    m_DevicePixelRatio = 1;
}

Viewport::~Viewport() {}

void Viewport::SetViewPort(uint32_t width, uint32_t height) {
    if (width == m_Size.x && height == m_Size.y) { return; }

    m_Size.x = width;
    m_Size.y = height;
    this->Modified();
}

igm::uvec2 Viewport::GetViewPort() { return m_Size; }

igm::uvec2 Viewport::GetScaledViewPort() { return m_Size * m_DevicePixelRatio; }

void Viewport::SetDevicePixelRatio(unsigned int devicePixelRatio) {
    if (devicePixelRatio == m_DevicePixelRatio) { return; }

    m_DevicePixelRatio = devicePixelRatio;
    this->Modified();
}

unsigned int Viewport::GetDevicePixelRatio() const {
    return m_DevicePixelRatio;
}

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

void Viewer::SetClippngRange(float near, float far) {
    if (near == m_ClippingRange.x && far == m_ClippingRange.y) { return; }

    if (near <= 0.0f) {
        Logger::LogError("Near plane value must be greater than 0.0f.");
    }

    if (near > far) {
        Logger::LogError(
                "Near plane value cannot be greater than the far plane value.");
    }

    if (far - near < 1e-10f) {
        Logger::LogError(
                "The difference between the near and far planes is too small.");
    }

    m_ClippingRange.x = near;
    m_ClippingRange.y = far;
    this->Modified();
}

igm::vec2 Viewer::GetClippingRange() { return m_ClippingRange; }

void Viewer::SetFov(float fov) {
    if (fov == m_Fov) { return; }

    if (fov < 1.0f || fov > 179.0f) {
        Logger::LogInfo("fov provided is out of range (1.0 - 179.0 degrees), "
                        "clamping to valid range.");
        m_Fov = std::clamp(fov, 1.0f, 179.0f);
    } else {
        m_Fov = fov;
    }

    this->Modified();
}

float Viewer::GetFov() const { return m_Fov; }


/** Depth Map Visualization(farz is infinite):
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
//igm::mat4 Viewer::GetProjectionMatrix() {
//    return igm::perspectiveRH_ZO(static_cast<float>(igm::radians(m_Fov)),
//                                 aspect<float>(), m_ClippingRange.x,
//                                 m_ClippingRange.y);
//};

/** Depth Map Visualization(farz is infinite):
    *          -far           -near              near            far
    *           |--------------|------->eye------->|--------------|
    *           0             -1     -INF/INF      1              0
    */
//float LinearizeDepthReverseZ(float z) const {
//    float depth = nearPlane / z;
//    return depth;
//}

Camera::Camera() {
    m_Type = Type::PERSPECTIVE;

    m_Position = igm::vec3(0.0f, 0.0f, 1.0f);
    m_Focal = igm::vec3(0.0f, 0.0f, 0.0f);
    m_Up = igm::vec3(0.0f, 1.0f, 0.0f);
    UpdateVectors();
}

Camera::~Camera() {}

float Camera::GetLengthToFocal() { return (m_Focal - m_Position).length(); }

void Camera::SetPosition(const igm::vec3& pos) {
    if (pos == m_Position) { return; }

    m_Position = pos;
    UpdateVectors();
    this->Modified();
}

void Camera::SetPosition(float posX, float posY, float posZ) {
    if (posX == m_Position.x && posY == m_Position.y && posZ == m_Position.z) {
        return;
    }

    m_Position = {posX, posY, posZ};
    UpdateVectors();
    this->Modified();
}

igm::vec3 Camera::GetPosition() const { return m_Position; }

void Camera::SetFocal(const igm::vec3& focal) {
    if (focal == m_Focal) { return; }

    m_Focal = focal;
    UpdateVectors();
    this->Modified();
}

void Camera::SetFocal(float focalX, float focalY, float focalZ) {
    if (focalX == m_Focal.x && focalY == m_Focal.y && focalZ == m_Focal.z) {
        return;
    }

    m_Focal = {focalX, focalY, focalZ};
    UpdateVectors();
    this->Modified();
}

igm::vec3 Camera::GetFocal() const { return m_Focal; }

void Camera::SetUp(const igm::vec3& up) {
    if (up == m_Up) { return; }

    m_Up = up;
    UpdateVectors();
    this->Modified();
}

void Camera::SetUp(float upX, float upY, float upZ) {
    if (upX == m_Up.x && upY == m_Up.y && upZ == m_Up.z) { return; }

    m_Up = igm::vec3{upX, upY, upZ};
    UpdateVectors();
    this->Modified();
}

igm::vec3 Camera::GetUp() const { return m_Up; }

void Camera::SetType(Type type) {
    if (type == m_Type) { return; }

    m_Type = type;
    this->Modified();
}

Camera::Type Camera::GetType() const { return m_Type; }

igm::vec3 Camera::GetFront() const { return m_Front; }

igm::vec3 Camera::GetRight() const { return m_Right; }

igm::mat4 Camera::GetViewMatrix() {
    return igm::lookAtRH(m_Position, m_Focal, m_Up);
}

igm::mat4 Camera::GetProjectionMatrix() {
    if (m_Type == Camera::Type::PERSPECTIVE) {
        return igm::perspectiveRH_OZ(static_cast<float>(igm::radians(m_Fov)),
                                     aspect<float>(), m_ClippingRange.x,
                                     m_ClippingRange.y);
    } else if (m_Type == Camera::Type::ORTHOGRAPHIC) {
        float dist = GetLengthToFocal();
        float orthoHeight = dist / 3.0f;
        float orthoWidth = orthoHeight * aspect<float>();

        return igm::orthoRH_OZ(-orthoWidth, orthoWidth, -orthoHeight,
                               orthoHeight, -m_ClippingRange.y,
                               m_ClippingRange.y);
    }
    return igm::mat4(1.0f);
}

void Camera::UpdateVectors() {
    // Calculate the new m_Front vector
    m_Front = (m_Focal - m_Position).normalized();
    // Also re-calculate the Right and Up vector
    m_Right = (igm::cross(m_Front, m_Up)).normalized();
    this->Modified();
}

IGAME_NAMESPACE_END