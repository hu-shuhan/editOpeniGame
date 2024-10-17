#pragma once

#include "GLImageHandle.h"
#include "GLObject.h"
#include "GLTextureHandle.h"

IGAME_NAMESPACE_BEGIN

class GLTexture2dMultisample : public GLObject<GLTexture2dMultisample> {
public:
    I_OBJECT(GLTexture2dMultisample);
    static Pointer New() { return new GLTexture2dMultisample; }

    static void
    CopyImageSubData(const GLTexture2dMultisample::Pointer source,
                     GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ,
                     const GLTexture2dMultisample::Pointer destination,
                     GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ,
                     GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
        glCopyImageSubData(source->handle, GL_TEXTURE_2D_MULTISAMPLE, srcLevel,
                           srcX, srcY, srcZ, destination->handle,
                           GL_TEXTURE_2D_MULTISAMPLE, dstLevel, dstX, dstY,
                           dstZ, srcWidth, srcHeight, srcDepth);
    }

public:
    // GLenum internal_format: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    // Sized Internal Format: GL_R8, GL_RG8, GL_RGB8, GL_RGBA8
    // Sized Internal Format: GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24
    // Sized Internal Format: GL_DEPTH32F_STENCIL8, GL_DEPTH24_STENCIL8
    // Sized Internal Format: GL_STENCIL_INDEX8
    void Storage(unsigned samples, GLenum internal_format, unsigned width,
                 unsigned height, bool fixedsamplelocations) const {
        glTextureStorage2DMultisample(handle, samples, internal_format, width,
                                      height, fixedsamplelocations);
    }

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15
    // GL_TEXTURE0 is reserved to prevent other binding operations from being performed after a texture unit is activated.
    void Active(GLenum texture) {
        if (texture == GL_TEXTURE0) {
            throw std::runtime_error("GL_TEXTURE0 is reserved.");
        }
        glActiveTexture(texture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, handle);
        glActiveTexture(GL_TEXTURE0);
    };

    // GLenum pname: GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER
    // GLint param: GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, GL_NEAREST, GL_LINEAR
    void Parameteri(GLenum pname, GLint param) {
        glTextureParameteri(handle, pname, param);
    }

    // glUniformHandleui64ARB(glGetUniformLocation(shaderProgram,"tex0"), TextureHandle);
    GLTextureHandle GetTextureHandle() const {
        return GLTextureHandle(glGetTextureHandleARB(handle));
    }

    GLImageHandle GetImageHandle(unsigned level, bool layered, unsigned layer,
                                 GLenum format) const {
        return GLImageHandle(
                glGetImageHandleARB(handle, level, layered, layer, format));
    }

    void Bind() const { glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, handle); }
    void Release() const { glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0); }

protected:
    GLTexture2dMultisample() = default;
    //GLTexture2dMultisample(GLuint handle) : GLObject<GLTexture2dMultisample>{handle} {}
    ~GLTexture2dMultisample() override = default;

    friend class GLObject<GLTexture2dMultisample>;
    static void CreateHandle(GLsizei count, GLuint* handles) {
        glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, count, handles);
    }
    static void DestroyHandle(GLsizei count, GLuint* handles) {
        glDeleteTextures(count, handles);
    }
};

IGAME_NAMESPACE_END