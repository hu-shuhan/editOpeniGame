#include "iGameScene.h"
#include "iGameCommand.h"
#include "iGameInteractor.h"
#include "iGameRenderingLogger.h"
#include "iGameStructuredMesh.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>

IGAME_NAMESPACE_BEGIN

#ifdef __EMSCRIPTEN__
namespace
{
void ClearPendingGLErrors() {
    while (glGetError() != GL_NO_ERROR) {}
}

const char* WebGLErrorName(GLenum error) {
    switch (error) {
        case GL_INVALID_ENUM:
            return "INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "INVALID_OPERATION";
        case GL_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "INVALID_FRAMEBUFFER_OPERATION";
        default:
            return "UNKNOWN_ERROR";
    }
}

void CheckTransparentPassError(const char* step) {
    GLenum err = GL_NO_ERROR;
    while ((err = glGetError()) != GL_NO_ERROR) {
        IGAME_RENDERING_ERROR("TransparentPass GL error after {}: {} ({})",
                              step, WebGLErrorName(err), err);
    }
}
} // namespace
#endif

Scene::Scene() {
    m_ModelPool = HandlePool<SmartPointer<Model>>::New();
    m_CurrentModelID = 0;

    m_UpdateFunctor = nullptr;
    m_MakeCurrentFunctor = nullptr;
    m_DoneCurrentFunctor = nullptr;

    m_Camera = Camera::New();
    //m_Light = Light::New();

    m_Axes = Axes::New();
    m_Axes->SetScene(this);
    m_ColorBar2DActor = ColorBar2DActor::New();
    m_ColorBar2DActor->SetScene(this);
    m_TextOverlay2DActor = TextOverlay2DActor::New();
    m_TextOverlay2DActor->SetScene(this);

    m_Interactor = Interactor::New();
    m_FontManager = FontManager::New();
    m_ShaderManager = ShaderManager::New();
#ifndef __EMSCRIPTEN__
    m_EnvironmentLight = EnvironmentLight::New();
#endif

    m_ModelRotate = igm::mat4{1.0f};
    m_ModelMatrix = igm::mat4{1.0f};
    // m_BackgroundColor = {0.247f, 0.247f, 0.247f};
    m_BackgroundColor = {0.1333f, 0.1333f, 0.1333f};

    m_VisibleModelsCount = 0;
    m_ModelsBoundingSphere = igm::vec4{0.0f, 0.0f, 0.0f, 1.0f};

    m_EmptyVAO = GLVertexArray::New();

#ifdef GL_SUPPORT_MSAA
    samples = 4;
    m_FramebufferMultisampled = GLFramebuffer::New();
    m_ColorTextureMultisampled = GLTexture2dMultisample::New();
    m_DepthTextureMultisampled = GLTexture2dMultisample::New();
#endif

    m_Framebuffer = GLFramebuffer::New();
    m_ColorTexture = GLTexture2d::New();
    m_DepthTexture = GLTexture2d::New();

    m_FramebufferBackup = GLFramebuffer::New();
    m_ColorTextureBackup = GLTexture2d::New();

    m_OITHeadPointerTexture = GLTexture2d::New();
    m_OITHeadPointerInitializer = GLBuffer::New();
    m_OITAtomicCounterBuffer = GLBuffer::New();
    m_OITLinkedListBuffer = GLBuffer::New();
    m_OITLinkedListTexture = GLTextureBuffer::New();

    m_VolumeTexture3d = GLTexture3d::New();
    m_VolumeMaskTexture3d = GLTexture3d::New();
    m_VolumeTransferFunction = GLTexture2d::New();
    m_VolumeTexture3d2 = GLTexture3d::New();
    m_VolumeMaskTexture3d2 = GLTexture3d::New();
    m_VolumeTransferFunction2 = GLTexture2d::New();

    m_HzbWidth = 0;
    m_HzbHeight = 0;
    m_HzbLevels = 0;
    m_HzbTexture = GLTexture2d::New();
    m_Painter2D = Painter2D::New();
    m_Painter2D->SetScene(this);
    m_Painter3D = Painter3D::New();
    m_Painter3D->SetScene(this);

    m_FinishInit = false;
    m_EnableVolumeRendering = false;

    m_CenterAxesModel = CenterAxesModel::New();
}

Scene::~Scene() {
    std::cout << "[iGameDestroy] Scene::~Scene this=" << this << " finishInit=" << (m_FinishInit ? "true" : "false")
              << '\n';
    if (m_FinishInit) { glDeleteQueries(2, m_TimeQueries); }
}

void Scene::Finalize() {
    std::cout << "[iGameDestroy] Scene::Finalize this=" << this << '\n';
    m_ModelPool = nullptr;
#ifndef __EMSCRIPTEN__
    if (m_EnvironmentLight) m_EnvironmentLight->Release();
    m_EnvironmentLight = nullptr;
#endif
    m_CurrentModelID = 0;
    m_Interactor = nullptr;
    m_Axes = nullptr;
    m_ColorBar2DActor = nullptr;
    m_TextOverlay2DActor = nullptr;
    m_CenterAxesModel = nullptr;
    m_Painter2D = nullptr;
    m_Painter3D = nullptr;
    m_FontManager = nullptr;
    m_ShaderManager = nullptr;
    m_EmptyVAO = nullptr;
    m_Framebuffer = nullptr;
    m_ColorTexture = nullptr;
    m_DepthR32FTexture = nullptr;
    m_DepthTexture = nullptr;
    m_FramebufferBackup = nullptr;
    m_ColorTextureBackup = nullptr;
#ifdef GL_SUPPORT_MSAA
    m_FramebufferMultisampled = nullptr;
    m_ColorTextureMultisampled = nullptr;
    m_DepthTextureMultisampled = nullptr;
#endif
    m_OITHeadPointerTexture = nullptr;
    m_OITHeadPointerInitializer = nullptr;
    m_OITAtomicCounterBuffer = nullptr;
    m_OITLinkedListBuffer = nullptr;
    m_OITLinkedListTexture = nullptr;
    m_VolumeFramebuffer = nullptr;
    m_VolumeColorTexture = nullptr;
    m_VolumeDepthTexture = nullptr;
    m_VolumeTexture3d = nullptr;
    m_VolumeMaskTexture3d = nullptr;
    m_VolumeTransferFunction = nullptr;
    m_VolumeTexture3d2 = nullptr;
    m_VolumeMaskTexture3d2 = nullptr;
    m_VolumeTransferFunction2 = nullptr;
    m_VolumeSourceObject = nullptr;
    m_HzbTexture = nullptr;
    m_Camera = nullptr;
}

void Scene::BindFramebuffer() const {
#ifdef __EMSCRIPTEN__
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#else
    #ifdef GL_SUPPORT_MSAA
    m_FramebufferMultisampled->Bind();
    #else
    m_Framebuffer->Bind();
    #endif
#endif
}

bool Scene::ShouldRenderThisCall() const {
    if (!m_FramePacingEnabled || !m_LastRenderEndValid) { return true; }

    double targetMs = 0.0;
    if (m_TargetFps > 0u) {
        targetMs =
                std::max(targetMs, 1000.0 / static_cast<double>(m_TargetFps));
    }

    if (m_GpuUsageLimit > 0.0f) {
        double gpuMs = m_SmoothedGpuTimeMs > 0.0 ? m_SmoothedGpuTimeMs
                                                 : m_LastGpuTimeMs;
        if (gpuMs > 0.0) {
            double expectedFrameMs =
                    gpuMs / static_cast<double>(m_GpuUsageLimit);
            targetMs = std::max(targetMs, expectedFrameMs);
        }
    }

    if (targetMs <= 0.0) { return true; }

    auto now = std::chrono::steady_clock::now();
    double elapsedMs =
            std::chrono::duration<double, std::milli>(now - m_LastRenderEnd)
                    .count();
    return elapsedMs >= targetMs;
}

bool Scene::Initialize() {
    if (m_FinishInit) {
        IGAME_RENDERING_WARN("Scene is already init.");
        return false;
    }

    this->InitOpenGL();
#ifndef __EMSCRIPTEN__
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
#ifndef IGAME_OPENGL_VERSION_GLES2
    this->InitOIT();
#endif
    this->InitAxes();

    this->ResetCameraView();

    // 添加中心坐标轴到模型池
    m_CenterAxesModel->AddViewStyle(
            IG_WIREFRAME);                   // 添加线框视图样式（默认不显示线）
    m_CenterAxesModel->SetAlwaysOnTop(true);
    m_CenterAxesModel->ConvertToDrawableData(); // 初始化几何数据
    m_CenterAxesModel->SyncGpuBuffers();        // 上传GPU数据
    this->AddModel(m_CenterAxesModel);          // 加入模型池
    m_CenterAxesModel->SetVisibility(m_CenterAxesVisible);
    // 添加中心坐标轴到模型池

    // Web端下禁用所有模型的壳渲染选项，避免自动抽壳模型造成不必要开销
    #if __EMSCRIPTEN__
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        if (!model->GetDataObject()->IsDrawable()) { continue; }
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        if (!drawObject) { continue; }
        drawObject->SetShellRenderingOption(false);
        // Propagate to sub-objects to fully disable shell extraction chain
        drawObject->ProcessSubDataObjects(&DrawObject::SetShellRenderingOption,
                                          false);
    }
    #endif

    m_FinishInit = true;
    return true;
}

IGuint Scene::AddModel(SmartPointer<DataObject> obj) {
    SmartPointer<Model> model = Model::New();
    model->SetDataObject(obj);
    model->SetScene(this);
    auto modelID = m_ModelPool->AllocateObject(model);
    m_CurrentModelID = modelID;
    model->SetVisibility(false);
    ChangeModelVisibility(model, true);

    // 场景数据变了，体数据需要重建（若已开启 compute 体绘制）
    m_VolumeDataDirty = true;

    #if __EMSCRIPTEN__
    auto drawObject = DynamicCast<DrawObject>(obj);
    if (drawObject) {
        drawObject->SetShellRenderingOption(false);
        drawObject->ProcessSubDataObjects(&DrawObject::SetShellRenderingOption,
                                          false);
    }
    #endif

    Update();
    return modelID;
}

void Scene::RemoveModel(IGuint modelID) {
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto id = it->first;
        auto& m = it->second;

        if (id == modelID) {
            if (auto visibility = m->GetVisibility()) {
                m_VisibleModelsCount--;
            }
            m->GetDataObject()->InvokeEvent(Command::DeleteEvent);
            m_ModelPool->ReleaseHandle(id);
            if (id == m_CurrentModelID) {
                if (m_ModelPool->GetObjectCount() == 0) {
                    m_CurrentModelID = 0;
                } else {
                    m_CurrentModelID = m_ModelPool->Begin()->first;
                }
            }
            UpdateModelsBoundingSphere();
            Update();
            return;
        }
    }
    IGAME_RENDERING_WARN("Model with id {} does not exist in the scene.",
                         modelID);
}

void Scene::RemoveModel(SmartPointer<Model> model) {
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto id = it->first;
        auto m = it->second;

        if (m == model) {
            if (auto visibility = model->GetVisibility()) {
                m_VisibleModelsCount--;
            }
            m->GetDataObject()->InvokeEvent(Command::DeleteEvent);
            m_ModelPool->ReleaseHandle(id);
            if (id == m_CurrentModelID) {
                if (m_ModelPool->GetObjectCount() == 0) {
                    m_CurrentModelID = 0;
                } else {
                    m_CurrentModelID = m_ModelPool->Begin()->first;
                }
            }
            UpdateModelsBoundingSphere();
            Update();
            return;
        }
    }
    IGAME_RENDERING_WARN("Model does not exist in the scene.");
}

void Scene::RemoveCurrentModel() {
    auto model = m_ModelPool->GetObjectByHandle(m_CurrentModelID);
    if (auto visibility = model->GetVisibility()) { m_VisibleModelsCount--; }
    RemoveModel(m_CurrentModelID);
}

void Scene::SetCurrentModel(int modelID) {
    if (m_ModelPool->CheckHandle(modelID)) {
        m_CurrentModelID = modelID;
        return;
    }
    IGAME_RENDERING_WARN("Model with id {} does not exist in the scene.",
                         modelID);
}

void Scene::SetCurrentModel(SmartPointer<Model> model) {
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto id = it->first;
        auto m = it->second;

        if (m == model) {
            m_CurrentModelID = id;
            return;
        }
    }
    IGAME_RENDERING_WARN("Model does not exist in the scene.");
}

SmartPointer<Model> Scene::GetCurrentModel() {
    return m_ModelPool->GetObjectByHandle(m_CurrentModelID);
}
IGuint Scene::GetCurrentModelID() { return m_CurrentModelID; }

SmartPointer<Model> Scene::GetModelById(int id) {
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto modelID = it->first;
        auto model = it->second;

        if (modelID == id) { return model; }
    }
    return nullptr;
}
bool Scene::SetModelById(int id, SmartPointer<Model>model) {
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto modelID = it->first;
        if (modelID == id) {
            it->second = model;
            return true;
        }
    }
    return false;
}

SmartPointer<DataObject> Scene::GetDataObjectById(int id) {
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto modelID = it->first;
        auto model = it->second;

        if (modelID == id) { return model->GetDataObject(); }
    }
    return nullptr;
}

SmartPointer<HandlePool<SmartPointer<Model>>> Scene::GetModelList() {
    return m_ModelPool;
}

void Scene::ChangeModelVisibility(int modelID, bool visibility) {
    auto model = GetModelById(modelID);
    if (model != nullptr) { ChangeModelVisibility(model, visibility); }
}

void Scene::ChangeModelVisibility(SmartPointer<Model> model, bool visibility) {
    // Safely update DrawObject visibility (if drawable) and keep Model visibility
    // in sync. Adjust visible models counter only when actual visibility changes.
    auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());

    // previous model-level visibility
    const bool prevVisibility = model->GetVisibility();

    // apply visibility to draw object if available
    if (drawObject) { drawObject->SetVisibility(visibility); }

    // ensure model-level visibility is updated to reflect draw object
    model->SetVisibility(visibility);

    // update visible models count only when visibility state changes
    if (prevVisibility != visibility) {
        if (visibility) {
            m_VisibleModelsCount++;
            if (m_VisibleModelsCount == 1 || m_VisibleModelsCount == 2) {
                // if this is the first or second visible model, reset camera to include it
                ResetCameraView(model->GetDataObject());
                UpdateAxisSize();
            }
        } else {
            if (m_VisibleModelsCount > 0) { m_VisibleModelsCount--; }
        }
    }

    UpdateCameraClippingRange();
}

void Scene::SetBackGround(const Color& color) {
    auto c = ColorUtils::Map(color);
    m_BackgroundColor = c;
    m_BackgroundGradientEnabled = false;
    m_BackgroundGradientMode = 0;
    this->Modified();
}

void Scene::SetBackGround(int R, int G, int B) {
    auto c = ColorUtils::Map(R, G, B);
    m_BackgroundColor = c;
    m_BackgroundGradientEnabled = false;
    m_BackgroundGradientMode = 0;
    this->Modified();
}

void Scene::SetBackGroundGradient(int R1, int G1, int B1, int R2, int G2,
                                  int B2, int mode) {
    mode = mode < 0 ? 0 : mode;
    mode = mode > 2 ? 2 : mode;
    m_BackgroundColor = ColorUtils::Map(R1, G1, B1);
    m_BackgroundGradientColor = ColorUtils::Map(R2, G2, B2);
    m_BackgroundGradientMode = mode;
    m_BackgroundGradientEnabled = mode != 0;
    this->Modified();
}

igm::vec3 Scene::GetBackGround() { return m_BackgroundColor; }

void Scene::SetInteractor(SmartPointer<Interactor> interactor) {
    m_Interactor = interactor;
}

SmartPointer<Interactor> Scene::GetInteractor() { return m_Interactor; }

void Scene::ResetCameraView() {
    UpdateModelsBoundingSphere();
    igm::vec4 boundingSphere = GetRotationBoundingSphere();

    m_ModelMatrix = igm::mat4{1.0f};
    m_ModelRotate = igm::mat4{1.0f};
    m_Camera->SetPosition(boundingSphere.x, boundingSphere.y,
                          boundingSphere.z + 3.0f * boundingSphere.w);
    m_Camera->SetFocal(boundingSphere.xyz());
    this->UpdateCameraClippingRange();
    UpdateAxisSize();
}

void Scene::ResetCameraView(const BoundingBox& bbox) {
    double* center = bbox.center().pointer();
    float x = static_cast<float>(center[0]);
    float y = static_cast<float>(center[1]);
    float z = static_cast<float>(center[2]);

    double diameter = bbox.diag();
    float r = static_cast<float>(diameter / 2.0);

    this->SetRotationBoundingSphere(igm::vec4{x, y, z, r});
    m_ModelMatrix = igm::mat4{1.0f};
    m_ModelRotate = igm::mat4{1.0f};
    m_Camera->SetPosition(x, y, z + 3.0f * r);
    m_Camera->SetFocal(igm::vec3{x, y, z});
    this->UpdateCameraClippingRange();
    UpdateAxisSize();
}

void Scene::ResetCameraView(SmartPointer<DataObject> dataObject) {
    ResetCameraView(dataObject->GetBoundingBox());
    UpdateAxisSize();
}

SmartPointer<Camera> Scene::GetCamera() { return m_Camera; }
void Scene::SetSurfaceShadingMode(int mode) {
    if (mode < 0 || mode > 2) { mode = 0; }
    m_SurfaceShadingMode = mode;
    this->Modified();
}
int Scene::GetSurfaceShadingMode() const { return m_SurfaceShadingMode; }

#ifndef __EMSCRIPTEN__
void Scene::SetEnvironmentEnabled(bool enabled) {
    m_EnvironmentEnabled = enabled;
    Update();
}

void Scene::SetEnvironmentBackgroundVisible(bool visible) {
    m_EnvironmentBackgroundVisible = visible;
    Update();
}
void Scene::SetEnvironmentIntensity(float intensity) {
    if (!std::isfinite(intensity)) return;
    m_EnvironmentIntensity = std::clamp(intensity, 0.0f, 4.0f);
    Update();
}
void Scene::SetEnvironmentRotation(float degrees) {
    if (!std::isfinite(degrees)) return;
    m_EnvironmentRotation = std::fmod(degrees, 360.0f);
    if (m_EnvironmentRotation < 0.0f) m_EnvironmentRotation += 360.0f;
    Update();
}
void Scene::ApplyEnvironmentUniforms(const SmartPointer<GLShaderProgram>& shader) const {
    if (m_EnvironmentLight)
        m_EnvironmentLight->Bind(shader, m_EnvironmentEnabled,
                                m_EnvironmentIntensity, m_EnvironmentRotation);
}
#endif

void Scene::SetMaterialPreset(MaterialPreset preset) {
    m_MaterialPreset = preset;

    float metallic = 0.0f;
    float roughness = 0.35f;
    switch (m_MaterialPreset) {
        case MaterialPreset::Matte:
            metallic = 0.0f;
            roughness = 0.75f;
            break;
        case MaterialPreset::PolishedMetal:
            metallic = 1.0f;
            roughness = 0.25f;
            break;
        case MaterialPreset::BrushedMetal:
            metallic = 1.0f;
            roughness = 0.55f;
            break;
        case MaterialPreset::GlossyPlastic:
        default:
            metallic = 0.0f;
            roughness = 0.35f;
            break;
    }
    this->SetMaterialParameters(metallic, roughness);
}

Scene::MaterialPreset Scene::GetMaterialPreset() const {
    return m_MaterialPreset;
}

void Scene::SetMaterialParameters(float metallic, float roughness) {
    if (metallic < 0.0f) { metallic = 0.0f; }
    if (metallic > 1.0f) { metallic = 1.0f; }
    if (roughness < 0.0f) { roughness = 0.0f; }
    if (roughness > 1.0f) { roughness = 1.0f; }

    m_SurfaceMetallic = metallic;
    m_SurfaceRoughness = roughness;
    this->Modified();
}

void Scene::GetMaterialParameters(float& metallic, float& roughness) const {
    metallic = m_SurfaceMetallic;
    roughness = m_SurfaceRoughness;
}

void Scene::SetSurfaceAo(float strength) {
    if (strength < 0.0f) { strength = 0.0f; }
    if (strength > 1.0f) { strength = 1.0f; }

    m_SurfaceAo = strength;
    this->Modified();
}

float Scene::GetSurfaceAo() const {
    return m_SurfaceAo;
}

void Scene::SetBlinnPhongParameters(float strength, float shininess) {
    if (strength < 0.0f) { strength = 0.0f; }
    if (strength > 1.0f) { strength = 1.0f; }
    if (shininess < 1.0f) { shininess = 1.0f; }
    if (shininess > 100.0f) { shininess = 100.0f; }

    m_SurfaceSpecularStrength = strength;
    m_SurfaceShininess = shininess;
    this->Modified();
}

void Scene::GetBlinnPhongParameters(float& strength, float& shininess) const {
    strength = m_SurfaceSpecularStrength;
    shininess = m_SurfaceShininess;
}

void Scene::ChangeCameraType(Camera::Type type) {
    this->ResetCameraView();
    switch (type) {
        case Camera::Type::PERSPECTIVE: {
            m_Camera->SetType(Camera::Type::PERSPECTIVE);
        } break;
        case Camera::Type::ORTHOGRAPHIC: {
            m_Camera->SetType(Camera::Type::ORTHOGRAPHIC);
        } break;
        default:
            break;
    }
}

igm::mat4 Scene::GetModelMatrix() { return m_ModelMatrix; }

SmartPointer<GLShaderProgram> Scene::GetShader(ShaderType type) {
    return m_ShaderManager->GetShader(type);
}

void Scene::InitOpenGL() {
#ifndef __EMSCRIPTEN__
    if (!gladLoadGL()) { IGAME_RENDERING_ERROR("Failed to initialize GLAD"); }
#endif

    // log opengl info
    {
        IGAME_RENDERING_INFO("OpenGL Info:");
        const GLubyte* vendor = glGetString(GL_VENDOR);
        IGAME_RENDERING_INFO("    Vendor: {}",
                             reinterpret_cast<const char*>(vendor));
        const GLubyte* renderer = glGetString(GL_RENDERER);
        IGAME_RENDERING_INFO("    Renderer: {}",
                             reinterpret_cast<const char*>(renderer));
        const GLubyte* version = glGetString(GL_VERSION);
        IGAME_RENDERING_INFO("    Version: {}",
                             reinterpret_cast<const char*>(version));
    }

#ifdef GL_SUPPORT_MSAA
    glEnable(GL_MULTISAMPLE);
#endif

    // reversed-z buffer, depth range: 1.0(near plane) -> 0.0(far plane)
#ifdef IGAME_OPENGL_VERSION_460
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
#endif

    // create empty VAO to render full-screen triangle
    m_EmptyVAO->Create();

    // initilize shader
    m_ShaderManager->Initialize();
    m_ColorBar2DActor->Initialize();
    m_TextOverlay2DActor->Initialize();

    // init framebuffer
    ResizeFrameBuffer();

    // init gpu timer queries
    glGenQueries(2, m_TimeQueries);
    m_TimeQueryIndex = 0;
    m_LastGpuTimeMs = 0.0;
    m_SmoothedGpuTimeMs = 0.0;

    // painter2d test
    {
        m_Painter2D->SetPen(Color::Red);
        m_Painter2D->SetPen(5);
        m_Painter2D->SetBrush(Color::Green);

        // m_Painter2D->DrawPoint({300, 300});
        // m_Painter2D->DrawLine({100, 100}, {200, 200});
        // m_Painter2D->DrawTriangle({100, 100}, {200, 100}, {100, 200});
        // m_Painter2D->DrawRect({100, 100}, {200, 200});
        // m_Painter2D->DrawCircle({100, 100}, 100, 100);
    }

    // painter3d test
    {
        m_Painter3D->SetPen(Color::Red);
        m_Painter3D->SetPen(5);
        m_Painter3D->SetBrush(Color::Green);

        // m_Painter3D->DrawPoint({-1.0f, -1.0f, 0.0f});
        // m_Painter3D->DrawLine({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
        // m_Painter3D->DrawTriangle({-1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f},
        //                           {1.0f, -1.0f, 0.0f});
        // m_Painter3D->DrawRect({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f});
        // m_Painter3D->DrawCube({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, -1.0f});
        // m_Painter3D->DrawCircle({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 1,
        //                         100);
        // m_Painter3D->DrawSphere({0.0f, 0.0f, 0.0f}, 1.0f, 100, 100);
        // m_Painter3D->DrawIcoSphere({0.0f, 0.0f, 0.0f}, 1.0f, 5);
        // m_Painter3D->DrawCubeSphere({0.0f, 0.0f, 0.0f}, 1.0f, 8);
        // m_Painter3D->DrawCylinder({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1,
        //                           1.0f, 16);
        // m_Painter3D->DrawCone({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1, 1.0f,
        //                       16);
        // m_Painter3D->DrawPyramid({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1,
        //                          1.0f, 8, 8);
        // m_Painter3D->DrawFrustum({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1.0f,
        //                          1.0f, 0.5f, 8);
    }

    GLCheckError();
}

void Scene::InitOIT() {
#ifdef IGAME_OPENGL_VERSION_460
    GLuint* data = nullptr;
    // 8192x8192 会申请约 256MB(map) + 2GB(list)。在 4GB 显卡上 MapRange 常失败，
    // 若对空指针 memset 会变成 0xC0000005。失败时跳过 OIT，不阻断普通显示。
    size_t totalPixels = MAX_FRAMEBUFFER_WIDTH * MAX_FRAMEBUFFER_HEIGHT;

    m_OITHeadPointerTexture->Create();
    m_OITHeadPointerTexture->Storage(1, GL_R32UI, MAX_FRAMEBUFFER_WIDTH,
                                     MAX_FRAMEBUFFER_HEIGHT);

    m_OITHeadPointerInitializer->Create();
    m_OITHeadPointerInitializer->Target(GL_PIXEL_UNPACK_BUFFER);
    m_OITHeadPointerInitializer->Allocate(totalPixels * sizeof(GLuint), nullptr,
                                          GL_STATIC_DRAW);
    data = static_cast<GLuint*>(m_OITHeadPointerInitializer->MapRange(
            0, totalPixels * sizeof(GLuint), GL_MAP_WRITE_BIT));
    if (data == nullptr) {
        IGAME_RENDERING_ERROR(
                "InitOIT: MapRange failed (need ~{} MB). Skip OIT buffers.",
                (totalPixels * sizeof(GLuint)) / (1024 * 1024));
        GLCheckError();
        return;
    }
    // 0xFF is equivalent to the end of the linked list
    memset(data, 0xFF, totalPixels * sizeof(GLuint));
    m_OITHeadPointerInitializer->Unmap();

    m_OITAtomicCounterBuffer->Create();
    m_OITAtomicCounterBuffer->Target(GL_ATOMIC_COUNTER_BUFFER);
    m_OITAtomicCounterBuffer->Allocate(sizeof(GLuint), nullptr,
                                       GL_DYNAMIC_COPY);

    m_OITLinkedListBuffer->Create();
    m_OITLinkedListBuffer->Target(GL_TEXTURE_BUFFER);
    m_OITLinkedListBuffer->Allocate(2 * totalPixels * sizeof(igm::vec4),
                                    nullptr, GL_DYNAMIC_COPY);

    m_OITLinkedListTexture->Create();
    m_OITLinkedListTexture->Buffer(GL_RGBA32UI, m_OITLinkedListBuffer);
#endif
    GLCheckError();
}

void Scene::InitAxes() {
    const wchar_t* text = L"XYZ";
    m_FontManager->RegisterWords(text);
    m_Axes->Initialize();

    GLCheckError();
}

void Scene::InitInterator() {
    m_Interactor->Initialize(this);
    m_Interactor->CreateDefaultStyle();
}

void Scene::ResizeFrameBuffer() {
    auto viewport = m_Camera->GetScaledViewPort();
    uint32_t width = viewport.x;
    uint32_t height = viewport.y;

#ifdef GL_SUPPORT_MSAA
    // resize multisample framebuffer
    {
        //glGetIntegerv(GL_MAX_SAMPLES, &samples);

        auto fbo = GLFramebuffer::New();
        fbo->Create();
        fbo->Target(GL_FRAMEBUFFER);
        fbo->Bind();

        auto colorTexture = GLTexture2dMultisample::New();
        colorTexture->Create();
        colorTexture->Bind();
        colorTexture->Storage(samples, GL_RGBA8, width, height, GL_TRUE);
        fbo->Texture(GL_COLOR_ATTACHMENT0, colorTexture, 0);

        auto depthTexture = GLTexture2dMultisample::New();
        depthTexture->Create();
        depthTexture->Bind();
        depthTexture->Storage(samples, GL_DEPTH_COMPONENT24, width, height,
                              GL_TRUE);
        fbo->Texture(GL_DEPTH_ATTACHMENT, depthTexture, 0);

        fbo->Release();

        m_ColorTextureMultisampled = colorTexture;
        m_DepthTextureMultisampled = depthTexture;
        m_FramebufferMultisampled = fbo;
        if (m_FramebufferMultisampled->CheckStatus() !=
            GL_FRAMEBUFFER_COMPLETE) {
            IGAME_RENDERING_ERROR("{}, framebuffer is not complete!",
                                  this->GetName());
        }
    }
#endif

    //resize single sample framebuffer
    {
        auto fbo = GLFramebuffer::New();
        fbo->Create();
        fbo->Target(GL_FRAMEBUFFER);
        fbo->Bind();

        auto colorTexture = GLTexture2d::New();
        colorTexture->Create();
        colorTexture->Bind();
        colorTexture->Storage(1, GL_RGBA8, width, height);
        colorTexture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        colorTexture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        colorTexture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        colorTexture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        fbo->Texture(GL_COLOR_ATTACHMENT0, colorTexture, 0);

        auto colorTextureBackup = GLTexture2d::New();
        colorTextureBackup->Create();
        colorTextureBackup->Bind();
        colorTextureBackup->Storage(1, GL_RGBA8, width, height);
        colorTextureBackup->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        colorTextureBackup->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        colorTextureBackup->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        colorTextureBackup->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        auto depthR32FTexture = GLTexture2d::New();
        depthR32FTexture->Create();
        depthR32FTexture->Bind();
        depthR32FTexture->Storage(1, GL_R32F, width, height);
        depthR32FTexture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        depthR32FTexture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        depthR32FTexture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        depthR32FTexture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        fbo->Texture(GL_COLOR_ATTACHMENT1, depthR32FTexture, 0);

        GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        fbo->DrawBuffers(2, buffers);

        auto depthTexture = GLTexture2d::New();
        depthTexture->Create();
        depthTexture->Bind();
        depthTexture->Storage(1, GL_DEPTH_COMPONENT24, width, height);
        fbo->Texture(GL_DEPTH_ATTACHMENT, depthTexture, 0);

        fbo->Release();

        m_ColorTexture = colorTexture;
        m_DepthR32FTexture = depthR32FTexture;
        m_DepthTexture = depthTexture;
        m_Framebuffer = fbo;
        if (m_Framebuffer->CheckStatus() != GL_FRAMEBUFFER_COMPLETE) {
            IGAME_RENDERING_ERROR("{}, framebuffer is not complete!",
                                  this->GetName());
        }
    }

    //resize backup framebuffer
    {
        auto fbo = GLFramebuffer::New();
        fbo->Create();
        fbo->Target(GL_FRAMEBUFFER);
        fbo->Bind();

        auto colorTexture = GLTexture2d::New();
        colorTexture->Create();
        colorTexture->Bind();
        colorTexture->Storage(1, GL_RGBA8, width, height);
        colorTexture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        colorTexture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        colorTexture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        colorTexture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        fbo->Texture(GL_COLOR_ATTACHMENT0, colorTexture, 0);

        auto depthTexture = GLTexture2d::New();
        depthTexture->Create();
        depthTexture->Bind();
        depthTexture->Storage(1, GL_DEPTH_COMPONENT24, width, height);
        fbo->Texture(GL_DEPTH_ATTACHMENT, depthTexture, 0);

        fbo->Release();

        m_ColorTextureBackup = colorTexture;
        m_FramebufferBackup = fbo;
        if (m_FramebufferBackup->CheckStatus() != GL_FRAMEBUFFER_COMPLETE) {
            IGAME_RENDERING_ERROR("{}, framebuffer is not complete!",
                                  this->GetName());
        }
    }

#ifdef IGAME_OPENGL_VERSION_460
    auto fbo = GLFramebuffer::New();
    fbo->Create();
    fbo->Target(GL_FRAMEBUFFER);
    fbo->Bind();

    auto colorTexture = GLTexture2d::New();
    colorTexture->Create();
    colorTexture->Bind();
    colorTexture->Storage(1, GL_RGBA8, VOLUME_RENDERING_FRAMEBUFFER_WIDTH,
                          VOLUME_RENDERING_FRAMEBUFFER_WIDTH /
                                  m_Camera->aspect<float>());
    colorTexture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    colorTexture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    colorTexture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    colorTexture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fbo->Texture(GL_COLOR_ATTACHMENT0, colorTexture, 0);

    auto depthTexture = GLTexture2d::New();
    depthTexture->Create();
    depthTexture->Bind();
    depthTexture->Storage(
            1, GL_DEPTH_COMPONENT24, VOLUME_RENDERING_FRAMEBUFFER_WIDTH,
            VOLUME_RENDERING_FRAMEBUFFER_WIDTH / m_Camera->aspect<float>());
    fbo->Texture(GL_DEPTH_ATTACHMENT, depthTexture, 0);

    fbo->Release();

    m_VolumeColorTexture = colorTexture;
    m_VolumeDepthTexture = depthTexture;
    m_VolumeFramebuffer = fbo;
    if (m_VolumeFramebuffer->CheckStatus() != GL_FRAMEBUFFER_COMPLETE) {
        IGAME_RENDERING_ERROR("{}, framebuffer is not complete!",
                              this->GetName());
    }
#endif

    ResizeHzb();
}
void Scene::ResizeHzb() {
#ifdef IGAME_OPENGL_VERSION_460
    auto width = m_Camera->GetScaledViewPort().x;
    auto height = m_Camera->GetScaledViewPort().y;

    static auto previousPow2 = [](uint32_t v) {
        uint32_t r = 1;
        while (r * 2 < v) { r *= 2; }
        return r;
    };
    static auto compDepthMipLevels = [](uint32_t width, uint32_t height) {
        uint32_t result = 1;
        while (width > 1 || height > 1) {
            result++;
            width /= 2;
            height /= 2;
        }
        return result;
    };

    m_HzbWidth = previousPow2(width);
    m_HzbHeight = previousPow2(height);
    m_HzbLevels = compDepthMipLevels(m_HzbWidth, m_HzbHeight);

    SmartPointer<GLTexture2d> texture = GLTexture2d::New();
    texture->Create();
    texture->Bind();
    texture->Storage(m_HzbLevels, GL_R32F, m_HzbWidth, m_HzbHeight);
    texture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    texture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    texture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    texture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    texture->Release();

    m_HzbTexture = std::move(texture);
#endif
}

void Scene::Draw() {
    // save default framebuffer, because it is not 0 in Qt
    GLint defaultFramebuffer = GL_NONE;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFramebuffer);

    // If throttling is enabled and the time point for the next frame has not been reached, return in advance (do not render)
    if (m_FramePacingEnabled && m_LastRenderEndValid) {
        if (!ShouldRenderThisCall()) {
            // Still copy the result of the previous frame to Qt's default frame buffer to avoid flickering
#ifndef __EMSCRIPTEN__
            RenderToSpecificFrame(defaultFramebuffer);
#endif
            return;
        }
    }

    // reset camera
    UpdateCameraClippingRange();

    // Start counting the rendering time consumption
#ifndef __EMSCRIPTEN__
    int curIdx = m_TimeQueryIndex;
    glBeginQuery(GL_TIME_ELAPSED, m_TimeQueries[curIdx]);
#endif

    // render
    DrawFrame();
#ifndef __EMSCRIPTEN__
    RenderToSpecificFrame(defaultFramebuffer);
#endif

    // End counting the rendering time consumption
#ifndef __EMSCRIPTEN__
    glEndQuery(GL_TIME_ELAPSED);
    {
        int prevIdx = 1 - curIdx;
        // Only query if the previous query has actually been issued at least once
        if (m_TimeQueryReady[prevIdx]) {
            GLint available = GL_FALSE;
            glGetQueryObjectiv(m_TimeQueries[prevIdx],
                               GL_QUERY_RESULT_AVAILABLE, &available);
            if (available == GL_TRUE) {
                GLuint64 ns = 0ULL;
                glGetQueryObjectui64v(m_TimeQueries[prevIdx], GL_QUERY_RESULT,
                                      &ns);
                m_LastGpuTimeMs = static_cast<double>(ns) / 1000000.0;
                if (m_SmoothedGpuTimeMs <= 0.0) {
                    m_SmoothedGpuTimeMs = m_LastGpuTimeMs;
                } else {
                    const double alpha = 0.2;
                    m_SmoothedGpuTimeMs = alpha * m_LastGpuTimeMs +
                                          (1.0 - alpha) * m_SmoothedGpuTimeMs;
                }
                // We've consumed the previous result; mark it not-ready until next issued
                m_TimeQueryReady[prevIdx] = false;
            }
        }
        // Mark current query as issued so that it can be polled next frame
        m_TimeQueryReady[curIdx] = true;
        m_TimeQueryIndex = 1 - curIdx;
    }
#endif

    // Record the end time of this frame
    m_LastRenderEnd = std::chrono::steady_clock::now();
    m_LastRenderEndValid = true;

    // 实测帧率：1 秒滑动窗口统计“真正重绘”的次数。渲染是事件驱动的，
    // 空闲时不重绘，此时不推进窗口（界面按“空闲”显示），长时间空闲后的
    // 第一个窗口样本直接作废，避免出现 0.03 FPS 这种误导读数。
    if (!m_FpsWindowStartValid) {
        m_FpsWindowStart = m_LastRenderEnd;
        m_FpsWindowStartValid = true;
        m_FpsFrameCount = 0;
    } else {
        ++m_FpsFrameCount;
        const double windowMs =
                std::chrono::duration<double, std::milli>(m_LastRenderEnd -
                                                         m_FpsWindowStart)
                        .count();
        if (windowMs >= 1000.0) {
            m_Fps = windowMs <= 2000.0
                            ? static_cast<double>(m_FpsFrameCount) * 1000.0 /
                                      windowMs
                            : 0.0;
            m_FpsFrameCount = 0;
            m_FpsWindowStart = m_LastRenderEnd;
        }
    }

    GLCheckError();
}

void Scene::RefreshHzb() {
#ifdef IGAME_OPENGL_VERSION_460
    auto shader = this->GetShader(ShaderType::DEPTHREDUCE);
    shader->Use();

    ResolveFrameBuffer();
    GLCheckError();

    m_DepthR32FTexture->Active(GL_TEXTURE1);
    shader->SetUniformi("screenDepth", 1);

    m_HzbTexture->Active(GL_TEXTURE2);
    shader->SetUniformi("myDepthPyramid", 2);

    // generate level 0
    {
        unsigned int level = 0;
        uint32_t width = m_HzbWidth;
        uint32_t height = m_HzbHeight;
        shader->Use();
        shader->SetUniformui("level", level);
        shader->SetUniform2ui("outDepthPyramidSize", igm::uvec2{width, height});
        m_HzbTexture->BindImage(0, level, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
        glDispatchCompute((width + 31) / 32, (height + 31) / 32, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                        GL_TEXTURE_FETCH_BARRIER_BIT);
        GLCheckError();
    }

    // generate other level
    for (unsigned int level = 1; level < m_HzbLevels; ++level) {
        uint32_t width = m_HzbWidth >> (level - 1);
        uint32_t height = m_HzbHeight >> (level - 1);
        if (width < 1) width = 1;
        if (height < 1) height = 1;

        uint32_t levelWidth = width >> 1;
        uint32_t levelHeight = height >> 1;
        if (levelWidth < 1) levelWidth = 1;
        if (levelHeight < 1) levelHeight = 1;

        shader->Use();

        shader->Use();
        shader->SetUniformui("level", level);
        shader->SetUniform2ui("inDepthPyramidSize", igm::uvec2{width, height});
        m_HzbTexture->BindImage(0, level, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
        glDispatchCompute((levelWidth + 31) / 32, (levelHeight + 31) / 32, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                        GL_TEXTURE_FETCH_BARRIER_BIT);
        GLCheckError();
    }
#endif
}

void Scene::Update() {
    if (m_UpdateFunctor) { m_UpdateFunctor(); }
}

void Scene::SetTargetFps(unsigned int fps) {
    if (fps == 0u) {
        m_TargetFps = 0u;
    } else {
        if (fps > 1000u) { fps = 1000u; }
        m_TargetFps = fps;
        m_FramePacingEnabled = true;
    }
}

void Scene::SetGpuUsageLimit(float usagePercent) {
    if (usagePercent <= 0.0f) {
        m_GpuUsageLimit = 0.0f;
        return;
    }
    if (usagePercent > 1.0f) usagePercent = 1.0f;
    m_GpuUsageLimit = usagePercent;
    m_FramePacingEnabled = true;
}

void Scene::EnableFramePacing(bool enable) { m_FramePacingEnabled = enable; }

void Scene::Resize(int width, int height, int pixelRatio) {
    m_Camera->SetViewPort(width, height);
    m_Camera->SetDevicePixelRatio(pixelRatio);
    ResizeFrameBuffer();
}

void Scene::ClearSceneFramebuffer(float depth, int width, int height) {
    glClearDepth(depth); // reversed-z: near=1.0, far=0.0

    auto drawEnvironmentBackground = [&]() {
#ifndef __EMSCRIPTEN__
        if (m_EnvironmentBackgroundVisible && m_EnvironmentLight) {
            // The panorama's +Y axis is the sky. Align it with the chosen ground
            // normal, then apply the same scene rotation as the ground itself.
            igm::mat4 skyOrientation{1.0f};
            const float quarterTurn = static_cast<float>(igm::radians(90.0f));
            if (m_GroundGridPlane == GroundGridPlane::XY) {
                skyOrientation = igm::rotate(skyOrientation, quarterTurn, igm::vec3{1, 0, 0});
            } else if (m_GroundGridPlane == GroundGridPlane::YZ) {
                skyOrientation = igm::rotate(skyOrientation, -quarterTurn, igm::vec3{0, 0, 1});
            }
            if (m_GroundGridReversed) {
                skyOrientation = igm::rotate(skyOrientation, 2.0f * quarterTurn, igm::vec3{1, 0, 0});
            }
            const auto skyView = m_Camera->GetViewMatrix() * m_ModelRotate * skyOrientation;
            m_EnvironmentLight->DrawBackground(skyView, m_Camera->GetProjectionMatrix(),
                                               m_EnvironmentRotation, width, height);
        }
#endif
    };

    GLboolean wasScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    GLint previousScissor[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_SCISSOR_BOX, previousScissor);
    glDisable(GL_SCISSOR_TEST);

    if (!m_BackgroundGradientEnabled || m_BackgroundGradientMode == 0 ||
        width <= 0 || height <= 0) {
        glClearColor(m_BackgroundColor.r, m_BackgroundColor.g,
                     m_BackgroundColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);
        drawEnvironmentBackground();
        if (wasScissorEnabled) {
            glEnable(GL_SCISSOR_TEST);
            glScissor(previousScissor[0], previousScissor[1],
                      previousScissor[2], previousScissor[3]);
        }
        return;
    }

    glClearColor(m_BackgroundGradientColor.r, m_BackgroundGradientColor.g,
                 m_BackgroundGradientColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_SCISSOR_TEST);
    const int stripCount = 96;
    const bool horizontal = m_BackgroundGradientMode == 2;
    const int length = horizontal ? width : height;

    for (int i = 0; i < stripCount; ++i) {
        int start = (i * length) / stripCount;
        int end = ((i + 1) * length) / stripCount;
        if (end <= start) { continue; }

        float t = stripCount > 1 ? static_cast<float>(i) /
                                           static_cast<float>(stripCount - 1)
                                 : 0.0f;
        if (!horizontal) { t = 1.0f - t; }

        float r = m_BackgroundColor.r * (1.0f - t) +
                  m_BackgroundGradientColor.r * t;
        float g = m_BackgroundColor.g * (1.0f - t) +
                  m_BackgroundGradientColor.g * t;
        float b = m_BackgroundColor.b * (1.0f - t) +
                  m_BackgroundGradientColor.b * t;

        glClearColor(r, g, b, 1.0f);
        if (horizontal) {
            glScissor(start, 0, end - start, height);
        } else {
            glScissor(0, start, width, end - start);
        }
        glClear(GL_COLOR_BUFFER_BIT);
    }

    drawEnvironmentBackground();

    if (wasScissorEnabled) {
        glScissor(previousScissor[0], previousScissor[1], previousScissor[2],
                  previousScissor[3]);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void Scene::DrawFrame() {
    auto viewport = m_Camera->GetScaledViewPort();
#ifndef __EMSCRIPTEN__
    if ((m_EnvironmentEnabled || m_EnvironmentBackgroundVisible) && m_EnvironmentLight) {
        m_EnvironmentLight->Initialize();
    }
#endif

    // Convert to drawable data
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        model->SyncGpuBuffers();
    }

#ifndef __EMSCRIPTEN__
    // 仅消费显式适配请求；放在 SyncGpuBuffers 之后，确保新导入几何已就绪。
    // 裁剪预览、显隐及相机更新不再让底板跟随变化的结果包围盒移动。
    if (m_GroundGridDirty) {
        UpdateGroundGrid();
        UpdateCameraClippingRange();
    }

#endif

    // Update camera data block in GPU
    UpdateCameraDataBlock();
    {
        // Clear default framebuffer before rendering
        BindFramebuffer();
        ClearSceneFramebuffer(0.0f, viewport.x, viewport.y);

#ifndef __EMSCRIPTEN__
        m_VolumeFramebuffer->Bind();
        ClearSceneFramebuffer(0.0f, VOLUME_RENDERING_FRAMEBUFFER_WIDTH,
                              static_cast<int>(VOLUME_RENDERING_FRAMEBUFFER_WIDTH /
                                               m_Camera->aspect<float>()));

    #ifdef GL_SUPPORT_MSAA
        m_FramebufferMultisampled->Bind();
        ClearSceneFramebuffer(0.0f, viewport.x, viewport.y);
    #endif
#endif

        // Render to framebuffer
        glViewport(0, 0, viewport.x, viewport.y);
#ifdef IGAME_OPENGL_VERSION_330
        ShadowPass();
        ForwardPass();
    #ifdef __EMSCRIPTEN__
        TransparentPass();
    #endif
#else
        // 两种体绘制实现互斥：Raycast = compute 光线步进（结构化网格），
        // Fragment = 旧的片元链表实现；都未开启时走普通渲染
        if (m_EnableVolumeRaycasting) {
            // 目前体绘制结果直接覆盖视口（与旧的体绘制行为一致，暂不与几何做
            // 深度合成），后续可改为先画不透明几何再做深度感知合成
            VolumeRaycastPass();
        } else if (!m_EnableVolumeRendering) {
            ShadowPass();
            ForwardPass();
            TransparentPass();
        } else {
            // 旧的“片元链表”体绘制通道
            VolumeRenderingPass();
        }
#endif

        // Draw painter 2d and axes
        m_Framebuffer->Bind();
        glViewport(0, 0, viewport.x, viewport.y);
        {
            // Draw painter 2D in the image top
            m_Painter2D->Draw();
            if (m_ColorBarVisible && m_ColorBar2DActor) {
                m_ColorBar2DActor->Draw();
            }
            if (m_TextOverlay2DActor) { m_TextOverlay2DActor->Draw(); }

            if (m_AxesVisible) {
                int mx = std::max(viewport.x, viewport.y);
                glViewport(0, 0, mx / 12, mx / 12);
                m_Axes->Draw();
            }
        }
    }
}

void Scene::RenderToSpecificFrame(GLint frameBuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    auto viewport = m_Camera->GetScaledViewPort();
    glViewport(0, 0, viewport.x, viewport.y);
    glDisable(GL_DEPTH_TEST);

    auto shader = this->GetShader(ShaderType::SCREEN);
    //auto shader = GetShader(Scene::FXAA);
    shader->Use();

    m_ColorTexture->Active(GL_TEXTURE1);
    m_DepthTexture->Active(GL_TEXTURE2);
    m_HzbTexture->Active(GL_TEXTURE3);
    shader->SetUniformi("screenColorSampler", 1);

    // Note:
    // 1. To enable depth rendering, ensure the screen.frag file is updated to handle the depth texture input.
    //    Specifically, the shader should read from the depth sampler and implement the desired depth-based operations.
    // 2. Additionally, disable any depth-buffer-clearing code in the coordinate axis rendering logic.
    //    Failing to do so could overwrite or invalidate the depth information required for rendering.
    shader->SetUniformi("screenColorSampler", 1);
    //shader->SetUniformf("near", m_Camera->GetClippingRange().x);
    //shader->SetUniformf("far", m_Camera->GetClippingRange().y);

    m_EmptyVAO->DrawArrays(GL_TRIANGLES, 0, 3);
}

void Scene::ResolveFrameBuffer() {
    m_Framebuffer->Bind();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    {
        auto viewport = m_Camera->GetScaledViewPort();
        glViewport(0, 0, viewport.x, viewport.y);

        auto shader = this->GetShader(ShaderType::ATTACHMENTRESOLVE);
        shader->Use();

        GLFramebuffer::Blit(m_Framebuffer, m_FramebufferBackup, 0, 0,
                            viewport.x, viewport.y, 0, 0, viewport.x,
                            viewport.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        shader->SetUniformi("numSamples", 1);
        m_ColorTextureBackup->Active(GL_TEXTURE1);
        shader->SetUniformi("colorTexture", 1);
        m_DepthTexture->Active(GL_TEXTURE2);
        shader->SetUniformi("depthTexture", 2);

#ifdef GL_SUPPORT_MSAA
        shader->SetUniformi("numSamples", samples);
        m_ColorTextureMultisampled->Active(GL_TEXTURE3);
        shader->SetUniformi("colorTextureMS", 3);
        m_DepthTextureMultisampled->Active(GL_TEXTURE4);
        shader->SetUniformi("depthTextureMS", 4);
#endif

        m_EmptyVAO->DrawArrays(GL_TRIANGLES, 0, 3);
    }
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Scene::ShadowPass() {
    // use reversed-z buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);

    glDisable(GL_DEPTH_TEST);

    GLCheckError();
}

void Scene::ForwardPass() {
    // Use reversed-z buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);

#ifdef IGAME_OPENGL_VERSION_330
    BindFramebuffer();
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        model->Draw();
        if (model->GetVisibility()) {
            auto& painter3Ds = model->GetAllPainter3Ds();
            for (auto& painter: painter3Ds) { painter.second->Draw(); }
        }
    }
#elif IGAME_OPENGL_VERSION_460
    // normal mesh
    BindFramebuffer();
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;

        // draw mesh
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        //if (!drawObject->IsAlwaysOnTop()) { model->Draw(); }
        model->Draw();

        // draw painter(since painter does not support transparency)
        if (drawObject->GetVisibility()) {
            auto& painter3Ds = model->GetAllPainter3Ds();
            for (auto& painter: painter3Ds) { painter.second->Draw(); }
        }
    }

    // Web 保持原有绘制顺序；桌面端在实体底板之后重绘置顶对象。
#ifdef __EMSCRIPTEN__
    // 第二次遍历：专门渲染AlwaysOnTop模型（最后绘制）
    glDisable(GL_DEPTH_TEST);
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;

        // draw mesh
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        if (drawObject->IsAlwaysOnTop()) { 
            model->Draw(); 
        }
    }
    glEnable(GL_DEPTH_TEST);
#endif

    // meshleter mesh
    #ifdef GL_SUPPORTS_MESH_SHADER
    {
        // draw phase1: draw visible meshlet
        // Note: The first HZB culling pass must use the previous frame's data
        for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
            auto model = it->second;
            model->DrawPhase1();
        }

        // refresh phase 1: generate loacl hierarchical z-buffer & cull data
        RefreshHzb();

        // draw phase2: draw invisible meshlet
        RefreshDrawCullDataBuffer();
        for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
            auto model = it->second;
            model->DrawPhase2();
        }

        // refresh phase2: generate global hierarchical z-buffer
        RefreshHzb();
    }
    #else
    {
        // pre pass: test occlusion results for phase 1
        for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
            auto model = it->second;
            model->TestOcclusionResults();
        }

        // draw phase1: draw visible meshlet
        BindFramebuffer();
        for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
            auto model = it->second;
            model->DrawPhase1();
        }

        // refresh phase1: generate loacl hierarchical z-buffer
        RefreshHzb();

        // draw phase2: draw invisible meshlet
        BindFramebuffer();
        RefreshDrawCullDataBuffer();
        for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
            auto model = it->second;
            model->DrawPhase2();
        }

        // refresh phase2: generate global hierarchical z-buffer
        RefreshHzb();
    }
    #endif
#endif

    // Draw scene painter
    BindFramebuffer();
    m_Painter3D->Draw();

#ifndef __EMSCRIPTEN__
    // 桌面端解析式底板；Web 保持原有绘制流程。
    if (m_GroundGridVisible && m_GroundGridBuilt) {
        DrawGroundGrid();
    }

#ifdef IGAME_OPENGL_VERSION_460
    glDisable(GL_DEPTH_TEST);
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        if (drawObject && drawObject->IsAlwaysOnTop()) { model->Draw(); }
    }
    glEnable(GL_DEPTH_TEST);
#endif
#endif

#ifndef __EMSCRIPTEN__
    ResolveFrameBuffer();
#ifdef GL_SUPPORT_MSAA
    // Resolve 只写 R32F 颜色附件，透明绘制使用的单采样深度还需要底板。
    if (m_GroundGridVisible && m_GroundGridBuilt) {
        GLboolean colorMask[4];
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        m_Framebuffer->Bind();
        DrawGroundGrid();
        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    }
#endif
#endif
    GLCheckError();
}

void Scene::TransparentPass() {
#ifdef __EMSCRIPTEN__
    ClearPendingGLErrors();
    BindFramebuffer();
    CheckTransparentPassError("BindFramebuffer");

    auto drawTransparentModels = [&](const char* stage) {
        for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
            auto model = it->second;
            CheckTransparentPassError(stage);
            model->DrawWithTransparency();
            CheckTransparentPassError("DrawWithTransparency");
        }
    };

    glEnable(GL_DEPTH_TEST);
    CheckTransparentPassError("glEnable(GL_DEPTH_TEST)");
    glDepthFunc(GL_GREATER);
    CheckTransparentPassError("glDepthFunc(GL_GREATER)");

    // WebGL fallback: first capture only the nearest transparent surface in
    // the reversed-z depth buffer. This prevents dense meshes from blending
    // many internal/back-facing layers and making small opacity values look
    // much more opaque than requested.
    glDisable(GL_BLEND);
    CheckTransparentPassError("depth prepass glDisable(GL_BLEND)");
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    CheckTransparentPassError("depth prepass glColorMask false");
    glDepthMask(GL_TRUE);
    CheckTransparentPassError("depth prepass glDepthMask(GL_TRUE)");
    drawTransparentModels("depth prepass before DrawWithTransparency");

    glDepthMask(GL_FALSE);
    CheckTransparentPassError("glDepthMask(GL_FALSE)");
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    CheckTransparentPassError("color pass glColorMask true");
    glDepthFunc(GL_GEQUAL);
    CheckTransparentPassError("color pass glDepthFunc(GL_GEQUAL)");
    glEnable(GL_BLEND);
    CheckTransparentPassError("glEnable(GL_BLEND)");
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    CheckTransparentPassError("glBlendFunc");
    drawTransparentModels("color pass before DrawWithTransparency");

    glDisable(GL_BLEND);
    CheckTransparentPassError("glDisable(GL_BLEND)");
    glDepthMask(GL_TRUE);
    CheckTransparentPassError("glDepthMask(GL_TRUE)");
    glDepthFunc(GL_GREATER);
    CheckTransparentPassError("restore glDepthFunc(GL_GREATER)");
#elif defined(IGAME_OPENGL_VERSION_460)
    // Bind framebuffer
    m_Framebuffer->Bind();

    // 先把不透明前景结果拷贝到备份纹理，排序阶段采样它，
    // 避免“写入 m_ColorTexture 的同时采样同一纹理”的反馈回路
    {
        auto viewport = m_Camera->GetScaledViewPort();
        GLFramebuffer::Blit(m_Framebuffer, m_FramebufferBackup, 0, 0,
                            viewport.x, viewport.y, 0, 0, viewport.x,
                            viewport.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // Enable blending to use the alpha channel for transparency.
    // Without blending, the alpha value in the color will be ignored.
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // use reversed-z buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);

    // 1.reset oit pipeline status
    {
        auto shader = this->GetShader(ShaderType::TRANSPARENCYLINK);
        shader->Use();

        m_OITAtomicCounterBuffer->BindBase(0);
        const GLuint zero = 0;
        m_OITAtomicCounterBuffer->SubData(0, sizeof(zero), &zero);

        m_OITHeadPointerInitializer->Bind();
        m_OITHeadPointerTexture->Bind();
        m_OITHeadPointerTexture->SubImage(
                0, 0, 0, MAX_FRAMEBUFFER_WIDTH, MAX_FRAMEBUFFER_HEIGHT,
                GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        m_OITHeadPointerTexture->Release();
        m_OITHeadPointerInitializer->Release();

        m_OITHeadPointerTexture->BindImage(0, 0, GL_FALSE, 0, GL_READ_WRITE,
                                           GL_R32UI);
        m_OITLinkedListTexture->BindImage(1, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                                          GL_RGBA32UI);
    }

    // 2.build the oit link list
    glDepthMask(GL_FALSE);
    {
        // add the result of drawing opaque objects
        for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
            auto model = it->second;
            model->DrawWithTransparency();
        }
    }
    glDepthMask(GL_TRUE);

    // 3.sorting and blending colors
    glDisable(GL_DEPTH_TEST);
    {
        auto shader = this->GetShader(ShaderType::TRANSPARENCYSORT);
        shader->Use();

        m_ColorTextureBackup->Active(GL_TEXTURE1);
        shader->SetUniformi("forwardPassColor", 1);

        m_OITHeadPointerTexture->BindImage(0, 0, GL_FALSE, 0, GL_READ_ONLY,
                                           GL_R32UI);
        m_OITLinkedListTexture->BindImage(1, 0, GL_FALSE, 0, GL_READ_ONLY,
                                          GL_RGBA32UI);

        m_EmptyVAO->DrawArrays(GL_TRIANGLES, 0, 3);
    }
    glEnable(GL_DEPTH_TEST);

    // glDisable(GL_BLEND);
#endif
    glDisable(GL_DEPTH_TEST);
    GLCheckError();
}

void Scene::VolumeRenderingPass() {
#ifdef IGAME_OPENGL_VERSION_460
    auto width = VOLUME_RENDERING_FRAMEBUFFER_WIDTH;
    auto height = static_cast<uint32_t>(VOLUME_RENDERING_FRAMEBUFFER_WIDTH /
                                        m_Camera->aspect<float>());

    m_VolumeFramebuffer->Bind();
    glViewport(0, 0, width, height);

    // use reversed-z buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glDisable(GL_BLEND);

    // 1.reset oit pipeline status
    {
        auto shader = this->GetShader(ShaderType::VOLUMERENDERINGLINK);
        shader->Use();

        m_OITAtomicCounterBuffer->BindBase(0);
        const GLuint zero = 0;
        m_OITAtomicCounterBuffer->SubData(0, sizeof(zero), &zero);

        m_OITHeadPointerInitializer->Bind();
        m_OITHeadPointerTexture->Bind();
        m_OITHeadPointerTexture->SubImage(
                0, 0, 0, MAX_FRAMEBUFFER_WIDTH, MAX_FRAMEBUFFER_HEIGHT,
                GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        m_OITHeadPointerTexture->Release();
        m_OITHeadPointerInitializer->Release();

        m_OITHeadPointerTexture->BindImage(0, 0, GL_FALSE, 0, GL_READ_WRITE,
                                           GL_R32UI);
        m_OITLinkedListTexture->BindImage(1, 0, GL_FALSE, 0, GL_READ_WRITE,
                                          GL_RGBA32UI);
    }

    // 2.build the oit link list
    glDepthMask(GL_FALSE);
    {
        // add the result of drawing opaque objects
        for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
           auto model = it->second;
           model->DrawWithVolume();
       }
    }
    glDepthMask(GL_TRUE);

    // 一次性自检（旧实现）：统计参与绘制的对象、子对象以及实际插入的片元数。
    // fragments == 0 说明几何没有被光栅化（可见性/样式/索引为空等问题）；
    // fragments > 0 说明片元已生成，问题在排序/合成或 blit 环节。
    if (m_VolumeLegacyDiagPending) {
        m_VolumeLegacyDiagPending = false;

        int modelCount = 0;
        int visibleModelCount = 0;
        int objectCount = 0;
        int visibleObjectCount = 0;
        int surfaceObjectCount = 0;
        int emptyIndexObjectCount = 0;
        int renderableWithTriangles = 0;
        std::string detail;
        for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
            ++modelCount;
            auto model = it->second;
            if (model == nullptr) { continue; }
            if (model->GetVisibility()) { ++visibleModelCount; }

            auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
            if (drawObject == nullptr) { continue; }

            auto accumulate = [&](const SmartPointer<DataObject>& object) {
                auto candidate = DynamicCast<DrawObject>(object);
                if (candidate == nullptr) { return; }
                ++objectCount;
                if (candidate->GetVisibility()) { ++visibleObjectCount; }
                if (candidate->GetViewStyle() & IG_SURFACE) {
                    ++surfaceObjectCount;
                }
                if (candidate->m_TriangleIndices == nullptr ||
                    candidate->m_TriangleIndices->GetNumberOfValues() == 0) {
                    ++emptyIndexObjectCount;
                }
                // 回退用的渲染对象是否有几何（体绘制现在会在原始体为空时回退）
                auto renderable = candidate->GetRenderableObject(false);
                if (renderable != nullptr &&
                    renderable->m_TriangleIndices != nullptr &&
                    renderable->m_TriangleIndices->GetNumberOfValues() > 0) {
                    ++renderableWithTriangles;
                }

                // 每个可见对象打一条明细，便于定位“没有几何/几何在别的对象上”
                if (candidate->GetVisibility() && detail.size() < 900) {
                    const IGsize ownTri =
                            candidate->m_TriangleIndices != nullptr
                                    ? candidate->m_TriangleIndices
                                              ->GetNumberOfValues()
                                    : 0;
                    const IGsize renderTri =
                            renderable != nullptr &&
                                            renderable->m_TriangleIndices != nullptr
                                    ? renderable->m_TriangleIndices
                                              ->GetNumberOfValues()
                                    : 0;
                    detail += " [" + candidate->GetName() + " type=" +
                              std::to_string(candidate->GetDataObjectType()) +
                              " ownTri=" + std::to_string(ownTri) +
                              " renderTri=" + std::to_string(renderTri) +
                              " style=" + std::to_string(candidate->GetViewStyle()) +
                              " shell=" +
                              (candidate->GetShellRenderingOption() ? "1" : "0") +
                              "]";
                }
            };

            if (!drawObject->HasSubDataObject()) {
                accumulate(drawObject);
                continue;
            }
            for (auto sit = drawObject->SubDataObjectIteratorBegin();
                 sit != drawObject->SubDataObjectIteratorEnd(); ++sit) {
                accumulate(sit->second);
            }
        }

        GLuint fragmentCount = 0;
        if (m_OITAtomicCounterBuffer != nullptr) {
            m_OITAtomicCounterBuffer->GetSubData(0, sizeof(fragmentCount),
                                                 &fragmentCount);
        }
        IGAME_RENDERING_INFO(
                "[Scene::VolumeRenderingPass][diag] models={} visibleModels={} "
                "objects={} visibleObjects={} surfaceObjects={} "
                "emptyIndexObjects={} renderableTriObjects={} fragments={} "
                "viewport={}x{}.",
                modelCount, visibleModelCount, objectCount, visibleObjectCount,
                surfaceObjectCount, emptyIndexObjectCount, renderableWithTriangles,
                fragmentCount, width, height);
        if (!detail.empty()) {
            IGAME_RENDERING_INFO(
                    "[Scene::VolumeRenderingPass][diag] visible object detail:{}",
                    detail);
        }
        GLCheckError();
    }

    // 3.sorting and blending colors
    glDisable(GL_DEPTH_TEST);
    {
        auto shader = this->GetShader(ShaderType::VOLUMERENDERINGSORT);
        shader->Use();

        m_VolumeColorTexture->Active(GL_TEXTURE1);
        shader->SetUniformi("forwardPassColor", 1);

        m_OITHeadPointerTexture->BindImage(0, 0, GL_FALSE, 0, GL_READ_ONLY,
                                           GL_R32UI);
        m_OITLinkedListTexture->BindImage(1, 0, GL_FALSE, 0, GL_READ_ONLY,
                                          GL_RGBA32UI);

        m_EmptyVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_EmptyVAO->Release();
    }

    // 4.resolve to main framebuffer
    auto viewport = m_Camera->GetScaledViewPort();
    GLFramebuffer::Blit(m_VolumeFramebuffer, m_Framebuffer, 0, 0, width, height,
                        0, 0, viewport.x, viewport.y, GL_COLOR_BUFFER_BIT,
                        GL_LINEAR);

#endif
    glDisable(GL_DEPTH_TEST);
    GLCheckError();
}

void Scene::BuildVolumeTransferFunctionInto(SmartPointer<GLTexture2d> target,
                                            ScalarsToColors::Pointer colorMapper,
                                            float valueMin, float valueMax) {
#ifdef IGAME_OPENGL_VERSION_460
    if (target == nullptr) { return; }

    // 256x1 传输函数 LUT：
    //  * 颜色：优先取数据对象的色标，使体绘制与表面绘制共用同一套颜色映射
    //  * 不透明度：默认每个体素贡献相同（常数 1.0，由 opacityScale 整体调节）；
    //    仅在勾选“不透明度映射”时才取色标的不透明度通道
    constexpr int kLutSize = 256;
    const float controlPos[3] = {0.0f, 0.5f, 1.0f};
    // 默认使用的三段式颜色映射：蓝-灰-红，若后期修改默认色标，记得同步这块
    const float controlColor[3][3] = {{0.20f, 0.25f, 0.75f},
                                      {0.85f, 0.85f, 0.85f},
                                      {0.90f, 0.15f, 0.10f}};

    const bool useMapper = colorMapper != nullptr;
    const bool useMapperOpacity =
            useMapper && colorMapper->GetOpacityMappingEnabled();
    float shift = 0.0f;
    float scale = 1.0f;
    if (useMapper) { colorMapper->ComputeShiftScale(shift, scale); }

    const float valueRange = valueMax - valueMin;
    std::vector<unsigned char> lut(kLutSize * 4, 0);
    for (int i = 0; i < kLutSize; ++i) {
        const float t =
                static_cast<float>(i) / static_cast<float>(kLutSize - 1);

        float rgb[3] = {0.0f, 0.0f, 0.0f};
        if (useMapper) {
            // 体纹理存的是归一化标量，这里还原到原始量纲再查色标
            colorMapper->GetColor(valueMin + t * valueRange, rgb, shift, scale);
        } else if (t <= controlPos[1]) {
            const float s = t / controlPos[1];
            for (int c = 0; c < 3; ++c) {
                rgb[c] = controlColor[0][c] * (1.0f - s) +
                         controlColor[1][c] * s;
            }
        } else {
            const float s =
                    (t - controlPos[1]) / (controlPos[2] - controlPos[1]);
            for (int c = 0; c < 3; ++c) {
                rgb[c] = controlColor[1][c] * (1.0f - s) +
                         controlColor[2][c] * s;
            }
        }

        // 不透明度：默认每个体素贡献相同（常数 1.0，再由 opacityScale 整体调节）；
        // 只有标量面板勾选“不透明度映射”时才改由色标的不透明度通道决定。
        const float alpha =
                useMapperOpacity
                        ? colorMapper->GetOpacity(valueMin + t * valueRange,
                                                  shift, scale)
                        : 1.0f;
        for (int c = 0; c < 3; ++c) {
            lut[i * 4 + c] = static_cast<unsigned char>(
                    std::clamp(rgb[c], 0.0f, 1.0f) * 255.0f + 0.5f);
        }
        lut[i * 4 + 3] = static_cast<unsigned char>(
                std::clamp(alpha, 0.0f, 1.0f) * 255.0f + 0.5f);
    }

    // 同上：immutable 纹理不能重复指定存储，需要重建纹理对象
    target->Destroy();
    target->Create();
    target->Storage(1, GL_RGBA8, kLutSize, 1);
    target->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    target->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    target->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    target->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    target->SubImage(0, 0, 0, kLutSize, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                     lut.data());
    target->Release();
#endif
}

void Scene::BuildVolumeTransferFunction(ScalarsToColors::Pointer colorMapper,
                                        float valueMin, float valueMax) {
    this->BuildVolumeTransferFunctionInto(m_VolumeTransferFunction, colorMapper,
                                          valueMin, valueMax);
}

bool Scene::BuildVolumeData(SmartPointer<DataObject> dataObject,
                            int attributeIndex, int attributeDimension) {
#ifdef IGAME_OPENGL_VERSION_460
    if (dataObject == nullptr || m_VolumeTexture3d == nullptr) {
        IGAME_RENDERING_WARN("[Scene::BuildVolumeData] invalid data object.");
        return false;
    }

    // 只接受结构化网格：点按 (i, j, k) 规则存放（GetPointIndex 为
    // i + j * nx + k * nx * ny），标量场可直接映射为 3D 纹理。
    // 非结构化网格不走这里（由 SetVolumeRendering 分派到旧的片元链表实现）；
    // 若想用光线步进渲染，请先用 ResampleToImageFilter 转成结构化网格。
    auto structuredMesh = DynamicCast<StructuredMesh>(dataObject);
    if (structuredMesh == nullptr) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeData] ray marching accepts structured mesh "
                "only; unstructured mesh is handled by the legacy fragment "
                "implementation (or should be resampled to a structured grid "
                "first).");
        return false;
    }

    igIndex* dimension = structuredMesh->GetDimensionSize();
    const igIndex nx = dimension[0];
    const igIndex ny = dimension[1];
    const igIndex nz = dimension[2];
    if (nx <= 1 || ny <= 1 || nz <= 1) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeData] invalid structured dimension: "
                "{}x{}x{}.",
                nx, ny, nz);
        return false;
    }

    const IGsize pointCount = static_cast<IGsize>(nx) *
                              static_cast<IGsize>(ny) *
                              static_cast<IGsize>(nz);
    if (structuredMesh->GetNumberOfPoints() != pointCount) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeData] point count ({}) does not match "
                "dimension ({}x{}x{}).",
                structuredMesh->GetNumberOfPoints(), nx, ny, nz);
        return false;
    }

    // 体绘制参数属于数据对象。Scene 的成员是供 shader/uniform 使用的缓存，
    // 每次重建都从当前模型同步，避免两个模型共用一个参数槽位。
    const auto& volumeSettings = dataObject->GetVolumeRenderingSettings();
    m_VolumeLayer0UserEnabled = volumeSettings.enabled;
    m_VolumeOpacityScale =
            std::clamp(volumeSettings.opacityScale, 0.0f, 1.0f);
    const int layer0MaskMode =
            std::clamp(volumeSettings.maskMode, 0, 2);
    m_VolumeMaskMode =
            static_cast<VolumeMaskMode>(layer0MaskMode);
    m_VolumeShellOpacity =
            std::clamp(volumeSettings.shellOpacity, 0.0f, 1.0f);

    auto attributeSet = structuredMesh->GetAttributeSet();
    if (attributeSet == nullptr || attributeIndex < 0 ||
        static_cast<size_t>(attributeIndex) >=
                attributeSet->GetNumberOfAttributes()) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeData] invalid attribute index {}.",
                attributeIndex);
        return false;
    }

    auto& attribute =
            attributeSet->GetAttribute(static_cast<IGsize>(attributeIndex));
    if (attribute.pointer == nullptr) {
        IGAME_RENDERING_WARN("[Scene::BuildVolumeData] attribute is empty.");
        return false;
    }

    // 取出标量场并归一化到 [0, 1]，shader 中直接作为传输函数查表坐标。
    // 注意：不能用 GetValue —— 那是按“扁平值索引”读取，多维属性会把
    // x/y/z 分量当成不同点的值；这里统一按“第 i 个元素的第 k 个分量”取。
    const int componentCount = attribute.pointer->GetDimension();
    const IGsize elementCount = attribute.pointer->GetNumberOfElements();
    const bool isPointData = attribute.attachmentType != IG_CELL;
    const IGsize cellCount = static_cast<IGsize>(nx - 1) *
                             static_cast<IGsize>(ny - 1) *
                             static_cast<IGsize>(nz - 1);

    // 越界保护：ghost 之类的单元数据只有 cellCount 个元素，
    // 若按 pointCount 读取会越界（Release 下直接崩溃）
    if (isPointData && elementCount != pointCount) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeData] attribute {} element count ({}) does "
                "not match point count ({}).",
                attributeIndex, elementCount, pointCount);
        return false;
    }
    if (!isPointData && elementCount != cellCount) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeData] cell attribute {} element count ({}) "
                "does not match cell count ({}).",
                attributeIndex, elementCount, cellCount);
        return false;
    }

    // 维度选择：优先用界面选中的分量；非法或多维未指定时用模长
    int component = attributeDimension;
    if (componentCount <= 1) {
        component = 0;
    } else if (component < -1 || component >= componentCount) {
        component = -1;
    }
    const bool useMagnitude = component < 0;

    std::vector<float> voxels(static_cast<size_t>(pointCount), 0.0f);
    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();

    if (isPointData) {
        for (IGsize i = 0; i < pointCount; ++i) {
            const float value = static_cast<float>(
                    attribute.pointer->GetElementValue(i, component));
            voxels[static_cast<size_t>(i)] = value;
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
        }
    } else {
        // 单元数据（如 vtkGhostType）：把每个单元的值散射到它的 8 个角点后取平均
        std::vector<unsigned int> counts(static_cast<size_t>(pointCount), 0u);
        const IGsize cellsPerSlice =
                static_cast<IGsize>(nx - 1) * static_cast<IGsize>(ny - 1);
        for (igIndex k = 0; k + 1 < nz; ++k) {
            for (igIndex j = 0; j + 1 < ny; ++j) {
                for (igIndex i = 0; i + 1 < nx; ++i) {
                    const IGsize cellIndex =
                            static_cast<IGsize>(i) +
                            static_cast<IGsize>(j) * (nx - 1) +
                            static_cast<IGsize>(k) * cellsPerSlice;
                    const float value = static_cast<float>(
                            attribute.pointer->GetElementValue(cellIndex,
                                                               component));
                    for (int corner = 0; corner < 8; ++corner) {
                        const IGsize pi = static_cast<IGsize>(i) + (corner & 1);
                        const IGsize pj =
                                static_cast<IGsize>(j) + ((corner >> 1) & 1);
                        const IGsize pk =
                                static_cast<IGsize>(k) + ((corner >> 2) & 1);
                        const IGsize pointIndex =
                                pi + pj * static_cast<IGsize>(nx) +
                                pk * static_cast<IGsize>(nx) *
                                        static_cast<IGsize>(ny);
                        voxels[static_cast<size_t>(pointIndex)] += value;
                        counts[static_cast<size_t>(pointIndex)] += 1u;
                    }
                    minValue = std::min(minValue, value);
                    maxValue = std::max(maxValue, value);
                }
            }
        }
        for (size_t i = 0; i < voxels.size(); ++i) {
            if (counts[i] > 0u) {
                voxels[i] /= static_cast<float>(counts[i]);
            }
        }
    }

    const float valueRange = maxValue - minValue;
    if (valueRange > 0.0f) {
        for (auto& value : voxels) { value = (value - minValue) / valueRange; }
    } else {
        std::fill(voxels.begin(), voxels.end(), 0.0f);
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeData] degenerate scalar range ({}), "
                "volume data will be uniform.",
                minValue);
    }

    // 注意：glTextureStorage* 只能指定一次（纹理变为 immutable），
    // 重建体数据时再次调用会得到 INVALID_OPERATION(0x502)，
    // 因此必须先销毁再创建纹理对象。
    m_VolumeTexture3d->Destroy();
    m_VolumeTexture3d->Create();
    m_VolumeTexture3d->Storage(1, GL_R16F, nx, ny, nz);
    m_VolumeTexture3d->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_VolumeTexture3d->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_VolumeTexture3d->Parameteri(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    m_VolumeTexture3d->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_VolumeTexture3d->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_VolumeTexture3d->SubImage(0, 0, 0, 0, nx, ny, nz, GL_RED, GL_FLOAT,
                                voxels.data());
    m_VolumeTexture3d->Release();

    // ---- 体数据掩膜（可选） ----
    // 兼容重采样结果的两种来源：
    //   * 点数组 "vtkValidPointMask"：1 = 落在原网格内（有效）
    //   * 单元数组 "vtkGhostType"   ：0 = 有效，非 0 = ghost（车体内部 / 域外）
    // 掩膜只做“有/无”二值（R32F），shader 里 alpha *= mask：
    // 无效体素不贡献颜色，于是车体内部会被挖成空腔，轮廓因此可辨。
    {
        std::vector<unsigned char> mask(static_cast<size_t>(pointCount), 0);
        bool hasMask = false;
        std::string source;

        // 数组名大小写不敏感比较：读取器会把 FIELD 数组名整体小写化
        // （"vtkvalidpointmask" / "vtkghosttype"），精确比较必然漏掉
        auto nameIs = [](const std::string& name, const char* expected) {
            size_t i = 0;
            for (; i < name.size() && expected[i] != '\0'; ++i) {
                if (std::tolower(static_cast<unsigned char>(name[i])) !=
                    std::tolower(static_cast<unsigned char>(expected[i]))) {
                    return false;
                }
            }
            return i == name.size() && expected[i] == '\0';
        };

        auto attributeSet = structuredMesh->GetAttributeSet();
        auto allAttributes =
                attributeSet != nullptr ? attributeSet->GetAllAttributes()
                                        : nullptr;
        if (allAttributes != nullptr) {
            // 1) 优先用逐点的 vtkValidPointMask
            for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
                auto& attribute = allAttributes->GetElement(i);
                if (attribute.isDeleted || attribute.pointer == nullptr) {
                    continue;
                }
                if (!nameIs(attribute.pointer->GetName(), "vtkValidPointMask")) {
                    continue;
                }
                if (attribute.attachmentType != IG_POINT) { continue; }
                if (attribute.pointer->GetNumberOfElements() != pointCount) {
                    continue;
                }
                for (IGsize p = 0; p < pointCount; ++p) {
                    mask[static_cast<size_t>(p)] =
                            attribute.pointer->GetElementValue(p, 0) != 0.0
                                    ? 1u
                                    : 0u;
                }
                hasMask = true;
                source = attribute.pointer->GetName() + " (point, 1=valid)";
                break;
            }

            // 2) 否则用单元级 vtkGhostType 映射到点：
            //    任一相邻单元有效 ⇒ 该点有效（避免把有效区侵蚀一圈）
            if (!hasMask) {
                const IGsize cellsPerSlice =
                        static_cast<IGsize>(nx - 1) * static_cast<IGsize>(ny - 1);
                for (IGsize i = 0;
                     i < allAttributes->GetNumberOfElements() && !hasMask; ++i) {
                    auto& attribute = allAttributes->GetElement(i);
                    if (attribute.isDeleted || attribute.pointer == nullptr) {
                        continue;
                    }
                    if (!nameIs(attribute.pointer->GetName(), "vtkGhostType")) {
                        continue;
                    }
                    if (attribute.attachmentType != IG_CELL) { continue; }
                    if (attribute.pointer->GetNumberOfElements() != cellCount) {
                        continue;
                    }

                    for (igIndex k = 0; k + 1 < nz; ++k) {
                        for (igIndex j = 0; j + 1 < ny; ++j) {
                            for (igIndex ii = 0; ii + 1 < nx; ++ii) {
                                const IGsize cellIndex =
                                        static_cast<IGsize>(ii) +
                                        static_cast<IGsize>(j) *
                                                static_cast<IGsize>(nx - 1) +
                                        static_cast<IGsize>(k) * cellsPerSlice;
                                if (attribute.pointer->GetElementValue(
                                            cellIndex, 0) != 0.0) {
                                    continue; // ghost 单元
                                }
                                for (int corner = 0; corner < 8; ++corner) {
                                    const IGsize pi =
                                            static_cast<IGsize>(ii) + (corner & 1);
                                    const IGsize pj =
                                            static_cast<IGsize>(j) +
                                            ((corner >> 1) & 1);
                                    const IGsize pk =
                                            static_cast<IGsize>(k) +
                                            ((corner >> 2) & 1);
                                    mask[pi + pj * static_cast<IGsize>(nx) +
                                         pk * static_cast<IGsize>(nx) *
                                                 static_cast<IGsize>(ny)] = 1u;
                                }
                            }
                        }
                    }
                    hasMask = true;
                    source = attribute.pointer->GetName() + " (cell, 0=valid)";
                }
            }
        }

        m_VolumeMaskSource = hasMask ? source : std::string{};
        m_VolumeMaskAvailable = hasMask;
        m_VolumeMaskEnabled =
                hasMask && m_VolumeMaskMode != VolumeMaskMode::Off;

        // 无效体素占比：便于确认掩膜语义（个位数或 >99% 通常说明语义反了）
        if (hasMask) {
            IGsize validCount = 0;
            for (IGsize p = 0; p < pointCount; ++p) {
                if (mask[static_cast<size_t>(p)] != 0) { ++validCount; }
            }
            m_VolumeMaskValidRatio =
                    pointCount > 0 ? static_cast<float>(validCount) /
                                             static_cast<float>(pointCount)
                                   : 0.0f;
            const IGsize invalidCount = pointCount - validCount;
            const int invalidPercent = static_cast<int>(
                    (1.0f - m_VolumeMaskValidRatio) * 100.0f + 0.5f);
            IGAME_RENDERING_INFO(
                    "[Scene::BuildVolumeData] blanking mask: source={}, "
                    "invalid={} of {} voxels (invalid_pct={}), mode={} "
                    "(0=off,1=carve,2=shell), applied={}.",
                    m_VolumeMaskSource.c_str(), invalidCount, pointCount,
                    invalidPercent, static_cast<int>(m_VolumeMaskMode),
                    m_VolumeMaskEnabled ? "yes" : "no");
        } else {
            m_VolumeMaskValidRatio = 0.0f;
        }

        if (m_VolumeMaskEnabled && m_VolumeMaskTexture3d != nullptr) {
            // 掩膜纹理用 R32F（而不是 R8）：1 字节/纹素时每行宽度不是 4 的倍数，
            // 默认 GL_UNPACK_ALIGNMENT(=4) 会按行错位；4 字节/纹素与体数据纹理
            // 一致，天然 4 字节对齐，行为可控。
            std::vector<float> maskFloats(static_cast<size_t>(pointCount), 0.0f);
            for (IGsize p = 0; p < pointCount; ++p) {
                maskFloats[static_cast<size_t>(p)] =
                        mask[static_cast<size_t>(p)] != 0 ? 1.0f : 0.0f;
            }
            m_VolumeMaskTexture3d->Destroy();
            m_VolumeMaskTexture3d->Create();
            m_VolumeMaskTexture3d->Storage(1, GL_R32F, nx, ny, nz);
            m_VolumeMaskTexture3d->Parameteri(GL_TEXTURE_WRAP_S,
                                              GL_CLAMP_TO_EDGE);
            m_VolumeMaskTexture3d->Parameteri(GL_TEXTURE_WRAP_T,
                                              GL_CLAMP_TO_EDGE);
            m_VolumeMaskTexture3d->Parameteri(GL_TEXTURE_WRAP_R,
                                              GL_CLAMP_TO_EDGE);
            m_VolumeMaskTexture3d->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            m_VolumeMaskTexture3d->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            m_VolumeMaskTexture3d->SubImage(0, 0, 0, 0, nx, ny, nz, GL_RED,
                                            GL_FLOAT, maskFloats.data());
            m_VolumeMaskTexture3d->Release();

            // 一次性自检：分别挑一个 CPU 侧“有效/无效”的体素，回读掩膜纹理对应
            // 纹素并与 CPU 值比对。若 gpu 全为 0（或与 cpu 不符），说明纹理上传/采样
            // 链路有问题 —— 这正是“整场白雾 / 整场被挖空”的典型来源。
            if (!m_VolumeMaskReadbackDone) {
                m_VolumeMaskReadbackDone = true;
                GLsizei validIndex = -1;
                GLsizei invalidIndex = -1;
                for (IGsize p = 0; p < pointCount; ++p) {
                    if (mask[static_cast<size_t>(p)] != 0) {
                        if (validIndex < 0) {
                            validIndex = static_cast<GLsizei>(p);
                        }
                    } else if (invalidIndex < 0) {
                        invalidIndex = static_cast<GLsizei>(p);
                    }
                    // 两类各找到一个就可以停了（避免整段扫描拖慢构建）
                    if (validIndex >= 0 && invalidIndex >= 0) { break; }
                }
                const auto readTexel = [&](GLsizei index) -> float {
                    if (index < 0) { return -1.0f; }
                    const GLsizei x = index % nx;
                    const GLsizei y = (index / nx) % ny;
                    const GLsizei z = index / (nx * ny);
                    float value = -2.0f;
                    glGetTextureSubImage(m_VolumeMaskTexture3d->Handle(), 0, x, y,
                                         z, 1, 1, 1, GL_RED, GL_FLOAT, 4, &value);
                    return value;
                };
                const auto cpuAt = [&](GLsizei index) -> float {
                    return index < 0
                                   ? -1.0f
                                   : static_cast<float>(
                                             mask[static_cast<size_t>(index)] != 0
                                                     ? 1.0f
                                                     : 0.0f);
                };
                IGAME_RENDERING_INFO(
                        "[Scene::BuildVolumeData] mask readback: "
                        "validVoxel cpu={} gpu={} , invalidVoxel cpu={} gpu={} ",
                        cpuAt(validIndex), readTexel(validIndex),
                        cpuAt(invalidIndex), readTexel(invalidIndex));
            }
        }
    }

    // 曲线坐标网格检测：当前实现把“索引空间”当作纹理空间，只有当点坐标
    // 可分离（p(i,j,k) = p(i,0,0) + p(0,j,0) + p(0,0,k) - 2 * p(0,0,0)，
    // 即轴对齐的直线网格，允许沿轴非均匀间距）时才精确；曲线坐标网格会形变，
    // 需要先重采样到规则体。这里给出告警，避免静默出错。
    {
        const BoundingBox& box = structuredMesh->GetBoundingBox();
        const double dx = box.max[0] - box.min[0];
        const double dy = box.max[1] - box.min[1];
        const double dz = box.max[2] - box.min[2];
        const double tolerance =
                1e-3 * std::sqrt(dx * dx + dy * dy + dz * dz);

        auto samplePoint = [&](igIndex i, igIndex j, igIndex k) {
            const Point& p = structuredMesh->GetPoint(
                    static_cast<IGsize>(i) + static_cast<IGsize>(j) * nx +
                    static_cast<IGsize>(k) * nx * ny);
            return std::array<double, 3>{static_cast<double>(p[0]),
                                         static_cast<double>(p[1]),
                                         static_cast<double>(p[2])};
        };

        bool separable = true;
        const auto origin = samplePoint(0, 0, 0);
        for (int a = 1; a < 3 && separable; ++a) {
            for (int b = 1; b < 3 && separable; ++b) {
                for (int c = 1; c < 3 && separable; ++c) {
                    const igIndex i = static_cast<igIndex>(a) * nx / 3;
                    const igIndex j = static_cast<igIndex>(b) * ny / 3;
                    const igIndex k = static_cast<igIndex>(c) * nz / 3;

                    const auto pi = samplePoint(i, 0, 0);
                    const auto pj = samplePoint(0, j, 0);
                    const auto pk = samplePoint(0, 0, k);
                    const auto pijk = samplePoint(i, j, k);

                    for (int d = 0; d < 3; ++d) {
                        const double expected =
                                pi[d] + pj[d] + pk[d] - 2.0 * origin[d];
                        if (std::abs(pijk[d] - expected) > tolerance) {
                            separable = false;
                            break;
                        }
                    }
                }
            }
        }

        if (!separable) {
            IGAME_RENDERING_WARN(
                    "[Scene::BuildVolumeData] grid is not an axis-aligned "
                    "rectilinear grid; the volume will look distorted. "
                    "Resampling to a regular volume is required.");
        }
    }

    // 体数据的局部包围盒：轴对齐直线网格可直接用“缩放 + 平移”映射到纹理空间。
    // 世界空间的映射每帧用当前模型矩阵重建（见 VolumeRaycastPass），
    // 这样交互旋转/平移时体数据才会跟随网格。注意模型矩阵的变化不会修改
    // 数据对象的 MTime，因此不能只在构建时计算一次。
    const BoundingBox& localBox = structuredMesh->GetBoundingBox();
    const igm::vec3 localMin{static_cast<float>(localBox.min[0]),
                             static_cast<float>(localBox.min[1]),
                             static_cast<float>(localBox.min[2])};
    const igm::vec3 localMax{static_cast<float>(localBox.max[0]),
                             static_cast<float>(localBox.max[1]),
                             static_cast<float>(localBox.max[2])};
    const igm::vec3 localExtent = localMax - localMin;
    if (localExtent.x <= 0.0f || localExtent.y <= 0.0f ||
        localExtent.z <= 0.0f) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeData] invalid volume bounding box: "
                "({}, {}, {}).",
                localExtent.x, localExtent.y, localExtent.z);
        return false;
    }

    // 局部索引空间 -> 体纹理空间([0,1]^3)：t = (p - localMin) / localExtent
    m_VolumeLocalMin = localMin;
    m_VolumeLocalExtent = localExtent;
    m_VolumeLocalToTexture = igm::mat4{1.0f};
    m_VolumeLocalToTexture[0][0] = 1.0f / localExtent.x;
    m_VolumeLocalToTexture[1][1] = 1.0f / localExtent.y;
    m_VolumeLocalToTexture[2][2] = 1.0f / localExtent.z;
    m_VolumeLocalToTexture[3][0] = -localMin.x / localExtent.x;
    m_VolumeLocalToTexture[3][1] = -localMin.y / localExtent.y;
    m_VolumeLocalToTexture[3][2] = -localMin.z / localExtent.z;

    m_VolumeDimension[0] = static_cast<int>(nx);
    m_VolumeDimension[1] = static_cast<int>(ny);
    m_VolumeDimension[2] = static_cast<int>(nz);
    // 记录数据来源与属性，供渲染循环做失效判定：属性切换不会修改 DataObject
    // 的 MTime，所以属性索引/分量要显式记住；色标变化则用 mapper 的 MTime 判断
    m_VolumeSourceObject = dataObject;
    m_VolumeSourceMTime = dataObject->GetMTime().GetMTime();
    m_VolumeAttributeIndex = attributeIndex;
    m_VolumeAttributeDimension = attributeDimension;
    // 色标本身也会变（换色带/改范围），它的 MTime 需要参与失效判定
    {
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        auto colorMapper =
                drawObject != nullptr ? drawObject->GetColorMapper() : nullptr;
        m_VolumeMapperMTime =
                colorMapper != nullptr ? colorMapper->GetMTime().GetMTime() : 0u;
        this->BuildVolumeTransferFunction(colorMapper, minValue, maxValue);
    }
    m_VolumeRaycastDiagnosed = false;
    m_VolumeRaycastReadbackDone = false;
    m_VolumeRaycastGpuDiagDone = false;
    m_VolumeMaskReadbackDone = false;

        IGAME_RENDERING_INFO(
            "[Scene::BuildVolumeData] attribute {} (dimension {}), scalar range "
            "[{}, {}], mask {}, local box [{}, {}, {}] - [{}, {}, {}].",
            attributeIndex, componentCount, minValue, maxValue,
            m_VolumeMaskEnabled ? "yes" : "no", localMin.x, localMin.y,
            localMin.z, localMax.x, localMax.y, localMax.z);

    return true;
#else
    (void) dataObject;
    (void) attributeIndex;
    (void) attributeDimension;
    IGAME_RENDERING_WARN(
            "[Scene::BuildVolumeData] volume ray casting requires the "
            "OpenGL 4.6 backend.");
    return false;
#endif
}

bool Scene::BuildVolumeMaskValues(SmartPointer<DataObject> mesh, igIndex nx,
                                  igIndex ny, igIndex nz,
                                  std::vector<float>& maskValues,
                                  std::string& source, float& validRatio) {
    maskValues.clear();
    source.clear();
    validRatio = 0.0f;
    auto structuredMesh = DynamicCast<StructuredMesh>(mesh);
    if (structuredMesh == nullptr) { return false; }

    const IGsize pointCount = static_cast<IGsize>(nx) *
                              static_cast<IGsize>(ny) *
                              static_cast<IGsize>(nz);
    const IGsize cellCount = static_cast<IGsize>(nx - 1) *
                             static_cast<IGsize>(ny - 1) *
                             static_cast<IGsize>(nz - 1);
    auto attributeSet = structuredMesh->GetAttributeSet();
    auto allAttributes =
            attributeSet != nullptr ? attributeSet->GetAllAttributes()
                                    : nullptr;
    if (allAttributes == nullptr) { return false; }

    // 数组名大小写不敏感（读取器会把 FIELD 名字整体小写化）
    auto nameIs = [](const std::string& name, const char* expected) {
        size_t i = 0;
        for (; i < name.size() && expected[i] != '\0'; ++i) {
            if (std::tolower(static_cast<unsigned char>(name[i])) !=
                std::tolower(static_cast<unsigned char>(expected[i]))) {
                return false;
            }
        }
        return i == name.size() && expected[i] == '\0';
    };

    std::vector<unsigned char> mask(static_cast<size_t>(pointCount), 0);
    bool hasMask = false;

    // 1) 点级 vtkValidPointMask：非 0 = 有效
    for (IGsize i = 0; i < allAttributes->GetNumberOfElements() && !hasMask;
         ++i) {
        auto& attribute = allAttributes->GetElement(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) { continue; }
        if (!nameIs(attribute.pointer->GetName(), "vtkValidPointMask")) {
            continue;
        }
        if (attribute.attachmentType != IG_POINT) { continue; }
        if (attribute.pointer->GetNumberOfElements() != pointCount) { continue; }
        for (IGsize p = 0; p < pointCount; ++p) {
            mask[static_cast<size_t>(p)] =
                    attribute.pointer->GetElementValue(p, 0) != 0.0 ? 1u : 0u;
        }
        hasMask = true;
        source = attribute.pointer->GetName() + " (point, 1=valid)";
    }

    // 2) 单元级 vtkGhostType：0 = 有效，映射到 8 个角点
    if (!hasMask) {
        const IGsize cellsPerSlice =
                static_cast<IGsize>(nx - 1) * static_cast<IGsize>(ny - 1);
        for (IGsize i = 0; i < allAttributes->GetNumberOfElements() && !hasMask;
             ++i) {
            auto& attribute = allAttributes->GetElement(i);
            if (attribute.isDeleted || attribute.pointer == nullptr) {
                continue;
            }
            if (!nameIs(attribute.pointer->GetName(), "vtkGhostType")) {
                continue;
            }
            if (attribute.attachmentType != IG_CELL) { continue; }
            if (attribute.pointer->GetNumberOfElements() != cellCount) {
                continue;
            }
            for (igIndex k = 0; k + 1 < nz; ++k) {
                for (igIndex j = 0; j + 1 < ny; ++j) {
                    for (igIndex ii = 0; ii + 1 < nx; ++ii) {
                        const IGsize cellIndex =
                                static_cast<IGsize>(ii) +
                                static_cast<IGsize>(j) *
                                        static_cast<IGsize>(nx - 1) +
                                static_cast<IGsize>(k) * cellsPerSlice;
                        if (attribute.pointer->GetElementValue(cellIndex, 0) !=
                            0.0) {
                            continue; // ghost 单元
                        }
                        for (int corner = 0; corner < 8; ++corner) {
                            const IGsize pi =
                                    static_cast<IGsize>(ii) + (corner & 1);
                            const IGsize pj =
                                    static_cast<IGsize>(j) +
                                    ((corner >> 1) & 1);
                            const IGsize pk =
                                    static_cast<IGsize>(k) +
                                    ((corner >> 2) & 1);
                            mask[pi + pj * static_cast<IGsize>(nx) +
                                 pk * static_cast<IGsize>(nx) *
                                         static_cast<IGsize>(ny)] = 1u;
                        }
                    }
                }
            }
            hasMask = true;
            source = attribute.pointer->GetName() + " (cell, 0=valid)";
        }
    }

    if (!hasMask) { return false; }

    maskValues.assign(static_cast<size_t>(pointCount), 0.0f);
    IGsize validCount = 0;
    for (IGsize p = 0; p < pointCount; ++p) {
        const bool valid = mask[static_cast<size_t>(p)] != 0;
        maskValues[static_cast<size_t>(p)] = valid ? 1.0f : 0.0f;
        if (valid) { ++validCount; }
    }
    validRatio = pointCount > 0 ? static_cast<float>(validCount) /
                                          static_cast<float>(pointCount)
                                : 0.0f;
    return true;
}

bool Scene::BuildVolumeLayer2(SmartPointer<DataObject> dataObject,
                              int attributeIndex, int attributeDimension) {
#ifdef IGAME_OPENGL_VERSION_460
    m_VolumeLayer2Enabled = false;
    if (dataObject == nullptr || m_VolumeTexture3d2 == nullptr ||
        m_VolumeTransferFunction2 == nullptr) {
        return false;
    }

    // 体素构建逻辑与 BuildVolumeData 相同（结构化网格 + 点/单元标量 →
    // 归一化到 [0,1] 的 R16F 体纹理）。这里单独立一份，避免改动已稳定的单层路径。
    auto structuredMesh = DynamicCast<StructuredMesh>(dataObject);
    if (structuredMesh == nullptr) { return false; }

    igIndex* dimension = structuredMesh->GetDimensionSize();
    const igIndex nx = dimension[0], ny = dimension[1], nz = dimension[2];
    if (nx <= 1 || ny <= 1 || nz <= 1) { return false; }

    // 第二层同样从自己的 DataObject 读取参数。
    const auto& volumeSettings = dataObject->GetVolumeRenderingSettings();
    m_VolumeLayer2UserEnabled = volumeSettings.enabled;
    m_VolumeOpacityScale2 =
            std::clamp(volumeSettings.opacityScale, 0.0f, 1.0f);
    m_VolumeMaskMode2 = static_cast<VolumeMaskMode>(
            std::clamp(volumeSettings.maskMode, 0, 2));
    m_VolumeShellOpacity2 =
            std::clamp(volumeSettings.shellOpacity, 0.0f, 1.0f);

    const IGsize pointCount = static_cast<IGsize>(nx) *
                              static_cast<IGsize>(ny) *
                              static_cast<IGsize>(nz);
    if (structuredMesh->GetNumberOfPoints() != pointCount) { return false; }

    auto attributeSet = structuredMesh->GetAttributeSet();
    if (attributeSet == nullptr || attributeIndex < 0 ||
        static_cast<size_t>(attributeIndex) >=
                attributeSet->GetNumberOfAttributes()) {
        return false;
    }
    auto& attribute =
            attributeSet->GetAttribute(static_cast<IGsize>(attributeIndex));
    if (attribute.pointer == nullptr) { return false; }

    const int componentCount = attribute.pointer->GetDimension();
    const IGsize elementCount = attribute.pointer->GetNumberOfElements();
    const bool isPointData = attribute.attachmentType != IG_CELL;
    const IGsize cellCount = static_cast<IGsize>(nx - 1) *
                             static_cast<IGsize>(ny - 1) *
                             static_cast<IGsize>(nz - 1);
    if ((isPointData && elementCount != pointCount) ||
        (!isPointData && elementCount != cellCount)) {
        return false;
    }

    int component = attributeDimension;
    if (componentCount <= 1) {
        component = 0;
    } else if (component < -1 || component >= componentCount) {
        component = -1;
    }

    std::vector<float> voxels(static_cast<size_t>(pointCount), 0.0f);
    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();
    if (isPointData) {
        for (IGsize i = 0; i < pointCount; ++i) {
            const float value = static_cast<float>(
                    attribute.pointer->GetElementValue(i, component));
            voxels[static_cast<size_t>(i)] = value;
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
        }
    } else {
        std::vector<unsigned int> counts(static_cast<size_t>(pointCount), 0u);
        const IGsize cellsPerSlice =
                static_cast<IGsize>(nx - 1) * static_cast<IGsize>(ny - 1);
        for (igIndex k = 0; k + 1 < nz; ++k) {
            for (igIndex j = 0; j + 1 < ny; ++j) {
                for (igIndex ii = 0; ii + 1 < nx; ++ii) {
                    const IGsize cellIndex =
                            static_cast<IGsize>(ii) +
                            static_cast<IGsize>(j) *
                                    static_cast<IGsize>(nx - 1) +
                            static_cast<IGsize>(k) * cellsPerSlice;
                    const float value = static_cast<float>(
                            attribute.pointer->GetElementValue(cellIndex,
                                                               component));
                    for (int corner = 0; corner < 8; ++corner) {
                        const IGsize pi =
                                static_cast<IGsize>(ii) + (corner & 1);
                        const IGsize pj =
                                static_cast<IGsize>(j) +
                                ((corner >> 1) & 1);
                        const IGsize pk =
                                static_cast<IGsize>(k) +
                                ((corner >> 2) & 1);
                        const IGsize pointIndex =
                                pi + pj * static_cast<IGsize>(nx) +
                                pk * static_cast<IGsize>(nx) *
                                        static_cast<IGsize>(ny);
                        voxels[static_cast<size_t>(pointIndex)] += value;
                        counts[static_cast<size_t>(pointIndex)] += 1u;
                    }
                    minValue = std::min(minValue, value);
                    maxValue = std::max(maxValue, value);
                }
            }
        }
        for (size_t i = 0; i < voxels.size(); ++i) {
            if (counts[i] > 0u) {
                voxels[i] /= static_cast<float>(counts[i]);
            }
        }
    }

    const float valueRange = maxValue - minValue;
    if (!(valueRange > 0.0f)) { return false; }
    for (auto& value : voxels) { value = (value - minValue) / valueRange; }

    const BoundingBox& localBox = structuredMesh->GetBoundingBox();
    const igm::vec3 localMin{static_cast<float>(localBox.min[0]),
                             static_cast<float>(localBox.min[1]),
                             static_cast<float>(localBox.min[2])};
    const igm::vec3 localMax{static_cast<float>(localBox.max[0]),
                             static_cast<float>(localBox.max[1]),
                             static_cast<float>(localBox.max[2])};
    const igm::vec3 localExtent = localMax - localMin;
    if (localExtent.x <= 0.0f || localExtent.y <= 0.0f ||
        localExtent.z <= 0.0f) {
        return false;
    }

    m_VolumeTexture3d2->Destroy();
    m_VolumeTexture3d2->Create();
    m_VolumeTexture3d2->Storage(1, GL_R16F, nx, ny, nz);
    m_VolumeTexture3d2->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_VolumeTexture3d2->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_VolumeTexture3d2->Parameteri(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    m_VolumeTexture3d2->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_VolumeTexture3d2->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_VolumeTexture3d2->SubImage(0, 0, 0, 0, nx, ny, nz, GL_RED, GL_FLOAT,
                                 voxels.data());
    m_VolumeTexture3d2->Release();

    // ---- 第二层掩膜（与第一层同样的两种来源）----
    m_VolumeMaskAvailable2 = false;
    m_VolumeMaskEnabled2 = false;
    m_VolumeMaskSource2.clear();
    m_VolumeMaskValidRatio2 = 0.0f;
    {
        std::vector<float> maskValues;
        std::string maskSource;
        float maskValidRatio = 0.0f;
        if (this->BuildVolumeMaskValues(structuredMesh, nx, ny, nz, maskValues,
                                        maskSource, maskValidRatio)) {
            m_VolumeMaskAvailable2 = true;
            m_VolumeMaskSource2 = maskSource;
            m_VolumeMaskValidRatio2 = maskValidRatio;
            // 第二层用自己的掩膜模式
            m_VolumeMaskEnabled2 =
                    m_VolumeMaskMode2 != VolumeMaskMode::Off;
            if (m_VolumeMaskTexture3d2 != nullptr) {
                m_VolumeMaskTexture3d2->Destroy();
                m_VolumeMaskTexture3d2->Create();
                m_VolumeMaskTexture3d2->Storage(1, GL_R32F, nx, ny, nz);
                m_VolumeMaskTexture3d2->Parameteri(GL_TEXTURE_WRAP_S,
                                                   GL_CLAMP_TO_EDGE);
                m_VolumeMaskTexture3d2->Parameteri(GL_TEXTURE_WRAP_T,
                                                   GL_CLAMP_TO_EDGE);
                m_VolumeMaskTexture3d2->Parameteri(GL_TEXTURE_WRAP_R,
                                                   GL_CLAMP_TO_EDGE);
                m_VolumeMaskTexture3d2->Parameteri(GL_TEXTURE_MIN_FILTER,
                                                   GL_LINEAR);
                m_VolumeMaskTexture3d2->Parameteri(GL_TEXTURE_MAG_FILTER,
                                                   GL_LINEAR);
                m_VolumeMaskTexture3d2->SubImage(0, 0, 0, 0, nx, ny, nz,
                                                 GL_RED, GL_FLOAT,
                                                 maskValues.data());
                m_VolumeMaskTexture3d2->Release();
            }
            const int invalidPercent = static_cast<int>(
                    (1.0f - maskValidRatio) * 100.0f + 0.5f);
            IGAME_RENDERING_INFO(
                    "[Scene::BuildVolumeData2] layer2 mask: source={}, "
                    "invalid_pct={}, applied={}.",
                    m_VolumeMaskSource2.c_str(), invalidPercent,
                    m_VolumeMaskEnabled2 ? "yes" : "no");
        }
    }

    m_VolumeLocalMin2 = localMin;
    m_VolumeLocalExtent2 = localExtent;
    m_VolumeLocalToTexture2 = igm::mat4{1.0f};
    m_VolumeLocalToTexture2[0][0] = 1.0f / localExtent.x;
    m_VolumeLocalToTexture2[1][1] = 1.0f / localExtent.y;
    m_VolumeLocalToTexture2[2][2] = 1.0f / localExtent.z;
    m_VolumeLocalToTexture2[3][0] = -localMin.x / localExtent.x;
    m_VolumeLocalToTexture2[3][1] = -localMin.y / localExtent.y;
    m_VolumeLocalToTexture2[3][2] = -localMin.z / localExtent.z;
    m_VolumeDimension2[0] = static_cast<int>(nx);
    m_VolumeDimension2[1] = static_cast<int>(ny);
    m_VolumeDimension2[2] = static_cast<int>(nz);

    auto drawObject = DynamicCast<DrawObject>(dataObject);
    auto colorMapper =
            drawObject != nullptr ? drawObject->GetColorMapper() : nullptr;
    this->BuildVolumeTransferFunctionInto(m_VolumeTransferFunction2,
                                          colorMapper, minValue, maxValue);

    m_VolumeSourceObject2 = dataObject;
    m_VolumeSourceMTime2 = dataObject->GetMTime().GetMTime();
    m_VolumeAttributeIndex2 = attributeIndex;
    m_VolumeAttributeDimension2 = attributeDimension;
    m_VolumeMapperMTime2 =
            colorMapper != nullptr ? colorMapper->GetMTime().GetMTime() : 0u;
    m_VolumeLayer2Enabled = true;

    IGAME_RENDERING_INFO(
            "[Scene::BuildVolumeData2] layer2: attribute {} (dimension {}), "
            "scalar range [{}, {}], dims {}x{}x{}, local box [{}, {}, {}] - "
            "[{}, {}, {}].",
            attributeIndex, componentCount, minValue, maxValue, nx, ny, nz,
            localMin.x, localMin.y, localMin.z, localMax.x, localMax.y,
            localMax.z);
    return true;
#else
    (void) dataObject;
    (void) attributeIndex;
    (void) attributeDimension;
    return false;
#endif
}

namespace {
// 挑选适合作为体数据的标量场：优先使用绘制对象当前激活的属性，
// 否则取第一个“挂在点上、维度为 1”的属性（跳过法线、矢量等）。
int PickVolumeScalarAttribute(AttributeSet* attributeSet, int preferred) {
    if (attributeSet == nullptr) { return -1; }

    const size_t attributeCount = attributeSet->GetNumberOfAttributes();
    if (preferred >= 0 && static_cast<size_t>(preferred) < attributeCount) {
        auto& attribute = attributeSet->GetAttribute(static_cast<IGsize>(preferred));
        if (attribute.pointer != nullptr) { return preferred; }
    }

    for (size_t i = 0; i < attributeCount; ++i) {
        auto& attribute = attributeSet->GetAttribute(static_cast<IGsize>(i));
        if (attribute.pointer == nullptr) { continue; }
        if (attribute.attachmentType != IG_POINT) { continue; }
        if (attribute.pointer->GetDimension() != 1) { continue; }
        return static_cast<int>(i);
    }
    return -1;
}

// 与 VolumeRaymarch.comp 中一致的光线-盒求交（仅用于自检日志）。
// 说明：dT 不归一化，因此返回的 t 是世界空间光线参数。
bool RayBoxIntersectTexture(const igm::vec3& origin, const igm::vec3& dir,
                            const igm::mat4& worldToTexture, float& tNear,
                            float& tFar) {
    const igm::vec3 oT = (worldToTexture * igm::vec4{origin, 1.0f}).xyz();
    const igm::vec3 dT = (worldToTexture * igm::vec4{dir, 0.0f}).xyz();
    const float o[3] = {oT.x, oT.y, oT.z};
    const float d[3] = {dT.x, dT.y, dT.z};

    tNear = 0.0f;
    tFar = 1.0e30f;
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(d[axis]) < 1.0e-12f) {
            if (o[axis] < 0.0f || o[axis] > 1.0f) { return false; }
            continue;
        }
        float t0 = (0.0f - o[axis]) / d[axis];
        float t1 = (1.0f - o[axis]) / d[axis];
        if (t0 > t1) { std::swap(t0, t1); }
        tNear = std::max(tNear, t0);
        tFar = std::min(tFar, t1);
    }
    return tFar > tNear;
}
} // namespace

bool Scene::HasMultiBlockData() {
    if (m_ModelPool == nullptr) { return false; }

    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        if (model == nullptr || !model->GetVisibility()) { continue; }

        auto dataObject = model->GetDataObject();
        if (dataObject == nullptr || !dataObject->IsDrawable()) { continue; }

        // 带子数据对象 = 多块 / 动画（播放时子对象会被 Clear/Add 重建）
        if (dataObject->HasSubDataObject()) { return true; }
    }
    return false;
}

SmartPointer<DrawObject> Scene::FindVolumeSource() {
    if (m_ModelPool == nullptr) { return nullptr; }

    SmartPointer<DrawObject> fallback;
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        if (model == nullptr || !model->GetVisibility()) { continue; }

        auto dataObject = model->GetDataObject();
        if (dataObject == nullptr || !dataObject->IsDrawable()) { continue; }

        // 结构化网格优先（可用 compute 光线步进）
        if (DynamicCast<StructuredMesh>(dataObject) != nullptr) {
            return DynamicCast<DrawObject>(dataObject);
        }
        // 其它网格（非结构化/体网格）作为候选，走旧的片元链表实现
        if (fallback == nullptr) {
            auto pointSet = DynamicCast<PointSet>(dataObject);
            if (pointSet != nullptr && pointSet->GetNumberOfPoints() > 0) {
                fallback = DynamicCast<DrawObject>(dataObject);
            }
        }
    }
    return fallback;
}

SmartPointer<DrawObject> Scene::FindVolumeSource2() {
    if (m_ModelPool == nullptr) { return nullptr; }

    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        if (model == nullptr || !model->GetVisibility()) { continue; }

        auto dataObject = model->GetDataObject();
        if (dataObject == nullptr || !dataObject->IsDrawable()) { continue; }
        if (DynamicCast<StructuredMesh>(dataObject) == nullptr) { continue; }
        // 跳过第一层的数据对象
        if (dataObject == m_VolumeSourceObject) { continue; }
        return DynamicCast<DrawObject>(dataObject);
    }
    return nullptr;
}

bool Scene::BuildVolumeDataFromCurrentModel() {
    auto drawObject = this->FindVolumeSource();
    if (drawObject == nullptr) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeDataFromCurrentModel] no mesh found in "
                "current scene.");
        return false;
    }

    // 本函数只服务 compute 光线步进路径，因此只接受结构化网格；
    // 非结构化网格由 SetVolumeRendering 分派到旧的片元链表实现
    if (DynamicCast<StructuredMesh>(drawObject) == nullptr) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeDataFromCurrentModel] current mesh is not "
                "a structured mesh; ray marching is unavailable.");
        return false;
    }

    const int attributeIndex = PickVolumeScalarAttribute(
            drawObject->GetAttributeSet(), drawObject->GetAttributeIndex());
    if (attributeIndex < 0) {
        IGAME_RENDERING_WARN(
                "[Scene::BuildVolumeDataFromCurrentModel] no usable scalar "
                "attribute found on the structured mesh.");
        return false;
    }

    return this->BuildVolumeData(drawObject, attributeIndex,
                                 drawObject->GetAttributeDimension());
}

bool Scene::IsVolumeRaycastingAvailable() {
#ifdef IGAME_OPENGL_VERSION_460
    if (m_VolumeRaycastingSupportChecked) { return m_VolumeRaycastingSupported; }
    m_VolumeRaycastingSupportChecked = true;

    // compute shader 需要 OpenGL 4.3+，这里运行时再确认一次（驱动/上下文级别）
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    m_VolumeRaycastingSupported =
            (major > 4) || (major == 4 && minor >= 3);
    return m_VolumeRaycastingSupported;
#else
    // 3.3 / GLES2 后端在编译期就没有 compute 相关实现
    m_VolumeRaycastingSupportChecked = true;
    m_VolumeRaycastingSupported = false;
    return false;
#endif
}

Scene::VolumeRaycastingStatus
Scene::CheckVolumeRaycastingSupport(std::string* message) {
    auto fail = [&](VolumeRaycastingStatus status, const char* text) {
        if (message != nullptr) { *message = text; }
        return status;
    };

    // 只有“后端支持 compute + 存在结构化网格 + 有可用标量属性”时才返回 Ok；
    // 调用方据此决定是走光线步进、回退到旧的片元链表实现，还是提示用户
    if (!this->IsVolumeRaycastingAvailable()) {
        return fail(VolumeRaycastingStatus::UnsupportedBackend,
                    "当前图形后端不支持体绘制：需要 OpenGL 4.6（compute shader）。");
    }

    // 多块/动画数据（含子数据对象）不支持 compute 光线步进
    if (this->HasMultiBlockData()) {
        return fail(VolumeRaycastingStatus::MultiBlockUnsupported,
                    "当前数据是多块/动画网格（含子数据对象），暂不支持 compute "
                    "光线步进体绘制；请使用旧的片元链表实现。");
    }

    auto drawObject = this->FindVolumeSource();
    if (drawObject == nullptr) {
        return fail(VolumeRaycastingStatus::NoStructuredMesh,
                    "场景中没有可用于体绘制的网格数据。");
    }

    auto structuredMesh = DynamicCast<StructuredMesh>(drawObject);
    if (structuredMesh == nullptr) {
        return fail(VolumeRaycastingStatus::NoStructuredMesh,
                    "当前数据不是结构化网格（STRUCTURED_GRID），"
                    "无法使用 compute 光线步进；可改用旧的片元链表实现，"
                    "或先用 ResampleToImageFilter 转换为结构化网格。");
    }

    igIndex* dimension = structuredMesh->GetDimensionSize();
    const IGsize pointCount = static_cast<IGsize>(dimension[0]) *
                              static_cast<IGsize>(dimension[1]) *
                              static_cast<IGsize>(dimension[2]);
    if (dimension[0] <= 1 || dimension[1] <= 1 || dimension[2] <= 1 ||
        structuredMesh->GetNumberOfPoints() != pointCount) {
        return fail(VolumeRaycastingStatus::InvalidAttribute,
                    "体绘制需要三维结构化网格（各方向维度大于 1，且点数与维度一致）。");
    }

    const int attributeIndex = PickVolumeScalarAttribute(
            drawObject->GetAttributeSet(), drawObject->GetAttributeIndex());
    if (attributeIndex < 0) {
        return fail(VolumeRaycastingStatus::NoScalarAttribute,
                    "未找到可用的标量属性：请先选择一个标量场（或矢量场的模长/分量）。");
    }

    // 校验属性规模：单元数据（如 vtkGhostType）的元素数必须等于单元数，
    // 点数据必须等于点数，否则会越界读取
    auto& attribute = drawObject->GetAttributeSet()->GetAttribute(
            static_cast<IGsize>(attributeIndex));
    if (attribute.pointer == nullptr) {
        return fail(VolumeRaycastingStatus::NoScalarAttribute,
                    "所选属性为空，无法用于体绘制。");
    }

    const IGsize elementCount = attribute.pointer->GetNumberOfElements();
    const IGsize cellCount = static_cast<IGsize>(dimension[0] - 1) *
                             static_cast<IGsize>(dimension[1] - 1) *
                             static_cast<IGsize>(dimension[2] - 1);
    const bool isCellData = attribute.attachmentType == IG_CELL;
    if ((!isCellData && elementCount != pointCount) ||
        (isCellData && elementCount != cellCount)) {
        return fail(VolumeRaycastingStatus::InvalidAttribute,
                    "所选属性的数据规模与网格不匹配，无法用于体绘制。");
    }

    if (message != nullptr) { message->clear(); }
    return VolumeRaycastingStatus::Ok;
}

Scene::VolumeRaycastingStatus
Scene::SetVolumeRaycasting(bool toggled, std::string* message) {
    if (!toggled) {
        m_EnableVolumeRaycasting = false;
        this->Update();
        return VolumeRaycastingStatus::Ok;
    }

    // 本函数只负责 compute 实现：不满足条件时返回状态，
    // 由调用方（SetVolumeRendering 的分派逻辑）决定回退到旧的“片元链表”
    // 实现还是提示用户。
    const auto status = this->CheckVolumeRaycastingSupport(message);
    if (status != VolumeRaycastingStatus::Ok) {
            m_EnableVolumeRaycasting = false;
            IGAME_RENDERING_WARN(
                    "[Scene::SetVolumeRaycasting] volume ray casting is not "
                    "available (status={}); see UI message for details.",
                    static_cast<int>(status));
            return status;
    }

    m_EnableVolumeRendering = false;
    m_EnableVolumeRaycasting = true;
    m_VolumeDataDirty = true;
    IGAME_RENDERING_INFO(
            "[Scene::SetVolumeRaycasting] compute volume ray casting "
            "requested, volume data will be built on next frame.");
    this->Update();
    return VolumeRaycastingStatus::Ok;
}

bool Scene::ConsumeVolumeRaycastFailure(std::string* message) {
    if (m_VolumeRaycastFailureMessage.empty()) { return false; }
    if (message != nullptr) { *message = m_VolumeRaycastFailureMessage; }
    m_VolumeRaycastFailureMessage.clear();
    return true;
}

void Scene::SetVolumeRaycastingQuality(int numSteps, float opacityScale) {
    m_VolumeStepCount = std::max(numSteps, 1);
    m_VolumeOpacityScale = std::clamp(opacityScale, 0.0f, 1.0f);
    if (auto layer0 = this->GetVolumeLayerDataObject(0)) {
        layer0->GetVolumeRenderingSettings().opacityScale =
                m_VolumeOpacityScale;
    }
    this->Update();
}

void Scene::SetVolumeRaycastMaskEnabled(bool enabled) {
    this->SetVolumeRaycastMaskMode(enabled ? VolumeMaskMode::Carve
                                           : VolumeMaskMode::Off);
}

void Scene::SetVolumeRaycastMaskMode(VolumeMaskMode mode) {
    const int modeValue = static_cast<int>(mode);
    if (modeValue < 0 || modeValue > 2) { mode = VolumeMaskMode::Off; }
    if (auto layer0 = this->GetVolumeLayerDataObject(0)) {
        layer0->GetVolumeRenderingSettings().maskMode =
                static_cast<int>(mode);
    }
    if (m_VolumeMaskMode == mode) { return; }

    const bool needRebuild = (m_VolumeMaskMode == VolumeMaskMode::Off) !=
                             (mode == VolumeMaskMode::Off);
    m_VolumeMaskMode = mode;
    // 开/关掩膜需要在构建体数据时生成掩膜纹理，因此标脏重建；只在
    // Carve/Shell 之间切换时只影响 shader 的合成方式，不需要重建。
    if (needRebuild) { m_VolumeDataDirty = true; }
    // 重建时会打印掩膜来源与无效体素占比，便于确认模式是否真的起作用。
    this->Update();
}

void Scene::SetVolumeRaycastShellOpacity(float opacity) {
    const float clamped = std::clamp(opacity, 0.0f, 1.0f);
    if (auto layer0 = this->GetVolumeLayerDataObject(0)) {
        layer0->GetVolumeRenderingSettings().shellOpacity = clamped;
    }
    if (m_VolumeShellOpacity == clamped) { return; }
    m_VolumeShellOpacity = clamped;
    // 只改 shader uniform，不需要重建体数据
    this->Update();
}

void Scene::SetVolumeLayerEnabled(int layer, bool enabled) {
    auto dataObject = this->GetVolumeLayerDataObject(layer);
    if (dataObject == nullptr) { return; }
    dataObject->GetVolumeRenderingSettings().enabled = enabled;
    if (layer == 0) {
        m_VolumeLayer0UserEnabled = enabled;
    } else if (layer == 1) {
        m_VolumeLayer2UserEnabled = enabled;
    } else {
        return;
    }
    this->Update();
}

bool Scene::GetVolumeLayerEnabled(int layer) const {
    if (layer == 0) { return m_VolumeLayer0UserEnabled; }
    if (layer == 1) { return m_VolumeLayer2UserEnabled; }
    return false;
}

void Scene::SetVolumeLayerOpacityScale(int layer, float scale) {
    const float clamped = std::clamp(scale, 0.0f, 1.0f);
    auto dataObject = this->GetVolumeLayerDataObject(layer);
    if (dataObject == nullptr) { return; }
    dataObject->GetVolumeRenderingSettings().opacityScale = clamped;
    if (layer == 0) {
        m_VolumeOpacityScale = clamped;
    } else if (layer == 1) {
        m_VolumeOpacityScale2 = clamped;
    } else {
        return;
    }
    this->Update();
}

float Scene::GetVolumeLayerOpacityScale(int layer) const {
    if (layer == 0) { return m_VolumeOpacityScale; }
    if (layer == 1) { return m_VolumeOpacityScale2; }
    return 0.0f;
}

void Scene::SetVolumeLayerMaskMode(int layer, VolumeMaskMode mode) {
    if (layer == 0) {
        this->SetVolumeRaycastMaskMode(mode);
        return;
    }
    if (layer != 1) { return; }
    const int modeValue = static_cast<int>(mode);
    if (modeValue < 0 || modeValue > 2) { mode = VolumeMaskMode::Off; }
    auto dataObject = this->GetVolumeLayerDataObject(1);
    if (dataObject == nullptr) { return; }
    dataObject->GetVolumeRenderingSettings().maskMode =
            static_cast<int>(mode);

    const bool needRebuild = (m_VolumeMaskMode2 == VolumeMaskMode::Off) !=
                             (mode == VolumeMaskMode::Off);
    m_VolumeMaskMode2 = mode;
    m_VolumeMaskEnabled2 =
            m_VolumeMaskAvailable2 && mode != VolumeMaskMode::Off;
    // 掩膜关闭时不会分配纹理；从 Off 切到 Carve/Shell 必须重建并上传纹理。
    if (needRebuild) { m_VolumeDataDirty = true; }
    this->Update();
}

Scene::VolumeMaskMode Scene::GetVolumeLayerMaskMode(int layer) const {
    if (layer == 0) { return m_VolumeMaskMode; }
    if (layer == 1) { return m_VolumeMaskMode2; }
    return VolumeMaskMode::Off;
}

void Scene::SetVolumeLayerShellOpacity(int layer, float opacity) {
    if (layer == 0) {
        this->SetVolumeRaycastShellOpacity(opacity);
        return;
    }
    if (layer != 1) { return; }
    const float clamped = std::clamp(opacity, 0.0f, 1.0f);
    auto dataObject = this->GetVolumeLayerDataObject(1);
    if (dataObject == nullptr) { return; }
    dataObject->GetVolumeRenderingSettings().shellOpacity = clamped;
    m_VolumeShellOpacity2 = clamped;
    this->Update();
}

float Scene::GetVolumeLayerShellOpacity(int layer) const {
    if (layer == 0) { return m_VolumeShellOpacity; }
    if (layer == 1) { return m_VolumeShellOpacity2; }
    return 0.9f;
}

SmartPointer<DataObject> Scene::GetVolumeLayerDataObject(int layer) const {
    if (layer == 0) { return m_VolumeSourceObject; }
    if (layer == 1 && m_VolumeLayer2Enabled) {
        return m_VolumeSourceObject2;
    }
    return nullptr;
}

Scene::VolumeRenderingInfo Scene::GetVolumeRenderingInfo() {
    VolumeRenderingInfo info;
    info.mode = this->GetVolumeRenderingMode();

    if (m_Camera != nullptr) {
        const igm::uvec2 viewport = m_Camera->GetViewPort();
        const igm::uvec2 scaled = m_Camera->GetScaledViewPort();
        info.viewportWidth = static_cast<int>(viewport.x);
        info.viewportHeight = static_cast<int>(viewport.y);
        info.renderWidth = static_cast<int>(scaled.x);
        info.renderHeight = static_cast<int>(scaled.y);
    }

    // 体素间距 = 局部包围盒尺寸 / (维度 - 1)，与 STRUCTURED_POINTS 的 SPACING
    // 语义一致；维度不足 2 时无法推断，记 0 由界面显示 “-”
    const float extent[3] = {m_VolumeLocalExtent.x, m_VolumeLocalExtent.y,
                             m_VolumeLocalExtent.z};
    for (int axis = 0; axis < 3; ++axis) {
        info.volumeDimension[axis] = m_VolumeDimension[axis];
        info.voxelSpacing[axis] =
                m_VolumeDimension[axis] > 1
                        ? extent[axis] /
                                  static_cast<float>(m_VolumeDimension[axis] - 1)
                        : 0.0f;
    }

    info.stepCount = m_VolumeStepCount;
    info.opacityScale = m_VolumeOpacityScale;
    info.maskMode = static_cast<int>(m_VolumeMaskMode);
    info.maskAvailable = m_VolumeMaskAvailable;
    info.maskEnabled = m_VolumeMaskEnabled;
    info.maskInvalidRatio =
            m_VolumeMaskAvailable ? (1.0f - m_VolumeMaskValidRatio) : 0.0f;
    info.shellOpacity = m_VolumeShellOpacity;
    info.layerCount = m_VolumeLayer2Enabled ? 2 : 1;
    info.layer0Enabled = m_VolumeLayer0UserEnabled;
    info.layer1Enabled = m_VolumeLayer2Enabled && m_VolumeLayer2UserEnabled;
    info.layer0Opacity = m_VolumeOpacityScale;
    info.layer1Opacity = m_VolumeOpacityScale2;
    if (m_VolumeSourceObject != nullptr) {
        info.layer0Name = m_VolumeSourceObject->GetName();
    }
    if (m_VolumeSourceObject2 != nullptr) {
        info.layer1Name = m_VolumeSourceObject2->GetName();
    }
    info.layer0MaskMode = static_cast<int>(m_VolumeMaskMode);
    info.layer1MaskMode = static_cast<int>(m_VolumeMaskMode2);
    info.layer0ShellOpacity = m_VolumeShellOpacity;
    info.layer1ShellOpacity = m_VolumeShellOpacity2;
    info.layer0MaskAvailable = m_VolumeMaskAvailable;
    info.layer1MaskAvailable = m_VolumeMaskAvailable2;
    // 当前面板编辑哪一层：由模型树当前选中的模型决定（与 ColorMapper 一致）
    if (auto currentModel = this->GetCurrentModel()) {
        auto currentData = currentModel->GetDataObject();
        if (currentData != nullptr) {
            if (currentData == m_VolumeSourceObject) {
                info.currentLayer = 0;
            } else if (m_VolumeLayer2Enabled &&
                       currentData == m_VolumeSourceObject2) {
                info.currentLayer = 1;
            }
        }
    }

    // 帧率与 GPU 帧时间：帧时间来自 Draw() 里的 GL_TIME_ELAPSED 双缓冲查询，
    // 已经是平滑值；“active”用于区分“空闲不重绘”和“真的卡住”
    info.fps = m_Fps;
    info.gpuFrameTimeMs = m_SmoothedGpuTimeMs;
    if (m_LastRenderEndValid) {
        const double sinceLastFrameMs =
                std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - m_LastRenderEnd)
                        .count();
        info.active = sinceLastFrameMs < 1500.0;
    }
    return info;
}

void Scene::VolumeRaycastPass() {
#ifdef IGAME_OPENGL_VERSION_460
    if (!m_EnableVolumeRaycasting) { return; }
    if (m_VolumeTexture3d == nullptr || m_VolumeTransferFunction == nullptr ||
        m_ColorTexture == nullptr) {
        return;
    }

    // 逐调用检查 GL 错误，便于定位（只在出错时打印）
    auto checkGL = [](const char* step) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            IGAME_RENDERING_ERROR(
                    "[Scene::VolumeRaycastPass] OpenGL error 0x{:X} after {}.",
                    error, step);
        }
    };
    // 先清一次：把进入本 pass 之前遗留的错误与本次调用区分开
    checkGL("previous pass (entry check)");

    // 体数据来源发生变化时重建 3D 纹理，触发条件包括：
    //   1. 首次开启 / 数据对象被修改（MTime 变化）
    //   2. 切换属性或切换分量（不会修改 DataObject 的 MTime，需显式比较）
    //   3. 色标变化（换色带、改范围、开不开不透明度映射）
    // 构建需要 GL 上下文，放在这里执行（渲染循环内上下文必然有效）。
    bool needRebuild =
            m_VolumeDataDirty || m_VolumeSourceObject == nullptr ||
            m_VolumeSourceObject->GetMTime().GetMTime() != m_VolumeSourceMTime;
    if (!needRebuild) {
        auto drawObject = this->FindVolumeSource();
        if (drawObject == nullptr) {
            needRebuild = true;
        } else {
            const int attributeIndex = PickVolumeScalarAttribute(
                    drawObject->GetAttributeSet(),
                    drawObject->GetAttributeIndex());
            auto colorMapper = drawObject->GetColorMapper();
            const unsigned int mapperMTime =
                    colorMapper != nullptr
                            ? colorMapper->GetMTime().GetMTime()
                            : 0u;
            // 可见模型顺序变化后“第一层”可能换成另一个数据对象；
            // 即使新旧模型的属性和 MTime 恰好相同，也必须重建对应纹理。
            needRebuild = drawObject != m_VolumeSourceObject ||
                          attributeIndex != m_VolumeAttributeIndex ||
                          drawObject->GetAttributeDimension() !=
                                  m_VolumeAttributeDimension ||
                          mapperMTime != m_VolumeMapperMTime;
        }
    }
    // 第二体层的失效判定：可见结构化模型数量 / 属性 / 色标变化都要重建
    if (!needRebuild) {
        auto drawObject2 = this->FindVolumeSource2();
        const bool hasSecond = drawObject2 != nullptr;
        if (hasSecond != m_VolumeLayer2Enabled) {
            needRebuild = true;
        } else if (hasSecond) {
            const int attributeIndex2 = PickVolumeScalarAttribute(
                    drawObject2->GetAttributeSet(),
                    drawObject2->GetAttributeIndex());
            auto colorMapper2 = drawObject2->GetColorMapper();
            const unsigned int mapperMTime2 =
                    colorMapper2 != nullptr
                            ? colorMapper2->GetMTime().GetMTime()
                            : 0u;
            needRebuild = drawObject2 != m_VolumeSourceObject2 ||
                          attributeIndex2 != m_VolumeAttributeIndex2 ||
                          drawObject2->GetAttributeDimension() !=
                                  m_VolumeAttributeDimension2 ||
                          mapperMTime2 != m_VolumeMapperMTime2 ||
                          drawObject2->GetMTime().GetMTime() !=
                                  m_VolumeSourceMTime2;
        }
    }

    if (needRebuild) {
        if (!this->BuildVolumeDataFromCurrentModel()) {
            std::string reason;
            const auto status = this->CheckVolumeRaycastingSupport(&reason);
            if (reason.empty()) {
                reason = "体绘制数据构建失败（数据可能已被移除或修改）。";
            }
            m_EnableVolumeRaycasting = false;

            // 数据已经不适合 compute 光线步进（例如开着体绘制切换到四面体网格，
            // 或变成多块/动画数据）：按入口的选型规则改用旧的“片元链表”实现，
            // 不弹窗打断。
            auto source = this->FindVolumeSource();
            const bool stillStructured =
                    source != nullptr &&
                    DynamicCast<StructuredMesh>(source) != nullptr &&
                    !source->HasSubDataObject() && !this->HasMultiBlockData();
            if (!stillStructured) {
                IGAME_RENDERING_WARN(
                        "[Scene::VolumeRaycastPass] source is no longer a "
                        "single structured mesh, switch to legacy fragment "
                        "volume rendering.");
                m_EnableVolumeRendering = true;
                this->Update();
                return;
            }

            // 结构化网格但构建失败：属于异常，关闭体绘制并交由界面提示
            // （日志保持 ASCII，中文原因只用于界面弹窗，避免日志乱码）
            const char* statusText = "Unknown";
            switch (status) {
                case VolumeRaycastingStatus::Ok:
                    statusText = "Ok";
                    break;
                case VolumeRaycastingStatus::UnsupportedBackend:
                    statusText = "UnsupportedBackend";
                    break;
                case VolumeRaycastingStatus::NoStructuredMesh:
                    statusText = "NoStructuredMesh";
                    break;
                case VolumeRaycastingStatus::NoScalarAttribute:
                    statusText = "NoScalarAttribute";
                    break;
                case VolumeRaycastingStatus::InvalidAttribute:
                    statusText = "InvalidAttribute";
                    break;
            }
            IGAME_RENDERING_ERROR(
                    "[Scene::VolumeRaycastPass] failed to build volume data, "
                    "volume rendering disabled (status={}); see UI message.",
                    statusText);
            m_VolumeRaycastFailureMessage = reason;
            this->Update();
            return;
        }
        m_VolumeDataDirty = false;
        m_VolumeRaycastFailureMessage.clear();
        IGAME_RENDERING_INFO(
                "[Scene::VolumeRaycastPass] volume data ready: {}x{}x{}, "
                "{} steps, opacity scale {}.",
                m_VolumeDimension[0], m_VolumeDimension[1], m_VolumeDimension[2],
                m_VolumeStepCount, m_VolumeOpacityScale);
        // 第二体层：第二个可见结构化网格（与第一层不同的数据对象）
        auto drawObject2 = this->FindVolumeSource2();
        if (drawObject2 != nullptr && m_VolumeTexture3d2 != nullptr) {
            const int attributeIndex2 = PickVolumeScalarAttribute(
                    drawObject2->GetAttributeSet(),
                    drawObject2->GetAttributeIndex());
            const int attributeDimension2 =
                    drawObject2->GetAttributeDimension();
            if (!this->BuildVolumeLayer2(drawObject2, attributeIndex2,
                                         attributeDimension2)) {
                m_VolumeLayer2Enabled = false;
                IGAME_RENDERING_WARN(
                        "[Scene::VolumeRaycastPass] second volume layer could "
                        "not be built; draw single layer only.");
            }
        } else {
            m_VolumeLayer2Enabled = false;
            m_VolumeSourceObject2 = nullptr;
        }
        IGAME_RENDERING_INFO(
                "[Scene::VolumeRaycastPass] volume layers active: {}.",
                m_VolumeLayer2Enabled ? 2 : 1);
        checkGL("build volume data");
    }

    auto shader = this->GetShader(ShaderType::VOLUMERAYMARCH);
    if (shader == nullptr) { return; }

    // 直接在视口分辨率上做光线步进，结果写入主颜色纹理（显示路径读取的纹理），
    // 因此不需要再 blit，也就避免了不同格式附件之间的 blit 兼容性问题。
    const igm::uvec2 viewport = m_Camera->GetScaledViewPort();
    if (viewport.x == 0u || viewport.y == 0u) { return; }
    const GLuint groupCountX = (viewport.x + 7u) / 8u;
    const GLuint groupCountY = (viewport.y + 7u) / 8u;

    const igm::mat4 projView =
            m_Camera->GetProjectionMatrix() * m_Camera->GetViewMatrix();
    const igm::mat4 invProjView = projView.invert();

    // 世界空间 -> 体纹理空间：每帧用当前模型矩阵重建，
    // 保证交互旋转/平移（只改 m_ModelMatrix）时体数据跟随网格
    const igm::mat4 worldToTexture =
            m_VolumeLocalToTexture * m_ModelMatrix.invert();

    // 【临时诊断】一次性自检：用与 shader 相同的数学在 CPU 上验证“屏幕中心光线是否与
    // 体盒相交”。hit=0 表示映射/相机有问题，而不是 shader 的问题。
    if (!m_VolumeRaycastDiagnosed) {
        m_VolumeRaycastDiagnosed = true;

        const bool isOrtho =
                m_Camera->GetType() == Camera::Type::ORTHOGRAPHIC;
        const igm::vec4 nearH =
                invProjView * igm::vec4{0.0f, 0.0f, 1.0f, 1.0f};
        const igm::vec4 farH =
                invProjView * igm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
        const igm::vec3 nearP{nearH.x / nearH.w, nearH.y / nearH.w,
                              nearH.z / nearH.w};
        const igm::vec3 farP{farH.x / farH.w, farH.y / farH.w,
                             farH.z / farH.w};
        const igm::vec3 camPos = m_Camera->GetPosition();
        // 与 shader 保持一致：统一用 near -> far 定方向，近远点重合时回退相机位置
        igm::vec3 origin = nearP;
        igm::vec3 ray = farP - nearP;
        if (ray.length() < 1.0e-6f) {
            origin = camPos;
            ray = nearP - camPos;
        }
        ray = ray.normalized();

        float tNear = 0.0f;
        float tFar = 0.0f;
        const bool hit = RayBoxIntersectTexture(origin, ray, worldToTexture,
                                                tNear, tFar);
        const igm::vec2 clip = m_Camera->GetClippingRange();

        IGAME_RENDERING_INFO(
                "[Scene::VolumeRaycastPass][diag] viewport {}x{}, ortho={}, "
                "camPos=({}, {}, {}), rayOrigin=({}, {}, {}), rayDir=({}, {}, "
                "{}), clip=({}, {}), hit={}, tNear={}, tFar={}.",
                viewport.x, viewport.y, isOrtho ? 1 : 0, camPos.x, camPos.y,
                camPos.z, origin.x, origin.y, origin.z, ray.x, ray.y, ray.z,
                clip.x, clip.y, hit ? 1 : 0, tNear, tFar);
        IGAME_RENDERING_INFO(
                "[Scene::VolumeRaycastPass][diag] local box min=({}, {}, {}), "
                "extent=({}, {}, {}), model translate=({}, {}, {}).",
                m_VolumeLocalMin.x, m_VolumeLocalMin.y, m_VolumeLocalMin.z,
                m_VolumeLocalExtent.x, m_VolumeLocalExtent.y,
                m_VolumeLocalExtent.z, m_ModelMatrix[3][0], m_ModelMatrix[3][1],
                m_ModelMatrix[3][2]);
    }

    // m_ColorTexture 是主帧缓冲的颜色附件，作为 image 写入时不能同时挂在
    // “当前绑定的帧缓冲”上（否则构成读写反馈回路），这里改绑体绘制帧缓冲。
    m_VolumeFramebuffer->Bind();

    shader->Use();
    checkGL("shader->Use");

    shader->SetUniformMatrix4x4("invProjView", invProjView);
    shader->SetUniformMatrix4x4("worldToTexture", worldToTexture);
    shader->SetUniformi("numSteps", m_VolumeStepCount);
    shader->SetUniformf("opacityScale", m_VolumeOpacityScale);
    shader->SetUniformf("jitterSeed",
                        static_cast<float>(m_VolumeFrameSeed % 1024u));
    shader->SetUniform3f("backgroundColor", m_BackgroundColor);

    m_VolumeTexture3d->Active(GL_TEXTURE1);
    shader->SetUniformi("volumeTexture", 1);

    // 掩膜纹理（可选）：未启用时该分支在 shader 中整体跳过，
    // 这里仍绑定一个合法单元（未创建时绑定 0 也无害）
    m_VolumeMaskTexture3d->Active(GL_TEXTURE3);
    shader->SetUniformi("volumeMask", 3);
    shader->SetUniformi("volumeMaskEnabled", m_VolumeMaskEnabled ? 1 : 0);
    // 第二层掩膜（可选）：各用自己的掩膜纹理与掩膜模式
    const bool layer2MaskOn = m_VolumeLayer2Enabled && m_VolumeMaskEnabled2 &&
                              m_VolumeMaskTexture3d2 != nullptr;
    m_VolumeMaskTexture3d2->Active(GL_TEXTURE6);
    shader->SetUniformi("volumeMask2", 6);
    shader->SetUniformi("volumeMask2Enabled", layer2MaskOn ? 1 : 0);
    // 掩膜合成方式：1=挖空无效体素；2=把无效体素画成实体车壳
    shader->SetUniformi("maskMode",
                        m_VolumeMaskEnabled
                                ? static_cast<int>(m_VolumeMaskMode)
                                : 0);
    shader->SetUniformf("shellOpacity", m_VolumeShellOpacity);
    shader->SetUniformi("maskMode2",
                        layer2MaskOn ? static_cast<int>(m_VolumeMaskMode2) : 0);
    shader->SetUniformf("shellOpacity2", m_VolumeShellOpacity2);
    shader->SetUniform3f("shellColor", m_VolumeShellColor);

    m_VolumeTransferFunction->Active(GL_TEXTURE2);
    shader->SetUniformi("transferFunction", 2);

    // 第二体层（可选）：两个可见结构化模型一起体绘制
    const bool twoLayers = m_VolumeLayer2Enabled &&
                           m_VolumeTexture3d2 != nullptr &&
                           m_VolumeTransferFunction2 != nullptr;
    shader->SetUniformi("layerCount", twoLayers ? 2 : 1);
    shader->SetUniformi("layer0On", m_VolumeLayer0UserEnabled ? 1 : 0);
    shader->SetUniformi("layer1On",
                        (twoLayers && m_VolumeLayer2UserEnabled) ? 1 : 0);
    shader->SetUniformf("opacityScale2", m_VolumeOpacityScale2);
    if (twoLayers) {
        const igm::mat4 worldToTexture2 =
                m_VolumeLocalToTexture2 * m_ModelMatrix.invert();
        shader->SetUniformMatrix4x4("worldToTexture2", worldToTexture2);
        m_VolumeTexture3d2->Active(GL_TEXTURE4);
        shader->SetUniformi("volumeTexture2", 4);
        m_VolumeTransferFunction2->Active(GL_TEXTURE5);
        shader->SetUniformi("transferFunction2", 5);
    }
    checkGL("bind volume textures");

    m_ColorTexture->BindImage(0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    checkGL("bind output image");

    glDispatchCompute(groupCountX, groupCountY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                    GL_TEXTURE_FETCH_BARRIER_BIT);
    checkGL("glDispatchCompute");

    // 【临时诊断】一次性回读结果像素：用于确认 compute 是否真的写出了内容，以及是不是
    // 所有光线都走了“未命中”分支（写出的是纯背景色）。
    if (!m_VolumeRaycastReadbackDone) {
        m_VolumeRaycastReadbackDone = true;

        const GLint sampleX = static_cast<GLint>(viewport.x / 2u);
        const GLint sampleY = static_cast<GLint>(viewport.y / 2u);
        unsigned char center[4] = {0, 0, 0, 0};
        unsigned char corner[4] = {0, 0, 0, 0};
        glGetTextureSubImage(m_ColorTexture->Handle(), 0, sampleX, sampleY, 0,
                             1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 4, center);
        glGetTextureSubImage(m_ColorTexture->Handle(), 0, 4, 4, 0, 1, 1, 1,
                             GL_RGBA, GL_UNSIGNED_BYTE, 4, corner);
        IGAME_RENDERING_INFO(
                "[Scene::VolumeRaycastPass][diag] result pixel center=({}, {}, "
                "{}, {}) at ({}, {}), corner=({}, {}, {}, {}) at (4, 4).",
                center[0], center[1], center[2], center[3], sampleX, sampleY,
                corner[0], corner[1], corner[2], corner[3]);
        checkGL("read back result pixel");
    }

    // 【临时诊断】一次性 GPU 侧命中自检：以 debug 模式再派发一帧并回读。
    // 这一步用于区分“GPU 认为光线没命中”（R=0）与“命中了但画面看不见”（R=255）。
    if (!m_VolumeRaycastGpuDiagDone) {
        m_VolumeRaycastGpuDiagDone = true;

        const GLint sampleX = static_cast<GLint>(viewport.x / 2u);
        const GLint sampleY = static_cast<GLint>(viewport.y / 2u);

        shader->SetUniformi("debugMode", 1);
        glDispatchCompute(groupCountX, groupCountY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                        GL_TEXTURE_FETCH_BARRIER_BIT);

        unsigned char center[4] = {0, 0, 0, 0};
        unsigned char corner[4] = {0, 0, 0, 0};
        glGetTextureSubImage(m_ColorTexture->Handle(), 0, sampleX, sampleY, 0,
                             1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 4, center);
        glGetTextureSubImage(m_ColorTexture->Handle(), 0, 4, 4, 0, 1, 1, 1,
                             GL_RGBA, GL_UNSIGNED_BYTE, 4, corner);

        IGAME_RENDERING_INFO(
                "[Scene::VolumeRaycastPass][diag] GPU hit test: center=(R={}, "
                "G={}, B={}) corner=(R={}, G={}, B={}) "
                "[R=255 means ray hit; G=tNear/100; B=tFar/100].",
                center[0], center[1], center[2], corner[0], corner[1],
                corner[2]);
        checkGL("gpu debug dispatch");

        // 恢复：用正常模式重新渲染一次，保证本帧画面正确
        shader->SetUniformi("debugMode", 0);
        glDispatchCompute(groupCountX, groupCountY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                        GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    ++m_VolumeFrameSeed;

    glDisable(GL_DEPTH_TEST);
#endif
}

void Scene::UpdateCameraDataBlock() {
    m_ShaderManager->UpdateCameraBlock(m_Camera);
}
void Scene::UpdateObjectDataBlock(SmartPointer<DataObject> obj) {
    m_ShaderManager->UpdateObjectBlock(obj, m_ModelMatrix);
}
void Scene::UpdateUniformBufferObjectBlock(SmartPointer<DataObject> obj) {
    m_ShaderManager->UpdateUBOBlock(obj);
}

void Scene::UpdateCameraClippingRange() {
    // If a model is changed, bounding-box will not be notified to Scene
    UpdateModelsBoundingSphere();

    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
    igm::vec3 centerInWorld = (m_ModelMatrix * igm::vec4{center, 1.0f}).xyz();
    float radius = m_ModelsBoundingSphere.w;
    igm::vec3 cameraPos = m_Camera->GetPosition();

    igm::vec3 front = m_Camera->GetFront();
    igm::vec3 v = centerInWorld - cameraPos;
    float dist = std::abs(front.dot(v) / front.length());

    float nearPlane = dist - radius;
    float farPlane = dist + radius;

#ifndef __EMSCRIPTEN__
    // 底板范围远大于模型，它的远角会落在"按模型包围球算出的"裁剪范围之外，
    // 表现为底板被裁掉一部分。这里把底板一并计入裁剪范围（只影响 near/far，
    // 不影响取景使用的包围球）。桌面端网格要一直铺到渐隐半径之外才淡出，
    // 所以远裁剪面必须先跟到这个半径。
    UpdateGroundGridExtent();
    if (m_GroundGridBuilt && m_GroundGridVisible && m_GroundGridRadius > 0.0f) {
        const igm::vec3 groundCenterInWorld =
                (m_ModelMatrix * igm::vec4{m_GroundGridCenter, 1.0f}).xyz();
        const igm::vec3 vGround = groundCenterInWorld - cameraPos;
        const float groundDist =
                std::abs(front.dot(vGround) / front.length());
        const float groundFar = groundDist + m_GroundGridRadius;
        const float groundNear = groundDist - m_GroundGridRadius;
        if (groundFar > farPlane) { farPlane = groundFar; }
        if (groundNear < nearPlane) { nearPlane = groundNear; }
    }

#endif

    // https://3dgumshoe.com/maya-fixing-z-fighting/
    const float minGap = 0.0001f;
#ifndef __EMSCRIPTEN__
    // 桌面端底板使用更大的近裁剪下限；Web 保持原有计算。
    const float minNear = std::max(minGap * farPlane, 0.01f * farPlane);
    if (nearPlane < minNear) { nearPlane = minNear; }
#else
    if (nearPlane < minGap * farPlane) { nearPlane = minGap * farPlane; }
#endif

    // std::cout << std::format("near: {}, far: {}.", nearPlane, farPlane)
    //           << std::endl;

    m_Camera->SetClippingRange(nearPlane, farPlane);
}

void Scene::RefreshDrawCullDataBuffer() {
    m_ShaderManager->UpdateCullDataBuffer(m_Camera, m_ModelMatrix, m_HzbWidth,
                                          m_HzbHeight);
}

void Scene::ResetCameraViewToPositiveX() {
    ResetCameraView();

    igm::vec3 center = this->GetRotationBoundingSphere().xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -center);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, center);

    auto radians = static_cast<float>(igm::radians(90.0f));
    auto rotate =
            igm::rotate(igm::mat4{}, -radians, igm::vec3{1.0f, 0.0f, 0.0f}) *
            igm::rotate(igm::mat4{}, radians, igm::vec3{0.0f, 0.0f, 1.0f});
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::ResetCameraViewToNegativeX() {
    ResetCameraView();

    igm::vec3 center = this->GetRotationBoundingSphere().xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -center);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, center);

    auto radians = static_cast<float>(igm::radians(90.0f));
    auto rotate =
            igm::rotate(igm::mat4{}, -radians, igm::vec3{0.0f, 0.0f, 1.0f}) *
            igm::rotate(igm::mat4{}, -radians, igm::vec3{0.0f, 1.0f, 0.0f});
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::ResetCameraViewToPositiveY() {
    ResetCameraView();

    igm::vec3 center = this->GetRotationBoundingSphere().xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -center);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, center);

    auto radians = static_cast<float>(igm::radians(90.0f));
    auto rotate =
            igm::rotate(igm::mat4{}, -radians, igm::vec3{1.0f, 0.0f, 0.0f});
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::ResetCameraViewToNegativeY() {
    ResetCameraView();

    igm::vec3 center = this->GetRotationBoundingSphere().xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -center);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, center);

    auto radians = static_cast<float>(igm::radians(90.0f));
    auto rotate =
            igm::rotate(igm::mat4{}, radians, igm::vec3{1.0f, 0.0f, 0.0f}) *
            igm::rotate(igm::mat4{}, 2 * radians, igm::vec3{0.0f, 1.0f, 0.0f});
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::ResetCameraViewToPositiveZ() {
    ResetCameraView();

    igm::vec3 center = this->GetRotationBoundingSphere().xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -center);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, center);

    auto radians = static_cast<float>(igm::radians(90.0f));
    auto rotate =
            igm::rotate(igm::mat4{}, 2 * radians, igm::vec3{0.0f, 1.0f, 0.0f});
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::ResetCameraViewToNegativeZ() {
    ResetCameraView();

    igm::vec3 center = this->GetRotationBoundingSphere().xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -center);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, center);

    auto rotate = igm::mat4{};
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::ResetCameraViewToIsometric() {
    ResetCameraView();

    igm::vec3 center = this->GetRotationBoundingSphere().xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -center);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, center);

    auto radians = static_cast<float>(igm::radians(45.0f));
    auto rotate =
            igm::rotate(igm::mat4{}, radians, igm::vec3{1.0f, 0.0f, 0.0f}) *
            igm::rotate(igm::mat4{}, -radians, igm::vec3{0.0f, 1.0f, 0.0f});
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::RotateNinetyClockwise() { return this->RotateClockwise(90.0f); }

void Scene::RotateNinetyCounterClockwise() {
    return this->RotateClockwise(-90.0f);
}

void Scene::RotateClockwise(float angle) {
    igm::vec4 center = igm::vec4{GetRotationBoundingSphere().xyz(), 1.0f};
    igm::vec3 centerInWorld = (m_ModelMatrix * center).xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -centerInWorld);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, centerInWorld);

    auto radians = static_cast<float>(igm::radians(angle));
    auto rotate =
            igm::rotate(igm::mat4{}, -radians, igm::vec3{0.0f, 0.0f, 1.0f});
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

// 切换显示状态实现
void Scene::ToggleCenterAxes() {
    m_CenterAxesVisible = !m_CenterAxesVisible;
    SetCenterAxesVisible(m_CenterAxesVisible);
}
void Scene::SetCenterAxesVisible(bool visible) {
    m_CenterAxesVisible = visible;
    if (m_CenterAxesModel) { m_CenterAxesModel->SetVisibility(visible); }
    this->Modified();
}
void Scene::ToggleAxes() { m_AxesVisible = !m_AxesVisible; }
void Scene::SetAxesVisible(bool visible) {
    m_AxesVisible = visible;
    this->Modified();
}
bool Scene::GetAxesVisible() const { return m_AxesVisible; }

// ===================== 底板（地面网格） =====================

#ifndef __EMSCRIPTEN__
namespace
{
// ---- 底板（地面网格）参数 ----
// 刻度按"内容包围盒最长边（含 Y）/ 模型跨几格"自动选档（取 1/2/5 x 10^n），
// 只在内容变化时重算一次；同一批内容下刻度固定，也没有任何远处变宽的分级。
constexpr float kGroundGridCellsPerModel = 5.0f;
// 底板边长 = 内容包围盒最长边 x 该倍数（3 倍：模型占底板中间三分之一，
// 四周各留半个模型边长；再小就会显得"贴着模型"、边缘很明显）
constexpr float kGroundGridPlateScale = 3.0f;
// 空场景（从未加载过模型）时，把内容当作一个"边长 1 的单位立方体"（中心在
// 原点），底板尺寸沿用这个虚拟内容范围；初始格距单独设置。
constexpr float kGroundGridVirtualContentHalfExtent = 0.5f;
constexpr float kGroundGridDefaultCell = 0.4f; // 首次导入模型前的默认格距
// 底板相对模型高度的下沉比例，避免与模型底面共面闪烁
constexpr float kGroundGridSinkRatio = 0.002f;
// 桌面端底板的可见半径下限还会跟着相机高度放大：拉远时网格跟着铺开，
// 近处不浪费、远处也不会看到边界。
constexpr float kGroundGridCameraRadiusFactor = 6.0f;

const igm::vec3 kGroundGridColor {176.0f / 255.0f, 171.0f / 255.0f,
                                 167.0f / 255.0f}; // 网格线 #B0ABA7
const igm::vec3 kGroundSurfaceColor {106.0f / 255.0f, 99.0f / 255.0f,
                                    97.0f / 255.0f}; // 参考图暖灰底色

int GroundGridNormalAxis(Scene::GroundGridPlane plane) {
    switch (plane) {
    case Scene::GroundGridPlane::XY: return 2;
    case Scene::GroundGridPlane::YZ: return 0;
    default: return 1;
    }
}

// 把刻度收敛到 1/2/5 x 10^n 里最接近的档，让网格刻度保持可读
float NiceGridSpacing(float target) {
    if (!(target > 0.0f)) { return 1.0f; }
    const float exponent = std::floor(std::log10(target));
    const float base = std::pow(10.0f, exponent);
    const float mantissa = target / base; // [1, 10)
    float mantissaNice = 1.0f;
    if (mantissa > 1.5f) { mantissaNice = 2.0f; }
    if (mantissa > 3.5f) { mantissaNice = 5.0f; }
    if (mantissa > 7.5f) { mantissaNice = 10.0f; }
    return mantissaNice * base;
}
} // namespace

BoundingBox Scene::ComputeGroundGridContentBox() {
    // 解析式底板不在模型池里，因此不会把自己的尺寸再算回来；
    // 中心坐标轴属于辅助对象，一并排除。
    BoundingBox box;
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        if (!model->GetVisibility()) { continue; }
        auto dataObject = model->GetDataObject();
        if (!dataObject) { continue; }
        if (dataObject.get() == m_CenterAxesModel.get()) { continue; }
        box.add(dataObject->GetBoundingBox());
    }
    return box;
}

void Scene::RequestGroundGridFit() {
    m_GroundGridDirty = true;
    this->Modified();
    Update();
}

void Scene::UpdateGroundGrid() {
    m_GroundGridDirty = false;

    BoundingBox box = ComputeGroundGridContentBox();
    const bool useDefaultGrid = box.isNull() && m_GroundGridLastContentBox.isNull();

    // 没有可见模型时，把"内容"退化为：优先沿用上一次加载过的内容包围盒
    // （这样删掉/再加载同量级模型时不会跳），完全没加载过则使用默认的
    // 单位立方体确定底板位置和范围，刻度采用独立的默认格距。
    if (!box.isNull()) {
        m_GroundGridLastContentBox = box;
    } else if (!m_GroundGridLastContentBox.isNull()) {
        box = m_GroundGridLastContentBox;
    } else {
        const double r =
                static_cast<double>(kGroundGridVirtualContentHalfExtent);
        box = BoundingBox(Vector3d{-r, -r, -r}, Vector3d{r, r, r});
    }

    const double xmin = box.min[0], xmax = box.max[0];
    const double ymin = box.min[1], ymax = box.max[1];
    const double zmin = box.min[2], zmax = box.max[2];

    // 刻度自动选档：按内容包围盒最长边（含 Y）定一个可读刻度。
    // 与位置共用显式适配时机，相机移动或结果几何变化均不重新选档。
    const float longestEdge = static_cast<float>(
            std::max({xmax - xmin, ymax - ymin, zmax - zmin}));
    const float cell = useDefaultGrid ? kGroundGridDefaultCell
            : NiceGridSpacing(longestEdge / kGroundGridCellsPerModel);

    const int normalAxis = GroundGridNormalAxis(m_GroundGridPlane);
    igm::vec3 center;
    for (int axis = 0; axis < 3; ++axis) {
        const float minimum = static_cast<float>(box.min[axis]);
        const float maximum = static_cast<float>(box.max[axis]);
        // 正向放在最小边界外侧，反转放在最大边界外侧；退化模型也保留间隙。
        const float gap = std::max(longestEdge, 1e-6f) * kGroundGridSinkRatio;
        center[axis] = axis == normalAxis
                ? (m_GroundGridReversed ? maximum + gap : minimum - gap)
                : std::round((minimum + maximum) * 0.5f / cell) * cell;
    }

    // 底板中心与可见半径（模型空间），供相机裁剪范围与着色器使用
    m_GroundGridCenter = center;
    m_GroundGridCell = cell;
    m_GroundGridContentScale = longestEdge;
    m_GroundGridBuilt = true;
    UpdateGroundGridExtent();
}

void Scene::UpdateGroundGridExtent() {
    if (!m_GroundGridBuilt) { return; }

    // 可见半径（模型空间）：下限保证盖住内容本身，再跟随相机到平面的高度
    // 一起放大，于是拉近/拉远都不会把网格"看出边"。
    const float contentScale = std::max(m_GroundGridContentScale, 1e-6f);
    float radius = kGroundGridPlateScale * contentScale;

    const igm::vec3 cameraInModel =
            (m_ModelMatrix.invert() *
             igm::vec4{m_Camera->GetPosition(), 1.0f})
                    .xyz();
    const int normalAxis = GroundGridNormalAxis(m_GroundGridPlane);
    const float cameraHeight =
            std::abs(cameraInModel[normalAxis] - m_GroundGridCenter[normalAxis]);
    radius = std::max(radius, kGroundGridCameraRadiusFactor * cameraHeight);

    m_GroundGridRadius = radius;
}

void Scene::DrawGroundGrid() {
    if (!m_GroundGridBuilt || m_GroundGridCell <= 0.0f) { return; }

    auto shader = this->GetShader(ShaderType::GROUNDGRID);
    if (shader == nullptr) { return; }

    // 网格在"模型空间"里解析生成，所以射线要反投影到模型空间；平面深度则
    // 由交点正投影得到（本引擎是反向 Z，写窗口深度时才换算）。
    const igm::mat4 model = m_ModelMatrix;
    const igm::mat4 invModel = model.invert();
    const igm::mat4 modelProjView =
            m_Camera->GetProjectionMatrix() * m_Camera->GetViewMatrix() * model;
    const igm::vec3 cameraInModel =
            (invModel * igm::vec4{m_Camera->GetPosition(), 1.0f}).xyz();

    // 渐隐半径用"沿视线到平面中心的距离 + 可见半径"，它正是相机远裁剪面在该
    // 方向上的深度，于是网格总在远平面切到它之前淡到 0，不会出现硬边；同时
    // 它保证近处的模型区（也就是当前取景范围）永远不会被误淡。
    const igm::vec3 frontInModel =
            (invModel * igm::vec4{m_Camera->GetFront(), 0.0f}).xyz();
    const float frontLength = frontInModel.length();
    const float distanceAlongView =
            frontLength > 0.0f
                    ? std::abs(frontInModel.dot(m_GroundGridCenter -
                                                cameraInModel)) /
                              frontLength
                    : 0.0f;
    const float fadeRadius =
            std::max(distanceAlongView + m_GroundGridRadius,
                     m_GroundGridCell);

    shader->Use();
    shader->SetUniformMatrix4x4("uInvModelProjView", modelProjView.invert());
    shader->SetUniformMatrix4x4("uModelProjView", modelProjView);
    shader->SetUniform3f("uCameraPosModel", cameraInModel);
    igm::vec3 planeNormal {0.0f, 0.0f, 0.0f};
    planeNormal[GroundGridNormalAxis(m_GroundGridPlane)] =
            m_GroundGridReversed ? -1.0f : 1.0f;
    shader->SetUniform3f("uPlaneCenter", m_GroundGridCenter);
    shader->SetUniform3f("uPlaneNormal", planeNormal);
    shader->SetUniformf("uBaseCell", m_GroundGridCell);
    shader->SetUniformf("uFadeRadius", fadeRadius);
    shader->SetUniform3f("uLineColor", kGroundGridColor);
    shader->SetUniform3f("uSurfaceColor", kGroundSurfaceColor);

    // 实体底板参与深度测试并写深度，供后续透明物体正确遮挡；不做 alpha 混合。
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthWriteWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteWasEnabled);

    glDepthRange(0.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    m_EmptyVAO->DrawArrays(GL_TRIANGLES, 0, 3);

    glDepthMask(depthWriteWasEnabled);
    if (blendWasEnabled) { glEnable(GL_BLEND); }
    glDepthFunc(GL_GREATER);
    glDepthRange(0.0, 1.0);
}

void Scene::SetGroundGridPlane(GroundGridPlane plane) {
    if (plane != GroundGridPlane::XY && plane != GroundGridPlane::XZ &&
        plane != GroundGridPlane::YZ) { return; }
    if (m_GroundGridPlane == plane) { return; }
    m_GroundGridPlane = plane;
    RequestGroundGridFit();
}

void Scene::SetGroundGridReversed(bool reversed) {
    if (m_GroundGridReversed == reversed) { return; }
    m_GroundGridReversed = reversed;
    RequestGroundGridFit();
}
#endif

void Scene::ToggleGroundGrid() {
#ifndef __EMSCRIPTEN__
    SetGroundGridVisible(!m_GroundGridVisible);
#endif
}

void Scene::SetGroundGridVisible(bool visible) {
#ifndef __EMSCRIPTEN__
    m_GroundGridVisible = visible;
    this->Modified();
#else
    (void)visible;
#endif
}

bool Scene::GetGroundGridVisible() const {
#ifndef __EMSCRIPTEN__
    return m_GroundGridVisible;
#else
    return false;
#endif
}

void Scene::ToggleColorBar() { SetColorBarVisible(!m_ColorBarVisible); }

void Scene::SetColorBarVisible(bool visible) {
    m_ColorBarVisible = visible;
    if (m_ColorBar2DActor) { m_ColorBar2DActor->SetVisible(visible); }
    this->Modified();
}

bool Scene::GetColorBarVisible() const { return m_ColorBarVisible; }

void Scene::ToggleOpacityMappingEnabled() { SetOpacityMappingEnabled(!m_OpacityMappingEnabled); }

void Scene::SetOpacityMappingEnabled(bool enabled) {
    m_OpacityMappingEnabled = enabled;
    m_VolumeDataDirty = true; // 体绘制需要重新构建数据
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); it++) {
        auto model = it->second;
        if (!model->GetDataObject()->IsDrawable()) { continue; }
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        drawObject->SetOpacityMappingEnabled(enabled);
        drawObject->ProcessSubDataObjects(&DrawObject::SetOpacityMappingEnabled,
                                          enabled);
    }
    this->Modified();
}

bool Scene::GetOpacityMappingEnabled() const { return m_OpacityMappingEnabled; }

SmartPointer<ColorBar2DActor> Scene::GetColorBar2DActor() const {
    return m_ColorBar2DActor;
}

void Scene::SetCornerAnnotationText(const std::string& text) {
    if (m_TextOverlay2DActor) {
        m_TextOverlay2DActor->SetText(text);
        this->Modified();
    }
}

void Scene::SetCornerAnnotationPosition(float left, float top) {
    if (m_TextOverlay2DActor) {
        m_TextOverlay2DActor->SetPosition(left, top);
        this->Modified();
    }
}

void Scene::SetCornerAnnotationAnchorToBottomRight(bool anchorToBottomRight) {
    if (m_TextOverlay2DActor) {
        m_TextOverlay2DActor->SetAnchorToBottomRight(anchorToBottomRight);
        this->Modified();
    }
}

void Scene::SetCornerAnnotationVisible(bool visible) {
    if (m_TextOverlay2DActor) {
        m_TextOverlay2DActor->SetVisible(visible);
        this->Modified();
    }
}

SmartPointer<TextOverlay2DActor> Scene::GetTextOverlay2DActor() const {
    return m_TextOverlay2DActor;
}

SmartPointer<CenterAxesModel> Scene::GetCenterAxesModel() const {
    return m_CenterAxesModel;
}

igm::vec4 Scene::GetRotationBoundingSphere() const {
    return m_UseCustomRotationBoundingSphere ? m_CustomRotationBoundingSphere
                                             : m_ModelsBoundingSphere;
}

void Scene::SetRotationBoundingSphere(const igm::vec4 boundingSphere) {
    m_UseCustomRotationBoundingSphere = true;
    m_CustomRotationBoundingSphere = boundingSphere;
    m_CenterAxesModel->SetRotationCenter(boundingSphere.xyz());
    this->Modified();
}

void Scene::ResetRotationBoundingSphere() {
    m_UseCustomRotationBoundingSphere = false;
    this->Modified();
}

float Scene::GetRotationCenterDepth() const {
    igm::vec3 center = GetRotationBoundingSphere().xyz();
    igm::vec4 viewPos = m_Camera->GetViewMatrix() * igm::vec4(center, 1.0f);
    return -viewPos.z; // OpenGL相机看向-z方向
}


void Scene::UpdateAxisSize() {
    if (m_CenterAxesModel && m_Camera) {
        igm::vec3 rotationCenter = m_CenterAxesModel->GetRotationCenter();
        igm::vec3 cameraPos = m_Camera->GetPosition();
        float cameraDistance = (cameraPos - rotationCenter).length();

        auto viewport = m_Camera->GetViewPort();
        float viewportHeight = viewport.y;

        // 使用相机的实际FOV
        float fov = m_Camera->GetFov(); // 弧度值

        m_CenterAxesModel->UpdateAxisScale(cameraDistance, fov, viewportHeight);
    }
}


igm::vec3 Scene::ScreenToWorld(const igm::vec2& screenPos, float depth) const {
    // 将屏幕坐标转换为标准化设备坐标
    const igm::uvec2 viewport = m_Camera->GetViewPort();
    float x = (2.0f * screenPos.x) / viewport.x - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y) / viewport.y;

    // 获取投影和视图矩阵
    igm::mat4 projection = m_Camera->GetProjectionMatrix();
    igm::mat4 view = m_Camera->GetViewMatrix();

    // 计算逆矩阵
    igm::mat4 invVP = (projection * view).invert();

    // 创建近平面和远平面点
    igm::vec4 nearPoint(x, y, -1.0f, 1.0f);
    igm::vec4 farPoint(x, y, 1.0f, 1.0f);

    // 转换为世界坐标
    igm::vec4 nearResult = invVP * nearPoint;
    igm::vec4 farResult = invVP * farPoint;
    nearResult /= nearResult.w;
    farResult /= farResult.w;

    // 计算射线方向
    igm::vec3 rayDir = igm::vec3(farResult) - igm::vec3(nearResult);
    rayDir = rayDir.normalize();

    // 根据深度计算交点
    return igm::vec3(m_Camera->GetPosition()) + rayDir * depth;
}

// 旧的“片元链表”体绘制实现。保留原有行为（关闭抽壳渲染、重置不透明度映射），
// 作为非结构化网格以及 compute 不可用时的实现；常规入口请使用 SetVolumeRendering。
void Scene::SetFragmentVolumeRendering(bool toggled) {
    m_EnableVolumeRendering = toggled;
    if (toggled) {
        m_EnableVolumeRaycasting = false;
        m_VolumeLegacyDiagPending = true; // 下一帧输出一次旧实现自检
    }
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;

        if (!model->GetDataObject()->IsDrawable()) { continue; }
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        drawObject->SetShellRenderingOption(!toggled);
    }
    // 默认关闭不透明度映射
    SetOpacityMappingEnabled(false);
    Update();
}

Scene::VolumeRenderingMode Scene::GetVolumeRenderingMode() const {
    if (m_EnableVolumeRaycasting) { return VolumeRenderingMode::Raycast; }
    if (m_EnableVolumeRendering) { return VolumeRenderingMode::Fragment; }
    return VolumeRenderingMode::Off;
}

Scene::VolumeRenderingMode Scene::SetVolumeRendering(bool toggled,
                                                     std::string* message) {
    if (!toggled) {
        if (message != nullptr) { message->clear(); }
        m_EnableVolumeRaycasting = false;
        m_EnableVolumeRendering = false;
        this->Update();
        return VolumeRenderingMode::Off;
    }

    // 多块/动画数据优先判定：这类数据的子对象会被动画组件 Clear/Add 重建，
    // compute 路径既无法用单个体纹理覆盖多块，也不能安全持有子对象指针，
    // 统一交给旧的“片元链表”实现（它按子对象逐个绘制）。
    if (this->HasMultiBlockData()) {
        this->SetFragmentVolumeRendering(true);
        IGAME_RENDERING_INFO(
                "[Scene::SetVolumeRendering] multi-block/animation data, use "
                "legacy fragment volume rendering.");
        if (message != nullptr) {
            *message =
                    "当前数据是多块/动画网格，已使用旧的片元链表体绘制实现。";
        }
        return VolumeRenderingMode::Fragment;
    }

    auto source = this->FindVolumeSource();
    if (source == nullptr) {
        if (message != nullptr) {
            *message = "未找到可用于体绘制的网格：请先加载网格数据。";
        }
        return VolumeRenderingMode::Off;
    }

    // 结构化网格优先走 compute 光线步进（性能与画质都更好）
    if (DynamicCast<StructuredMesh>(source) != nullptr) {
        std::string reason;
        if (this->SetVolumeRaycasting(true, &reason) ==
            VolumeRaycastingStatus::Ok) {
            IGAME_RENDERING_INFO(
                    "[Scene::SetVolumeRendering] structured mesh, use compute "
                    "ray marching.");
            if (message != nullptr) { message->clear(); }
            return VolumeRenderingMode::Raycast;
        }

        // 结构化网格但当前后端不支持 compute（GL 版本不合适）→ 回退旧实现
        this->SetFragmentVolumeRendering(true);
        IGAME_RENDERING_INFO(
                "[Scene::SetVolumeRendering] compute ray marching unavailable, "
                "use legacy fragment volume rendering.");
        if (message != nullptr) {
            *message = reason + "\n已改用旧的片元链表体绘制实现。";
        }
        return VolumeRenderingMode::Fragment;
    }

    // 非结构化网格（四面体/六面体等）→ 旧的片元链表实现
    this->SetFragmentVolumeRendering(true);
    IGAME_RENDERING_INFO(
            "[Scene::SetVolumeRendering] non-structured mesh, use legacy "
            "fragment volume rendering.");
    if (message != nullptr) {
        *message = "当前数据不是结构化网格，使用旧的片元链表体绘制。通过ResampleToImage Filter可将网格重采样为结构化网格。";
    }
    return VolumeRenderingMode::Fragment;
}

void Scene::UpdateModelsBoundingSphere() {
    // update all models bounding sphere
    igm::vec3 min(FLT_MAX);
    igm::vec3 max(-FLT_MAX);

    auto box = m_Painter3D->GetBoundingBox();
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;

        if (!model->GetVisibility()) { continue; }
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        box.combine(model->GetDataObject()->GetBoundingBox());
        box.combine(model->GetPainter3D()->GetBoundingBox());
    }

    if (box.isNull()) {
        m_ModelsBoundingSphere = igm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
        return;
    }

    Vector3f boxMin = box.min;
    Vector3f boxMax = box.max;
    min = igm::vec3{boxMin[0], boxMin[1], boxMin[2]};
    max = igm::vec3{boxMax[0], boxMax[1], boxMax[2]};

    igm::vec3 center = (min + max) / 2;
    float radius = (max - min).length() / 2;

    m_ModelsBoundingSphere = igm::vec4{center, radius};
}

std::vector<unsigned char> Scene::CaptureScreen(int x, int y, int width,
                                                int height,
                                                GLFramebuffer::Type type,
                                                bool mirrored) {

    std::vector<unsigned char> colorBuffer;

    m_Framebuffer->Bind();
    {
        // Read pixels from the OpenGL buffer (bottom-left corner)
        //
        //  y↑
        //   |
        //   |
        //   +-----→x
        //
        switch (type) {
            case GLFramebuffer::Type::RGB:
                colorBuffer.resize(width * height * 3);
                glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE,
                             colorBuffer.data());
                break;
            case GLFramebuffer::Type::RGBA:
                colorBuffer.resize(width * height * 4);
                glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                             colorBuffer.data());
                break;
            case GLFramebuffer::Type::DEPTH:
                colorBuffer.resize(width * height);
                glReadPixels(x, y, width, height, GL_DEPTH_COMPONENT, GL_FLOAT,
                             colorBuffer.data());
            default:
                break;
        }
    }

    if (mirrored) {
        std::vector<unsigned char> tmp_flip(colorBuffer.size());
        // Flip data Line
        for (int row = 0; row < height; ++row) {
            std::copy(colorBuffer.begin() + row * width * 4,
                      colorBuffer.begin() + (row + 1) * width * 4,
                      tmp_flip.begin() + (height - 1 - row) * width * 4);
        }
        colorBuffer = tmp_flip;
    }
    return colorBuffer;
}

SmartPointer<Painter2D> Scene::GetPainter2D() { return m_Painter2D; }

SmartPointer<Painter3D> Scene::GetPainter3D() { return m_Painter3D; }

void Scene::MakeCurrent() {
    if (m_MakeCurrentFunctor) { m_MakeCurrentFunctor(); }
}

void Scene::DoneCurrent() {
    if (m_DoneCurrentFunctor) { m_DoneCurrentFunctor(); }
}

IGAME_NAMESPACE_END
