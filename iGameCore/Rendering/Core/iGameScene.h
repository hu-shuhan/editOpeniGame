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
#include "OpenGL/GLTexture3d.h"
#include "OpenGL/GLTextureBuffer.h"
#include "iGameAxes.h"
#include "iGameCamera.h"
#include "iGameCenterAxesModel.h"
#include "iGameColorBar2DActor.h"
#include "iGameInteractor.h"
#include "iGameLight.h"
#include "iGameModel.h"
#include "iGameShaderManager.h"
#include "iGameTextOverlay2DActor.h"
#include <chrono>
#ifndef __EMSCRIPTEN__
#include "iGameEnvironmentLight.h"
#endif

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
    IGuint GetCurrentModelID();
    /**
     * @brief 根据索引获取模型。
     * @param modelID 模型索引。
     * @return 模型指针。
     */
    SmartPointer<Model> GetModelById(int modelID);
    bool SetModelById(int modelID, SmartPointer<Model>model);

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
     * @brief 设置场景背景颜色渐变，三原色范围0~255。
     * @param R1 起始颜色红色像素值。
     * @param G1 起始颜色绿色像素值。
     * @param B1 起始颜色蓝色像素值。
     * @param R2 结束颜色红色像素值。
     * @param G2 结束颜色绿色像素值。
     * @param B2 结束颜色蓝色像素值。
     * @param mode 渐变模式。
     */
    void SetBackGroundGradient(int R1, int G1, int B1, int R2, int G2, int B2,
                               int mode);

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
     * @brief 释放场景资源。
     */
    void Finalize();

    /**
     * @brief 重置相机视角到默认视图。
     */
    void ResetCameraView();
    void ResetCameraView(const BoundingBox& bbox);
    void ResetCameraView(SmartPointer<DataObject> dataObject);

    /**
     * @brief 获取相机。
     * @return 相机指针。
     */
    SmartPointer<Camera> GetCamera();

    /**
     * @brief 设置表面着色模式。
     *
     * mode: 0 = Blinn-Phong（旧观感）、1 = 无光照（仅Web端生效）、
     *       2 = PBR（仅桌面端生效，默认值）。超出 [0, 2] 时回退为 0。
     *
     * @param mode 着色模式。
     */
    void SetSurfaceShadingMode(int mode);
    /**
     * @brief 获取表面着色模式。
     */
    int GetSurfaceShadingMode() const;

#ifndef __EMSCRIPTEN__
    // Shared by desktop surface shaders; never drawn as the background.
    void SetEnvironmentEnabled(bool enabled);
    bool GetEnvironmentEnabled() const { return m_EnvironmentEnabled; }
    void SetEnvironmentIntensity(float intensity);
    float GetEnvironmentIntensity() const { return m_EnvironmentIntensity; }
    void SetEnvironmentRotation(float degrees);
    float GetEnvironmentRotation() const { return m_EnvironmentRotation; }
    void ApplyEnvironmentUniforms(const SmartPointer<GLShaderProgram>& shader) const;
#endif

    /**
     * @brief 表面材质预设，仅作用于桌面端使用 PBR 着色的不透明表面。
     */
    enum class MaterialPreset {
        GlossyPlastic = 0, ///< 反光塑料（默认）
        Matte,             ///< 哑光
        PolishedMetal,     ///< 光滑金属
        BrushedMetal,      ///< 拉丝金属（较粗糙）
    };

    /**
     * @brief 设置表面材质预设。
     * @param preset 材质预设。
     */
    void SetMaterialPreset(MaterialPreset preset);
    /**
     * @brief 获取表面材质预设。
     */
    MaterialPreset GetMaterialPreset() const;
    /**
     * @brief 获取当前材质预设对应的金属度与粗糙度。
     * @param metallic 输出金属度。
     * @param roughness 输出粗糙度。
     */
    void GetMaterialParameters(float& metallic, float& roughness) const;
    /**
     * @brief 直接设置 PBR 材质参数（金属度 / 粗糙度）。
     *
     * 与 SetMaterialPreset 共用同一份状态：预设等价于「把金属度与粗糙度一次
     * 置为某组常用值」，因此界面上的两个滑块与预设可以互相覆盖。
     * 参数是全局的（Scene 级），对所有使用 PBR 着色的不透明表面生效。
     *
     * @param metallic 金属度，取值 [0, 1]。
     * @param roughness 粗糙度，取值 [0, 1]（着色器内部还会再夹到最小粗糙度）。
     */
    void SetMaterialParameters(float metallic, float roughness);

    /**
     * @brief 设置 PBR 环境遮蔽强度（简易半球近似）。
     *
     * 与金属度/粗糙度一样是全局（Scene 级）参数，只作用于桌面端使用 PBR 着色的
     * 不透明表面。0 = 关闭（画面与未引入本项时一致），1 = 全强度。实现见
     * PBR.frag 的 HemisphereAo()：按「法线与世界 +Y 的夹角」压暗间接光，
     * 不是屏幕空间 AO，不反映真实遮挡；色带模型在着色器内强制关闭。
     *
     * @param strength 环境遮蔽强度，取值 [0, 1]。
     */
    void SetSurfaceAo(float strength);
    /**
     * @brief 获取 PBR 环境遮蔽强度。
     * @return 环境遮蔽强度，取值 [0, 1]。
     */
    float GetSurfaceAo() const;


    /**
     * @brief 设置 Blinn-Phong 高光参数。
     *
     * 仅作用于桌面端使用 Blinn-Phong 着色的表面。高光强度默认为 0，
     * 设置为 0 可关闭镜面高光及环境镜面反射。
     *
     * @param strength 高光强度，取值 [0, 1]。
     * @param shininess 高光指数，取值 [1, 100]，越大高光越集中。
     */
    void SetBlinnPhongParameters(float strength, float shininess);
    /**
     * @brief 获取 Blinn-Phong 高光参数。
     * @param strength 输出高光强度。
     * @param shininess 输出高光指数。
     */
    void GetBlinnPhongParameters(float& strength, float& shininess) const;

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
     * @brief 设置中心坐标轴的显示状态
     * @param visible 是否显示中心坐标轴
     */
    void SetCenterAxesVisible(bool visible);

    /**
     * @brief 切换底板（地面网格）的显示状态
     */
    void ToggleGroundGrid();
    /**
     * @brief 设置底板（地面网格）的显示状态
     * @param visible 是否显示底板
     */
    void SetGroundGridVisible(bool visible);
    /**
     * @brief 获取底板（地面网格）的显示状态
     * @return 是否显示底板
     */
    bool GetGroundGridVisible() const;

#ifndef __EMSCRIPTEN__
    /**
     * @brief 依据当前可见模型的包围盒更新底板参数。
     *
     * 底板是一层水平网格，位于内容包围盒底部（XZ 平面，y 为包围盒 ymin 略向
     * 下沉）。参考格距按"最长边（含 Y）/ 10"自动选档（1/2/5 x 10^n），只在
     * 内容变化时重算一次，同一批内容下参考格距固定；网格锚定世界原点，因此
     * 调整格距档位或扩大可见范围时，已有线都不会移动。
     *
     * 桌面端底板由片元着色器逐像素解析生成（无限延伸、无边界、无几何），这里
     * 只准备参数；不参与 Web 渲染流程。
     *
     * "内容"= 可见模型的联合包围盒；没有可见模型时优先沿用上一次加载过的
     * 内容（删掉/再加载同量级模型不会跳），完全没加载过则退化为引擎默认的
     * 单位包围球（中心原点、半径 1）——相机取景用的也是这个球，所以空场景
     * 和"加载一个同等大小的模型"观感一致。模型集合变化后由 Scene 内部自动
     * 更新，外部一般无需直接调用。
     */
    void UpdateGroundGrid();

    /**
     * @brief 更新底板的可见半径（模型空间）。
     *
     * 既保证能盖住模型本身，又跟随"相机到平面的高度"一起变大（拉远时网格
     * 跟着铺开），桌面端用它作为距离渐隐半径并撑开相机远裁剪面；每帧相机变化
     * 后调用，CameraClippingRange 更新前生效。
     */
    void UpdateGroundGridExtent();

    /**
     * @brief 绘制桌面端的解析式无限底板（仅桌面端，见 GroundGrid.frag）。
     */
    void DrawGroundGrid();
#endif

    /**
     * @brief 切换XYZ坐标轴的显示状态
     */
    void ToggleAxes();
    /**
     * @brief 设置XYZ坐标轴的显示状态
     * @param visible 是否显示XYZ坐标轴
     */
    void SetAxesVisible(bool visible);
    /**
     * @brief 获取XYZ坐标轴的显示状态
     * @return 是否显示XYZ坐标轴
     */
    bool GetAxesVisible() const;
    
    /**
     * @brief 切换ColorBar的显示状态（仅Web端），桌面端ColorBar为Qt路径实现
     * @return 是否显示ColorBar
     */
    void ToggleColorBar();
    /**
     * @brief 设置ColorBar的显示状态（仅Web端）
     */
    void SetColorBarVisible(bool visible);
    /**
     * @brief 获取ColorBar的显示状态（仅Web端）
     * @return 是否显示ColorBar
     */
    bool GetColorBarVisible() const;
    /**
     * @brief 获取ColorBar2DActor对象（仅Web端）
     * @return ColorBar2DActor对象的智能指针
     */
    void ToggleOpacityMappingEnabled();
    /**
     * @brief 设置是否启用不透明度映射
     */
    void SetOpacityMappingEnabled(bool enabled);
    /**
     * @brief 获取ColorBar的显示状态（仅Web端）
     * @return 是否显示ColorBar
     */
    bool GetOpacityMappingEnabled() const;
    /**
     * @brief 获取ColorBar2DActor对象（仅Web端）
     * @return ColorBar2DActor对象的智能指针
     */
    SmartPointer<ColorBar2DActor> GetColorBar2DActor() const;
    /**
     * @brief 设置角标文本内容
     * @param text 角标文本内容
     */
    void SetCornerAnnotationText(const std::string& text);
    /**
     * @brief 设置角标文本位置
     * @param left 左上角X坐标
     * @param top 左上角Y坐标
     */
    void SetCornerAnnotationPosition(float left, float top);
    /**
     * @brief 设置角标锚点位置
     * @param anchorToBottomRight 是否锚定到底部右侧
     */
    void SetCornerAnnotationAnchorToBottomRight(bool anchorToBottomRight);
    /**
     * @brief 设置角标的可见性
     * @param visible 是否可见
     */
    void SetCornerAnnotationVisible(bool visible);
    /**
     * @brief 获取TextOverlay2DActor对象的智能指针
     * @return TextOverlay2DActor对象的智能指针
     */
    SmartPointer<TextOverlay2DActor> GetTextOverlay2DActor() const;

    /**
     * @brief 获取中心坐标轴模型
     * @return 中心坐标轴模型指针
     */
    SmartPointer<CenterAxesModel> GetCenterAxesModel() const;

    /**
     * @brief 获取当前旋转包围球信息
     */
    igm::vec4 GetRotationBoundingSphere() const;

    void UpdateAxisSize();

    /**
     * @brief 设置自定义旋转中心（世界坐标）
     */
    void SetRotationBoundingSphere(const igm::vec4 boundingSphere);

    /**
     * @brief 重置旋转中心到包围球中心
     */
    void ResetRotationBoundingSphere();

    /**
     * @brief 获取旋转中心在相机空间的深度
     */
    float GetRotationCenterDepth() const;

    igm::vec3 ScreenToWorld(const igm::vec2& screenPos, float depth) const;

    /**
     * @enum  VolumeRenderingMode
     * @brief 体绘制的实现方式。
     */
    enum class VolumeRenderingMode {
        Off = 0,   ///< 未开启
        Fragment,  ///< 旧的“片元链表”实现（非结构化网格 / 不支持 compute 时使用）
        Raycast    ///< compute shader 光线步进实现（结构化网格）
    };

    /**
     * @brief 启用或禁用体绘制（唯一对外入口，按数据类型自动选择实现）。
     * @param toggled 是否启用。
     * @param message 选型说明或失败原因（可为空）。
     * @return 实际采用的实现方式；Off 表示开启失败。
     *
     * @details
     *  结构化网格 → compute shader 光线步进（SetVolumeRaycasting）；
     *  其它情况   → 旧的“片元链表”实现（SetFragmentVolumeRendering），包括：
     *  非结构化网格、多块/动画数据（含子数据对象，播放时子对象会被重建，
     *  无法安全持有）、以及当前后端不支持 compute 的情况。
     *  这样既保留了老实现的能力，又让结构化网格走高效路径。
     */
    VolumeRenderingMode SetVolumeRendering(bool toggled,
                                           std::string* message = nullptr);

    /**
     * @brief 获取当前体绘制采用的实现方式。
     */
    VolumeRenderingMode GetVolumeRenderingMode() const;

    /**
     * @brief 显式启用旧的“片元链表”体绘制实现。
     * @note 仅供对照/回退调试；常规入口请使用 SetVolumeRendering。
     */
    void SetFragmentVolumeRendering(bool toggled);

    /**
     * @enum  VolumeRaycastingStatus
     * @brief compute shader 光线步进体绘制的可用性检查结果。
     */
    enum class VolumeRaycastingStatus {
        Ok = 0,             ///< 可用
        UnsupportedBackend,  ///< 当前图形后端/驱动不支持（需要 OpenGL 4.3+ 的 compute）
        NoStructuredMesh,    ///< 场景中没有可用于光线步进的结构化网格（非结构化网格请走 Fragment 实现）
        NoScalarAttribute,   ///< 没有可用的标量属性
        InvalidAttribute,    ///< 属性数据规模与网格不匹配（例如 ghost 之类的单元数据）
        MultiBlockUnsupported ///< 多块/动画数据（含子数据对象），暂不支持 compute 光线步进
    };

    /**
     * @brief 启用或禁用 compute shader 光线步进体绘制（仅结构化网格）。
     * @param toggled 是否启用。
     * @param message 失败原因（可为空）。
     * @return 检查结果；只有 Ok 时才真正开启。
     *
     * @details
     *  这是“只走 compute 实现”的显式接口，供 SetVolumeRendering 的分派逻辑
     *  以及需要固定实现的调用方使用；普通业务请直接用 SetVolumeRendering，
     *  它会按数据类型自动选择实现。开启前先做可用性检查：后端是否支持 compute、
     *  场景中是否存在结构化网格、是否存在可用的标量属性；任何一项不满足都返回
     *  对应状态，由调用方决定回退（Fragment）还是提示用户。
     *  仅在 OpenGL 4.6 后端可用（Web / OpenGL 3.3 不支持 compute 与 3D 纹理）。
     */
    VolumeRaycastingStatus SetVolumeRaycasting(bool toggled,
                                               std::string* message = nullptr);

    /**
     * @brief 检查 compute 体绘制当前是否可用。
     * @param message 失败原因（可为空）。
     * @note 只做 CPU 侧检查与运行时 GL 版本查询，不创建 GPU 资源；
     *       调用时请确保有活动的 OpenGL 上下文（GL 版本查询需要）。
     */
    VolumeRaycastingStatus
    CheckVolumeRaycastingSupport(std::string* message = nullptr);

    /**
     * @brief 运行时判断当前 OpenGL 上下文是否支持 compute 体绘制（GL 4.3+）。
     * @warning 需要当前线程有活动的 OpenGL 上下文。
     */
    bool IsVolumeRaycastingAvailable();

    /**
     * @brief 当前场景是否存在多块/动画数据（带子数据对象的可绘制对象）。
     *
     * @details
     *  动画型多块数据在播放时会 Clear/Add 子数据对象，compute 光线步进既无法用
     *  单个体纹理覆盖多块，也不能安全持有子对象指针（随时可能被重建），
     *  因此这类数据统一交给旧的“片元链表”实现。
     */
    bool HasMultiBlockData();

    /**
     * @brief 取出并清空“体绘制运行期失败”的待提示消息。
     * @param message 失败原因（可为空）。
     * @return 是否有待提示的消息。
     *
     * @details
     *  用于渲染循环内发现“光线步进体绘制构建失败”时（例如结构化网格的属性
     *  被移除、数据被移出场景），由界面层轮询后弹窗提示。
     *  注意：切换到非结构化网格属于正常选型切换，会自动改用旧的片元链表
     *  实现，不会走这里。
     *  每次取出后即清空，避免重复弹窗。
     */
    bool ConsumeVolumeRaycastFailure(std::string* message = nullptr);

    /**
     * @brief 获取 compute 光线步进是否处于开启状态。
     * @note 只反映 compute 实现的开关；要判断“当前用的是哪种实现”请用
     *       GetVolumeRenderingMode()。
     */
    bool GetVolumeRaycasting() const { return m_EnableVolumeRaycasting; }

    /**
     * @brief 由数据对象构建体数据（结构化网格映射到 3D 纹理）。
     * @param dataObject 数据对象，当前仅支持 StructuredMesh。
     * @param attributeIndex 作为标量场的属性索引。
     * @param attributeDimension 属性分量（-1 表示取模长；单分量属性忽略该参数）。
     * @return 是否构建成功。
     */
    bool BuildVolumeData(SmartPointer<DataObject> dataObject,
                         int attributeIndex = 0,
                         int attributeDimension = -1);

    /**
     * @brief 使用当前场景中的第一个体网格构建体数据。
     * @return 是否构建成功。
     */
    bool BuildVolumeDataFromCurrentModel();

    /**
     * @brief 设置 compute 体绘制的采样步数与不透明度缩放。
     * @param numSteps 每条光线的最大采样步数（越大越精细、越慢）。
     * @param opacityScale 不透明度缩放（越大越浓）。
     */
    void SetVolumeRaycastingQuality(int numSteps, float opacityScale);

    /**
     * @brief 体绘制掩膜（blanking ）的工作模式。
     *
     * @details
     *  数据里带 vtkValidPointMask（点，1 = 有效）或 vtkGhostType（单元，0 = 有效）
     *  时可用（ParaView 的 ResampleToImage 勾 "Mark Blank Points and Cells" 就会写）。
     *  掩膜纹理是 R8：1 = 有效（流体），0 = 无效（车体内部 / 域外）。同一个步进
     *  循环里对掩膜层与标量层做同坐标合成，所以两层空间关系是正确的。
     */
    enum class VolumeMaskMode {
        Off = 0,   ///< 不使用掩膜（默认）
        Carve = 1, ///< 挖空：无效体素完全不贡献颜色（车体变成空腔）
        Shell = 2  ///< 车壳实体：无效体素用固定颜色 + 固定不透明度画成实体
    };

    /// 设置掩膜模式（Off/Carve/Shell）。数据里没有可用数组时模式不生效（只记日志）。
    void SetVolumeRaycastMaskMode(VolumeMaskMode mode);
    VolumeMaskMode GetVolumeRaycastMaskMode() const { return m_VolumeMaskMode; }
    /// 兼容写法：等价于 SetVolumeRaycastMaskMode(enabled ? Carve : Off)。
    void SetVolumeRaycastMaskEnabled(bool enabled);
    /// 车壳实体模式下的外壳不透明度（0~1，默认 0.9）。
    void SetVolumeRaycastShellOpacity(float opacity);
    float GetVolumeRaycastShellOpacity() const { return m_VolumeShellOpacity; }
    /// 掩膜是否实际生效（模式 != Off && 数据里确实有可用的空白数组）。
    bool GetVolumeRaycastMaskEnabled() const { return m_VolumeMaskEnabled; }
    /// 数据里是否存在可用的空白数组（上一次构建体数据时的探测结果）。
    bool GetVolumeRaycastMaskAvailable() const { return m_VolumeMaskAvailable; }

    /**
     * @brief 体绘制的只读状态快照（供「体绘制」设置面板显示）。
     *
     * @details
     *  视口大小是逻辑像素（窗口内渲染区），渲染分辨率是设备像素，也就是
     *  compute shader 实际派发的分辨率（两者之比即 devicePixelRatio）。
     *  体数据维度/体素间距来自已构建的体纹理，未构建时维度为 0。
     */
    struct VolumeRenderingInfo {
        VolumeRenderingMode mode = VolumeRenderingMode::Off; ///< 当前实现方式
        int viewportWidth = 0;       ///< 视口宽度（逻辑像素）
        int viewportHeight = 0;      ///< 视口高度（逻辑像素）
        int renderWidth = 0;         ///< 渲染分辨率宽度（设备像素）
        int renderHeight = 0;        ///< 渲染分辨率高度（设备像素）
        int volumeDimension[3] = {0, 0, 0};          ///< 体数据维度
        float voxelSpacing[3] = {0.0f, 0.0f, 0.0f};  ///< 体素间距（局部单位）
        int stepCount = 0;           ///< 每条光线最大采样步数
        float opacityScale = 0.0f;   ///< 不透明度缩放
        int maskMode = 0;              ///< 掩膜模式：0=关闭 1=挖空 2=车壳实体
        bool maskAvailable = false;    ///< 数据里是否有可用的空白/ghost 数组
        bool maskEnabled = false;      ///< 掩膜是否实际生效（模式 != 0 && 可用）
        float maskInvalidRatio = 0.0f; ///< 无效体素占比 0..1（诊断用）
        float shellOpacity = 0.9f;     ///< 实体模式的外壳不透明度
        bool active = false;         ///< 最近 1.5 秒内是否发生过重绘（否则为空闲）
        double fps = 0.0;            ///< 实测帧率（1 秒滑动窗口）
        double gpuFrameTimeMs = 0.0; ///< GPU 帧时间（平滑值，整帧）
    };

    /// 取体绘制状态快照（设置面板显示用）。
    VolumeRenderingInfo GetVolumeRenderingInfo();

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

#ifndef __EMSCRIPTEN__
    // 计算底板的"内容包围盒"= 可见模型的联合包围盒（跳过中心坐标轴等
    // 辅助对象，也不包含底板自身）。没有可见模型时返回空包围盒。
    // 取包围盒是廉价的（DataObject::ComputeBoundingBox 内部有 MTime 守卫），
    // 因此可以每帧调用，用来判断底板是否需要重建。
    BoundingBox ComputeGroundGridContentBox();
#endif
    void InitOpenGL();
    void InitOIT();
    void InitAxes(); //初始化场景坐标轴
    void InitInterator();

    //缓冲区相关
    void ResizeFrameBuffer();
    void ResizeHzb();
    void RefreshHzb();
    void RefreshDrawCullDataBuffer();

    void DrawFrame();
    void RenderToSpecificFrame(GLint frameBuffer);
    void ResolveFrameBuffer();

    //渲染通道相关
    void ShadowPass();
    void ForwardPass();
    void TransparentPass();
    // 旧的“片元链表”体绘制通道（非结构化网格、或 compute 不可用时使用）
    void VolumeRenderingPass();
    // compute shader 光线步进体绘制通道（结构化网格）
    void VolumeRaycastPass();
    // 构建默认传输函数 LUT（256x1 RGBA8）
    void BuildVolumeTransferFunction(ScalarsToColors::Pointer colorMapper,
                                     float valueMin, float valueMax);
    // 查找当前场景中的体数据来源（第一个可见的结构化网格）
    SmartPointer<DrawObject> FindVolumeSource();
    void ClearSceneFramebuffer(float depth, int width, int height);

    //更新各种UBO（用来存储着色语言中Uniform类型变量的缓冲区对象）
    void UpdateCameraDataBlock();
    void UpdateObjectDataBlock(SmartPointer<DataObject> obj);
    void UpdateUniformBufferObjectBlock(SmartPointer<DataObject> obj);
    void UpdateCameraClippingRange();

    void BindFramebuffer() const;
    bool ShouldRenderThisCall() const;

    SmartPointer<HandlePool<SmartPointer<Model>>> m_ModelPool; //模型池
    IGuint m_CurrentModelID;                                   //当前模型id


    std::function<void()> m_UpdateFunctor;
    std::function<void()> m_MakeCurrentFunctor;
    std::function<void()> m_DoneCurrentFunctor;

    SmartPointer<Camera> m_Camera;
    //Light> m_Light;
    bool m_AxesVisible {true};
    SmartPointer<Axes> m_Axes;
    bool m_ColorBarVisible {false};
    bool m_OpacityMappingEnabled {false};
    SmartPointer<ColorBar2DActor> m_ColorBar2DActor;
    SmartPointer<TextOverlay2DActor> m_TextOverlay2DActor;

    SmartPointer<Interactor> m_Interactor;

    SmartPointer<FontManager> m_FontManager;
    SmartPointer<ShaderManager> m_ShaderManager; //着色管理器

    igm::mat4
            m_ModelRotate; //Rotation matrix passing through the origin //绕原点的旋转矩阵
    igm::mat4 m_ModelMatrix; //模型变换矩阵
    igm::vec3 m_BackgroundColor;
    igm::vec3 m_BackgroundGradientColor;
    bool m_BackgroundGradientEnabled = false;
    int m_BackgroundGradientMode = 0;

    uint32_t m_VisibleModelsCount;    //可见模型数量
    igm::vec4 m_ModelsBoundingSphere; //场景包围球（中心坐标+半径）

    bool m_UseCustomRotationBoundingSphere = false;
    igm::vec4 m_CustomRotationBoundingSphere;

    bool m_CenterAxesVisible = false;
    SmartPointer<CenterAxesModel> m_CenterAxesModel;

    // used to draw full-screen triangle
    SmartPointer<GLVertexArray> m_EmptyVAO;

    SmartPointer<GLFramebuffer> m_Framebuffer;
    SmartPointer<GLTexture2d> m_ColorTexture;
    SmartPointer<GLTexture2d> m_DepthR32FTexture;
    SmartPointer<GLTexture2d> m_DepthTexture;

    SmartPointer<GLFramebuffer> m_FramebufferBackup;
    SmartPointer<GLTexture2d> m_ColorTextureBackup;

#ifdef GL_SUPPORT_MSAA
    GLint samples;
    SmartPointer<GLFramebuffer> m_FramebufferMultisampled;
    SmartPointer<GLTexture2dMultisample> m_ColorTextureMultisampled;
    SmartPointer<GLTexture2dMultisample> m_DepthTextureMultisampled;
#endif

    //OIT(顺序无关透明度)相关
    SmartPointer<GLTexture2d> m_OITHeadPointerTexture;
    SmartPointer<GLBuffer> m_OITHeadPointerInitializer;
    SmartPointer<GLBuffer> m_OITAtomicCounterBuffer;
    SmartPointer<GLBuffer> m_OITLinkedListBuffer;
    SmartPointer<GLTextureBuffer> m_OITLinkedListTexture;

    SmartPointer<GLFramebuffer> m_VolumeFramebuffer;
    SmartPointer<GLTexture2d> m_VolumeColorTexture;
    SmartPointer<GLTexture2d> m_VolumeDepthTexture;

    //HZB(层次Z缓冲)相关
    unsigned int m_HzbWidth, m_HzbHeight, m_HzbLevels;
    SmartPointer<GLTexture2d> m_HzbTexture;
    //绘制工具
    SmartPointer<Painter2D> m_Painter2D;
    SmartPointer<Painter3D> m_Painter3D;

#ifndef __EMSCRIPTEN__
    // 桌面端解析式底板：不进模型池，不参与包围盒统计、相机取景与拾取。
    bool m_GroundGridVisible {true};  // 底板显示开关
    bool m_GroundGridDirty {true};    // 需要重建底板几何
    bool m_GroundGridBuilt {false};   // 当前是否已有可用底板几何
    // 上一次加载过的"内容包围盒"：空场景时沿用它，使删掉/再加载同量级模型
    // 时底板不跳（完全没加载过则用默认单位包围球）。
    BoundingBox m_GroundGridLastContentBox;
    // 上一次用于建板的"内容"（可见模型联合包围盒，可能为空）。每帧与当前
    // 内容比较，只要变了就重建底板，避免漏掉"包围盒晚一帧才就绪"的情况。
    BoundingBox m_GroundGridContentBox;
    igm::vec3 m_GroundGridCenter {0.0f, 0.0f, 0.0f}; // 底板中心（模型空间）
    float m_GroundGridRadius {0.0f};  // 底板外接圆半径（模型空间）
    float m_GroundGridCell {0.02f};   // 参考格距（模型空间，1/2/5 x 10^n）
    float m_GroundGridContentScale {1.0f}; // 内容包围盒最长边（含 Y，模型空间）
#endif

    bool m_FinishInit;            // 是否完成初始化
    // 是否启用旧的“片元链表”体绘制（VolumeRenderingMode::Fragment）
    bool m_EnableVolumeRendering;

    // compute shader 光线步进体绘制相关（VolumeRenderingMode::Raycast）
    bool m_EnableVolumeRaycasting = false; // 是否启用光线步进体绘制
    bool m_VolumeDataDirty = true;         // 需要（重新）构建体数据
    SmartPointer<GLTexture3d> m_VolumeTexture3d; // 体数据 3D 纹理
    SmartPointer<GLTexture3d> m_VolumeMaskTexture3d; // 体数据掩膜（R8，1=有效）
    VolumeMaskMode m_VolumeMaskMode = VolumeMaskMode::Off; // 掩膜模式（默认关闭）
    bool m_VolumeMaskAvailable = false; // 数据里是否有可用的空白/ghost 数组
    bool m_VolumeMaskEnabled = false;   // 掩膜是否实际生效（模式 != Off && 可用）
    std::string m_VolumeMaskSource;      // 掩膜来源数组名（日志用）
    float m_VolumeMaskValidRatio = 0.0f; // 有效体素占比 0..1（日志用）
    bool m_VolumeMaskReadbackDone = false; // 掩膜纹理一次性回读自检
    // 车壳实体模式（VolumeMaskMode::Shell）的参数：固定颜色 + 不透明度
    float m_VolumeShellOpacity = 0.9f;
    igm::vec3 m_VolumeShellColor{0.85f, 0.86f, 0.88f};
    SmartPointer<GLTexture2d> m_VolumeTransferFunction; // 传输函数 LUT
    SmartPointer<DataObject> m_VolumeSourceObject; // 体数据来源对象
    unsigned int m_VolumeSourceMTime = 0; // 来源对象的修改时间（用于失效检测）
    // 属性切换不会修改 DataObject 的 MTime，需要显式缓存这三项用于失效判定
    int m_VolumeAttributeIndex = -1;
    int m_VolumeAttributeDimension = -1;
    unsigned int m_VolumeMapperMTime = 0;
    // 局部索引空间 -> 体纹理空间；每帧再左乘模型矩阵的逆得到世界空间的映射，
    // 这样交互旋转/平移体数据才会跟随网格（模型矩阵变化不会触发 MTime）
    igm::mat4 m_VolumeLocalToTexture{1.0f};
    igm::vec3 m_VolumeLocalMin{0.0f, 0.0f, 0.0f};      // 局部包围盒最小角
    igm::vec3 m_VolumeLocalExtent{1.0f, 1.0f, 1.0f};   // 局部包围盒尺寸
    int m_VolumeDimension[3] = {0, 0, 0};   // 体数据维度
    // 每条光线的采样步数：现在直接在视口分辨率下步进，
    // 步数是主要性能开关（interactive 时可用 SetVolumeRaycastingQuality 调低）
    int m_VolumeStepCount = 128;
    float m_VolumeOpacityScale = 0.1f;     // 不透明度缩放
    unsigned int m_VolumeFrameSeed = 0;     // 抖动种子（逐帧变化）
    // 以下三项为一次性自检开关：用于定位体绘制“不可见/无效果”的问题，
    // 稳定后可整体移除（对应 VolumeRaycastPass 中的诊断代码）
    bool m_VolumeRaycastDiagnosed = false;  // 是否已输出过一次性自检日志
    bool m_VolumeRaycastReadbackDone = false; // 是否已回读过结果像素
    bool m_VolumeRaycastGpuDiagDone = false;  // 是否已做过 GPU 侧命中自检
    bool m_VolumeRaycastingSupportChecked = false; // 是否已查询过运行时 compute 支持
    bool m_VolumeRaycastingSupported = false;      // 运行时是否支持 compute 光线步进
    std::string m_VolumeRaycastFailureMessage;     // 运行期失败待提示消息
    bool m_VolumeLegacyDiagPending = false;        // 旧实现的一次性自检（下一帧输出）

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

    // 实测帧率：以 1 秒滑动窗口统计实际重绘次数（渲染是事件驱动的，
    // 空闲不重绘时窗口不推进，界面上按“空闲”显示而不是 0 FPS）
    double m_Fps = 0.0;
    unsigned int m_FpsFrameCount = 0;
    std::chrono::steady_clock::time_point m_FpsWindowStart;
    bool m_FpsWindowStartValid = false;

    // 上一帧结束的时间点（用于“提前返回”判定）
    std::chrono::steady_clock::time_point m_LastRenderEnd;
    bool m_LastRenderEndValid = false;

    // 记录是否处于交互状态
    bool m_IsInteracting = false;

    //表面着色模式（0: Blinn-Phong，1: 无光照，2: PBR）
    int m_SurfaceShadingMode = 2;

#ifndef __EMSCRIPTEN__
    SmartPointer<EnvironmentLight> m_EnvironmentLight;
    bool m_EnvironmentEnabled = true;
    float m_EnvironmentIntensity = 1.0f;
    float m_EnvironmentRotation = 0.0f;
#endif

    //表面材质预设（仅桌面端 PBR 着色使用，仅作为金属度/粗糙度的一键回填）
    MaterialPreset m_MaterialPreset = MaterialPreset::GlossyPlastic;

    //PBR 材质参数（全局，仅桌面端 PBR 着色使用）
    float m_SurfaceMetallic = 0.0f;
    float m_SurfaceRoughness = 0.35f;
    //PBR 环境遮蔽强度（全局，半球近似；0 = 关闭，1 = 全强度）
    float m_SurfaceAo = 1.0f;

    //Blinn-Phong 高光参数（全局，仅桌面端 Blinn-Phong 着色使用）
    float m_SurfaceSpecularStrength = 0.0f;
    float m_SurfaceShininess = 32.0f;

    friend class RenderWindow;
    friend class Model;
    friend class Axes;
    friend class Interactor;
    friend class BasicStyle;
    friend class DragCenterStyle; // 拖拽中心样式
    friend class PainterBase;
    friend class Painter2D;
    friend class Painter3D;
    friend class ColorBar2DActor;
    friend class TextOverlay2DActor;
};

IGAME_NAMESPACE_END
