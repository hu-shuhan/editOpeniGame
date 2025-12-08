#include "iGameScene.h"
#include "iGameCommand.h"
#include "iGameInteractor.h"
#include "iGameRenderingLogger.h"
#include <algorithm>
#include <chrono>

IGAME_NAMESPACE_BEGIN
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

    m_Interactor = Interactor::New();
    m_FontManager = FontManager::New();
    m_ShaderManager = ShaderManager::New();

    m_ModelRotate = igm::mat4{1.0f};
    m_ModelMatrix = igm::mat4{1.0f};
    // m_BackgroundColor = {0.247f, 0.247f, 0.247f};
    m_BackgroundColor = {0.5f, 0.5f, 0.5f};

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
    if (m_FinishInit) { glDeleteQueries(2, m_TimeQueries); }
}

void Scene::BindFramebuffer() const {
#ifdef GL_SUPPORT_MSAA
    m_FramebufferMultisampled->Bind();
#else
    m_Framebuffer->Bind();
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
    this->InitOIT();
    this->InitAxes();

    this->ResetCameraView();

    // 添加中心坐标轴到模型池
    m_CenterAxesModel->AddViewStyle(
            IG_WIREFRAME);                   // 添加线框视图样式（默认不显示线）
    m_CenterAxesModel->SetAlwaysOnTop(true); // 设置为总在最上层
    m_CenterAxesModel->ConvertToDrawableData(); // 初始化几何数据
    m_CenterAxesModel->SyncGpuBuffers();        // 上传GPU数据
    this->AddModel(m_CenterAxesModel);          // 加入模型池
    m_CenterAxesModel->SetVisibility(m_CenterAxesVisible);
    // 添加中心坐标轴到模型池

    m_FinishInit = true;
    return true;
}

IGuint Scene::AddModel(SmartPointer<DataObject> obj) {
    SmartPointer<Model> model = Model::New();
    model->SetDataObject(obj);
    model->SetScene(this);

    auto modelID = m_ModelPool->AllocateObject(model);
    m_CurrentModelID = modelID;

    ChangeModelVisibility(model, true);
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

SmartPointer<Model> Scene::GetModelById(int id) {
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto modelID = it->first;
        auto model = it->second;

        if (modelID == id) { return model; }
    }
    return nullptr;
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
    auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
    drawObject->SetVisibility(visibility);

    if (visibility) {
        m_VisibleModelsCount++;
        if (m_VisibleModelsCount == 2 || m_VisibleModelsCount == 1) {
            ResetCameraView(model->GetDataObject());
            UpdateAxisSize();
        } // CenterAxesModel is visible
    } else {
        m_VisibleModelsCount--;
    }

    UpdateCameraClippingRange();
}

void Scene::SetBackGround(const Color& color) {
    auto c = ColorUtils::Map(color);
    m_BackgroundColor = c;
    this->Modified();
}

void Scene::SetBackGround(int R, int G, int B) {
    auto c = ColorUtils::Map(R, G, B);
    m_BackgroundColor = c;
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
    if (!gladLoadGL()) { IGAME_RENDERING_ERROR("Failed to initialize GLAD"); }

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
    GLuint* data;
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
            RenderToSpecificFrame(defaultFramebuffer);
            return;
        }
    }

    // reset camera
    UpdateCameraClippingRange();

    // Start counting the rendering time consumption
    int curIdx = m_TimeQueryIndex;
    glBeginQuery(GL_TIME_ELAPSED, m_TimeQueries[curIdx]);

    // render
    DrawFrame();
    RenderToSpecificFrame(defaultFramebuffer);

    // End counting the rendering time consumption
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

    // Record the end time of this frame
    m_LastRenderEnd = std::chrono::steady_clock::now();
    m_LastRenderEndValid = true;

    GLCheckError();
}

void Scene::RefreshHzb() {
#ifdef IGAME_OPENGL_VERSION_460
    auto shader = this->GetShader(ShaderType::DEPTHREDUCE);
    shader->Use();

    ResolveFrameBuffer();

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

void Scene::DrawFrame() {
    auto viewport = m_Camera->GetScaledViewPort();

    // Convert to drawable data
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        model->SyncGpuBuffers();
    }

    // Update camera data block in GPU
    UpdateCameraDataBlock();
    {
        auto ClearFramebuffer = [&](float depth = 0.0f) {
            glClearColor(m_BackgroundColor.r, m_BackgroundColor.g,
                         m_BackgroundColor.b, 1.0f);
            glClearDepth(depth); // reversed-z: near=1.0, far=0.0
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        };

        // Clear default framebuffer before rendering
        m_Framebuffer->Bind();
        ClearFramebuffer();

#ifdef GL_SUPPORT_MSAA
        m_FramebufferMultisampled->Bind();
        ClearFramebuffer();
#endif

        // Draw scene painter
        glViewport(0, 0, viewport.x, viewport.y);
        m_Painter2D->Draw();
        m_Painter3D->Draw();

        // Draw axes in bottom left
        if(m_AxesVisible){
            int mx = std::max(viewport.x, viewport.y);
            glViewport(0, 0, mx / 10, mx / 10);
            m_Axes->Draw();
        }


        // Render to framebuffer
        glViewport(0, 0, viewport.x, viewport.y);
#ifdef IGAME_OPENGL_VERSION_330
        ShadowPass();
        ForwardPass();
#else
        if (!m_EnableVolumeRendering) {
            ShadowPass();
            ForwardPass();
            TransparentPass();
        } else {
            VolumeRenderingPass();
        }
#endif
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
    glDepthFunc(GL_GEQUAL);

    glDisable(GL_DEPTH_TEST);

    GLCheckError();
}

void Scene::ForwardPass() {
    // Use reversed-z buffer
    glDepthFunc(GL_GREATER);
    glEnable(GL_DEPTH_TEST);

#ifdef IGAME_OPENGL_VERSION_330
    BindFramebuffer();
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;
        model->Draw();
        model->GetPainter3D()->Draw();
    }
#elif IGAME_OPENGL_VERSION_460
    // normal mesh
    BindFramebuffer();
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;

        // draw mesh
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        if (!drawObject->IsAlwaysOnTop()) { model->Draw(); }

        // draw painter(since painter does not support transparency)
        if (drawObject->GetVisibility()) {
            auto& painter3Ds = model->GetAllPainter3Ds();
            for (auto& painter: painter3Ds) { painter.second->Draw(); }
        }
    }

    // 第二次遍历：专门渲染AlwaysOnTop模型（最后绘制）
    glDisable(GL_DEPTH_TEST);
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;

        // draw mesh
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        if (drawObject->IsAlwaysOnTop()) { model->Draw(); }
    }
    glEnable(GL_DEPTH_TEST);

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

    ResolveFrameBuffer();
    glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
    GLCheckError();
}

void Scene::TransparentPass() {
#ifdef IGAME_OPENGL_VERSION_460
    // Bind framebuffer
    m_Framebuffer->Bind();

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

        m_ColorTexture->Active(GL_TEXTURE1);
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
    m_Framebuffer->Bind();

    // use reversed-z buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);

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

    // 3.sorting and blending colors
    glDisable(GL_DEPTH_TEST);
    {
        auto shader = this->GetShader(ShaderType::VOLUMERENDERINGSORT);
        shader->Use();

        m_ColorTexture->Active(GL_TEXTURE1);
        shader->SetUniformi("forwardPassColor", 1);

        m_OITHeadPointerTexture->BindImage(0, 0, GL_FALSE, 0, GL_READ_ONLY,
                                           GL_R32UI);
        m_OITLinkedListTexture->BindImage(1, 0, GL_FALSE, 0, GL_READ_ONLY,
                                          GL_RGBA32UI);

        m_EmptyVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_EmptyVAO->Release();
    }
    glEnable(GL_DEPTH_TEST);
#endif
    glDisable(GL_DEPTH_TEST);
    GLCheckError();
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

    // https://3dgumshoe.com/maya-fixing-z-fighting/
    const float minGap = 0.0001f;
    if (nearPlane < minGap * farPlane) { nearPlane = minGap * farPlane; }

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
    m_CenterAxesModel->SetVisibility(m_CenterAxesVisible);
}
void Scene::ToggleAxes() {
    m_AxesVisible = !m_AxesVisible;
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

void Scene::SetVolumeRendering(bool toggled) {
    m_EnableVolumeRendering = toggled;
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;

        if (!model->GetDataObject()->IsDrawable()) { continue; }
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        drawObject->SetShellRenderingOption(!toggled);
    }
    Update();
}

void Scene::UpdateModelsBoundingSphere() {
    // update all models bounding sphere
    igm::vec3 min(FLT_MAX);
    igm::vec3 max(-FLT_MAX);

    auto box = m_Painter3D->GetBoundingBox();
    for (auto it = m_ModelPool->Begin(); it != m_ModelPool->End(); ++it) {
        auto model = it->second;

        if (!model->GetVisibility()) { continue; }
        //坐标轴不计算包围盒
        auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());
        if (drawObject->IsAlwaysOnTop()) { continue; }
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
