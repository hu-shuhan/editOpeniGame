//
// Created by Sumzeek on 7/4/2024.
//

#pragma once

#include "OpenGL/GLFramebuffer.h"
#include "OpenGL/GLIndirectCommand.h"
#include "OpenGL/GLShader.h"
#include "OpenGL/GLTextureBUffer.h"
#include "iGameAxes.h"
#include "iGameCamera.h"
#include "iGameFontSet.h"
#include "iGameLight.h"
#include "iGameModel.h"
#include "iGameSelection.h"

IGAME_NAMESPACE_BEGIN
class Interactor;

class Scene : public Object {
public:
    I_OBJECT(Scene);
    static Pointer New() { return new Scene; }

    /* Model Related */
    int AddModel(DataObject::Pointer);
    int AddModel(Model::Pointer);
    void ResetCameraView();
    Model::Pointer CreateModel(DataObject::Pointer);
    void RemoveModel(int index);
    void RemoveModel(Model*);
    void RemoveCurrentModel();
    void SetCurrentModel(int index);
    void SetCurrentModel(Model*);

    Painter3D::Pointer GetPainter() { return m_Painter3D; }

    /* Interactor Related */
    void SetInteractor(Interactor* i);
    Interactor* GetInteractor();

    Model* GetCurrentModel();
    Model* GetModelById(int index);
    DataObject* GetDataObjectById(int index);
    std::map<int, Model::Pointer>& GetModelList();

    void ChangeModelVisibility(int index, bool visibility);
    void ChangeModelVisibility(Model* m, bool visibility);

    /* Rendering Related */
    struct CameraDataBuffer {
        alignas(16) igm::vec3 camera_position;
        alignas(4) int isOrtho;
        alignas(16) igm::mat4 view;
        alignas(16) igm::mat4 proj;
        alignas(16) igm::mat4 proj_view; // proj * view
    };
    struct ObjectDataBuffer {
        alignas(4) float transparent;
        alignas(16) igm::mat4 model;
        alignas(16) igm::mat4 normal; // transpose(inverse(model))
        alignas(16) igm::vec4 sphereBounds;
    };
    struct UniformBufferObjectBuffer {
        alignas(4) int useColor{0};
        alignas(4) int useNormalSmooth{0};
    };

    struct DrawCullData {
        alignas(16) igm::mat4 view_model;
        alignas(4) float P00, P11, zNear; // symmetric projection parameters
        alignas(16) igm::vec4
                frustum; // data for left/right/top/bottom frustum planes
        alignas(4) float pyramidWidth,
                pyramidHeight; // depth pyramid size in texels
    };

    enum ShaderType {
        BLINNPHONG = 0,
        PBR,
        NOLIGHT,
        PURECOLOR,
        SINGLEPASSWIREFRAME,
        TRANSPARENCYLINK,
        TRANSPARENCYSORT,
        VOLUMERENDERINGLINK,
        VOLUMERENDERINGSORT,
        AXES,
        FONT,
        ATTACHMENTRESOLVE,
        DEPTHREDUCE,
        MESHLETCULL,
        SCREEN,
        FXAA,
        SHADERTYPE_COUNT
    };

    Camera::Pointer GetCamera() { return m_Camera; }
    void ChangeCameraType(IGenum type);
    GLTexture2d::Pointer DepthPyramid() { return m_DepthPyramid; }

    CameraDataBuffer& CameraData() { return m_CameraData; }
    ObjectDataBuffer& ObjectData() { return m_ObjectData; }
    UniformBufferObjectBuffer& UBO() { return m_UBO; }
    igm::vec4& ModelsBoundingSphere() { return m_ModelsBoundingSphere; }
    igm::mat4& ModelRotate() { return m_ModelRotate; }
    igm::mat4& ModelMatrix() { return m_ModelMatrix; }

    //void UseColor();
    void UpdateUniformBuffer();

    void SetShader(IGenum type, GLShaderProgram::Pointer);
    GLShaderProgram::Pointer GenShader(IGenum type);
    GLShaderProgram::Pointer GetShaderWithType(IGenum type);
    GLShaderProgram::Pointer GetShader(IGenum type);
    bool HasShader(IGenum type);
    void UseShader(IGenum type);

    bool Initialize();
    void Draw();
    void Resize(int width, int height, int pixelRatio);
    void Update();

    void LookAtPositiveX();
    void LookAtNegativeX();
    void LookAtPositiveY();
    void LookAtNegativeY();
    void LookAtPositiveZ();
    void LookAtNegativeZ();
    void LookAtIsometric();
    void RotateNinetyClockwise();
    void RotateNinetyCounterClockwise();

    void SetVolumeRendering(bool toggled);
    std::vector<unsigned char> CaptureScreen(int x, int y, int width,
                                             int height);
    GLBuffer::Pointer GetDrawCullDataBuffer() { return m_DrawCullData; }

    void MakeCurrent();
    void DoneCurrent();
    template<typename Functor, typename... Args>
    void SetUpdateFunctor(Functor&& functor, Args&&... args) {
        m_UpdateFunctor = std::bind(std::forward<Functor>(functor),
                                    std::forward<Args>(args)...);
    }
    template<typename Functor, typename... Args>
    void SetMakeCurrentFunctor(Functor&& functor, Args&&... args) {
        m_MakeCurrentFunctor = std::bind(std::forward<Functor>(functor),
                                         std::forward<Args>(args)...);
    }
    template<typename Functor, typename... Args>
    void SetDoneCurrentFunctor(Functor&& functor, Args&&... args) {
        m_DoneCurrentFunctor = std::bind(std::forward<Functor>(functor),
                                         std::forward<Args>(args)...);
    }

protected:
    Scene();
    ~Scene() override;

    void UpdateModelsBoundingSphere();

    void InitOpenGL();
    void InitOIT();
    void InitFont();
    void InitAxes();

    void ResizeFrameBuffer();
    void ResizeDepthPyramid();
    void RefreshDepthPyramid();
    void RefreshDrawCullDataBuffer();

    void DrawFrame();
    void ResolveFrame();
    void RenderToQtFrame();

    void ShadowPass();
    void ForwardPass();
    void TransparentPass();
    void VolumeRenderingPass();

    void UpdateCameraDataBlock();
    void UpdateObjectDataBlock(DataObject* obj);
    void UpdateUniformBufferObjectBlock(DataObject* obj);

    void DrawAxes(igm::ivec4 drawRange);
    static void CalculateFrameRate();

    /* Data Object Related */
    std::map<int, Model::Pointer> m_Models;
    int m_IncrementModelId{0};
    int m_CurrentModelId{-1};
    Model* m_CurrentModel{nullptr};

    std::function<void()> m_UpdateFunctor{nullptr};
    std::function<void()> m_MakeCurrentFunctor{nullptr};
    std::function<void()> m_DoneCurrentFunctor{nullptr};

    Camera::Pointer m_Camera{};
    Light::Pointer m_Light{};
    Axes::Pointer m_Axes{};
    Interactor* m_Interactor{nullptr};

    /* Rendering related */
    CameraDataBuffer m_CameraData;
    ObjectDataBuffer m_ObjectData;
    UniformBufferObjectBuffer m_UBO;

    igm::mat4 m_ModelRotate{}; //Rotation matrix passing through the origin
    igm::mat4 m_ModelMatrix{};
    igm::vec3 m_BackgroundColor{};

    uint32_t m_VisibleModelsCount = 0;
    igm::vec4 m_ModelsBoundingSphere{0.0f, 0.0f, 0.0f, 1.0f};

    GLBuffer::Pointer m_CameraDataBlock, m_ObjectDataBlock, m_UBOBlock;
    std::map<IGenum, GLShaderProgram::Pointer> m_ShaderPrograms;

    // used to draw full-screen triangle
    GLVertexArray::Pointer m_EmptyVAO;

#ifdef MSAA
    GLint samples = 8;
    GLFramebuffer::Pointer m_FramebufferMultisampled;
    GLTexture2dMultisample::Pointer m_ColorTextureMultisampled;
    GLTexture2dMultisample::Pointer m_DepthTextureMultisampled;

    GLFramebuffer::Pointer m_FramebufferResolved;
    GLTexture2d::Pointer m_ColorTextureResolved;
    GLTexture2d::Pointer m_DepthTextureResolved;
#else
    GLFramebuffer::Pointer m_Framebuffer;
    GLTexture2d::Pointer m_ColorTexture;
    GLTexture2d::Pointer m_DepthTexture;
#endif

    GLTexture2d::Pointer m_OITHeadPointerTexture;
    GLBuffer::Pointer m_OITHeadPointerInitializer;
    GLBuffer::Pointer m_OITAtomicCounterBuffer;
    GLBuffer::Pointer m_OITLinkedListBuffer;
    GLTextureBuffer::Pointer m_OITLinkedListTexture;

    GLBuffer::Pointer m_DrawCullData;
    int m_DepthPyramidWidth, m_DepthPyramidHeight, m_DepthPyramidLevels;
    GLTexture2d::Pointer m_DepthPyramid;

    Painter2D::Pointer m_Painter2D{Painter2D::New()};
    Painter3D::Pointer m_Painter3D{Painter3D::New()};

    bool m_FinishInit{false};
    bool m_EnableVolumeRendering{false};

    friend class Model;
    friend class Interactor;
};

IGAME_NAMESPACE_END