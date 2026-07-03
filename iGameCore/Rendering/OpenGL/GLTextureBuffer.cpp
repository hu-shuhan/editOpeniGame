//
// Created by Sumzeek on 12/9/2024.
//
#include "GLTextureBuffer.h"

IGAME_NAMESPACE_BEGIN

GLTextureBuffer::GLTextureBuffer() {}

GLTextureBuffer::GLTextureBuffer(GLuint handle)
    : GLObject<GLTextureBuffer>{handle} {}

GLTextureBuffer::~GLTextureBuffer() {}

void GLTextureBuffer::Buffer(GLenum internalformat,
                             const SmartPointer<GLBuffer> buffer) {
#ifdef __EMSCRIPTEN__
    (void) internalformat;
    (void) buffer;
    return;
#endif
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_BUFFER, this->m_Handle);
    glTexBuffer(GL_TEXTURE_BUFFER, internalformat, buffer->Handle());
    glBindTexture(GL_TEXTURE_BUFFER, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureBuffer(m_Handle, internalformat, buffer->Handle());
#endif
}

void GLTextureBuffer::Active(GLenum texture) {
#ifdef __EMSCRIPTEN__
    (void) texture;
    return;
#endif
    if (texture == GL_TEXTURE0) {
        IGAME_RENDERING_ERROR(
                "[GLTextureBuffer::Active] Error: GL_TEXTURE0 is reserved and "
                "cannot be used for binding.");
        return;
    }
    glActiveTexture(texture);
    glBindTexture(GL_TEXTURE_BUFFER, m_Handle);
    glActiveTexture(GL_TEXTURE0);
}

void GLTextureBuffer::Bind() const {
#ifdef __EMSCRIPTEN__
    return;
#endif
    glBindTexture(GL_TEXTURE_BUFFER, m_Handle);
}

void GLTextureBuffer::Release() const {
#ifdef __EMSCRIPTEN__
    return;
#endif
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void GLTextureBuffer::BindImage(unsigned int binding_index,
                                unsigned int mip_level, bool layered, int layer,
                                GLenum access, GLenum format) {
#ifdef __EMSCRIPTEN__
    (void) binding_index;
    (void) mip_level;
    (void) layered;
    (void) layer;
    (void) access;
    (void) format;
    return;
#endif
#ifdef IGAME_OPENGL_VERSION_330
    IGAME_RENDERING_ERROR(
            "[GLTextureBuffer::BindImage] Error: This function is not "
            "supported in OpenGL 3.3. Please use OpenGL 4.6 or higher.");
#elif IGAME_OPENGL_VERSION_460
    glBindImageTexture(binding_index, m_Handle, mip_level, layered, layer,
                       access, format);
#endif
}

void GLTextureBuffer::CreateHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    #ifdef __EMSCRIPTEN__
    glGenTextures(count, handles);
    #else
    glGenTextures(count, handles);
    #endif
#elif IGAME_OPENGL_VERSION_460
    glCreateTextures(GL_TEXTURE_BUFFER, count, handles);
#endif
}

void GLTextureBuffer::DestroyHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glDeleteTextures(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glDeleteTextures(count, handles);
#endif
}

IGAME_NAMESPACE_END
