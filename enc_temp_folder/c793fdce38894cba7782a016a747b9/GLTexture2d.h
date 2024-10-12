#pragma once

#include "GLImageHandle.h"
#include "GLObject.h"
#include "GLTextureHandle.h"

IGAME_NAMESPACE_BEGIN

class GLTexture2d : public GLObject<GLTexture2d> {
private:
    friend class GLObject<GLTexture2d>;
    static void createHandle(GLsizei count, GLuint* handles) {
        glCreateTextures(GL_TEXTURE_2D, count, handles);
    }
    static void destroyHandle(GLsizei count, GLuint* handles) {
         glDeleteTextures(count, handles);
    }

private:
    GLTexture2d(GLuint handle) : GLObject<GLTexture2d>{handle} {}

public:
    static void copyImageSubData(const GLTexture2d& source,
                                 GLint srcLevel, GLint srcX, GLint srcY,
                                 GLint srcZ, const GLTexture2d& destination,
                                  GLint dstLevel, GLint dstX,
                                 GLint dstY, GLint dstZ, GLsizei srcWidth,
                                 GLsizei srcHeight, GLsizei srcDepth) {
        glCopyImageSubData(source.handle, GL_TEXTURE_2D, srcLevel, srcX, srcY, srcZ,
                           destination.handle, GL_TEXTURE_2D, dstLevel, dstX, dstY,
                           dstZ, srcWidth, srcHeight, srcDepth);
    }

public:
    GLTexture2d() = default;
    static GLTexture2d view(GLenum target, const GLTexture2d& original,
                            GLenum internal_format, unsigned first_mip_level,
                            unsigned mip_level_count, unsigned first_layer,
                            unsigned layer_count) {
        GLuint handle;
        glGenTextures(1, &handle);
        glTextureView(handle, target, original, internal_format,
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
    void storage(unsigned mip_levels, GLenum internal_format, unsigned width,
                 unsigned height) const {
        glTextureStorage2D(handle, mip_levels, internal_format, width, height);
    }

    // GLenum format(Base Internal Format): GL_RED, GL_RG, GL_RGB, GL_RGBA
    // GLenum type:GL_UNSIGNED_BYTE, GL_FLOAT
    void subImage(unsigned mip_level, unsigned xoffset, unsigned yoffset,
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
    void parameteri(GLenum pname, GLint param) {
        glTextureParameteri(handle, pname, param);
    }
    // GLenum pname: GL_TEXTURE_BORDER_COLOR
    void parameterfv(GLenum pname, const GLfloat* params) {
        glTextureParameterfv(handle, pname, params);
    }

    void generateMipmap() { glGenerateTextureMipmap(handle); }

    // glUniformHandleui64ARB(glGetUniformLocation(shaderProgram,"tex0"), TextureHandle);
    GLTextureHandle getTextureHandle() const {
        return GLTextureHandle(glGetTextureHandleARB(handle));
    }

    GLImageHandle getImageHandle(unsigned level, bool layered, unsigned layer,
                                 GLenum format) const {
        return GLImageHandle(
                glGetImageHandleARB(handle, level, layered, layer, format));
    }

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15
    // GL_TEXTURE0 is reserved to prevent other binding operations from being performed after a texture unit is activated.
    void active(GLenum texture) {
        if (texture == GL_TEXTURE0) {
            throw std::runtime_error("GL_TEXTURE0 is reserved.");
        }
        glActiveTexture(texture);
        glBindTexture(GL_TEXTURE_2D, handle);
        glActiveTexture(GL_TEXTURE0);
    };

    void bind() const { glBindTexture(GL_TEXTURE_2D, handle); }
    void release() const { glBindTexture(GL_TEXTURE_2D, 0); }

    void bindImage(unsigned int binding_index, unsigned int mip_level,
                   bool layered, int layer, GLenum access, GLenum format) {
        glBindImageTexture(binding_index, handle, mip_level, layered, layer,
                           access, format);
    }
};

IGAME_NAMESPACE_END