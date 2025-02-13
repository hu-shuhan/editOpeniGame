#include "iGameScene.h"
#include "iGameCommand.h"
#include "iGameInteractor.h"
#include "iGameRenderingLogger.h"
#include <chrono>

IGAME_NAMESPACE_BEGIN
Scene::Scene() {
    m_IncrementModelId = 0;
    m_CurrentModelId = 1;
    m_CurrentModel = nullptr;

    m_UpdateFunctor = nullptr;
    m_MakeCurrentFunctor = nullptr;
    m_DoneCurrentFunctor = nullptr;

    m_Camera = Camera::New();
    //m_Light = Light::New();
    m_Axes = Axes::New();

    m_Interactor = Interactor::New();

    m_FontManager = FontManager::New();
    m_ShaderManager = ShaderManager::New();

    m_ModelRotate = igm::mat4{1.0f};
    m_ModelMatrix = igm::mat4{1.0f};
    m_BackgroundColor = {0.5f, 0.5f, 0.5f};

    m_VisibleModelsCount = 0;
    m_ModelsBoundingSphere = igm::vec4{0.0f, 0.0f, 0.0f, 1.0f};

    m_EmptyVAO = GLVertexArray::New();

#ifdef GL_SUPPORTS_MSAA
    samples = 8;
    m_FramebufferMultisampled = GLFramebuffer::New();
    m_ColorTextureMultisampled = GLTexture2dMultisample::New();
    m_DepthTextureMultisampled = GLTexture2dMultisample::New();

    m_FramebufferResolved = GLFramebuffer::New();
    m_ColorTextureResolved = GLTexture2d::New();
    m_DepthTextureResolved = GLTexture2d::New();
#else
    m_Framebuffer = GLFramebuffer::New();
    m_ColorTexture = GLTexture2d::New();
    m_DepthTexture = GLTexture2d::New();
#endif

    m_OITHeadPointerTexture = GLTexture2d::New();
    m_OITHeadPointerInitializer = GLBuffer::New();
    m_OITAtomicCounterBuffer = GLBuffer::New();
    m_OITLinkedListBuffer = GLBuffer::New();
    m_OITLinkedListTexture = GLTextureBuffer::New();

    //m_DrawCullData = GLBuffer::New();
    m_HzbWidth = 0;
    m_HzbHeight = 0;
    m_HzbLevels = 0;
    m_HzbTexture = GLTexture2d::New();

    m_Painter2D = Painter2D::New();
    m_Painter3D = Painter3D::New();

    m_FinishInit = false;
    m_EnableVolumeRendering = false;
}
Scene::~Scene() {}

bool Scene::Initialize() {
    if (m_FinishInit) {
        Logger::LogWarn("Scene is already init.");
        return false;
    }

    InitOpenGL();
    InitOIT();
    InitAxes();

    ResetCameraView();

    m_FinishInit = true;
    return true;
}

int Scene::AddModel(SmartPointer<DataObject> obj) {
    SmartPointer<Model> model = Model::New();
    model->SetDataObject(obj);
    return AddModel(model);
}

int Scene::AddModel(SmartPointer<Model> model) {
    int newModelId = m_IncrementModelId++;
    m_Models.insert(std::make_pair<>(newModelId, model));
    m_CurrentModelId = newModelId;
    m_CurrentModel = model.get();
    model->m_Scene = this;

    ChangeModelVisibility(model.get(), true);

    this->Update();
    return newModelId;
}

void Scene::RemoveModel(int index) {
    auto it = m_Models.find(index);
    if (it == m_Models.end()) {
        Logger::LogWarn("Model with index {} does not exist in the scene.",
                        index);
        return;
    }

    m_Models.erase(index);
    if (index == m_CurrentModelId) {
        if (m_Models.empty()) {
            m_CurrentModelId = -1;
            m_CurrentModel = nullptr;
        } else {
            m_CurrentModelId = m_Models.begin()->first;
            m_CurrentModel = m_Models.begin()->second;
        }
    }
    UpdateModelsBoundingSphere();
}

void Scene::RemoveModel(SmartPointer<Model> model) {
    for (auto it = m_Models.begin(); it != m_Models.end(); ++it) {
        if (it->second.get() == model) {
            m_Models.erase(it);
            if (it->first == m_CurrentModelId) {
                if (m_Models.empty()) {
                    m_CurrentModelId = -1;
                    m_CurrentModel = nullptr;
                } else {
                    m_CurrentModelId = m_Models.begin()->first;
                    m_CurrentModel = m_Models.begin()->second;
                }
            }
            break;
        }
    }
    UpdateModelsBoundingSphere();
}

void Scene::RemoveCurrentModel() {
    if (auto visibility = m_CurrentModel->GetVisibility()) {
        m_VisibleModelsCount--;
    }

    m_CurrentModel->GetDataObject()->InvokeEvent(Command::DeleteEvent);
    m_Models.erase(m_CurrentModelId);
    if (m_Models.empty()) {
        m_CurrentModelId = -1;
        m_CurrentModel = nullptr;
    } else {
        m_CurrentModelId = m_Models.begin()->first;
        m_CurrentModel = m_Models.begin()->second;
    }
    UpdateModelsBoundingSphere();
}

void Scene::SetCurrentModel(int index) {
    auto it = m_Models.find(index);
    if (it == m_Models.end()) {
        Logger::LogWarn("Model with index {} does not exist in the scene.",
                        index);
        return;
    }

    for (auto& [id, model]: m_Models) {
        if (id == index) {
            m_CurrentModelId = id;
            m_CurrentModel = model.get();
            return;
        }
    }
}

void Scene::SetCurrentModel(SmartPointer<Model> model) {
    for (auto& [id, m]: m_Models) {
        if (m == model) {
            m_CurrentModelId = id;
            m_CurrentModel = m.get();
            return;
        }
    }
}

void Scene::SetBackGround(const Color& color) {
    auto c = ColorUtils::Map(color);
    m_BackgroundColor = c;
    this->Modified();
}

void Scene::SetInteractor(SmartPointer<Interactor> interactor) {
    m_Interactor = interactor;
}

SmartPointer<Interactor> Scene::GetInteractor() { return m_Interactor; }

SmartPointer<Model> Scene::GetCurrentModel() { return m_CurrentModel; }

SmartPointer<Model> Scene::GetModelById(int index) {
    for (auto& [id, model]: m_Models) {
        if (id == index) { return model; }
    }
    return nullptr;
}

SmartPointer<DataObject> Scene::GetDataObjectById(int index) {
    for (auto& [id, model]: m_Models) {
        if (id == index) { return model->m_DataObject; }
    }
    return nullptr;
}

std::map<int, SmartPointer<Model>>& Scene::GetModelList() { return m_Models; }

void Scene::ChangeModelVisibility(int index, bool visibility) {
    auto model = GetModelById(index);
    if (model != nullptr) { ChangeModelVisibility(model, visibility); }
}

void Scene::ResetCameraView() {
    UpdateModelsBoundingSphere();
    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
    float radius = m_ModelsBoundingSphere.w;

    m_ModelMatrix = igm::mat4{1.0f};
    m_ModelRotate = igm::mat4{1.0f};
    m_Camera->SetPosition(center.x, center.y, center.z + 3.0f * radius);
    m_Camera->SetClippngRange(2.0f * radius, 4.0f * radius);
    m_Camera->SetFocal(center);
}

void Scene::ChangeModelVisibility(SmartPointer<Model> model, bool visibility) {
    auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
    drawObject->SetVisibility(visibility);

    if (visibility) {
        m_VisibleModelsCount++;
        if (m_VisibleModelsCount == 1) { ResetCameraView(); }
    } else {
        m_VisibleModelsCount--;
    }

    UpdateModelsBoundingSphere();
}

SmartPointer<Camera> Scene::GetCamera() { return m_Camera; }

void Scene::ChangeCameraType(Camera::Type type) {
    ResetCameraView();
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
    if (!gladLoadGL()) { Logger::LogError("Failed to initialize GLAD"); }

    // log opengl info
    {
        Logger::LogDebug(
                "==================== OpenGL Info ====================");
        const GLubyte* vendor = glGetString(GL_VENDOR);
        Logger::LogDebug("Vendor:   {}", reinterpret_cast<const char*>(vendor));
        const GLubyte* renderer = glGetString(GL_RENDERER);
        Logger::LogDebug("Renderer:   {}",
                         reinterpret_cast<const char*>(renderer));
        const GLubyte* version = glGetString(GL_VERSION);
        Logger::LogDebug("Version:   {}",
                         reinterpret_cast<const char*>(version));
        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        Logger::LogDebug(
                "=====================================================");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    //glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // reversed-z buffer, depth range: 1.0(near plane) -> 0.0(far plane)
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

    // create empty VAO to render full-screen triangle
    m_EmptyVAO->Create();

    // initilize shader
    m_ShaderManager->Initialize();

    // init framebuffer
    ResizeFrameBuffer();

    // painter2d test
    {
        m_Painter2D->SetPen(Color::Red);
        m_Painter2D->SetPen(5);
        m_Painter2D->SetBrush(Color::Green);

        //m_Painter2D->DrawPoint({300, 300});
        //m_Painter2D->DrawLine({100, 100}, {200, 200});
        //m_Painter2D->DrawTriangle({100, 100}, {200, 100}, {100, 200});
        //m_Painter2D->DrawRect({100, 100}, {200, 200});
        //m_Painter2D->DrawCircle({100, 100}, 100, 100);
    }

    // painter3d test
    {
        m_Painter3D->SetPen(Color::Red);
        m_Painter3D->SetPen(5);
        m_Painter3D->SetBrush(Color::Green);

        //m_Painter3D->DrawPoint({-1.0f, -1.0f, 0.0f});
        //m_Painter3D->DrawLine({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
        //m_Painter3D->DrawTriangle({-1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f},
        //                          {1.0f, -1.0f, 0.0f});
        //m_Painter3D->DrawRect({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f});
        //m_Painter3D->DrawCube({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, -1.0f});
        //m_Painter3D->DrawCircle({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 1,
        //                        100);
        //m_Painter3D->DrawSphere({0.0f, 0.0f, 0.0f}, 1.0f, 100, 100);
        //m_Painter3D->DrawIcoSphere({0.0f, 0.0f, 0.0f}, 1.0f, 5);
        //m_Painter3D->DrawCubeSphere({0.0f, 0.0f, 0.0f}, 1.0f, 8);
        //m_Painter3D->DrawCylinder({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1,
        //                          1.0f, 16);
        //m_Painter3D->DrawCone({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1, 1.0f,
        //                      16);
        //m_Painter3D->DrawPyramid({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1,
        //                         1.0f, 8, 8);
        //m_Painter3D->DrawFrustum({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1.0f,
        //                         1.0f, 0.5f, 8);
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

#ifdef GL_SUPPORTS_MSAA
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
            Logger::LogError("{}, framebuffer is not complete!",
                             this->GetName());
        }
    }

    // resize resolve framebuffer(form multisamples to single sample)
    {
        auto width = m_Camera->GetScaledViewPort().x;
        auto height = m_Camera->GetScaledViewPort().y;
        //int mipLevels =
        //        static_cast<int>(std::ceil(std::log2(std::max(width, height))));

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
        depthTexture->Storage(1, GL_R32F, width, height);
        depthTexture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        depthTexture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        depthTexture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        depthTexture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        fbo->Texture(GL_COLOR_ATTACHMENT1, depthTexture, 0);

        GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        fbo->DrawBuffers(2, buffers);

        fbo->Release();

        m_ColorTextureResolved = colorTexture;
        m_DepthTextureResolved = depthTexture;
        m_FramebufferResolved = fbo;

        if (m_FramebufferResolved->CheckStatus() != GL_FRAMEBUFFER_COMPLETE) {
            Logger::LogError("{}, framebuffer is not complete!",
                             this->GetName());
        }
    }
#else
    //resize resolve framebuffer(form multisamples to single sample)
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

        m_ColorTexture = colorTexture;
        m_DepthTexture = depthTexture;
        m_Framebuffer = fbo;

        if (m_Framebuffer->CheckStatus() != GL_FRAMEBUFFER_COMPLETE) {
            Logger::LogError("{}, framebuffer is not complete!",
                             this->GetName());
        }
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
    // reset camera
    UpdateCameraClippingRange();

    // save default framebuffer, because it is not 0 in Qt
    GLint defaultFramebuffer = GL_NONE;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFramebuffer);

#ifdef GL_SUPPORTS_MSAA
    // render to multisample framebuffer
    m_FramebufferMultisampled->Bind();
    DrawFrame();
    m_FramebufferMultisampled->Release();

    // resolve to single sample framebuffer
    m_FramebufferResolved->Bind();
    ResolveFrame();
    m_FramebufferResolved->Release();

    // render to qt framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
    RenderToQtFrame();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#else
    // render to my framebuffer
    m_Framebuffer->Bind();
    DrawFrame();
    m_Framebuffer->Release();

    // render to qt framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
    RenderToQtFrame();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    CalculateFrameRate();
    GLCheckError();
}

void Scene::RefreshHzb() {
#ifdef IGAME_OPENGL_VERSION_460
    auto shader = this->GetShader(ShaderType::DEPTHREDUCE);
    shader->Use();
    m_DepthTextureMultisampled->Active(GL_TEXTURE1);
    m_HzbTexture->Active(GL_TEXTURE2);
    shader->SetUniformi("screenDepthMS", 1);
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
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
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
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
#endif
}

void Scene::Update() {
    if (m_UpdateFunctor) { m_UpdateFunctor(); }
}

void Scene::Resize(int width, int height, int pixelRatio) {
    m_Camera->SetViewPort(width, height);
    m_Camera->SetDevicePixelRatio(pixelRatio);
    ResizeFrameBuffer();
}

void Scene::DrawFrame() {
    auto viewport = m_Camera->GetScaledViewPort();

    // convert to drawable data

#ifndef GL_DEBUG_CULLING
    for (auto& [id, model]: m_Models) {
        if (!model->m_DataObject->IsDrawable()) { continue; }

        auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
        drawObject->ReAllocateDisplayBuffer();
    }
#endif

    // update camera data block in GPU
    UpdateCameraDataBlock();

    // draw models, render to multisample framebuffer
    {
        glViewport(0, 0, viewport.x, viewport.y);

        // reversed-z buffer, depth range: 1.0(near plane) -> 0.0(far plane)
        glClearColor(m_BackgroundColor.r, m_BackgroundColor.g,
                     m_BackgroundColor.b, 1.0f);
        glClearDepth(0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use reversed-z buffer
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);

        if (!m_EnableVolumeRendering) {
            ShadowPass();
            ForwardPass();
            TransparentPass();
        } else {
            VolumeRenderingPass();
        }

        // draw scene painter
        m_Painter2D->Draw(this);
        m_Painter3D->Draw(this);
    }

    // draw axes in bottom left
    {
        // Note: If depth rendering is enabled, please comment out this line to preserve depth information.
        glClear(GL_DEPTH_BUFFER_BIT);
        m_Axes->Draw(this);
    }
}

void Scene::ResolveFrame() {
#ifdef GL_SUPPORTS_MSAA
    auto viewport = m_Camera->GetScaledViewPort();
    glViewport(0, 0, viewport.x, viewport.y);
    glDisable(GL_DEPTH_TEST);

    auto shader = this->GetShader(ShaderType::ATTACHMENTRESOLVE);
    shader->Use();

    shader->SetUniformi("numSamples", samples);
    m_ColorTextureMultisampled->Active(GL_TEXTURE1);
    shader->SetUniformi("colorTextureMS", 1);
    m_DepthTextureMultisampled->Active(GL_TEXTURE2);
    shader->SetUniformi("depthTextureMS", 2);

    m_EmptyVAO->DrawArrays(GL_TRIANGLES, 0, 3);
#endif
}

void Scene::RenderToQtFrame() {
    auto viewport = m_Camera->GetScaledViewPort();

    glViewport(0, 0, viewport.x, viewport.y);
    glDisable(GL_DEPTH_TEST);

    auto shader = this->GetShader(ShaderType::SCREEN);
    //auto shader = GetShader(Scene::FXAA);
    shader->Use();

#ifdef GL_SUPPORTS_MSAA
    m_ColorTextureResolved->GenerateMipmap();
    m_ColorTextureResolved->Active(GL_TEXTURE1);
    m_DepthTextureResolved->Active(GL_TEXTURE2);
    m_HzbTexture->Active(GL_TEXTURE3);

    // Note:
    // 1. To enable depth rendering, ensure the screen.frag file is updated to handle the depth texture input.
    //    Specifically, the shader should read from the depth sampler and implement the desired depth-based operations.
    // 2. Additionally, disable any depth-buffer-clearing code in the coordinate axis rendering logic.
    //    Failing to do so could overwrite or invalidate the depth information required for rendering.
    shader->SetUniformi("screenColorSampler", 1);
    //shader->SetUniformf("near", m_Camera->GetClippingRange().x);
    //shader->SetUniformf("far", m_Camera->GetClippingRange().y);
#else
    m_ColorTexture->Active(GL_TEXTURE1);
    m_DepthTexture->Active(GL_TEXTURE2);
    m_HzbTexture->Active(GL_TEXTURE3);
    shader->SetUniformi("screenColorSampler", 1);
#endif

    m_EmptyVAO->DrawArrays(GL_TRIANGLES, 0, 3);
}

void Scene::ShadowPass() { GLCheckError(); }

void Scene::ForwardPass() {
#ifdef IGAME_OPENGL_VERSION_330
    for (auto& [id, model]: m_Models) {
        model->Draw(this);
        model->GetPainter3D()->Draw(this);
    }
#elif IGAME_OPENGL_VERSION_460

#ifdef GL_DEBUG_CULLING
#ifdef GL_SUPPORTS_MESH_SHADER
    // draw phase1: draw visible meshlet
    // Note: The first HZB culling pass must use the previous frame's data
    for (auto& [id, model]: m_Models) {
        auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
        if (drawObject->GetTransparency() == 1.0f) { model->DrawPhase1(this); }
    }

    // refresh phase 1: generate loacl hierarchical z-buffer & cull data
    RefreshDepthHzb();
    RefreshDrawCullDataBuffer();

    // draw phase2: draw invisible meshlet
    for (auto& [id, model]: m_Models) {
        auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
        if (drawObject->GetTransparency() == 1.0f) { model->DrawPhase2(this); }
    }

    // refresh phase2: generate global hierarchical z-buffer
    RefreshDepthHzb();
#else
    for (auto& [id, model]: m_Models) {
        auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
        if (drawObject->GetTransparency() == 1.0f) {
            model->TestOcclusionResults(this);
        }
    }

    // draw phase1: draw visible meshlet
    for (auto& [id, model]: m_Models) {
        auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
        if (drawObject->GetTransparency() == 1.0f) { model->DrawPhase1(this); }
    }

    // refresh phase1: generate loacl hierarchical z-buffer
    RefreshDepthHzb();
    RefreshDrawCullDataBuffer();

    // draw phase2: draw invisible meshlet
    for (auto& [id, model]: m_Models) {
        auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
        if (drawObject->GetTransparency() == 1.0f) { model->DrawPhase2(this); }
    }

    // refresh phase2: generate global hierarchical z-buffer
    RefreshDepthHzb();
#endif // GL_SUPPORTS_MESH_SHADER
#else
    for (auto& [id, model]: m_Models) {
        // draw mesh
        auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
        if (drawObject->GetTransparency() == 1.0f) { model->Draw(this); }

        // draw painter(since painter does not support transparency)
        if (drawObject->GetVisibility()) { model->GetPainter3D()->Draw(this); }
    }
#endif // GL_DEBUG_CULLING

#endif
    GLCheckError();
}

void Scene::TransparentPass() {
#ifdef IGAME_OPENGL_VERSION_460
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
        for (auto& [id, model]: m_Models) {
            auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
            if (drawObject->GetTransparency() != 1.0f) {
                model->DrawWithTransparency(this);
            }
        }
    }
    glDepthMask(GL_TRUE);

    // 3.sorting and blending colors
    glDisable(GL_DEPTH_TEST);
    {
        auto shader = this->GetShader(ShaderType::TRANSPARENCYSORT);
        shader->Use();

        shader->SetUniformi("numSamples", samples);
        m_ColorTextureMultisampled->Active(GL_TEXTURE1);
        shader->SetUniformi("forwardPassColorMS", 1);

        m_OITHeadPointerTexture->BindImage(0, 0, GL_FALSE, 0, GL_READ_ONLY,
                                           GL_R32UI);
        m_OITLinkedListTexture->BindImage(1, 0, GL_FALSE, 0, GL_READ_ONLY,
                                          GL_RGBA32UI);

        m_EmptyVAO->DrawArrays(GL_TRIANGLES, 0, 3);
    }
    glEnable(GL_DEPTH_TEST);
#endif
    GLCheckError();
}

void Scene::VolumeRenderingPass() {
#ifdef IGAME_OPENGL_VERSION_460
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
        for (auto& [id, model]: m_Models) { model->DrawWithVolume(this); }
    }
    glDepthMask(GL_TRUE);

    // 3.sorting and blending colors
    glDisable(GL_DEPTH_TEST);
    {
        auto shader = this->GetShader(ShaderType::VOLUMERENDERINGSORT);
        shader->Use();

        shader->SetUniformi("numSamples", samples);
        m_ColorTextureMultisampled->Active(GL_TEXTURE1);
        shader->SetUniformi("forwardPassColorMS", 1);

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
    float radius = m_ModelsBoundingSphere.w;
    igm::vec3 cameraPos = m_Camera->GetPosition();

    igm::vec3 front = m_Camera->GetFront();
    igm::vec3 v = center - cameraPos;
    float dist = std::abs(front.dot(v) / front.length());

    float nearPlane = dist - radius;
    float farPlane = dist + radius;

    // https://3dgumshoe.com/maya-fixing-z-fighting/
    const float minGap = 0.0001f;
    if (nearPlane < minGap * farPlane) { nearPlane = minGap * farPlane; }

    m_Camera->SetClippngRange(nearPlane, farPlane);
}

void Scene::RefreshDrawCullDataBuffer() {
    m_ShaderManager->UpdateCullDataBuffer(m_Camera, m_ModelMatrix, m_HzbWidth,
                                          m_HzbHeight);
}

void Scene::ResetCameraViewToPositiveX() {
    ResetCameraView();

    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
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

    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
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

    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
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

    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
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

    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
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

    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -center);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, center);

    auto rotate = igm::mat4{};
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::ResetCameraViewToIsometric() {
    ResetCameraView();

    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
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

void Scene::RotateNinetyClockwise() {
    igm::vec4 center = igm::vec4{m_ModelsBoundingSphere.xyz(), 1.0f};
    igm::vec3 centerInWorld = (m_ModelMatrix * center).xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -centerInWorld);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, centerInWorld);

    auto radians = static_cast<float>(igm::radians(90.0f));
    auto rotate =
            igm::rotate(igm::mat4{}, -radians, igm::vec3{0.0f, 0.0f, 1.0f});
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::RotateNinetyCounterClockwise() {
    igm::vec4 center = igm::vec4{m_ModelsBoundingSphere.xyz(), 1.0f};
    igm::vec3 centerInWorld = (m_ModelMatrix * center).xyz();
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -centerInWorld);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, centerInWorld);

    auto radians = static_cast<float>(igm::radians(90.0f));
    auto rotate =
            igm::rotate(igm::mat4{}, radians, igm::vec3{0.0f, 0.0f, 1.0f});

    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}

void Scene::SetVolumeRendering(bool toggled) {
    m_EnableVolumeRendering = toggled;
    for (auto& [id, model]: m_Models) {
        if (!model->m_DataObject->IsDrawable()) { continue; }
        auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
        drawObject->SetShellRenderingOption(!toggled);
    }
    Update();
}

void Scene::UpdateModelsBoundingSphere() {
    // update all models bounding sphere
    igm::vec3 min(FLT_MAX);
    igm::vec3 max(-FLT_MAX);

    auto box = m_Painter3D->GetBoundingBox();
    for (auto& [id, model]: m_Models) {
        if (!model->GetVisibility()) { continue; }
        box.combine(model->m_DataObject->GetBoundingBox());
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

void Scene::CalculateFrameRate() {
    static float framesPerSecond = 0.0f; // This will store our fps
    static auto lastTime = std::chrono::high_resolution_clock ::now();

    auto currentTime = std::chrono::high_resolution_clock ::now();
    ++framesPerSecond;

    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                         currentTime - lastTime)
                         .count();
    if (time > 1.0f) {
        lastTime = currentTime;
        //std::cout << framesPerSecond << std::endl;
        framesPerSecond = 0;
    }
}

std::vector<unsigned char> Scene::CaptureScreen(int x, int y, int width,
                                                int height,
                                                GLFramebuffer::Type type,
                                                bool mirrored) {

    std::vector<unsigned char> colorBuffer;

    m_FramebufferResolved->Bind();
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
    m_FramebufferResolved->Release();

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