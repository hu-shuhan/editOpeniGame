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
#include "iGameCenterAxesModel.h"
#include "iGameInteractor.h"
#include "iGameLight.h"
#include "iGameModel.h"
#include "iGameShaderManager.h"
#include <chrono>

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
    void ResetCameraView(SmartPointer<DataObject> dataObject = nullptr);

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
     * @brief 将渲染帧率限制为指定值（帧率上限）。
     * @param fps 目标帧率，等于 0 则关闭此限制。
     */
    void SetTargetFps(unsigned int fps);
    unsigned int GetTargetFps() const { return m_TargetFps; }

    /**
     * @brief 按 GPU 使用率进行帧率节流。举例：0.5 表示让 GPU 在一帧时间内仅繁忙 ~50%。
     *
     * @details
     * 其代表的具体含义为分配给当前场景的 GPU 时间比例。 例如，上一帧渲染需要花费 120ms，
     * 则设置为 0.5 则代表会将每帧渲染平均花费控制在 60ms 以内，从而节省 GPU 资源用于其他任务。
     *
     * @param usagePercent 使用率 0~1，<=0 关闭。
     */
    void SetGpuUsageLimit(float usagePercent);
    float GetGpuUsageLimit() const { return m_GpuUsageLimit; }

    /**
     * @brief 显式开关帧率节流（当设置了 TargetFps 或 GpuUsageLimit 时自动开启）。
     */
    void EnableFramePacing(bool enable);

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
    void RotateClockwise(float angle);

    /**
     * @brief 切换中心坐标轴的显示状态
     */
    void ToggleCenterAxes();

    /**
     * @brief 获取中心坐标轴模型
     * @return 中心坐标轴模型指针
     */
    SmartPointer<CenterAxesModel> GetCenterAxesModel() const {
        return m_CenterAxesModel;
    }

    /**
     * @brief 获取当前旋转中心（世界坐标）
     */
    igm::vec3 GetRotationCenter() const;

    void UpdateAxisSize();

    /**
     * @brief 设置自定义旋转中心（世界坐标）
     */
    void SetRotationCenter(const igm::vec3 center);

    /**
     * @brief 重置旋转中心到包围球中心
     */
    void ResetRotationCenter() {
        m_UseCustomRotationCenter = false;
        this->Modified();
    }

    /**
     * @brief 获取旋转中心在相机空间的深度
     */
    float GetRotationCenterDepth() const;

    igm::vec3 ScreenToWorld(const igm::vec2& screenPos, float depth) const;

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

    SmartPointer<GLShaderProgram>
    GetShader(ShaderType type); //获取指定类型的着色器程序
    void
    UpdateModelsBoundingSphere(); //更新场景中所有可见模型的包围球(用于视锥剔除和相机定位)
    void InitOpenGL();
    void InitOIT();
    void InitAxes(); //初始化场景坐标轴
    void InitInterator();

    //缓冲区相关
    void ResizeFrameBuffer();
    void ResizeHzb();
    void RefreshHzb();
    void RefreshDrawCullDataBuffer();

    void DrawFrame(); //主渲染流程(执行所有渲染通道)
    void ResolveFrame();
    void RenderToQtFrame(); //将最终渲染结果输出到Qt的帧缓冲

    //渲染通道相关
    void ShadowPass();
    void ForwardPass();
    void TransparentPass();
    void VolumeRenderingPass();
    //更新各种UBO（用来存储着色语言中Uniform类型变量的缓冲区对象）
    void UpdateCameraDataBlock();
    void UpdateObjectDataBlock(SmartPointer<DataObject> obj);
    void UpdateUniformBufferObjectBlock(SmartPointer<DataObject> obj);
    void UpdateCameraClippingRange();

    bool ShouldRenderThisCall() const;

    SmartPointer<HandlePool<SmartPointer<Model>>> m_ModelPool; //模型池
    IGuint m_CurrentModelID;                                   //当前模型id


    std::function<void()> m_UpdateFunctor;
    std::function<void()> m_MakeCurrentFunctor;
    std::function<void()> m_DoneCurrentFunctor;

    SmartPointer<Camera> m_Camera;
    //Light> m_Light;
    SmartPointer<Axes> m_Axes;

    SmartPointer<Interactor> m_Interactor;

    SmartPointer<FontManager> m_FontManager;
    SmartPointer<ShaderManager> m_ShaderManager; //着色管理器

    igm::mat4
            m_ModelRotate; //Rotation matrix passing through the origin //绕原点的旋转矩阵
    igm::mat4 m_ModelMatrix; //模型变换矩阵
    igm::vec3 m_BackgroundColor;

    uint32_t m_VisibleModelsCount;    //可见模型数量
    igm::vec4 m_ModelsBoundingSphere; //场景包围球（中心坐标+半径）

    // used to draw full-screen triangle
    SmartPointer<GLVertexArray> m_EmptyVAO;

#ifdef IGAME_OPENGL_SUPPORT_MSAA //MSAA相关
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
    //OIT(顺序无关透明度)相关
    SmartPointer<GLTexture2d> m_OITHeadPointerTexture;
    SmartPointer<GLBuffer> m_OITHeadPointerInitializer;
    SmartPointer<GLBuffer> m_OITAtomicCounterBuffer;
    SmartPointer<GLBuffer> m_OITLinkedListBuffer;
    SmartPointer<GLTextureBuffer> m_OITLinkedListTexture;
    //HZB(层次Z缓冲)相关
    unsigned int m_HzbWidth, m_HzbHeight, m_HzbLevels;
    SmartPointer<GLTexture2d> m_HzbTexture;
    //绘制工具
    SmartPointer<Painter2D> m_Painter2D;
    SmartPointer<Painter3D> m_Painter3D;

    bool m_FinishInit;            // 是否完成初始化
    bool m_EnableVolumeRendering; // 是否启用体绘制

    // 新增成员变量
    SmartPointer<CenterAxesModel> m_CenterAxesModel;
    bool m_CenterAxesVisible = false; // 控制显示开关

    bool m_UseCustomRotationCenter = false;
    igm::vec3 m_CustomRotationCenter;

    // 帧率/使用率节流控制
    bool m_FramePacingEnabled = false; // 全局开关
    unsigned int m_TargetFps = 0.0f;   // >0：按目标 FPS 节流
    float m_GpuUsageLimit = 0.0f;      // (0,1]：按 GPU 使用率节流

    // GPU 计时器查询（双缓冲，避免等待当帧结果导致卡顿）
    int m_TimeQueryIndex = 0;
    unsigned int m_TimeQueries[2] = {0, 0};
    bool m_TimeQueryReady[2] = {false, false};

    // 平滑后的 GPU 帧时（毫秒）
    double m_LastGpuTimeMs = 0.0;
    double m_SmoothedGpuTimeMs = 0.0;

    // 上一帧结束的时间点（用于“提前返回”判定）
    std::chrono::steady_clock::time_point m_LastRenderEnd;
    bool m_LastRenderEndValid = false;

    // 记录是否处于交互状态
    bool m_IsInteracting = false;

    friend class RenderWindow;
    friend class Model;
    friend class Axes;
    friend class Interactor;
    friend class BasicStyle;
    friend class DragCenterStyle; // 拖拽中心样式
    friend class PainterBase;
    friend class Painter2D;
    friend class Painter3D;
};

IGAME_NAMESPACE_END
