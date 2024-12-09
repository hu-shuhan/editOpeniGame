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

#pragma once

#include "iGameObject.h"
#include "iGameRenderingMacro.h"
#include "igm/igm.h"
#include "igm/transform.h"

IGAME_NAMESPACE_BEGIN

class Viewport : public Object {
public:
    I_OBJECT(Viewport)
    static Pointer New() { return new Viewport; }

    void SetViewPort(uint32_t width, uint32_t height);
    igm::ivec2 GetViewPort();
    igm::ivec2 GetScaledViewPort();

    void SetDevicePixelRatio(int devicePixelRatio);
    int GetDevicePixelRatio();

    template<typename FloatT>
    FloatT aspect() const;

protected:
    Viewport();
    ~Viewport() override;

    igm::ivec2 m_Offset;
    igm::ivec2 m_Size;
    int m_DevicePixelRatio;
};

class Viewer : public Viewport {
public:
    I_OBJECT(Viewer)
    static Pointer New() { return new Viewer; }

    void SetNearPlane(float nearz);
    float GetNearPlane();

    void SetFarPlane(float farz);
    float GetFarPlane();

    void SetFov(float fov);
    float GetFov() const;

    // depth range: 0.0(near plane) -> 1.0(far plane)
    virtual igm::mat4 GetProjectionMatrix();

protected:
    Viewer();
    ~Viewer() override;

    float m_Fov;
    float m_NearZ;
    float m_FarZ;
};

class Camera : public Viewer {
public:
    I_OBJECT(Camera)
    static Pointer New() { return new Camera; }

    enum Type { PERSPECTIVE = 0, ORTHOGRAPHIC, CAMERATYPE_COUNT };

    void ChangeCameraType(Type type);

    float GetLengthToFocal();

    void SetCameraPos(igm::vec3 pos);
    void SetCameraPos(float posX, float posY, float posZ);
    igm::vec3 GetCameraPos() const;

    void SetCameraFocal(igm::vec3 focal);
    void SetCameraFocal(float focalX, float focalY, float focalZ);
    igm::vec3 GetCameraFocal() const;

    void SetCameraUp(igm::vec3 up);
    void SetCameraUp(float upX, float upY, float upZ);
    igm::vec3 GetCameraUp() const;

    void SetCameraType(Type type);
    Type GetCameraType() const;

    igm::mat4 GetViewMatrix();

    igm::mat4 GetProjectionMatrix() override;

protected:
    Camera();
    ~Camera() override;

    void UpdateCameraVectors();

    // Camera attributes
    igm::vec3 m_Position;
    igm::vec3 m_Focal;
    igm::vec3 m_WorldUp;

    igm::vec3 m_Front;
    igm::vec3 m_Right;
    igm::vec3 m_Up;

    // Camera type
    Type m_CameraType;
};

IGAME_NAMESPACE_END
