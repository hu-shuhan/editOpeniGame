#include "iGameScene.h"
#include "iGameCommand.h"
#include "iGameInteractor.h"
#include <chrono>

IGAME_NAMESPACE_BEGIN
Scene::Scene() {
    m_Camera = Camera::New();
    m_Camera->Initialize(igm::vec3{0.0f, 0.0f, 1.0f});

    m_ModelRotate = igm::mat4{};
    m_ModelMatrix = igm::mat4{};
    m_BackgroundColor = {0.5f, 0.5f, 0.5f};

    m_CameraDataBlock = GLBuffer::New();
    m_ObjectDataBlock = GLBuffer::New();
    m_UBOBlock = GLBuffer::New();

    m_EmptyVAO = GLVertexArray::New();

#ifdef MSAA
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

    m_DrawCullData = GLBuffer::New();
    m_DepthPyramid = GLTexture2d::New();
}
Scene::~Scene() {}

bool Scene::Initialize() {
    if (m_FinishInit) {
        std::cout << "Scene is already init.\n";
        return false;
    }

    InitOpenGL();
    InitOIT();
    InitFont();
    InitAxes();
    m_FinishInit = true;

    return true;
}

int Scene::AddModel(DataObject::Pointer obj) {
    Model::Pointer model = Model::New();
    model->m_DataObject = obj;
    return AddModel(model);
}
int Scene::AddModel(Model::Pointer model) {
    int newModelId = m_IncrementModelId++;
    m_Models.insert(std::make_pair<>(newModelId, model));
    m_CurrentModelId = newModelId;
    m_CurrentModel = model.get();
    model->m_Scene = this;

    ChangeModelVisibility(model.get(), true);

    this->Update();
    return newModelId;
}

Model::Pointer Scene::CreateModel(DataObject::Pointer obj) {
    Model::Pointer model = Model::New();
    model->m_DataObject = obj;
    return model;
}

void Scene::RemoveModel(int index) {
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

void Scene::RemoveModel(Model* model) {
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
    for (auto& [id, model]: m_Models) {
        if (id == index) {
            m_CurrentModelId = id;
            m_CurrentModel = model.get();
            return;
        }
        //if (obj->m_DataObject->Has->SubDataObject()) {
        //    auto subObj = obj->m_DataObject->Get->SubDataObject(index);
        //    if (subObj != nullptr) {
        //        m_CurrentModelId = index;
        //        m_CurrentObject = subObj.get();
        //        return true;
        //    }
        //}
    }
}

void Scene::SetCurrentModel(Model* _model) {
    for (auto& [id, model]: m_Models) {
        if (model == _model) {
            m_CurrentModelId = id;
            m_CurrentModel = model.get();
            return;
        }
    }
}

void Scene::SetInteractor(Interactor* i) { m_Interactor = i; }

Interactor* Scene::GetInteractor() { return m_Interactor; }

Model* Scene::GetCurrentModel() { return m_CurrentModel; }

Model* Scene::GetModelById(int index) {
    for (auto& [id, model]: m_Models) {
        if (id == index) { return model; }
        //if (obj->m_DataObject->Has->SubDataObject()) {
        //    auto subObj = obj->m_DataObject->Get->SubDataObject(index);
        //    if (subObj != nullptr) { return subObj.get(); }
        //}
    }
    return nullptr;
}

DataObject* Scene::GetDataObjectById(int index) {
    for (auto& [id, model]: m_Models) {
        if (id == index) { return model->m_DataObject; }
    }
    return nullptr;
}

std::map<int, Model::Pointer>& Scene::GetModelList() { return m_Models; }

void Scene::ChangeModelVisibility(int index, bool visibility) {
    auto* model = GetModelById(index);
    if (model != nullptr) { ChangeModelVisibility(model, visibility); }
}

void Scene::ResetCameraView() {
    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
    float radius = m_ModelsBoundingSphere.w;

    m_ModelMatrix = igm::mat4{1.0f};
    m_ModelRotate = igm::mat4{1.0f};
    m_Camera->SetCameraPos(center.x, center.y, center.z + 3.0f * radius);
    m_Camera->SetNearPlane(2.0f * radius);
    m_Camera->SetFarPlane(4.0f * radius);
    m_Camera->SetCameraFocal(center);
}

void Scene::ChangeModelVisibility(Model* model, bool visibility) {
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

void Scene::ChangeCameraType(IGenum type) {
    ResetCameraView();
    switch (type) {
        case Camera::CameraType::PERSPECTIVE: {
            m_Camera->ChangeCameraType(Camera::CameraType::PERSPECTIVE);
        } break;
        case Camera::CameraType::ORTHOGRAPHIC: {
            m_Camera->ChangeCameraType(Camera::CameraType::ORTHOGRAPHIC);
        } break;
        default:
            break;
    }
}

GLShaderProgram::Pointer Scene::GetShader(IGenum type) {
    GLShaderProgram::Pointer sp = this->GetShaderWithType(type);
    if (sp != nullptr) { return sp; }

    sp = this->GenShader(type);
    if (sp == nullptr) {
        // std::cout << "Error for GenShader\n";
    }
    this->SetShader(type, sp);
    return sp;
}

GLShaderProgram::Pointer Scene::GetShaderWithType(IGenum type) {
    auto it = m_ShaderPrograms.find(type);
    if (it == m_ShaderPrograms.end()) { return nullptr; }
    return it->second;
}

GLShaderProgram::Pointer Scene::GenShader(IGenum type) {
    GLShaderProgram::Pointer sp = GLShaderProgram::New();
    switch (type) {
        case BLINNPHONG: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer blinnPhong_frag = CreateShader(
                    std::string("./Resources/Shaders/blinnPhong.frag"),
                    GL_FRAGMENT_SHADER);
            sp->AddShaders(vertex_vert, blinnPhong_frag);
        } break;
        case PBR: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer pbr_frag =
                    CreateShader(std::string("./Resources/Shaders/pbr.frag"),
                                 GL_FRAGMENT_SHADER);
            sp->AddShaders(vertex_vert, pbr_frag);
        } break;
        case NOLIGHT: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer noLight_frag = CreateShader(
                    std::string("./Resources/Shaders/noLight.frag"),
                    GL_FRAGMENT_SHADER);
            sp->AddShaders(vertex_vert, noLight_frag);
        } break;
        case PURECOLOR: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer pureColor_frag = CreateShader(
                    std::string("./Resources/Shaders/pureColor.frag"),
                    GL_FRAGMENT_SHADER);
            sp->AddShaders(vertex_vert, pureColor_frag);
        } break;
        case SINGLEPASSWIREFRAME: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer wireframe_geom = CreateShader(
                    std::string(
                            "./Resources/Shaders/single-passWireframe.geom"),
                    GL_GEOMETRY_SHADER);
            GLShader::Pointer wireframe_frag = CreateShader(
                    std::string(
                            "./Resources/Shaders/single-passWireframe.frag"),
                    GL_FRAGMENT_SHADER);
            sp->AddShaders(vertex_vert, wireframe_geom, wireframe_frag);
        } break;
        case TRANSPARENCYLINK: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer transparencyLink_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "transparencyLink.frag"),
                                 GL_FRAGMENT_SHADER);
            sp->AddShaders(vertex_vert, transparencyLink_frag);
        } break;
        case TRANSPARENCYSORT: {
            GLShader::Pointer fullScreenTriangle_vert =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "fullScreenTriangle.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer transparencySort_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "transparencySort.frag"),
                                 GL_FRAGMENT_SHADER);
            sp->AddShaders(fullScreenTriangle_vert, transparencySort_frag);
        } break;
        case VOLUMERENDERINGLINK: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer volumeRenderingLink_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "volumeRenderingLink.frag"),
                                 GL_FRAGMENT_SHADER);
            sp->AddShaders(vertex_vert, volumeRenderingLink_frag);
        } break;
        case VOLUMERENDERINGSORT: {
            GLShader::Pointer fullScreenTriangle_vert =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "fullScreenTriangle.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer volumeRenderingSort_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "volumeRenderingSort.frag"),
                                 GL_FRAGMENT_SHADER);
            sp->AddShaders(fullScreenTriangle_vert, volumeRenderingSort_frag);
        } break;
        case AXES: {
            GLShader::Pointer axis_vert =
                    CreateShader(std::string("./Resources/Shaders/axis.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer axis_frag =
                    CreateShader(std::string("./Resources/Shaders/axis.frag"),
                                 GL_FRAGMENT_SHADER);
            sp->AddShaders(axis_vert, axis_frag);
        } break;
        case FONT: {
            GLShader::Pointer font_vert =
                    CreateShader(std::string("./Resources/Shaders/font.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer font_frag =
                    CreateShader(std::string("./Resources/Shaders/font.frag"),
                                 GL_FRAGMENT_SHADER);
            sp->AddShaders(font_vert, font_frag);
        } break;
        case ATTACHMENTRESOLVE: {
            GLShader::Pointer attachmentResolve_vert =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "attachmentResolve.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer attachmentResolve_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "attachmentResolve.frag"),
                                 GL_FRAGMENT_SHADER);
            sp->AddShaders(attachmentResolve_vert, attachmentResolve_frag);
        } break;
        case DEPTHREDUCE: {
#ifdef IGAME_OPENGL_VERSION_460
            GLShader::Pointer depthReduce_comp = CreateShader(
                    std::string("./Resources/Shaders/depthReduce.comp"),
                    GL_COMPUTE_SHADER);
            sp->AddShaders(depthReduce_comp);
#endif
        } break;
        case MESHLETCULL: {
#ifdef IGAME_OPENGL_VERSION_460
            GLShader::Pointer meshletCull_comp = CreateShader(
                    std::string("./Resources/Shaders/meshletCull.comp"),
                    GL_COMPUTE_SHADER);
            sp->AddShaders(meshletCull_comp);
#endif
        } break;
        case SCREEN: {
            GLShader::Pointer fullScreenTriangle_vert =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "fullScreenTriangle.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer screenShader_frag = CreateShader(
                    std::string("./Resources/Shaders/screenShader.frag"),
                    GL_FRAGMENT_SHADER);
            sp->AddShaders(fullScreenTriangle_vert, screenShader_frag);
        } break;
        case FXAA: {
            GLShader::Pointer fxaa_vert =
                    CreateShader(std::string("./Resources/Shaders/fxaa.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer fxaa_frag =
                    CreateShader(std::string("./Resources/Shaders/fxaa.frag"),
                                 GL_FRAGMENT_SHADER);
            sp->AddShaders(fxaa_vert, fxaa_frag);
        } break;
        default:
            break;
    }
    return sp;
}

void Scene::SetShader(IGenum type, GLShaderProgram::Pointer sp) {
    if (sp == nullptr) { return; }
    m_ShaderPrograms[type] = sp;
}

bool Scene::HasShader(IGenum type) {
    return this->GetShaderWithType(type) != nullptr;
}

void Scene::UseShader(IGenum type) { this->GetShader(type)->Use(); }

void Scene::InitOpenGL() {
    if (!gladLoadGL()) {
        igError("Failed to initialize GLAD");
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // reversed-z buffer, depth range: 1.0(near plane) -> 0.0(far plane)
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

    // create empty VAO to render full-screen triangle
    m_EmptyVAO->Create();

    // allocate memory
    {

        m_CameraDataBlock->Create();
        m_CameraDataBlock->Target(GL_UNIFORM_BUFFER);
        m_CameraDataBlock->Allocate(sizeof(CameraDataBuffer), nullptr,
                                    GL_STATIC_DRAW);

        m_ObjectDataBlock->Create();
        m_ObjectDataBlock->Target(GL_UNIFORM_BUFFER);
        m_ObjectDataBlock->Allocate(sizeof(ObjectDataBuffer), nullptr,
                                    GL_STATIC_DRAW);

        m_UBOBlock->Create();
        m_UBOBlock->Target(GL_UNIFORM_BUFFER);
        m_UBOBlock->Allocate(sizeof(UniformBufferObjectBuffer), nullptr,
                             GL_STATIC_DRAW);

        // map shader block
        {
            auto shader = this->GetShader(BLINNPHONG);
            shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
            shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
            shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);
        }
        // map no light shader block
        {
            auto shader = this->GetShader(NOLIGHT);
            shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
            shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
            shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);
        }
        // map pure color shader block
        {
            auto shader = this->GetShader(PURECOLOR);
            shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
            shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
            shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);
        }
        // map transparency link shader block
        {
            auto shader = this->GetShader(TRANSPARENCYLINK);
            shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
            shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
            shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);
        }
        // map volume rendering link shader block
        {
            auto shader = this->GetShader(VOLUMERENDERINGLINK);
            shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
            shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
            shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);
        }
        // map culling computer shader block
        {
#ifdef IGAME_OPENGL_VERSION_460
            auto shader = this->GetShader(MESHLETCULL);
            shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
#endif
        }
    }

    m_UBO.useColor = false;

    // init drawculldata buffer
    m_DrawCullData->Create();
    m_DrawCullData->Target(GL_UNIFORM_BUFFER);
    m_DrawCullData->Allocate(sizeof(DrawCullData), nullptr, GL_DYNAMIC_DRAW);

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
        //m_Painter3D->DrawCircle({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 1, 100);
        //m_Painter3D->DrawSphere({0.0f, 0.0f, 0.0f}, 1.0f, 100, 100);
        //m_Painter3D->DrawIcoSphere({0.0f, 0.0f, 0.0f}, 1.0f, 5);
        //m_Painter3D->DrawCubeSphere({0.0f, 0.0f, 0.0f}, 1.0f, 8);
        //m_Painter3D->DrawCylinder({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1,
        //                          1.0f, 16);
        // m_Painter3D->DrawCone({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1, 1.0f,
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

void Scene::InitFont() {
    const wchar_t* text = L"XYZ";
    FontSet::Instance().RegisterWords(text);
    GLCheckError();
}

void Scene::InitAxes() {
    auto axesShader = this->GetShader(AXES);

    axesShader->Use();
    axesShader->SetUniformMatrix4x4("view", Axes::ViewMatrix());
    axesShader->SetUniformMatrix4x4("proj", Axes::ProjMatrix());
    axesShader->SetUniform3f("viewPos", Axes::CameraPos());

    m_Axes = Axes::New();
    GLCheckError();
}

void Scene::ResizeFrameBuffer() {
    auto viewport = m_Camera->GetScaledViewPort();
    uint32_t width = viewport.x;
    uint32_t height = viewport.y;

#ifdef MSAA
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

        if (m_FramebufferMultisampled->CheckStatus() != GL_FRAMEBUFFER_COMPLETE)
            igError("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
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

        if (m_FramebufferResolved->CheckStatus() != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not "
                         "complete!"
                      << std::endl;
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

        if (m_Framebuffer->CheckStatus() != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not "
                         "complete!"
                      << std::endl;
    }
#endif

    ResizeDepthPyramid();
}
void Scene::ResizeDepthPyramid() {
#ifdef IGAME_OPENGL_VERSION_460
    auto width = m_Camera->GetScaledViewPort().x;
    auto height = m_Camera->GetScaledViewPort().y;

    static auto previousPow2 = [](uint32_t v) {
        uint32_t r = 1;
        while (r * 2 < v) r *= 2;
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

    m_DepthPyramidWidth = previousPow2(width);
    m_DepthPyramidHeight = previousPow2(height);
    m_DepthPyramidLevels =
            compDepthMipLevels(m_DepthPyramidWidth, m_DepthPyramidHeight);

    GLTexture2d::Pointer texture = GLTexture2d::New();
    texture->Create();
    texture->Bind();
    texture->Storage(m_DepthPyramidLevels, GL_R32F, m_DepthPyramidWidth,
                     m_DepthPyramidHeight);
    texture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    texture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    texture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    texture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    texture->Release();

    m_DepthPyramid = std::move(texture);
#endif
}

void Scene::Draw() {
    auto viewport = m_Camera->GetScaledViewPort();

    // save default framebuffer, because it is not 0 in Qt
    GLint defaultFramebuffer = GL_NONE;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFramebuffer);

#ifdef MSAA
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

void Scene::RefreshDepthPyramid() {
#ifdef IGAME_OPENGL_VERSION_460
    auto shader = GetShader(DEPTHREDUCE);
    shader->Use();
    m_DepthTextureMultisampled->Active(GL_TEXTURE1);
    m_DepthPyramid->Active(GL_TEXTURE2);
    shader->SetUniformi("screenDepthMS", 1);
    shader->SetUniformi("myDepthPyramid", 2);

    // generate level 0
    {
        unsigned int level = 0;
        uint32_t width = m_DepthPyramidWidth;
        uint32_t height = m_DepthPyramidHeight;
        shader->Use();
        shader->SetUniformui("level", level);
        shader->SetUniform2ui("outDepthPyramidSize", igm::uvec2{width, height});
        m_DepthPyramid->BindImage(0, level, GL_FALSE, 0, GL_WRITE_ONLY,
                                  GL_R32F);
        glDispatchCompute((width + 31) / 32, (height + 31) / 32, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // generate other level
    for (unsigned int level = 1; level < m_DepthPyramidLevels; ++level) {
        uint32_t width = m_DepthPyramidWidth >> (level - 1);
        uint32_t height = m_DepthPyramidHeight >> (level - 1);
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
        m_DepthPyramid->BindImage(0, level, GL_FALSE, 0, GL_WRITE_ONLY,
                                  GL_R32F);

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
    for (auto& [id, model]: m_Models) {
        if (!model->m_DataObject->IsDrawable()) { continue; }

        auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
        drawObject->ReAllocateDisplayBuffer();
    }

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
        auto viewport = m_Camera->GetScaledViewPort();
        int scale = static_cast<int>(std::max(viewport.x, viewport.y)) / 10.0f;
        igm::ivec4 drawRange = igm::ivec4{0, 0, scale, scale};

        // Note: If depth rendering is enabled, please comment out this line to preserve depth information.
        glClear(GL_DEPTH_BUFFER_BIT);
        glViewport(drawRange.x, drawRange.y, drawRange.z, drawRange.w);
        DrawAxes(drawRange);
    }
}

void Scene::ResolveFrame() {
#ifdef MSAA
    auto viewport = m_Camera->GetScaledViewPort();

    glViewport(0, 0, viewport.x, viewport.y);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    auto shader = GetShader(Scene::ATTACHMENTRESOLVE);
    shader->Use();

    shader->SetUniformi("numSamples", samples);
    m_ColorTextureMultisampled->Active(GL_TEXTURE1);
    shader->SetUniformi("colorTextureMS", 1);
    m_DepthTextureMultisampled->Active(GL_TEXTURE2);
    shader->SetUniformi("depthTextureMS", 2);

    m_EmptyVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    m_EmptyVAO->Release();
#endif
}

void Scene::RenderToQtFrame() {
    auto viewport = m_Camera->GetScaledViewPort();

    glViewport(0, 0, viewport.x, viewport.y);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    auto shader = GetShader(Scene::SCREEN);
    //auto shader = GetShader(Scene::FXAA);
    shader->Use();

#ifdef MSAA
    m_ColorTextureResolved->GenerateMipmap();
    m_ColorTextureResolved->Active(GL_TEXTURE1);
    m_DepthTextureResolved->Active(GL_TEXTURE2);
    m_DepthPyramid->Active(GL_TEXTURE3);

    // Note:
    // 1. To enable depth rendering, ensure the screen.frag file is updated to handle the depth texture input.
    //    Specifically, the shader should read from the depth sampler and implement the desired depth-based operations.
    // 2. Additionally, disable any depth-buffer-clearing code in the coordinate axis rendering logic.
    //    Failing to do so could overwrite or invalidate the depth information required for rendering.
    shader->SetUniformi("screenColorSampler", 1);
#else
    m_ColorTexture->Active(GL_TEXTURE1);
    m_DepthTexture->Active(GL_TEXTURE2);
    m_DepthPyramid->Active(GL_TEXTURE3);
    shader->SetUniformi("screenColorSampler", 1);
#endif

    m_EmptyVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    m_EmptyVAO->Release();
}

void Scene::ShadowPass() { GLCheckError(); }

void Scene::ForwardPass() {
#ifdef IGAME_OPENGL_VERSION_330
    for (auto& [id, model]: m_Models) { model->Draw(this); }
#elif IGAME_OPENGL_VERSION_460
    bool debug = false;
    if (debug) {
        //std::cout << "-------:Draw:-------" << std::endl;
        RefreshDrawCullDataBuffer();

        for (auto& [id, model]: m_Models) {
            auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
            if (drawObject->GetTransparency() == 1.0f) {
                model->TestOcclusionResults(this);
            }
        }

        // draw phase1: draw visible meshlet
        for (auto& [id, model]: m_Models) {
            auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
            if (drawObject->GetTransparency() == 1.0f) {
                model->DrawPhase1(this);
            }
        }

        // refresh phase1: generate loacl hierarchical z-buffer
        RefreshDepthPyramid();

        // draw phase2: draw invisible meshlet
        for (auto& [id, model]: m_Models) {
            auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
            if (drawObject->GetTransparency() == 1.0f) {
                model->DrawPhase2(this);
            }
        }

        // refresh phase2: generate global hierarchical z-buffer
        RefreshDepthPyramid();
    } else {
        for (auto& [id, model]: m_Models) {
            // draw mesh
            auto drawObject = DynamicCast<DrawObject>(model->m_DataObject);
            if (drawObject->GetTransparency() == 1.0f) { model->Draw(this); }

            // draw painter(since painter does not support transparency)
            if (drawObject->GetVisibility()) {
                model->GetPainter()->Draw(this);
            }
        }
    }
#endif
    GLCheckError();
}

void Scene::TransparentPass() {
#ifdef IGAME_OPENGL_VERSION_460
    // 1.reset oit pipeline status
    {
        auto shader = GetShader(Scene::TRANSPARENCYLINK);
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
        auto shader = GetShader(Scene::TRANSPARENCYSORT);
        shader->Use();

        shader->SetUniformi("numSamples", samples);
        m_ColorTextureMultisampled->Active(GL_TEXTURE1);
        shader->SetUniformi("forwardPassColorMS", 1);

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
        auto shader = GetShader(Scene::TRANSPARENCYSORT);
        shader->Use();

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

void Scene::VolumeRenderingPass() {
#ifdef IGAME_OPENGL_VERSION_460
    // 1.reset oit pipeline status
    {
        auto shader = GetShader(Scene::VOLUMERENDERINGLINK);
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
        auto shader = GetShader(Scene::VOLUMERENDERINGSORT);
        shader->Use();

        shader->SetUniformi("numSamples", samples);
        m_ColorTextureMultisampled->Active(GL_TEXTURE1);
        shader->SetUniformi("forwardPassColorMS", 1);

        for (auto& [id, model]: m_Models) { model->DrawWithVolume(this); }
    }
    glDepthMask(GL_TRUE);

    // 3.sorting and blending colors
    glDisable(GL_DEPTH_TEST);
    {
        auto shader = GetShader(Scene::VOLUMERENDERINGSORT);
        shader->Use();

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
    // update camera data matrix
    m_CameraData.camera_position = m_Camera->GetCameraPos();
    m_CameraData.isOrtho = m_Camera->GetCameraType() == Camera::ORTHOGRAPHIC;
    m_CameraData.view = m_Camera->GetViewMatrix();
    m_CameraData.proj = m_Camera->GetProjectionMatrix();
    m_CameraData.proj_view =
            m_Camera->GetProjectionMatrix() * m_Camera->GetViewMatrix();

    // update camera data matrix
    m_CameraDataBlock->SubData(0, sizeof(CameraDataBuffer), &m_CameraData);
}
void Scene::UpdateObjectDataBlock(DataObject* obj) {
    // update object data matrix
    auto drawObject = DynamicCast<DrawObject>(obj);
    auto box = obj->GetBoundingBox();
    Vector3f center = box.center();

    m_ObjectData.transparent = drawObject->GetTransparency();
    m_ObjectData.model = m_ModelMatrix;
    m_ObjectData.normal = m_ObjectData.model.invert().transpose();
    m_ObjectData.sphereBounds = igm::vec4{center[0], center[1], center[2],
                                          static_cast<float>(box.diag() / 2)};

    // update object data matrix
    m_ObjectDataBlock->SubData(0, sizeof(ObjectDataBuffer), &m_ObjectData);
}
void Scene::UpdateUniformBufferObjectBlock(DataObject* obj) {
    auto drawObject = DynamicCast<DrawObject>(obj);

    m_UBO.useColor = drawObject->IsUseColor();
    m_UBO.useNormalSmooth = drawObject->IsUseNormalSmooth();

    // update other ubo
    m_UBOBlock->SubData(0, sizeof(UniformBufferObjectBuffer), &m_UBO);
}

void Scene::UpdateUniformBuffer() {
    // update camera data matrix
    m_CameraDataBlock->SubData(0, sizeof(CameraDataBuffer), &m_CameraData);

    // update object data matrix
    m_ObjectDataBlock->SubData(0, sizeof(ObjectDataBuffer), &m_ObjectData);

    // update other ubo
    m_UBOBlock->SubData(0, sizeof(UniformBufferObjectBuffer), &m_UBO);
}

void Scene::DrawAxes(igm::ivec4 drawRange) {
    auto axesShader = this->GetShader(Scene::AXES);
    axesShader->Use();

    axesShader->SetUniformMatrix4x4("model", m_ModelRotate);

    // draw Axes
    {
        axesShader->SetUniformi("isDrawFont", 0);
        m_Axes->DrawAxes();
    }

    // draw Axes Font
    {
        axesShader->SetUniformi("isDrawFont", 1);
        m_Axes->Update(Axes::ProjMatrix() * Axes::ViewMatrix() * m_ModelRotate,
                       {drawRange.x, drawRange.y, drawRange.z, drawRange.w});
        m_Axes->DrawXYZ(axesShader);
    }
}

void Scene::RefreshDrawCullDataBuffer() {
    igm::mat4 projection = m_Camera->GetProjectionMatrix();
    igm::mat4 projectionT = projection.transpose();

    igm::vec4 frustumX =
            (projectionT[3] + projectionT[0]).normalized(); // x + w < 0
    igm::vec4 frustumY =
            (projectionT[3] + projectionT[1]).normalized(); // y + w < 0

    DrawCullData cullData = {};
    cullData.view_model = m_Camera->GetViewMatrix() * m_ModelMatrix;
    cullData.P00 = projection[0][0];
    cullData.P11 = projection[1][1];
    cullData.zNear = projection[3][2];
    cullData.frustum[0] = frustumX.x;
    cullData.frustum[1] = frustumX.z;
    cullData.frustum[2] = frustumY.y;
    cullData.frustum[3] = frustumY.z;
    cullData.pyramidWidth = static_cast<float>(m_DepthPyramidWidth);
    cullData.pyramidHeight = static_cast<float>(m_DepthPyramidHeight);

    m_DrawCullData->SubData(0, sizeof(DrawCullData), &cullData);
}

void Scene::LookAtPositiveX() {
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
void Scene::LookAtNegativeX() {
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
void Scene::LookAtPositiveY() {
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
void Scene::LookAtNegativeY() {
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
void Scene::LookAtPositiveZ() {
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
void Scene::LookAtNegativeZ() {
    ResetCameraView();

    igm::vec3 center = igm::vec3{m_ModelsBoundingSphere};
    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -center);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, center);

    auto rotate = igm::mat4{};
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    m_ModelMatrix = rotateSelf * m_ModelMatrix;
    m_ModelRotate = rotate * m_ModelRotate;
}
void Scene::LookAtIsometric() {
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

    for (auto& [id, model]: m_Models) {
        if (!model->GetVisibility()) continue;

        auto box = model->m_DataObject->GetBoundingBox();
        Vector3f boxMin = box.min;
        Vector3f boxMax = box.max;

        min = igm::min(igm::vec3{boxMin[0], boxMin[1], boxMin[2]}, min);
        max = igm::max(igm::vec3{boxMax[0], boxMax[1], boxMax[2]}, max);
    };
    igm::vec3 center = (min + max) / 2;
    float radius = (max - min).length() / 2;

    m_ModelsBoundingSphere = igm::vec4{center, radius};

    // update camera setting
    auto dist = (m_Camera->GetCameraPos() - center).length();
    m_Camera->SetNearPlane(dist - radius);
    m_Camera->SetFarPlane(dist + radius);
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
                                                int height) {
    std::vector<unsigned char> colorBuffer(width * height * 3);

    m_FramebufferResolved->Bind();
    {
        // Read pixels from the OpenGL buffer (bottom-left corner, BGR format)
        //
        //  y↑
        //   |
        //   |
        //   +-----→x
        //
        glReadPixels(x, y, width, height, GL_BGR, GL_UNSIGNED_BYTE,
                     colorBuffer.data());
    }
    m_FramebufferResolved->Release();

    return colorBuffer;
}

void Scene::MakeCurrent() {
    if (m_MakeCurrentFunctor) { m_MakeCurrentFunctor(); }
}

void Scene::DoneCurrent() {
    if (m_DoneCurrentFunctor) { m_DoneCurrentFunctor(); }
}

IGAME_NAMESPACE_END