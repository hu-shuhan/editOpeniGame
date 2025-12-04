/**
 * @class    Viewport、Viewer、Camera
 * @brief    该文件定义了关于相机操作的三个核心类：Viewport、Viewer 和 Camera。
 *
 * @details
 * - **Viewport**: 提供视口管理功能，包括设置视口尺寸、获取视口大小、缩放视口等操作。
 *   视口的定义是渲染图像输出的区域，该类支持动态调整视口的宽高和设备像素比例。
 *
 * - **Viewer**: 扩展了 Viewport 的功能，添加了对视锥体剪裁范围（近裁剪面和远裁剪面）和
 *   视场角 (FOV) 的设置与管理。该类适用于定义投影矩阵所需的关键参数。
 *
 * - **Camera**: 在 Viewer 的基础上，提供完整的相机管理功能，包括相机的位置、焦点（观察点）、
 *   上方向、投影模式（透视投影或正交投影）的设置。该类还实现了相机视图矩阵和投影矩阵的
 *   计算，为 3D 场景渲染提供基础。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameObject.h"
#include "iGameRenderingMacro.h"
#include "igm/igm.h"
#include "igm/transform.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class Viewport
 * @brief 用于管理视口信息，例如大小和设备像素比。
 */
class Viewport : public Object {
public:
    I_OBJECT(Viewport)
    static Pointer New() { return new Viewport; }

    /**
     * @brief 设置视口的宽度和高度。
     * @param width 视口的宽度。
     * @param height 视口的高度。
     */
    void SetViewPort(uint32_t width, uint32_t height);

    /**
     * @brief 获取当前视口的大小。
     * @return 返回视口的宽高值 (igm::uvec2)。
     */
    igm::uvec2 GetViewPort();

    /**
     * @brief 获取经过设备像素比缩放后的视口大小。
     * @return 返回缩放后的视口宽高值 (igm::uvec2)。
     */
    igm::uvec2 GetScaledViewPort();

    /**
     * @brief 设置设备像素比（DPI 缩放）。
     * @param devicePixelRatio 设备像素比。
     */
    void SetDevicePixelRatio(unsigned int devicePixelRatio);

    /**
     * @brief 获取设备像素比。
     * @return 返回设备像素比值。
     */
    unsigned int GetDevicePixelRatio() const;

    /**
     * @brief 计算视口的纵横比（宽高比）。
     * @tparam FloatT 模板参数，指定返回值的浮点类型。
     * @return 返回视口的宽高比。
     */
    template<typename FloatT>
    FloatT aspect() const { return FloatT(m_Size.x) / FloatT(m_Size.y); }

protected:
    Viewport();
    ~Viewport() override;

    igm::uvec2 m_Offset;
    igm::uvec2 m_Size;
    unsigned int m_DevicePixelRatio;
};

/**
 * @class Viewer
 * @brief 在 Viewport 的基础上添加视锥裁剪范围和视野角度（FOV）。
 */
class Viewer : public Viewport {
public:
    I_OBJECT(Viewer)
    static Pointer New() { return new Viewer; }

    /**
     * @brief 设置视锥的近裁剪面和远裁剪面。
     * @param near 近裁剪面距离。
     * @param far 远裁剪面距离。
     */
    void SetClippingRange(float near, float far);

    /**
     * @brief 获取当前的裁剪范围。
     * @return 返回裁剪范围 (igm::vec2)，其中 x 为 near，y 为 far。
     */
    igm::vec2 GetClippingRange();

    /**
     * @brief 设置视角的垂直视场角 (FOV)。
     * @param fov 垂直视场角，单位为度。
     */
    void SetFov(float fov);

    /**
     * @brief 获取当前的视场角 (FOV)。
     * @return 返回视场角值。
     */
    float GetFov() const;

protected:
    Viewer();
    ~Viewer() override;

    igm::vec2 m_ClippingRange;
    float m_Fov;
};

/**
 * @class Camera
 * @brief 在 Viewer 的基础上扩展相机的功能，包括位置、焦点、类型等。
 */
class Camera : public Viewer {
public:
    I_OBJECT(Camera)
    static Pointer New() { return new Camera; }

    /**
     * @enum Type
     * @brief 定义相机的投影类型。
     */
    enum class Type {
        PERSPECTIVE = 0, ///< 透视投影。
        ORTHOGRAPHIC     ///< 正交投影。
    };

    /**
     * @brief 获取相机到焦点的距离。
     * @return 返回距离值。
     */
    float GetLengthToFocal();

    /**
     * @brief 设置相机的位置。
     * @param pos 相机的位置 (igm::vec3)。
     */
    void SetPosition(const igm::vec3& pos);

    /**
     * @brief 设置相机的位置。
     * @param posX X 坐标。
     * @param posY Y 坐标。
     * @param posZ Z 坐标。
     */
    void SetPosition(float posX, float posY, float posZ);

    /**
     * @brief 获取相机的位置。
     * @return 返回相机位置 (igm::vec3)。
     */
    igm::vec3 GetPosition() const;

    /**
     * @brief 设置相机的焦点位置。
     * @param focal 焦点的位置 (igm::vec3)。
     */
    void SetFocal(const igm::vec3& focal);

    /**
     * @brief 设置相机的焦点位置。
     * @param focalX 焦点的 X 坐标。
     * @param focalY 焦点的 Y 坐标。
     * @param focalZ 焦点的 Z 坐标。
     */
    void SetFocal(float focalX, float focalY, float focalZ);

    /**
     * @brief 获取相机的焦点位置。
     * @return 返回焦点位置 (igm::vec3)。
     */
    igm::vec3 GetFocal() const;

    /**
     * @brief 设置相机的上方向。
     * @param up 相机的上方向 (igm::vec3)。
     */
    void SetUp(const igm::vec3& up);

    /**
     * @brief 设置相机的上方向。
     * @param upX 上方向的 X 坐标。
     * @param upY 上方向的 Y 坐标。
     * @param upZ 上方向的 Z 坐标。
     */
    void SetUp(float upX, float upY, float upZ);

    /**
     * @brief 获取相机的上方向。
     * @return 返回相机的上方向 (igm::vec3)。
     */
    igm::vec3 GetUp() const;

    /**
     * @brief 设置相机的投影类型。
     * @param type 投影类型 (Type)。
     */
    void SetType(Type type);

    /**
     * @brief 获取当前的投影类型。
     * @return 返回投影类型 (Type)。
     */
    Type GetType() const;

    /**
     * @brief 获取相机的前方向。
     * @return 返回前方向 (igm::vec3)。
     */
    igm::vec3 GetFront() const;

    /**
     * @brief 获取相机的右方向。
     * @return 返回右方向 (igm::vec3)。
     */
    igm::vec3 GetRight() const;

    /**
     * @brief 获取相机的视图矩阵。
     * @return 返回视图矩阵 (igm::mat4)。
     */
    igm::mat4 GetViewMatrix();

    /**
     * @brief 获取相机的投影矩阵。
     * @return 返回投影矩阵 (igm::mat4)。
     */
    igm::mat4 GetProjectionMatrix();

protected:
    Camera();
    ~Camera() override;

    /**
     * @brief 更新相机的方向向量（前方向和右方向）。
     */
    void UpdateVectors();

    // Camera attributes
    igm::vec3 m_Position;
    igm::vec3 m_Focal;
    igm::vec3 m_Up;

    igm::vec3 m_Front;
    igm::vec3 m_Right;

    Type m_Type;
};

IGAME_NAMESPACE_END