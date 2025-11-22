#include "iGameShaderManager.h"

IGAME_NAMESPACE_BEGIN

ShaderManager::ShaderManager() {
    m_CameraDataBlock = GLBuffer::New();
    m_ObjectDataBlock = GLBuffer::New();
    m_UBOBlock = GLBuffer::New();
    m_CullDataBuffer = GLBuffer::New();
}

ShaderManager::~ShaderManager() {}

bool ShaderManager::Initialize() {
    MapBufferBlock();
    return true;
}

SmartPointer<GLShaderProgram> ShaderManager::GetShader(ShaderType type) {
    SmartPointer<GLShaderProgram> sp = this->GetShaderWithType(type);
    if (sp != nullptr) { return sp; }

    sp = this->GenShader(type);
    if (sp == nullptr) { IGAME_RENDERING_ERROR("Error for GenShader"); }
    this->SetShader(type, sp);

    return sp;
}

bool ShaderManager::HasShader(ShaderType type) {
    return this->GetShaderWithType(type) != nullptr;
}

void ShaderManager::UseShader(ShaderType type) { this->GetShader(type)->Use(); }

void ShaderManager::UpdateCameraBlock(SmartPointer<Camera> camera) {
    CameraDataBuffer buffer;
    buffer.camera_position = camera->GetPosition();
    buffer.isOrtho = camera->GetType() == Camera::Type::ORTHOGRAPHIC;
    buffer.view = camera->GetViewMatrix();
    buffer.proj = camera->GetProjectionMatrix();
    buffer.proj_view = camera->GetProjectionMatrix() * camera->GetViewMatrix();

    m_CameraDataBlock->SubData(0, sizeof(CameraDataBuffer), &buffer);
}

void ShaderManager::UpdateCameraBlock(CameraDataBuffer buffer) {
    m_CameraDataBlock->SubData(0, sizeof(CameraDataBuffer), &buffer);
}

void ShaderManager::UpdateObjectBlock(SmartPointer<DataObject> obj,
                                      igm::mat4 model) {
    ObjectDataBuffer buffer;

    auto drawObject = DynamicCast<DrawObject>(obj);
    auto box = obj->GetBoundingBox();
    Vector3f center = box.center();

    buffer.transparent = drawObject->GetTransparency();
    buffer.model = model;
    buffer.normal = model.invert().transpose();
    buffer.sphereBounds = igm::vec4{center[0], center[1], center[2],
                                    static_cast<float>(box.diag() / 2)};

    m_ObjectDataBlock->SubData(0, sizeof(ObjectDataBuffer), &buffer);
}

void ShaderManager::UpdateObjectBlock(ObjectDataBuffer buffer) {
    m_ObjectDataBlock->SubData(0, sizeof(ObjectDataBuffer), &buffer);
}

void ShaderManager::UpdateUBOBlock(SmartPointer<DataObject> obj) {
    UniformBufferObjectBuffer buffer;

    auto drawObject = DynamicCast<DrawObject>(obj);

    buffer.useColor = drawObject->IsUseColor();
    buffer.useNormalSmooth = drawObject->IsUseNormalSmooth();

    m_UBOBlock->SubData(0, sizeof(UniformBufferObjectBuffer), &buffer);
}

void ShaderManager::UpdateUBOBlock(UniformBufferObjectBuffer buffer) {
    m_UBOBlock->SubData(0, sizeof(UniformBufferObjectBuffer), &buffer);
}

void ShaderManager::UpdateCullDataBuffer(SmartPointer<Camera> camera,
                                         igm::mat4 model, unsigned int HzbWidth,
                                         unsigned int HzbHeight) {
    CullDataBuffer buffer;

    igm::mat4 projection = camera->GetProjectionMatrix();
    igm::mat4 projectionT = projection.transpose();

    igm::vec4 frustumX =
            (projectionT[3] + projectionT[0]).normalized(); // x + w < 0
    igm::vec4 frustumY =
            (projectionT[3] + projectionT[1]).normalized(); // y + w < 0

    buffer.view_model = camera->GetViewMatrix() * model;
    buffer.P00 = projection[0][0];
    buffer.P11 = projection[1][1];
    //cullData.zNear = projection[3][2];
    buffer.zNear = camera->GetClippingRange().x;
    buffer.zFar = camera->GetClippingRange().y;
    buffer.frustum[0] = frustumX.x;
    buffer.frustum[1] = frustumX.z;
    buffer.frustum[2] = frustumY.y;
    buffer.frustum[3] = frustumY.z;
    buffer.HzbWidth = static_cast<float>(HzbWidth);
    buffer.HzbHeight = static_cast<float>(HzbHeight);

    m_CullDataBuffer->SubData(0, sizeof(CullDataBuffer), &buffer);
}

void ShaderManager::UpdateCullDataBuffer(CullDataBuffer buffer) {
    m_CullDataBuffer->SubData(0, sizeof(CullDataBuffer), &buffer);
}

SmartPointer<GLBuffer> ShaderManager::GetCullDataBuffer() {
    return m_CullDataBuffer;
}

void ShaderManager::MapBufferBlock() {
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

    m_CullDataBuffer->Create();
    m_CullDataBuffer->Target(GL_UNIFORM_BUFFER);
    m_CullDataBuffer->Allocate(sizeof(CullDataBuffer), nullptr,
                               GL_DYNAMIC_DRAW);

    SmartPointer<GLShaderProgram> shader;

    // map blinnphong shader block
    shader = this->GetShader(ShaderType::BLINNPHONG);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
    shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
    shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);

    // map no light shader block
    shader = this->GetShader(ShaderType::NOLIGHT);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
    shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
    shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);

    // map pure color shader block
    shader = this->GetShader(ShaderType::PURECOLOR);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
    shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
    shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);

    // map transparency link shader block
#ifdef IGAME_OPENGL_VERSION_460
    shader = this->GetShader(ShaderType::TRANSPARENCYLINK);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
    shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
    shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);
#endif

    // map volume rendering link shader block
#ifdef IGAME_OPENGL_VERSION_460
    shader = this->GetShader(ShaderType::VOLUMERENDERINGLINK);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
    shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
    shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);
#endif

    // map culling computer shader block
#ifdef IGAME_OPENGL_VERSION_460
    shader = this->GetShader(ShaderType::MESHLETCULL);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
#endif

// map meshlet culling shader block
#if defined(IGAME_OPENGL_VERSION_460) && defined(GL_SUPPORTS_MESH_SHADER)
    shader = this->GetShader(ShaderType::CULLINGPHASE1);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
    shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
    shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);

    shader = this->GetShader(ShaderType::CULLINGPHASE2);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
    shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
    shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);
#endif
}

SmartPointer<GLShaderProgram>
ShaderManager::GetShaderWithType(ShaderType type) {
    auto it = m_ShaderPrograms.find(type);
    if (it == m_ShaderPrograms.end()) { return nullptr; }
    return it->second;
}

void ShaderManager::SetShader(ShaderType type,
                              SmartPointer<GLShaderProgram> sp) {
    if (sp == nullptr) { return; }
    m_ShaderPrograms[type] = sp;
}

SmartPointer<GLShaderProgram> ShaderManager::GenShader(ShaderType type) {
    SmartPointer<GLShaderProgram> sp = GLShaderProgram::New();
    switch (type) {
        case ShaderType::BLINNPHONG: {
            SmartPointer<GLShader> vertex_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Vertex.vert"),
                    GL_VERTEX_SHADER);

            SmartPointer<GLShader> blinnPhong_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/BlinnPhong.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("BLINNPHONG");
            sp->AddShaders(vertex_vert, blinnPhong_frag);
        } break;
        case ShaderType::PBR: {
            SmartPointer<GLShader> vertex_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Vertex.vert"),
                    GL_VERTEX_SHADER);

            SmartPointer<GLShader> pbr_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/PBR.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("PBR");
            sp->AddShaders(vertex_vert, pbr_frag);
        } break;
        case ShaderType::NOLIGHT: {
            SmartPointer<GLShader> vertex_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Vertex.vert"),
                    GL_VERTEX_SHADER);

            SmartPointer<GLShader> noLight_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/NoLight.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("NOLIGHT");
            sp->AddShaders(vertex_vert, noLight_frag);
        } break;
        case ShaderType::PURECOLOR: {
            SmartPointer<GLShader> vertex_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Vertex.vert"),
                    GL_VERTEX_SHADER);

            SmartPointer<GLShader> pureColor_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/PureColor.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("PURECOLOR");
            sp->AddShaders(vertex_vert, pureColor_frag);
        } break;
        case ShaderType::SINGLEPASSWIREFRAME: {
            SmartPointer<GLShader> vertex_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Vertex.vert"),
                    GL_VERTEX_SHADER);

            SmartPointer<GLShader> wireframe_geom = GLShader::CreateShader(
                    std::string("./Resources/Shaders/SinglePassWireframe.geom"),
                    GL_GEOMETRY_SHADER);

            SmartPointer<GLShader> wireframe_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/SinglePassWireframe.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("SINGLEPASSWIREFRAME");
            sp->AddShaders(vertex_vert, wireframe_geom, wireframe_frag);
        } break;
#ifdef IGAME_OPENGL_VERSION_460
        case ShaderType::TRANSPARENCYLINK: {
            SmartPointer<GLShader> vertex_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Vertex.vert"),
                    GL_VERTEX_SHADER);

            SmartPointer<GLShader> transparencyLink_frag =
                    GLShader::CreateShader(std::string("./Resources/Shaders/"
                                                       "TransparencyLink.frag"),
                                           GL_FRAGMENT_SHADER);

            sp->SetName("TRANSPARENCYLINK");
            sp->AddShaders(vertex_vert, transparencyLink_frag);
        } break;
        case ShaderType::TRANSPARENCYSORT: {
            SmartPointer<GLShader> fullScreenTriangle_vert =
                    GLShader::CreateShader(
                            std::string("./Resources/Shaders/"
                                        "FullScreenTriangle.vert"),
                            GL_VERTEX_SHADER);

            std::string fragPath = "";
    #ifdef GL_SUPPORT_MSAA
            fragPath = "./Resources/Shaders/TransparencySortMS.frag";
    #else
            fragPath = "./Resources/Shaders/TransparencySort.frag";
    #endif
            SmartPointer<GLShader> transparencySort_frag =
                    GLShader::CreateShader(fragPath, GL_FRAGMENT_SHADER);

            sp->SetName("TRANSPARENCYSORT");
            sp->AddShaders(fullScreenTriangle_vert, transparencySort_frag);
        } break;
        case ShaderType::VOLUMERENDERINGLINK: {
            SmartPointer<GLShader> vertex_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Vertex.vert"),
                    GL_VERTEX_SHADER);
            vertex_vert->SetName("Vertex.vert");

            SmartPointer<GLShader> volumeRenderingLink_frag =
                    GLShader::CreateShader(
                            std::string("./Resources/Shaders/"
                                        "VolumeRenderingLink.frag"),
                            GL_FRAGMENT_SHADER);
            volumeRenderingLink_frag->SetName("VolumeRenderingLink.frag");

            sp->SetName("VOLUMERENDERINGLINK");
            sp->AddShaders(vertex_vert, volumeRenderingLink_frag);
        } break;
        case ShaderType::VOLUMERENDERINGSORT: {
            SmartPointer<GLShader> fullScreenTriangle_vert =
                    GLShader::CreateShader(
                            std::string("./Resources/Shaders/"
                                        "FullScreenTriangle.vert"),
                            GL_VERTEX_SHADER);

            std::string fragPath = "";
    #ifdef GL_SUPPORT_MSAA
            fragPath = "./Resources/Shaders/VolumeRenderingSortMS.frag";
    #else
            fragPath = "./Resources/Shaders/VolumeRenderingSort.frag";
    #endif
            SmartPointer<GLShader> volumeRenderingSort_frag =
                    GLShader::CreateShader(fragPath, GL_FRAGMENT_SHADER);

            sp->SetName("VOLUMERENDERINGSORT");
            sp->AddShaders(fullScreenTriangle_vert, volumeRenderingSort_frag);
        } break;
#endif
        case ShaderType::AXES: {
            SmartPointer<GLShader> axis_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Axis.vert"),
                    GL_VERTEX_SHADER);
            SmartPointer<GLShader> axis_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Axis.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("AXES");
            sp->AddShaders(axis_vert, axis_frag);
        } break;
        case ShaderType::FONT: {
            SmartPointer<GLShader> font_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Font.vert"),
                    GL_VERTEX_SHADER);
            SmartPointer<GLShader> font_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Font.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("FONT");
            sp->AddShaders(font_vert, font_frag);
        } break;
        case ShaderType::ATTACHMENTRESOLVE: {
            SmartPointer<GLShader> attachmentResolve_vert =
                    GLShader::CreateShader(
                            std::string("./Resources/Shaders/"
                                        "AttachmentResolve.vert"),
                            GL_VERTEX_SHADER);

            SmartPointer<GLShader> attachmentResolve_frag =
                    GLShader::CreateShader(
                            std::string("./Resources/Shaders/"
                                        "AttachmentResolve.frag"),
                            GL_FRAGMENT_SHADER);

            sp->SetName("ATTACHMENTRESOLVE");
            sp->AddShaders(attachmentResolve_vert, attachmentResolve_frag);
        } break;
        case ShaderType::DEPTHREDUCE: {
#ifdef IGAME_OPENGL_VERSION_460
            std::string compPath = "";
    #ifdef GL_SUPPORT_MSAA
            compPath = "./Resources/Shaders/DepthReduceMS.comp";
    #else
            compPath = "./Resources/Shaders/DepthReduce.comp";
    #endif
            SmartPointer<GLShader> depthReduce_comp =
                    GLShader::CreateShader(compPath, GL_COMPUTE_SHADER);

            sp->SetName("DEPTHREDUCE");
            sp->AddShaders(depthReduce_comp);
#endif
        } break;
        case ShaderType::MESHLETCULL: {
#ifdef IGAME_OPENGL_VERSION_460
            SmartPointer<GLShader> meshletCull_comp = GLShader::CreateShader(
                    std::string("./Resources/Shaders/MeshletCull.comp"),
                    GL_COMPUTE_SHADER);

            sp->SetName("MESHLETCULL");
            sp->AddShaders(meshletCull_comp);
#endif
        } break;
        case ShaderType::SCREEN: {
            SmartPointer<GLShader> fullScreenTriangle_vert =
                    GLShader::CreateShader(
                            std::string("./Resources/Shaders/"
                                        "FullScreenTriangle.vert"),
                            GL_VERTEX_SHADER);

            SmartPointer<GLShader> screenShader_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/Screen.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("SCREEN");
            sp->AddShaders(fullScreenTriangle_vert, screenShader_frag);
        } break;
        case ShaderType::FXAA: {
            SmartPointer<GLShader> fxaa_vert = GLShader::CreateShader(
                    std::string("./Resources/Shaders/FullScreenTriangle.vert"),
                    GL_VERTEX_SHADER);

            SmartPointer<GLShader> fxaa_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/FXAA .frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("FXAA");
            sp->AddShaders(fxaa_vert, fxaa_frag);
        } break;
        case ShaderType::CULLINGPHASE1: {
#ifdef IGAME_OPENGL_VERSION_460
            SmartPointer<GLShader> shader_task = GLShader::CreateShader(
                    std::string("./Resources/Shaders/MeshShaders/"
                                "CullingPhase1.task"),
                    GL_TASK_SHADER_NV);

            SmartPointer<GLShader> shader_mesh = GLShader::CreateShader(
                    std::string("./Resources/Shaders/MeshShaders/"
                                "CullingPhase1.mesh"),
                    GL_MESH_SHADER_NV);

            SmartPointer<GLShader> shader_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/MeshShaders/Culling.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("CULLINGPHASE1");
            sp->AddShaders(shader_task, shader_mesh, shader_frag);
#endif
        } break;
        case ShaderType::CULLINGPHASE2: {
#ifdef IGAME_OPENGL_VERSION_460
            SmartPointer<GLShader> shader_task = GLShader::CreateShader(
                    std::string("./Resources/Shaders/MeshShaders/"
                                "CullingPhase2.task"),
                    GL_TASK_SHADER_NV);

            SmartPointer<GLShader> shader_mesh = GLShader::CreateShader(
                    std::string("./Resources/Shaders/MeshShaders/"
                                "CullingPhase2.mesh"),
                    GL_MESH_SHADER_NV);

            SmartPointer<GLShader> shader_frag = GLShader::CreateShader(
                    std::string("./Resources/Shaders/MeshShaders/Culling.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("CULLINGPHASE2");
            sp->AddShaders(shader_task, shader_mesh, shader_frag);
#endif
        } break;
        default:
            break;
    }
    return sp;
}

IGAME_NAMESPACE_END
