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
    igm::uvec2 GetViewPort();
    igm::uvec2 GetScaledViewPort();

    void SetDevicePixelRatio(unsigned int devicePixelRatio);
    unsigned int GetDevicePixelRatio() const;

    template<typename FloatT>
    FloatT aspect() const;

protected:
    Viewport();
    ~Viewport() override;

    igm::uvec2 m_Offset;
    igm::uvec2 m_Size;
    unsigned int m_DevicePixelRatio;
};

class Viewer : public Viewport {
public:
    I_OBJECT(Viewer)
    static Pointer New() { return new Viewer; }

    void SetClippngRange(float near, float far);
    igm::vec2 GetClippingRange();

    void SetFov(float fov);
    float GetFov() const;

    // depth range: 0.0(near plane) -> 1.0(far plane)
    virtual igm::mat4 GetProjectionMatrix();

protected:
    Viewer();
    ~Viewer() override;

    igm::vec2 m_ClippingRange;
    float m_Fov;
};

class Camera : public Viewer {
public:
    I_OBJECT(Camera)
    static Pointer New() { return new Camera; }

    enum Type { PERSPECTIVE = 0, ORTHOGRAPHIC, CAMERATYPE_COUNT };

    float GetLengthToFocal();

    void SetPosition(const igm::vec3& pos);
    void SetPosition(float posX, float posY, float posZ);
    igm::vec3 GetPosition() const;

    void SetFocal(const igm::vec3& focal);
    void SetFocal(float focalX, float focalY, float focalZ);
    igm::vec3 GetFocal() const;

    void SetUp(const igm::vec3& up);
    void SetUp(float upX, float upY, float upZ);
    igm::vec3 GetUp() const;

    void SetType(Type type);
    Type GetType() const;

    igm::vec3 GetFront() const;
    igm::vec3 GetRight() const;

    igm::mat4 GetViewMatrix();

    igm::mat4 GetProjectionMatrix() override;

protected:
    Camera();
    ~Camera() override;

    void UpdateVectors();

    // Camera attributes
    igm::vec3 m_Position;
    igm::vec3 m_Focal;
    igm::vec3 m_Up;

    igm::vec3 m_Front;
    igm::vec3 m_Right;

    // Camera type
    Type m_Type;
};

IGAME_NAMESPACE_END
