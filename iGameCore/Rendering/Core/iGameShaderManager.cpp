#include "iGameShaderManager.h"

IGAME_NAMESPACE_BEGIN

ShaderManager::ShaderManager() {
    m_CameraDataBlock = GLBuffer::New();
    m_ObjectDataBlock = GLBuffer::New();
    m_UBOBlock = GLBuffer::New();
    m_CullDataBuffer = GLBuffer::New();
}

ShaderManager::~ShaderManager() {}

GLShaderProgram::Pointer ShaderManager::GetShader(ShaderType type) {
    GLShaderProgram::Pointer sp = this->GetShaderWithType(type);
    if (sp != nullptr) { return sp; }

    sp = this->GenShader(type);
    if (sp == nullptr) { igError("Error for GenShader\n"); }
    this->SetShader(type, sp);

    return sp;
}

bool ShaderManager::HasShader(ShaderType type) {
    return this->GetShaderWithType(type) != nullptr;
}

void ShaderManager::UseShader(ShaderType type) { this->GetShader(type)->Use(); }

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

    GLShaderProgram::Pointer shader;

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
    shader = this->GetShader(ShaderType::TRANSPARENCYLINK);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
    shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
    shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);

    // map volume rendering link shader block
    shader = this->GetShader(ShaderType::VOLUMERENDERINGLINK);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
    shader->MapUniformBlock("ObjectDataBlock", 1, m_ObjectDataBlock);
    shader->MapUniformBlock("UniformBufferObjectBlock", 2, m_UBOBlock);

    // map culling computer shader block
#ifdef IGAME_OPENGL_VERSION_460
    shader = this->GetShader(ShaderType::MESHLETCULL);
    shader->MapUniformBlock("CameraDataBlock", 0, m_CameraDataBlock);
#endif

    // map meshlet culling shader block
#ifdef IGAME_OPENGL_VERSION_460
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

void ShaderManager::UpdateCameraBlock(Camera::Pointer camera) {
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

void ShaderManager::UpdateObjectBlock(DataObject::Pointer obj,
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

void ShaderManager::UpdateUBOBlock(DataObject::Pointer obj) {
    UniformBufferObjectBuffer buffer;

    auto drawObject = DynamicCast<DrawObject>(obj);

    buffer.useColor = drawObject->IsUseColor();
    buffer.useNormalSmooth = drawObject->IsUseNormalSmooth();

    m_UBOBlock->SubData(0, sizeof(UniformBufferObjectBuffer), &buffer);
}

void ShaderManager::UpdateUBOBlock(UniformBufferObjectBuffer buffer) {
    m_UBOBlock->SubData(0, sizeof(UniformBufferObjectBuffer), &buffer);
}

void ShaderManager::UpdateCullDataBuffer(Camera::Pointer camera,
                                         igm::mat4 model,
                                         unsigned int depthPyramidWidth,
                                         unsigned int depthPyramidHeight) {
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
    buffer.pyramidWidth = static_cast<float>(depthPyramidWidth);
    buffer.pyramidHeight = static_cast<float>(depthPyramidHeight);

    m_CullDataBuffer->SubData(0, sizeof(CullDataBuffer), &buffer);
}

void ShaderManager::UpdateCullDataBuffer(CullDataBuffer buffer) {
    m_CullDataBuffer->SubData(0, sizeof(CullDataBuffer), &buffer);
}

GLBuffer::Pointer ShaderManager::GetCullDataBuffer() {
    return m_CullDataBuffer;
}

GLShaderProgram::Pointer ShaderManager::GetShaderWithType(ShaderType type) {
    auto it = m_ShaderPrograms.find(type);
    if (it == m_ShaderPrograms.end()) { return nullptr; }
    return it->second;
}

void ShaderManager::SetShader(ShaderType type, GLShaderProgram::Pointer sp) {
    if (sp == nullptr) { return; }
    m_ShaderPrograms[type] = sp;
}

GLShaderProgram::Pointer ShaderManager::GenShader(ShaderType type) {
    GLShaderProgram::Pointer sp = GLShaderProgram::New();
    switch (type) {
        case ShaderType::BLINNPHONG: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);

            GLShader::Pointer blinnPhong_frag = CreateShader(
                    std::string("./Resources/Shaders/blinnPhong.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("BLINNPHONG");
            sp->AddShaders(vertex_vert, blinnPhong_frag);
        } break;
        case ShaderType::PBR: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);
            vertex_vert->SetName("vertex.vert");

            GLShader::Pointer pbr_frag =
                    CreateShader(std::string("./Resources/Shaders/pbr.frag"),
                                 GL_FRAGMENT_SHADER);
            pbr_frag->SetName("pbr.frag");

            sp->SetName("PBR");
            sp->AddShaders(vertex_vert, pbr_frag);
        } break;
        case ShaderType::NOLIGHT: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);

            GLShader::Pointer noLight_frag = CreateShader(
                    std::string("./Resources/Shaders/noLight.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("NOLIGHT");
            sp->AddShaders(vertex_vert, noLight_frag);
        } break;
        case ShaderType::PURECOLOR: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);

            GLShader::Pointer pureColor_frag = CreateShader(
                    std::string("./Resources/Shaders/pureColor.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("PURECOLOR");
            sp->AddShaders(vertex_vert, pureColor_frag);
        } break;
        case ShaderType::SINGLEPASSWIREFRAME: {
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

            sp->SetName("SINGLEPASSWIREFRAME");
            sp->AddShaders(vertex_vert, wireframe_geom, wireframe_frag);
        } break;
        case ShaderType::TRANSPARENCYLINK: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);

            GLShader::Pointer transparencyLink_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "transparencyLink.frag"),
                                 GL_FRAGMENT_SHADER);

            sp->SetName("TRANSPARENCYLINK");
            sp->AddShaders(vertex_vert, transparencyLink_frag);
        } break;
        case ShaderType::TRANSPARENCYSORT: {
            GLShader::Pointer fullScreenTriangle_vert =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "fullScreenTriangle.vert"),
                                 GL_VERTEX_SHADER);

            GLShader::Pointer transparencySort_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "transparencySort.frag"),
                                 GL_FRAGMENT_SHADER);

            sp->SetName("TRANSPARENCYSORT");
            sp->AddShaders(fullScreenTriangle_vert, transparencySort_frag);
        } break;
        case ShaderType::VOLUMERENDERINGLINK: {
            GLShader::Pointer vertex_vert =
                    CreateShader(std::string("./Resources/Shaders/vertex.vert"),
                                 GL_VERTEX_SHADER);
            vertex_vert->SetName("vertex.vert");

            GLShader::Pointer volumeRenderingLink_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "volumeRenderingLink.frag"),
                                 GL_FRAGMENT_SHADER);
            volumeRenderingLink_frag->SetName("volumeRenderingLink.frag");

            sp->SetName("VOLUMERENDERINGLINK");
            sp->AddShaders(vertex_vert, volumeRenderingLink_frag);
        } break;
        case ShaderType::VOLUMERENDERINGSORT: {
            GLShader::Pointer fullScreenTriangle_vert =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "fullScreenTriangle.vert"),
                                 GL_VERTEX_SHADER);

            GLShader::Pointer volumeRenderingSort_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "volumeRenderingSort.frag"),
                                 GL_FRAGMENT_SHADER);

            sp->SetName("VOLUMERENDERINGSORT");
            sp->AddShaders(fullScreenTriangle_vert, volumeRenderingSort_frag);
        } break;
        case ShaderType::AXES: {
            GLShader::Pointer axis_vert =
                    CreateShader(std::string("./Resources/Shaders/axis.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer axis_frag =
                    CreateShader(std::string("./Resources/Shaders/axis.frag"),
                                 GL_FRAGMENT_SHADER);

            sp->SetName("AXES");
            sp->AddShaders(axis_vert, axis_frag);
        } break;
        case ShaderType::FONT: {
            GLShader::Pointer font_vert =
                    CreateShader(std::string("./Resources/Shaders/font.vert"),
                                 GL_VERTEX_SHADER);
            GLShader::Pointer font_frag =
                    CreateShader(std::string("./Resources/Shaders/font.frag"),
                                 GL_FRAGMENT_SHADER);

            sp->SetName("FONT");
            sp->AddShaders(font_vert, font_frag);
        } break;
        case ShaderType::ATTACHMENTRESOLVE: {
            GLShader::Pointer attachmentResolve_vert =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "attachmentResolve.vert"),
                                 GL_VERTEX_SHADER);

            GLShader::Pointer attachmentResolve_frag =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "attachmentResolve.frag"),
                                 GL_FRAGMENT_SHADER);

            sp->SetName("ATTACHMENTRESOLVE");
            sp->AddShaders(attachmentResolve_vert, attachmentResolve_frag);
        } break;
        case ShaderType::DEPTHREDUCE: {
#ifdef IGAME_OPENGL_VERSION_460
            GLShader::Pointer depthReduce_comp = CreateShader(
                    std::string("./Resources/Shaders/depthReduce.comp"),
                    GL_COMPUTE_SHADER);

            sp->SetName("DEPTHREDUCE");
            sp->AddShaders(depthReduce_comp);
#endif
        } break;
        case ShaderType::MESHLETCULL: {
#ifdef IGAME_OPENGL_VERSION_460
            GLShader::Pointer meshletCull_comp = CreateShader(
                    std::string("./Resources/Shaders/meshletCull.comp"),
                    GL_COMPUTE_SHADER);

            sp->SetName("MESHLETCULL");
            sp->AddShaders(meshletCull_comp);
#endif
        } break;
        case ShaderType::SCREEN: {
            GLShader::Pointer fullScreenTriangle_vert =
                    CreateShader(std::string("./Resources/Shaders/"
                                             "fullScreenTriangle.vert"),
                                 GL_VERTEX_SHADER);

            GLShader::Pointer screenShader_frag = CreateShader(
                    std::string("./Resources/Shaders/screenShader.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("SCREEN");
            sp->AddShaders(fullScreenTriangle_vert, screenShader_frag);
        } break;
        case ShaderType::FXAA: {
            GLShader::Pointer fxaa_vert =
                    CreateShader(std::string("./Resources/Shaders/fxaa.vert"),
                                 GL_VERTEX_SHADER);

            GLShader::Pointer fxaa_frag =
                    CreateShader(std::string("./Resources/Shaders/fxaa.frag"),
                                 GL_FRAGMENT_SHADER);

            sp->SetName("FXAA");
            sp->AddShaders(fxaa_vert, fxaa_frag);
        } break;
        case ShaderType::CULLINGPHASE1: {
#ifdef IGAME_OPENGL_VERSION_460
            GLShader::Pointer shader_task =
                    CreateShader(std::string("./Resources/Shaders/mesh_shader/"
                                             "cullingPhase1.task"),
                                 GL_TASK_SHADER_NV);

            GLShader::Pointer shader_mesh =
                    CreateShader(std::string("./Resources/Shaders/mesh_shader/"
                                             "cullingPhase1.mesh"),
                                 GL_MESH_SHADER_NV);

            GLShader::Pointer shader_frag = CreateShader(
                    std::string("./Resources/Shaders/mesh_shader/culling.frag"),
                    GL_FRAGMENT_SHADER);

            sp->SetName("CULLINGPHASE1");
            sp->AddShaders(shader_task, shader_mesh, shader_frag);
#endif
        } break;
        case ShaderType::CULLINGPHASE2: {
#ifdef IGAME_OPENGL_VERSION_460
            GLShader::Pointer shader_task =
                    CreateShader(std::string("./Resources/Shaders/mesh_shader/"
                                             "cullingPhase2.task"),
                                 GL_TASK_SHADER_NV);

            GLShader::Pointer shader_mesh =
                    CreateShader(std::string("./Resources/Shaders/mesh_shader/"
                                             "cullingPhase2.mesh"),
                                 GL_MESH_SHADER_NV);

            GLShader::Pointer shader_frag = CreateShader(
                    std::string("./Resources/Shaders/mesh_shader/culling.frag"),
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
