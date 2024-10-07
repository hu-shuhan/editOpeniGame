/**
* @file
* @brief    Camera operations for rendering scenes.
* @details  iGameRendering module.
* @version  v1.0.0
* @date     4/13/2024
* @author   Sumzeek
* @par Copyright(c): Hangzhou Dianzi University iGame Laboratory
* @par History:
*    v1.0.0: Sumzeek, 4/13/2024, first create this file\n
*/

#ifndef OPENIGAME_CAMERA_H
#define OPENIGAME_CAMERA_H


#include "iGameObject.h"
#include "iGameRenderingMacro.h"
#include "igm/igm.h"
#include "igm/transform.h"

IGAME_NAMESPACE_BEGIN

class Viewport : public Object {
public:
    I_OBJECT(Viewport)
    static Pointer New() { return new Viewport; }

public:
    void SetViewPort(uint32_t width, uint32_t height) {
        m_Size.x = width;
        m_Size.y = height;
    };
    void SetDevicePixelRatio(int devicePixelRatio) {
        m_DevicePixelRatio = devicePixelRatio;
    };

    igm::ivec2 GetViewPort() { return m_Size; };
    int GetDevicePixelRatio() { return m_DevicePixelRatio; };
    igm::ivec2 GetScaledViewPort() { return m_Size * m_DevicePixelRatio; }

    template<typename FloatT>
    FloatT aspect() const {
        return FloatT(m_Size.x) / FloatT(m_Size.y);
    }

protected:
    igm::ivec2 m_Offset = igm::ivec2{0, 0};
    igm::ivec2 m_Size = igm::ivec2{800, 600};
    int m_DevicePixelRatio = 1;

protected:
    Viewport() = default;
    ~Viewport() override = default;
};

class Viewer : public Viewport {
public:
    I_OBJECT(Viewer)
    static Pointer New() { return new Viewer; }

public:
    void SetNearPlane(float _near) { nearPlane = _near; }
    float GetNearPlane() { return nearPlane; }

    void SetFarPlane(float _far) { farPlane = _far; }
    float GetFarPlane() { return farPlane; }

    void SetFOV(float _fov) { fov = _fov; }
    float GetFOV() const { return fov; }

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
    virtual igm::mat4 GetProjectionMatrix() {
        return igm::perspectiveRH_ZO(static_cast<float>(igm::radians(fov)),
                                     aspect<float>(), nearPlane, farPlane);
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

    // depth range: 1.0(near plane) -> 0.0(far plane)
    //igm::mat4 GetProjectionMatrixReversedZ() {
    //    return igm::perspectiveRH_OZ(static_cast<float>(igm::radians(fov)),
    //                                 aspect<float>(), nearPlane);
    //}

protected:
    float fov = 45.0f;
    float nearPlane = 0.01f;
    float farPlane = 100.0f;

protected:
    Viewer() = default;
    ~Viewer() override = default;
};

class Camera : public Viewer {
public:
    I_OBJECT(Camera)
    static Pointer New() { return new Camera; }

public:
    enum CameraType { PERSPECTIVE = 0, ORTHOGRAPHIC, CAMERATYPE_COUNT };

public:
    void Initialize(igm::vec3 position = igm::vec3(0.0f, 0.0f, 1.0f),
                    igm::vec3 focal = igm::vec3{0.0f, 0.0f, 0.0f},
                    igm::vec3 up = igm::vec3(0.0f, 1.0f, 0.0f)) {
        m_Position = position;
        m_Focal = focal;
        m_WorldUp = up;
        updateCameraVectors();
    }

    void Initialize(float posX, float posY, float posZ, float focalX,
                    float focalY, float focalZ, float upX, float upY,
                    float upZ) {
        m_Position = igm::vec3(posX, posY, posZ);
        m_Focal = igm::vec3{focalX, focalY, focalZ};
        m_WorldUp = igm::vec3(upX, upY, upZ);
        updateCameraVectors();
    }

    void ChangeCameraType(CameraType type) { m_CameraType = type; }

    void SetCameraPos(igm::vec3 pos) {
        m_Position = pos;
        updateCameraVectors();
    }
    void SetCameraPos(float posX, float posY, float posZ) {
        m_Position = {posX, posY, posZ};
        updateCameraVectors();
    }
    igm::vec3 GetCameraPos() const { return m_Position; }

    void SetCameraFocal(igm::vec3 focal) {
        m_Focal = focal;
        updateCameraVectors();
    }
    void SetCameraFocal(float focalX, float focalY, float focalZ) {
        m_Focal = {focalX, focalY, focalZ};
        updateCameraVectors();
    }
    igm::vec3 GetCameraFocal() const { return m_Focal; }

    void SetCameraUp(igm::vec3 up) {
        m_WorldUp = up;
        updateCameraVectors();
    }
    void SetCameraUp(float upX, float upY, float upZ) {
        m_WorldUp = {upX, upY, upZ};
        updateCameraVectors();
    }
    igm::vec3 GetCameraUp() const { return m_Up; }

    void SetCameraType(CameraType type) { m_CameraType = type; }
    CameraType GetCameraType() const { return m_CameraType; }

    igm::mat4 GetViewMatrix() {
        return igm::lookAtRH(m_Position, m_Position + m_Front, m_Up);
    }

    igm::mat4 GetProjectionMatrix() override {
        if (m_CameraType == PERSPECTIVE) {
            return igm::perspectiveRH_OZ(static_cast<float>(igm::radians(fov)),
                                         aspect<float>(), nearPlane);
        } else if (m_CameraType == ORTHOGRAPHIC) {
            auto dist = (m_Focal - m_Position).length();
            return igm::orthoRH_OZ(
                    m_Focal.x - dist * 0.5f, m_Focal.x + dist * 0.5f,
                    m_Focal.y - dist * 0.5f, m_Focal.y + dist * 0.5f,
                    m_Focal.z - dist * 10000.0f, m_Focal.z + dist * 10000.0f);
        }
        return igm::mat4(1.0f);
    }

private:
    Camera() { Initialize(); }
    ~Camera() override = default;

    void updateCameraVectors() {
        // Calculate the new m_Front vector
        m_Front = (m_Focal - m_Position).normalized();
        // Also re-calculate the Right and Up vector
        m_Right = (igm::cross(m_Front, m_WorldUp)).normalized();
        m_Up = (igm::cross(m_Right, m_Front)).normalized();
    }

    // Camera attributes
    igm::vec3 m_Position;
    igm::vec3 m_Focal;
    igm::vec3 m_WorldUp;

    igm::vec3 m_Front;
    igm::vec3 m_Right;
    igm::vec3 m_Up;

    // Camera type
    CameraType m_CameraType = CameraType::PERSPECTIVE;
};
IGAME_NAMESPACE_END

#endif // OPENIGAME_CAMERA_H
