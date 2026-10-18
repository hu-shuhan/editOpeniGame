#include "iGameEnvironmentLight.h"
#ifndef __EMSCRIPTEN__
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_HDR
#define STBI_WINDOWS_UTF8
#include "ThirdParty/stb/stb_image.h"
#include <algorithm>
#include <cmath>
#include <memory>

IGAME_NAMESPACE_BEGIN
namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr unsigned DiffuseUnit = 6;
constexpr unsigned SpecularUnit = 7;

// Capture is also used lazily during rendering: leave the caller's state intact.
struct CaptureState {
    GLint drawFbo, readFbo, program, vao, viewport[4], active, unpack, unpackBuffer;
    GLint textures2d[2], cubes[2], samplers[2], polygon[2];
    GLboolean colorMask[4];
    const GLenum capabilities[8] = {GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE,
        GL_SCISSOR_TEST, GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD,
        GL_COLOR_LOGIC_OP, GL_TEXTURE_CUBE_MAP_SEAMLESS};
    GLboolean enabled[8];
    CaptureState() {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
        glGetIntegerv(GL_POLYGON_MODE, polygon);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
        for (unsigned i = 0; i < 2; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &textures2d[i]);
            glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &cubes[i]);
            glGetIntegeri_v(GL_SAMPLER_BINDING, i, &samplers[i]);
            glBindSampler(i, 0);
        }
        for (unsigned i = 0; i < 8; ++i) {
            enabled[i] = glIsEnabled(capabilities[i]);
            glDisable(capabilities[i]);
        }
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    ~CaptureState() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
        glUseProgram(program);
        glBindVertexArray(vao);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, unpack);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
        glPolygonMode(GL_FRONT_AND_BACK, polygon[0]);
        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
        for (unsigned i = 0; i < 8; ++i) {
            if (enabled[i]) glEnable(capabilities[i]);
            else glDisable(capabilities[i]);
        }
        for (unsigned i = 0; i < 2; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures2d[i]);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubes[i]);
            glBindSampler(i, samplers[i]);
        }
        glActiveTexture(active);
    }
};

// Own transient capture objects on both successful and failed initialization.
struct CaptureObjects {
    GLuint panorama = 0, fbo = 0, vao = 0;
    ~CaptureObjects() {
        glDeleteTextures(1, &panorama);
        glDeleteFramebuffers(1, &fbo);
        glDeleteVertexArrays(1, &vao);
    }
};
} // namespace

bool EnvironmentLight::Initialize(const std::string& hdrPath,
                                  const std::string& shaderDirectory) {
    if (m_Attempted) return m_Ready;
    m_Attempted = true;
    int width = 0, height = 0, channels = 0;
    std::unique_ptr<float, decltype(&stbi_image_free)> pixels(
        stbi_loadf(hdrPath.c_str(), &width, &height, &channels, 3), stbi_image_free);
    if (!pixels || width != 2 * height || height < 2) {
        IGAME_RENDERING_WARN("HDR environment unavailable: {}. Using previous lighting.", hdrPath);
        return false;
    }

    // Normalize exposure, preserving the RGB ratios. This makes intensity useful
    // across differently exposed captures. Cos(latitude) accounts for solid angle.
    double luminance = 0.0, weight = 0.0;
    for (int y = 0; y < height; ++y) {
        const double w = std::sin(Pi * (y + 0.5) / height);
        for (int x = 0; x < width; ++x) {
            float* p = pixels.get() + (static_cast<size_t>(y) * width + x) * 3;
            for (int c = 0; c < 3; ++c) {
                if (!std::isfinite(p[c]) || p[c] < 0.0f) {
                    IGAME_RENDERING_WARN("Invalid radiance in HDR environment: {}", hdrPath);
                    return false;
                }
            }
            luminance += w * (0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2]);
            weight += w;
        }
    }
    if (luminance <= 1e-12) return false;
    const double scale = 0.35 * weight / luminance;
    for (size_t i = 0; i < static_cast<size_t>(width) * height * 3; ++i)
        pixels.get()[i] = static_cast<float>(std::min(60000.0, pixels.get()[i] * scale));

    CaptureState state;
    CaptureObjects capture;
    auto program = GLShaderProgram::New();
    program->AddShaders(GLShader::CreateShader(shaderDirectory + "/EnvironmentCapture.vert", GL_VERTEX_SHADER),
                        GLShader::CreateShader(shaderDirectory + "/EnvironmentCapture.frag", GL_FRAGMENT_SHADER));
    GLint linked = GL_FALSE;
    glGetProgramiv(program->ProgramID(), GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) return false;
    program->Use();
    program->SetUniformi("uPanorama", 0);
    program->SetUniformi("uEnvironment", 1);
    glGenVertexArrays(1, &capture.vao);
    glBindVertexArray(capture.vao);
    glGenFramebuffers(1, &capture.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, capture.fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &capture.panorama);
    glBindTexture(GL_TEXTURE_2D, capture.panorama);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, pixels.get());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    auto environment = GLTextureCube::New();
    auto diffuse = GLTextureCube::New();
    auto specular = GLTextureCube::New();
    environment->Create(); environment->Storage(10, 512);
    diffuse->Create(); diffuse->Storage(1, 32);
    specular->Create(); specular->Storage(8, 128);
    // Samplers of different types must use distinct units, even in the conversion pass.
    diffuse->Bind(1);

    auto render = [&](const SmartPointer<GLTextureCube>& target,
                      unsigned size, unsigned levels, int pass) {
        program->SetUniformi("uPass", pass);
        for (unsigned level = 0; level < levels; ++level) {
            const unsigned extent = std::max(1u, size >> level);
            glViewport(0, 0, extent, extent);
            program->SetUniformf("uSize", static_cast<float>(extent));
            program->SetUniformf("uRoughness", levels > 1 ? float(level) / (levels - 1) : 0.0f);
            for (int face = 0; face < 6; ++face) {
                program->SetUniformi("uFace", face);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, target->Handle(), level);
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return false;
                glDrawArrays(GL_TRIANGLES, 0, 3);
            }
        }
        return true;
    };
    // Conversion must not sample the cubemap being written.
    if (!render(environment, 512, 1, 0)) return false;
    environment->GenerateMipmap();
    environment->Bind(1);
    if (!render(diffuse, 32, 1, 1) || !render(specular, 128, 8, 2)) return false;
    m_Diffuse = diffuse;
    m_Specular = specular;
    m_Environment = environment;
    m_BackgroundProgram = GLShaderProgram::New();
    m_BackgroundProgram->AddShaders(
        GLShader::CreateShader(shaderDirectory + "/FullScreenTriangle.vert", GL_VERTEX_SHADER),
        GLShader::CreateShader(shaderDirectory + "/EnvironmentBackground.frag", GL_FRAGMENT_SHADER));
    glGetProgramiv(m_BackgroundProgram->ProgramID(), GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        m_BackgroundVAO = GLVertexArray::New();
        m_BackgroundVAO->Create();
    } else {
        m_BackgroundProgram = nullptr;
        IGAME_RENDERING_WARN("Environment background shader unavailable. Using existing background.");
    }
    m_Ready = true;
    IGAME_RENDERING_INFO("HDR environment ready: {} ({} x {})", hdrPath, width, height);
    return true;
}

void EnvironmentLight::Release() {
    m_Diffuse = nullptr;
    m_Specular = nullptr;
    m_Environment = nullptr;
    m_BackgroundProgram = nullptr;
    m_BackgroundVAO = nullptr;
    m_Attempted = false;
    m_Ready = false;
}

void EnvironmentLight::DrawBackground(const igm::mat4& view, const igm::mat4& projection,
                                      float rotationDegrees,
                                      int width, int height) const {
    if (!m_Ready || !m_BackgroundProgram || width <= 0 || height <= 0) return;
    CaptureState state;
    // No depth test or depth writes: geometry, OIT and HZB see the original clear depth.
    glViewport(0, 0, width, height);
    m_BackgroundProgram->Use();
    m_BackgroundProgram->SetUniformi("uEnvironment", 0);
    m_BackgroundProgram->SetUniformMatrix4x4("uView", view);
    m_BackgroundProgram->SetUniformMatrix4x4("uInverseProjection", projection.invert());
    m_BackgroundProgram->SetUniformf("uRotation", rotationDegrees * (Pi / 180.0f));
    m_Environment->Bind(0);
    m_BackgroundVAO->DrawArrays(GL_TRIANGLES, 0, 3);
}

void EnvironmentLight::Bind(const SmartPointer<GLShaderProgram>& shader,
                             bool enabled, float intensity, float rotationDegrees) const {
    shader->SetUniformi("uEnvironmentEnabled", enabled && m_Ready ? 1 : 0);
    shader->SetUniformf("uEnvironmentIntensity", intensity);
    shader->SetUniformf("uEnvironmentRotation", rotationDegrees * (Pi / 180.0f));
    shader->SetUniformi("uEnvironmentDiffuse", DiffuseUnit);
    shader->SetUniformi("uEnvironmentSpecular", SpecularUnit);
    if (enabled && m_Ready) {
        m_Diffuse->Bind(DiffuseUnit);
        m_Specular->Bind(SpecularUnit);
    }
}
IGAME_NAMESPACE_END
#endif
