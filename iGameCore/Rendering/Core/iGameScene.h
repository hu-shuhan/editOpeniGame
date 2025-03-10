/**
 * @class Scene
 * @brief 场景类，负责管理3D场景中的模型、相机、光源及渲染功能。
 *
 * @details
 *  Scene 类提供了对 3D 模型的添加、移除、渲染、可见性管理等功能。
 *  通过集成 OpenGL 渲染流程，支持多阶段渲染（阴影、透明、体积渲染等），并提供
 *  相机视角管理、屏幕捕获、交互器设置等操作。
 */

#pragma once

#include "OpenGL/GLFrameBuffer.h"
#include "OpenGL/GLIndirectCommand.h"
#include "OpenGL/GLShader.h"
#include "OpenGL/GLTextureBuffer.h"
#include "iGameAxes.h"
#include "iGameCamera.h"
#include "iGameFontManager.h"
#include "iGameInteractor.h"
#include "iGameLight.h"
#include "iGameModel.h"
#include "iGameShaderManager.h"

IGAME_NAMESPACE_BEGIN

class Scene : public Object {
public:
    I_OBJECT(Scene);

    /**
     * @brief 创建一个新的场景对象。
     * @return 场景对象的指针。
     */
    static Pointer New() { return new Scene; }

    /**
     * @brief 初始化场景。
     * @return 是否初始化成功。
     * @warning 调用函数前必须确保外部已正确创建OpenGL上下文，并且OpenGL上下文处于活跃状态。
     */
    bool Initialize();

    /**
     * @brief 添加一个数据对象作为模型。
     * @param meshleter 网格重建器对象指针。
     * @return 模型的唯一 ID。
     */
    IGuint AddModel(SmartPointer<Meshleter> meshleter);

    /**
     * @brief 添加一个数据对象作为模型。
     * @param dataObject 数据对象指针。
     * @return 模型的唯一 ID。
     */
    IGuint AddModel(SmartPointer<DataObject> dataObject);

    /**
     * @brief 根据索引移除模型。
     * @param modelID 模型索引。
     */
    void RemoveModel(IGuint modelID);

    /**
     * @brief 根据模型指针移除模型。
     * @param model 模型指针。
     */
    void RemoveModel(SmartPointer<Model> model);

    /**
     * @brief 移除当前选中的模型。
     */
    void RemoveCurrentModel();

    /**
     * @brief 设置当前模型。
     * @param modelID 模型索引。
     */
    void SetCurrentModel(int modelID);

    /**
     * @brief 设置当前模型。
     * @param model 模型指针。
     */
    void SetCurrentModel(SmartPointer<Model> model);

    /**
     * @brief 获取当前模型。
     * @return 当前模型的指针。
     */
    SmartPointer<Model> GetCurrentModel();

    /**
     * @brief 根据索引获取模型。
     * @param modelID 模型索引。
     * @return 模型指针。
     */
    SmartPointer<Model> GetModelById(int modelID);

    /**
     * @brief 根据索引获取数据对象。
     * @param modelID 数据对象索引。
     * @return 数据对象指针。
     */
    SmartPointer<DataObject> GetDataObjectById(int modelID);

    /**
     * @brief 获取模型列表。
     * @return 包含模型的映射表。
     */
    SmartPointer<HandlePool<SmartPointer<Model>>> GetModelList();

    /**
     * @brief 更改模型的可见性。
     * @param modelID 模型索引。
     * @param visibility 是否可见。
     */
    void ChangeModelVisibility(int modelID, bool visibility);

    /**
     * @brief 更改模型的可见性。
     * @param model 模型指针。
     * @param visibility 是否可见。
     */
    void ChangeModelVisibility(SmartPointer<Model> model, bool visibility);

    /**
     * @brief 设置场景背景颜色。
     * @param color 背景颜色。
     */
    void SetBackGround(const Color& color);

    /**
     * @brief 设置场景背景颜色，三原色范围0~255。
     * @param R 红色像素值。
     * @param G 绿色像素值。
     * @param B 蓝色像素值。
     */
    void SetBackGround(int R, int G, int B);

    /**
     * @brief 获取背景颜色。
     * @return 背景颜色RGB值。
     */
    igm::vec3 GetBackGround();

    /**
     * @brief 设置交互器。
     * @param interactor 交互器指针。
     */
    void SetInteractor(SmartPointer<Interactor> interactor);

    /**
     * @brief 获取交互器。
     * @return 交互器指针。
     */
    SmartPointer<Interactor> GetInteractor();

    /**
     * @brief 重置相机视角到默认视图。
     */
    void ResetCameraView();

    /**
     * @brief 获取相机。
     * @return 相机指针。
     */
    SmartPointer<Camera> GetCamera();

    /**
     * @brief 更改相机类型。
     * @param type 相机类型。
     */
    void ChangeCameraType(Camera::Type type);

    /**
     * @brief 获取当前模型的变换矩阵。
     * @return 模型变换矩阵。
     */
    igm::mat4 GetModelMatrix();

    /**
     * @brief 渲染一帧当前场景。
     */
    void Draw();

    /**
     * @brief 调整视口大小。
     * @param width 宽度。
     * @param height 高度。
     * @param pixelRatio 像素比例。
     */
    void Resize(int width, int height, int pixelRatio);

    /**
     * @brief 更新场景状态，调用前需要调用SetUpdateFunctor绑定自定义更新函数。
     */
    void Update();

    /**
     * @brief 将相机视角重置为各方向。
     */
    void ResetCameraViewToPositiveX();
    void ResetCameraViewToNegativeX();
    void ResetCameraViewToPositiveY();
    void ResetCameraViewToNegativeY();
    void ResetCameraViewToPositiveZ();
    void ResetCameraViewToNegativeZ();
    void ResetCameraViewToIsometric();
    void RotateNinetyClockwise();
    void RotateNinetyCounterClockwise();

    /**
     * @brief 启用或禁用体绘制。
     * @param toggled 是否启用。
     */
    void SetVolumeRendering(bool toggled);

    /**
     * @brief 捕获屏幕图像。
     * @param x 起始位置 X 坐标。
     * @param y 起始位置 Y 坐标。
     * @param width 宽度。
     * @param height 高度。
     * @param type 帧缓冲类型，有RGB, RGBA, DEPTH。
     * @param mirrored 是否镜像。
     * @return 屏幕图像数据。
     */
    std::vector<unsigned char> CaptureScreen(int x, int y, int width,
                                             int height,
                                             GLFramebuffer::Type type,
                                             bool mirrored);

    /**
     * @brief 获取 2D 绘制器。
     * @return Painter2D 指针。
     */
    SmartPointer<Painter2D> GetPainter2D();

    /**
     * @brief 获取 3D 绘制器。
     * @return Painter3D 指针。
     */
    SmartPointer<Painter3D> GetPainter3D();

    /**
     * @brief 设置当前 Scene 的 OpenGL 上下文为活动状态。
     *
     * @details
     * 调用此函数将当前 Scene 绑定为 OpenGL 渲染的活动上下文。
     * 在使用之前，需要通过 `SetMakeCurrentFunctor` 设置对应的函数指针。
     * 通常用于确保在渲染操作之前，OpenGL 的上下文已经切换到当前场景。
     */
    void MakeCurrent();

    /**
     * @brief 释放当前 Scene 的 OpenGL 上下文。
     *
     * @details
     * 调用此函数会释放当前 Scene 的 OpenGL 渲染上下文。
     * 在使用之前，需要通过 `SetDoneCurrentFunctor` 设置对应的函数指针。
     * 通常用于在完成当前场景的渲染后，清理和释放上下文资源。
     */
    void DoneCurrent();

    /**
     * @brief 绑定自定义更新函数。
     */
    template<typename Functor, typename... Args>
    void SetUpdateFunctor(Functor&& functor, Args&&... args) {
        m_UpdateFunctor = std::bind(std::forward<Functor>(functor),
                                    std::forward<Args>(args)...);
    }

    /**
     * @brief 绑定自定义的 OpenGL 上下文设置函数。
     */
    template<typename Functor, typename... Args>
    void SetMakeCurrentFunctor(Functor&& functor, Args&&... args) {
        m_MakeCurrentFunctor = std::bind(std::forward<Functor>(functor),
                                         std::forward<Args>(args)...);
    }

    /**
     * @brief 绑定自定义的 OpenGL 上下文释放函数。
     */
    template<typename Functor, typename... Args>
    void SetDoneCurrentFunctor(Functor&& functor, Args&&... args) {
        m_DoneCurrentFunctor = std::bind(std::forward<Functor>(functor),
                                         std::forward<Args>(args)...);
    }

protected:
    Scene();
    ~Scene() override;

    SmartPointer<GLShaderProgram> GetShader(ShaderType type);
    void UpdateModelsBoundingSphere();
    void InitOpenGL();
    void InitOIT();
    void InitAxes();
    void InitInterator();

    void ResizeFrameBuffer();
    void ResizeHzb();
    void RefreshHzb();
    void RefreshDrawCullDataBuffer();

    void DrawFrame();
    void ResolveFrame();
    void RenderToQtFrame();

    void ShadowPass();
    void ForwardPass();
    void TransparentPass();
    void VolumeRenderingPass();

    void UpdateCameraDataBlock();
    void UpdateObjectDataBlock(SmartPointer<DataObject> obj);
    void UpdateUniformBufferObjectBlock(SmartPointer<DataObject> obj);
    void UpdateCameraClippingRange();
    static void CalculateFrameRate();

    SmartPointer<HandlePool<SmartPointer<Model>>> m_Models;
    IGuint m_CurrentModelID;
    //SmartPointer<HandlePool<SmartPointer<Meshleter>>> m_Meshleters;

    std::function<void()> m_UpdateFunctor;
    std::function<void()> m_MakeCurrentFunctor;
    std::function<void()> m_DoneCurrentFunctor;

    SmartPointer<Camera> m_Camera;
    //Light> m_Light;
    SmartPointer<Axes> m_Axes;

    SmartPointer<Interactor> m_Interactor;

    SmartPointer<FontManager> m_FontManager;
    SmartPointer<ShaderManager> m_ShaderManager;

    igm::mat4 m_ModelRotate; //Rotation matrix passing through the origin
    igm::mat4 m_ModelMatrix;
    igm::vec3 m_BackgroundColor;

    uint32_t m_VisibleModelsCount;
    igm::vec4 m_ModelsBoundingSphere;

    // used to draw full-screen triangle
    SmartPointer<GLVertexArray> m_EmptyVAO;

#ifdef GL_SUPPORTS_MSAA
    GLint samples;
    SmartPointer<GLFramebuffer> m_FramebufferMultisampled;
    SmartPointer<GLTexture2dMultisample> m_ColorTextureMultisampled;
    SmartPointer<GLTexture2dMultisample> m_DepthTextureMultisampled;

    SmartPointer<GLFramebuffer> m_FramebufferResolved;
    SmartPointer<GLTexture2d> m_ColorTextureResolved;
    SmartPointer<GLTexture2d> m_DepthTextureResolved;
#else
    SmartPointer<GLFramebuffer> m_Framebuffer;
    SmartPointer<GLTexture2d> m_ColorTexture;
    SmartPointer<GLTexture2d> m_DepthTexture;
#endif

    SmartPointer<GLTexture2d> m_OITHeadPointerTexture;
    SmartPointer<GLBuffer> m_OITHeadPointerInitializer;
    SmartPointer<GLBuffer> m_OITAtomicCounterBuffer;
    SmartPointer<GLBuffer> m_OITLinkedListBuffer;
    SmartPointer<GLTextureBuffer> m_OITLinkedListTexture;

    unsigned int m_HzbWidth, m_HzbHeight, m_HzbLevels;
    SmartPointer<GLTexture2d> m_HzbTexture;

    SmartPointer<Painter2D> m_Painter2D;
    SmartPointer<Painter3D> m_Painter3D;

    bool m_FinishInit;
    bool m_EnableVolumeRendering;

    friend class RenderWindow;
    friend class Model;
    friend class Axes;
    friend class Interactor;
    friend class BasicStyle;
    friend class PainterBase;
    friend class Painter2D;
    friend class Painter3D;
};

IGAME_NAMESPACE_END