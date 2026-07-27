/**
 * @class    ShaderManager
 * @brief    ShaderManager类管理所有的着色器程序及相关数据块。
 *
 * ShaderManager提供了获取、设置、使用着色器的接口，并维护相关的缓冲数据块，
 * 如相机数据块、物体数据块和裁剪数据块。它支持对不同类型的Shader进行管理和动态生成。
 * 注意！该类的大部分函数使用必须保证OpenGL上下文，因此不建议在外部直接调用！
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "OpenGL/GLShader.h"
#include "iGameCamera.h"
#include "iGameDataObject.h"
#include "iGameDrawObject.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

/**
 * @enum     ShaderType
 * @brief    枚举类型，用于表示支持的Shader（着色器）类型。
 *
 * 各种Shader类型支持不同的渲染效果，例如Blinn-Phong光照、PBR渲染、
 * 单通道线框渲染、透明排序、体积渲染等。
 */
enum class ShaderType {
    BLINNPHONG = 0,      ///< Blinn-Phong光照模型
    PBR,                 ///< 基于物理渲染（Physically-Based Rendering），未完善
    NOLIGHT,             ///< 无光照Shader
    PURECOLOR,           ///< 单一颜色Shader
    SINGLEPASSWIREFRAME, ///< 单通道线框Shader
    TRANSPARENCYLINK,    ///< 透明渲染（链表法）
    TRANSPARENCYSORT,    ///< 透明渲染（排序法）
    VOLUMERENDERINGLINK, ///< 体积渲染（链表法）
    VOLUMERENDERINGSORT, ///< 体积渲染（排序法）
    AXES,                ///< 坐标轴显示Shader
    FONT,                ///< 字体渲染Shader
    ATTACHMENTRESOLVE,   ///< 解析多样本缓冲Shader
    DEPTHREDUCE,         ///< HZB生成Shader
    MESHLETCULL,         ///< Meshlet剔除Shader
    SCREEN,              ///< 屏幕渲染Shader
    FXAA,                ///< 快速抗锯齿（FXAA）
    CULLINGPHASE1,       ///< 裁剪阶段1
    CULLINGPHASE2        ///< 裁剪阶段2
};


class ShaderManager : public Object {
public:
    I_OBJECT(ShaderManager)
    static Pointer New() { return new ShaderManager; }

    /**
     * @struct   CameraDataBuffer
     * @brief    用于存储相机相关的Uniform缓冲数据。
     */
    struct CameraDataBuffer {
        alignas(16) igm::vec3 camera_position; ///< 相机位置
        alignas(4) int isOrtho;                ///< 是否为正交投影
        alignas(16) igm::vec4 orthoBounds;     ///< 正交投影参数（左右下上）
        alignas(4) float zNear;                ///< 近裁剪面距离
        alignas(4) float zFar;                 ///< 远裁剪面距离
        alignas(16) igm::mat4 view;            ///< 视图矩阵
        alignas(16) igm::mat4 proj;            ///< 投影矩阵
        alignas(16) igm::mat4 proj_view;       ///< 投影-视图矩阵（proj * view）
    };

    /**
     * @struct   ObjectDataBuffer
     * @brief    用于存储渲染物体相关的Uniform缓冲数据。
     */
    struct ObjectDataBuffer {
        alignas(4) float transparent; ///< 透明度
        alignas(16) igm::mat4 model;  ///< 模型矩阵
        alignas(16) igm::mat4 normal; ///< 法线矩阵（transpose(inverse(model))）
        alignas(16) igm::vec4 sphereBounds; ///< 包围球信息（中心+半径）
    };

    /**
     * @struct   UniformBufferObjectBuffer
     * @brief    用于存储通用Uniform缓冲数据。
     */
    struct UniformBufferObjectBuffer {
        alignas(4) int useColor{0}; ///< 渲染时是否使用自定义颜色数组进行渲染
        alignas(4) int useNormalSmooth{0}; ///< 渲染时是否使用法线平滑面片
    };

    /**
     * @struct   CullDataBuffer
     * @brief    用于存储裁剪相关的Uniform缓冲数据。
     */
    struct CullDataBuffer {
        alignas(16) igm::mat4 view_model;            ///< 视图-模型矩阵
        alignas(4) float P00, P11, zNear, zFar;      ///< 相关投影参数
        alignas(16) igm::vec4 frustum;               ///< 裁剪平面数据
        alignas(4) unsigned int HzbWidth, HzbHeight; ///< 深度金字塔大小（像素）
    };

    /**
     * @brief 初始化着色器管理器。
     * @return 是否初始化成功。
     */
    bool Initialize();

    /**
     * @brief 获取指定类型的Shader程序。
     * @param type Shader类型。
     * @return 对应的Shader程序指针。
     */
    SmartPointer<GLShaderProgram> GetShader(ShaderType type);

    /**
     * @brief 检查是否存在指定类型的Shader程序。
     * @param type Shader类型。
     * @return 如果存在返回true，否则返回false。
     */
    bool HasShader(ShaderType type);

    /**
     * @brief 使用指定类型的Shader程序。
     * @param type Shader类型。
     */
    void UseShader(ShaderType type);

#ifdef __EMSCRIPTEN__
    /**
     * @brief 为Web端设置Shader的回退Uniforms。
     * @param shader 要设置的Shader程序指针。
     */
    void ApplyWebFallbackUniforms(SmartPointer<GLShaderProgram> shader);
#endif

    /**
     * @brief 更新相机数据块。
     * @param camera 相机指针。
     */
    void UpdateCameraBlock(SmartPointer<Camera> camera);

    /**
     * @brief 更新相机数据块。
     * @param buffer 包含相机数据的缓冲。
     */
    void UpdateCameraBlock(CameraDataBuffer buffer);

    /**
     * @brief 更新物体数据块。
     * @param obj 数据对象指针。
     * @param model 模型矩阵。
     */
    void UpdateObjectBlock(SmartPointer<DataObject> obj, igm::mat4 model);

    /**
     * @brief 更新物体数据块。
     * @param buffer 包含物体数据的缓冲。
     */
    void UpdateObjectBlock(ObjectDataBuffer buffer);

    /**
     * @brief 更新Uniform数据块。
     * @param obj 数据对象指针。
     */
    void UpdateUBOBlock(SmartPointer<DataObject> obj);

    /**
     * @brief 更新Uniform数据块。
     * @param buffer 包含Uniform数据的缓冲。
     */
    void UpdateUBOBlock(UniformBufferObjectBuffer buffer);

    /**
     * @brief 更新裁剪数据块。
     * @param camera 相机指针。
     * @param model 模型矩阵。
     * @param HzbWidth 层级深度缓冲金字塔宽度。
     * @param HzbHeight 层级深度缓冲金字塔高度。
     */
    void UpdateCullDataBuffer(SmartPointer<Camera> camera, igm::mat4 model,
                              unsigned int HzbWidth, unsigned int HzbHeight);

    /**
     * @brief 更新裁剪数据块。
     * @param buffer 包含裁剪数据的缓冲。
     */
    void UpdateCullDataBuffer(CullDataBuffer buffer);

    /**
     * @brief 获取裁剪数据块的指针。
     * @return 裁剪数据块指针。
     */
    SmartPointer<GLBuffer> GetCullDataBuffer();

protected:
    ShaderManager();
    ~ShaderManager() override;

    /**
     * @brief 映射缓冲块到GPU。
     */
    void MapBufferBlock();

    /**
     * @brief 根据Shader类型获取Shader程序（内部实现）。
     * @param type Shader类型。
     * @return 对应的Shader程序指针。
     */
    SmartPointer<GLShaderProgram> GetShaderWithType(ShaderType type);

    /**
     * @brief 设置指定类型的Shader程序。
     * @param type Shader类型。
     * @param sp Shader程序指针。
     */
    void SetShader(ShaderType type, SmartPointer<GLShaderProgram> sp);

    /**
     * @brief 动态生成指定类型的Shader程序。
     * @param type Shader类型。
     * @return 生成的Shader程序指针。
     */
    SmartPointer<GLShaderProgram> GenShader(ShaderType type);

    std::map<ShaderType, SmartPointer<GLShaderProgram>> m_ShaderPrograms;

    SmartPointer<GLBuffer> m_CameraDataBlock;
    SmartPointer<GLBuffer> m_ObjectDataBlock;
    SmartPointer<GLBuffer> m_UBOBlock;
    SmartPointer<GLBuffer> m_CullDataBuffer;

#ifdef __EMSCRIPTEN__
    CameraDataBuffer m_WebCameraData{};
    ObjectDataBuffer m_WebObjectData{};
    UniformBufferObjectBuffer m_WebUboData{};
#endif
};

IGAME_NAMESPACE_END
