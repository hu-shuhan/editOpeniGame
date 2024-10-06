#pragma once

#include "GLBuffer.h"
#include "GLImageHandle.h"
#include "GLObject.h"
#include "GLTextureHandle.h"

IGAME_NAMESPACE_BEGIN

class GLTextureBuffer : public GLObject<GLTextureBuffer> {
private:
    friend class GLObject<GLTextureBuffer>;
    static void createHandle(GLsizei count, GLuint* handles) {
        glCreateTextures(GL_TEXTURE_BUFFER, count, handles);
    }
    static void destroyHandle(GLsizei count, GLuint* handles) {
        glDeleteTextures(count, handles);
    }

private:
    GLTextureBuffer(GLuint handle) : GLObject<GLTextureBuffer>{handle} {}

public:
    GLTextureBuffer() = default;

    // GLenum internal_format: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    // GLenum internal_format(Sized Internal Format): GL_R8, GL_RG8, GL_RGB8, GL_RGBA8
    // GLenum internal_format(Sized Internal Format): GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24
    // GLenum internal_format(Sized Internal Format): GL_DEPTH32F_STENCIL8, GL_DEPTH24_STENCIL8
    // GLenum internal_format(Sized Internal Format): GL_STENCIL_INDEX8
    void buffer(GLenum internalformat, const GLBuffer& buffer) {
        glTextureBuffer(handle, internalformat, buffer);
    }

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15
    // GL_TEXTURE0 is reserved to prevent other binding operations from being performed after a texture unit is activated.
    void active(GLenum texture) {
        if (texture == GL_TEXTURE0) {
            throw std::runtime_error("GL_TEXTURE0 is reserved.");
        }
        glActiveTexture(texture);
        glBindTexture(GL_TEXTURE_BUFFER, handle);
        glActiveTexture(GL_TEXTURE0);
    };

    void bind() const { glBindTexture(GL_TEXTURE_BUFFER, handle); }
    void release() const { glBindTexture(GL_TEXTURE_BUFFER, 0); }

    void bindImage(unsigned int binding_index, unsigned int mip_level,
                   bool layered, int layer, GLenum access, GLenum format) {
        glBindImageTexture(binding_index, handle, mip_level, layered, layer,
                           access, format);
    }
};

IGAME_NAMESPACE_END