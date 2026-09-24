// Real OpenGL regression: prepared CPU -> GPU -> release GPU -> re-upload.
// No Scene changes, network, large assets, or simplification-policy changes.
#include "iGameSurfaceMesh.h"
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <string>

static void Require(bool ok, const char* name) {
    if (!ok) throw std::runtime_error(name);
    std::cout << "PASS " << name << std::endl;
}
class Probe final : public iGame::SurfaceMesh {
public:
    using Pointer = iGame::SmartPointer<Probe>;
    static Pointer New() { return new Probe; }
    using iGame::DrawObject::SyncGpuBuffers;
    GLuint Vao() const { return m_TriangleVAO->Handle(); }
    GLint EdgeMaskBytes() const {
        GLint bytes = 0;
        glGetNamedBufferParameteriv(m_EdgeMaskBuffer->Handle(), GL_BUFFER_SIZE, &bytes);
        return bytes;
    }
    int ConstantEdgeMask() const { return m_ConstantEdgeMask; }
    void Prepare() {
        m_AttributeIndex = 0; m_AttributeDimension = 0;
        m_UseColor = true; m_AttributeChanged = true;
        ConvertToDrawableData();
        if (m_RenderableMesh.SimplifiedMesh) m_RenderableMesh.SimplifiedMesh->ConvertToDrawableData();
        ConvertToDrawableData();
    }
    void CheckBuffers() {
        std::array<float, 9> xyz{};
        std::array<unsigned, 3> ids{};
        m_PositionVBO->GetSubData(0, sizeof(xyz), xyz.data());
        m_TriangleEBO->GetSubData(0, sizeof(ids), ids.data());
        Require(xyz[0] == -0.8f && xyz[3] == 0.8f && ids[0] == 0 && ids[1] == 1 && ids[2] == 2,
                "GPU-readback-coordinates-and-connectivity");
        for (GLuint attribute : {2u, 3u}) {
            GLint enabled = 0;
            glGetVertexArrayIndexediv(Vao(), attribute, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            Require(enabled == GL_FALSE, "empty-normal-and-UV-arrays-are-disabled");
        }
    }
};
static GLuint Shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr); glCompileShader(shader);
    GLint ok = 0; glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    Require(ok, "test-shader-compiled"); return shader;
}

// BUG: sharing the new shader with ordinary files changed their render path.
// Compare the actual main and C/S shader resources in one GL context, including
// constant and nonconstant masks and a local draw after a remote draw.
// Fix commit: 待提交 (C/S rendering isolation).
static void CheckWireframeShaders() {
    // BUG: a packaged test used __FILE__ to find shaders, so it failed on machines
    // without the build machine's source tree. Use the executable's Resources,
    // then its parent's Resources (multi-config build layout), before source fallback.
    // Check the complete shader set in the first existing directory: an incomplete
    // deployment must fail instead of silently testing unrelated source shaders.
    // Fix commit: 待提交 (portable prepared CPU/GPU validation shader lookup).
    const auto applicationDirectory = std::filesystem::u8path(
        QCoreApplication::applicationDirPath().toUtf8().constData());
    const std::array<std::filesystem::path, 3> candidates = {
        applicationDirectory / "Resources/Shaders",
        applicationDirectory.parent_path() / "Resources/Shaders",
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()
            / "iGameCore/Rendering/Shaders/GLSL"
    };
    std::filesystem::path root;
    for (const auto& candidate : candidates) {
        if (!std::filesystem::is_directory(candidate)) continue;
        for (const char* name : {"SinglePassWireframe.frag", "SinglePassWireframe.geom",
                                 "RemoteSinglePassWireframe.geom"}) {
            if (!std::filesystem::is_regular_file(candidate / name)) {
                throw std::runtime_error("Missing wireframe shader: " + (candidate / name).string());
            }
        }
        root = candidate;
        break;
    }
    Require(!root.empty(), "wireframe-shader-directory-found");
    auto source = [&](const char* name) {
        std::ifstream input(root / name);
        Require(input.good(), "actual-wireframe-shader-resource-found");
        std::ostringstream text; text << input.rdbuf(); return text.str();
    };
    const char* vertex = R"(#version 330 core
#extension GL_ARB_separate_shader_objects : enable
layout(location=0) out vec3 mc;
layout(location=1) out vec3 vc;
layout(location=2) out vec4 color;
layout(location=3) out vec3 normal;
layout(location=4) out vec2 uv;
void main() {
    vec2 p[6]=vec2[6](vec2(-.8,-.8),vec2(.8,-.8),vec2(.8,.8),
                     vec2(-.8,-.8),vec2(.8,.8),vec2(-.8,.8));
    mc=vec3(p[gl_VertexID],0); vc=vec3(p[gl_VertexID],-1);
    color=vec4(.4,.6,.8,1); normal=vec3(0,0,1); uv=vec2(0);
    gl_Position=vec4(mc,1);
})";
    const auto fragment = source("SinglePassWireframe.frag");
    auto program = [&](const char* geometryFile) {
        const auto geometry = source(geometryFile);
        GLuint vs=Shader(GL_VERTEX_SHADER,vertex), gs=Shader(GL_GEOMETRY_SHADER,geometry.c_str());
        GLuint fs=Shader(GL_FRAGMENT_SHADER,fragment.c_str());
        GLuint result=glCreateProgram();
        glAttachShader(result,vs); glAttachShader(result,gs); glAttachShader(result,fs); glLinkProgram(result);
        GLint ok=0; glGetProgramiv(result,GL_LINK_STATUS,&ok);
        if (!ok) { char log[4096]{}; glGetProgramInfoLog(result,sizeof(log),nullptr,log); std::cerr<<log<<std::endl; }
        Require(ok,"actual-wireframe-program-linked");
        glDeleteShader(vs); glDeleteShader(gs); glDeleteShader(fs); return result;
    };
    GLuint localProgram=program("SinglePassWireframe.geom");
    GLuint remoteProgram=program("RemoteSinglePassWireframe.geom");
    Require(glGetUniformLocation(localProgram,"constantEdgeMask")<0 &&
            glGetUniformLocation(remoteProgram,"constantEdgeMask")>=0,"local-and-remote-shaders-are-distinct");
    GLuint ubo[3]{}; glCreateBuffers(3,ubo);
    for (int i=0;i<3;++i) {
        std::array<unsigned,128> data{};
        if(i==0)data[3]=1; // CameraDataBlock.isOrtho
        if(i==2)data[1]=1; // UniformBufferObjectBlock.useNormalSmooth
        glNamedBufferData(ubo[i],sizeof(data),data.data(),GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER,i,ubo[i]);
    }
    GLuint vao=0,fbo=0,color=0,maskBuffer=0,maskTexture=0;
    glCreateVertexArrays(1,&vao); glCreateFramebuffers(1,&fbo);
    glCreateTextures(GL_TEXTURE_2D,1,&color); glTextureStorage2D(color,1,GL_RGBA8,64,64);
    glNamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,color,0);
    Require(glCheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"wireframe-framebuffer-ready");
    glCreateBuffers(1,&maskBuffer); glCreateTextures(GL_TEXTURE_BUFFER,1,&maskTexture);
    auto render = [&](GLuint shader,int constantMask) {
        glBindFramebuffer(GL_FRAMEBUFFER,fbo); glViewport(0,0,64,64);
        glDisable(GL_DITHER); glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader); glUniform1f(glGetUniformLocation(shader,"lineWidth"),2.0f);
        glUniform4f(glGetUniformLocation(shader,"vpDims"),0,0,64,64);
        glUniform1i(glGetUniformLocation(shader,"edgeMasks"),1);
        glUniform1i(glGetUniformLocation(shader,"edgeColorMode"),1);
        glUniform3f(glGetUniformLocation(shader,"edgeColor"),0,0,0);
        const auto location=glGetUniformLocation(shader,"constantEdgeMask");
        if(location>=0)glUniform1i(location,constantMask);
        glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,0,6);
        std::array<unsigned char,64*64*4> pixels{};
        glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        Require(pixels[(32*64+24)*4+3]==255,"wireframe-surface-is-visible");
        Require(glGetError()==GL_NO_ERROR,"actual-wireframe-draw-has-no-GL-errors");
        return pixels;
    };
    for(bool constant : {true,false}) {
        const unsigned char masks[2]={static_cast<unsigned char>(constant?7:3),static_cast<unsigned char>(constant?7:6)};
        glNamedBufferData(maskBuffer,sizeof(masks),masks,GL_STATIC_DRAW);
        glTextureBuffer(maskTexture,GL_R8,maskBuffer); glBindTextureUnit(1,maskTexture);
        const auto reference=render(localProgram,-1);
        Require(render(remoteProgram,constant?7:-1)==reference,
                constant?"remote-constant-mask-pixels-match-main":"remote-varying-mask-pixels-match-main");
        Require(render(localProgram,-1)==reference,"local-wireframe-unchanged-after-remote-draw");
    }
    glBindVertexArray(0); glBindFramebuffer(GL_FRAMEBUFFER,0); glUseProgram(0);
    glBindTextureUnit(1,0);
    for(int i=0;i<3;++i)glBindBufferBase(GL_UNIFORM_BUFFER,i,0);
    glDeleteBuffers(3,ubo); glDeleteTextures(1,&maskTexture); glDeleteBuffers(1,&maskBuffer);
    glDeleteTextures(1,&color); glDeleteFramebuffers(1,&fbo); glDeleteVertexArrays(1,&vao);
    glDeleteProgram(localProgram); glDeleteProgram(remoteProgram);
}

int main(int argc, char** argv) {
    const bool compareLegacy = argc > 1 && std::string(argv[1]) == "--compare-legacy-upload";
    QGuiApplication app(argc, argv);
    QSurfaceFormat format; format.setVersion(4, 6); format.setProfile(QSurfaceFormat::CoreProfile);
    QOffscreenSurface surface; surface.setFormat(format); surface.create();
    QOpenGLContext context; context.setFormat(format);
    if (!context.create() || !context.makeCurrent(&surface)) return 77;
    if (!gladLoadGL()) return 77;
    try {
        std::cout << "GPU " << glGetString(GL_RENDERER) << std::endl;
        CheckWireframeShaders();
        auto points = iGame::Points::New();
        points->AddPoint(-0.8f,-0.8f,0); points->AddPoint(0.8f,-0.8f,0); points->AddPoint(0,0.8f,0);
        auto faces = iGame::CellArray::New(); faces->AddCellId3(0,1,2);
        auto cp = iGame::FloatArray::New(); cp->SetName("PressureCoefficient");
        cp->AddValue(-1); cp->AddValue(0); cp->AddValue(1);
        // BUG: remote constant-mask uploads were applied to local models.
        // Ensure local texture bytes and remote constant masks coexist in one GL context.
        // Fix commit: 待提交 (C/S rendering isolation).
        // Separate reads have independent source arrays. Sharing Points here would
        // make each model's conversion advance the other's source MTime.
        auto localPoints = iGame::Points::New();
        localPoints->AddPoint(-0.8f,-0.8f,0); localPoints->AddPoint(0.8f,-0.8f,0); localPoints->AddPoint(0,0.8f,0);
        auto localFaces = iGame::CellArray::New(); localFaces->AddCellId3(0,1,2);
        auto localCp = iGame::FloatArray::New(); localCp->SetName("PressureCoefficient");
        localCp->AddValue(-1); localCp->AddValue(0); localCp->AddValue(1);
        auto local = Probe::New(); local->SetPoints(localPoints); local->SetFaces(localFaces);
        local->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, localCp); local->Prepare();
        local->SyncGpuBuffers();
        Require(local->EdgeMaskBytes() == 1 && local->ConstantEdgeMask() < 0,
                "ordinary-model-uploads-main-edge-mask-texture");
        auto mesh = Probe::New(); mesh->SetRemoteRenderingEnabled(true);
        mesh->SetPoints(points); mesh->SetFaces(faces);
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, cp); mesh->Prepare();
        const auto before = mesh->InspectCpuDisplayCache(); Require(before.ready, "prepared-state-ready");
        GLuint vs = Shader(GL_VERTEX_SHADER, "#version 460 core\nlayout(location=0) in vec3 p; layout(location=1) in vec4 c; out vec4 color; void main(){gl_Position=vec4(p,1);color=c;}");
        GLuint fs = Shader(GL_FRAGMENT_SHADER, "#version 460 core\nin vec4 color; out vec4 pixel; void main(){pixel=vec4(color.rgb,1);}");
        GLuint program = glCreateProgram(); glAttachShader(program, vs); glAttachShader(program, fs); glLinkProgram(program);
        GLint linked=0; glGetProgramiv(program,GL_LINK_STATUS,&linked); Require(linked,"test-program-linked");
        GLuint fbo=0, texture=0; glCreateFramebuffers(1,&fbo); glCreateTextures(GL_TEXTURE_2D,1,&texture);
        glTextureStorage2D(texture,1,GL_RGBA8,32,32); glNamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,texture,0);
        Require(glCheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"test-framebuffer-ready");
        std::array<unsigned char,32*32*4> reference{};
        for(int round=0;round<4;++round) {
            if(round==0) mesh->SyncGpuBuffers();
            else if (compareLegacy) mesh->SyncGpuBuffers();
            else Require(mesh->UploadPreparedCpuData(),"prepared-upload-only-succeeded");
            mesh->CheckBuffers();
            Require(mesh->EdgeMaskBytes() == 0 && mesh->ConstantEdgeMask() == 7,
                    "only-remote-model-uses-constant-edge-mask");
            local->SyncGpuBuffers();
            Require(!local->GetRemoteRenderingEnabled() && local->EdgeMaskBytes() == 1,
                    "remote-upload-does-not-change-local-model-policy");
            // The ordinary renderer still calls SyncGpuBuffers each frame.
            mesh->SyncGpuBuffers(); mesh->CheckBuffers();
            glBindFramebuffer(GL_FRAMEBUFFER,fbo); glViewport(0,0,32,32);
            glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT); glUseProgram(program);
            glBindVertexArray(mesh->Vao()); glDrawElements(GL_TRIANGLES,3,GL_UNSIGNED_INT,nullptr);
            std::array<unsigned char,32*32*4> pixels{};
            glReadPixels(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
            Require(pixels[(16*32+16)*4+3]==255,"triangle-visible-after-GPU-completion");
            if(round==0)reference=pixels;
            else Require(pixels==reference,"rendered-colors-and-geometry-identical-after-reopen");
            Require(glGetError()==GL_NO_ERROR,"real-GL-cycle-no-errors");
            Require(mesh->InspectCpuDisplayCache().signature==before.signature,"no-CPU-geometry-or-scalar-rebuild");
            glBindVertexArray(0); mesh->ReleaseGpuResourcesKeepCpuData();
            Require(!mesh->HasGpuResources(),"all-model-GPU-handles-released");
        }
        mesh->ReleaseDrawableResources();
        local->ReleaseDrawableResources();
        glUseProgram(0); glDeleteProgram(program); glDeleteShader(vs); glDeleteShader(fs);
        glBindFramebuffer(GL_FRAMEBUFFER,0); glDeleteFramebuffers(1,&fbo); glDeleteTextures(1,&texture);
        return 0;
    } catch(const std::exception& e) { std::cerr << "FAIL " << e.what() << std::endl; return 1; }
}
