//
// Created by Sumzeek on 12/9/2024.
//
#include "GLRenderBuffer.h"

IGAME_NAMESPACE_BEGIN

GLRenderBuffer::GLRenderBuffer() {}

GLRenderBuffer::~GLRenderBuffer() {}

void GLRenderBuffer::Storage(GLenum internal_format, unsigned width,
                             unsigned height) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindRenderbuffer(GL_RENDERBUFFER, m_Handle);
    glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
#elif IGAME_OPENGL_VERSION_460
    glNamedRenderbufferStorage(m_Handle, internal_format, width, height);
#endif
}

void GLRenderBuffer::CreateHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glGenRenderbuffers(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glCreateRenderbuffers(count, handles);
#endif
}

void GLRenderBuffer::DestroyHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glDeleteRenderbuffers(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glDeleteRenderbuffers(count, handles);
#endif
}

IGAME_NAMESPACE_END