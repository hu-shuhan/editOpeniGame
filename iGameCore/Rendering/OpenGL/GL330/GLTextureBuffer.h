#pragma once

#include "GLBuffer.h"
#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLTextureBuffer : public GLObject<GLTextureBuffer> {
private:
    friend class GLObject<GLTextureBuffer>;
    static void createHandle(GLsizei count, GLuint* handles) {
        glGenTextures(count, handles);
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
        glTexBuffer(GL_TEXTURE_BUFFER, internalformat, buffer);
    }

    void bindImage(unsigned int binding_index, unsigned int mip_level,
                   bool layered, int layer, GLenum access, GLenum format) {
        throw std::runtime_error("You called the GLTextureBuffer::bindImage "
                                 "function on the opengl330. "
                                 "This function is currently not supported.");
        //glBindImageTexture(binding_index, handle, mip_level, layered, layer,
        //                   access, format);
    }
};

IGAME_NAMESPACE_END