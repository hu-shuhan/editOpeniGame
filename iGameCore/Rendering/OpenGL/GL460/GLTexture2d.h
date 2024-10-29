#pragma once

#include "GLImageHandle.h"
#include "GLObject.h"
#include "GLTextureHandle.h"

IGAME_NAMESPACE_BEGIN

class GLTexture2d : public GLObject<GLTexture2d> {
public:
    I_OBJECT(GLTexture2d);
    static Pointer New() { return new GLTexture2d; }

    static void CopyImageSubData(const GLTexture2d::Pointer source,
                                 GLint srcLevel, GLint srcX, GLint srcY,
                                 GLint srcZ,
                                 const GLTexture2d::Pointer destination,
                                 GLint dstLevel, GLint dstX, GLint dstY,
                                 GLint dstZ, GLsizei srcWidth,
                                 GLsizei srcHeight, GLsizei srcDepth) {
        glCopyImageSubData(source->handle, GL_TEXTURE_2D, srcLevel, srcX, srcY,
                           srcZ, destination->handle, GL_TEXTURE_2D, dstLevel,
                           dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
    }

public:
    static GLTexture2d View(GLenum target, const GLTexture2d::Pointer original,
                            GLenum internal_format, unsigned first_mip_level,
                            unsigned mip_level_count, unsigned first_layer,
                            unsigned layer_count) {
        GLuint handle;
        glGenTextures(1, &handle);
        glTextureView(handle, target, original->handle, internal_format,
                      first_mip_level, mip_level_count, first_layer,
                      layer_count);
        return GLTexture2d(handle);
    }

public:
    // GLenum internal_format: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    // GLenum internal_format(Sized Internal Format): GL_R8, GL_RG8, GL_RGB8, GL_RGBA8
    // GLenum internal_format(Sized Internal Format): GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24
    // GLenum internal_format(Sized Internal Format): GL_DEPTH32F_STENCIL8, GL_DEPTH24_STENCIL8
    // GLenum internal_format(Sized Internal Format): GL_STENCIL_INDEX8
    void Storage(unsigned mip_levels, GLenum internal_format, unsigned width,
                 unsigned height) const {
        glTextureStorage2D(handle, mip_levels, internal_format, width, height);
    }

    // GLenum format(Base Internal Format): GL_RED, GL_RG, GL_RGB, GL_RGBA
    // GLenum type:GL_UNSIGNED_BYTE, GL_FLOAT
    void SubImage(unsigned mip_level, unsigned xoffset, unsigned yoffset,
                  unsigned width, unsigned height, GLenum format, GLenum type,
                  const void* pixels) {
        glTextureSubImage2D(handle, mip_level, xoffset, yoffset, width, height,
                            format, type, pixels);
    }

    // GLenum pname: GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T
    // GLint param: GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    // -------------------------------------------------------------------------
    // GLenum pname: GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER
    // GLint param: GL_NEAREST, GL_LINEAR
    // GLint param(GL_TEXTURE_MIN_FILTER): GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST
    // GLint param(GL_TEXTURE_MIN_FILTER): GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
    void Parameteri(GLenum pname, GLint param) {
        glTextureParameteri(handle, pname, param);
    }
    // GLenum pname: GL_TEXTURE_BORDER_COLOR
    void Parameterfv(GLenum pname, const GLfloat* params) {
        glTextureParameterfv(handle, pname, params);
    }

    void GenerateMipmap() { glGenerateTextureMipmap(handle); }

    // glUniformHandleui64ARB(glGetUniformLocation(shaderProgram,"tex0"), TextureHandle);
    GLTextureHandle GetTextureHandle() const {
        return GLTextureHandle(glGetTextureHandleARB(handle));
    }

    GLImageHandle GetImageHandle(unsigned level, bool layered, unsigned layer,
                                 GLenum format) const {
        return GLImageHandle(
                glGetImageHandleARB(handle, level, layered, layer, format));
    }

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15
    // GL_TEXTURE0 is reserved to prevent other binding operations from being performed after a texture unit is activated.
    void Active(GLenum texture) {
        if (texture == GL_TEXTURE0) {
            igError("GL_TEXTURE0 is reserved.");
            throw std::runtime_error("GL_TEXTURE0 is reserved.");
        }
        glActiveTexture(texture);
        glBindTexture(GL_TEXTURE_2D, handle);
        glActiveTexture(GL_TEXTURE0);
    };

    void Bind() const { glBindTexture(GL_TEXTURE_2D, handle); }
    void Release() const { glBindTexture(GL_TEXTURE_2D, 0); }

    void BindImage(unsigned int binding_index, unsigned int mip_level,
                   bool layered, int layer, GLenum access, GLenum format) {
        glBindImageTexture(binding_index, handle, mip_level, layered, layer,
                           access, format);
    }

protected:
    GLTexture2d() = default;
    GLTexture2d(GLuint handle) : GLObject<GLTexture2d>{handle} {}
    ~GLTexture2d() override = default;

    friend class GLObject<GLTexture2d>;
    static void CreateHandle(GLsizei count, GLuint* handles) {
        glCreateTextures(GL_TEXTURE_2D, count, handles);
    }
    static void DestroyHandle(GLsizei count, GLuint* handles) {
        glDeleteTextures(count, handles);
    }
};

IGAME_NAMESPACE_END