//
// Created by Sumzeek on 12/9/2024.
//
#include "GLVertexArray.h"

IGAME_NAMESPACE_BEGIN

GLVertexAttribute::GLVertexAttribute(unsigned int location) {
    m_Index = location;
}

GLVertexAttribute::~GLVertexAttribute() {}

unsigned int GLVertexAttribute::Index() const { return m_Index; }

GLVertexArray::GLVertexArray() {}

GLVertexArray::~GLVertexArray() {}

void GLVertexArray::Bind() const { glBindVertexArray(m_Handle); }

void GLVertexArray::Release() const { glBindVertexArray(0); }

void GLVertexArray::DrawArrays(GLenum mode, GLint first, GLsizei count) {
    glBindVertexArray(m_Handle);
    glDrawArrays(mode, first, count);
    glBindVertexArray(0);
}

void GLVertexArray::DrawElements(GLenum mode, int elementCount, GLenum type,
                                 const void* indices) {
    glBindVertexArray(m_Handle);
    glDrawElements(mode, elementCount, type, indices);
    glBindVertexArray(0);
}

void GLVertexArray::DrawRangeElements(GLenum mode, GLuint start, GLuint end,
                                      GLsizei count, GLenum type,
                                      const void* indices) {
    glBindVertexArray(m_Handle);
    glDrawRangeElements(mode, start, end, count, type, indices);
    glBindVertexArray(0);
}

void GLVertexArray::VertexBuffer(unsigned int vbo_binding_index,
                                 SmartPointer<GLBuffer> buffer,
                                 ptrdiff_t offset, size_t stride) {
#ifdef IGAME_OPENGL_VERSION_330
    if (offset != 0) {
        Logger::LogError("You are trying to offset the VBO in the opengl330 "
                         "version, which is illegal. Please check your code.");
    }
    GLVertexArrayManager& manager = GLVertexArrayManager::Instance();
    manager.RegisterBufferToVertexArray(m_Handle, vbo_binding_index,
                                        buffer->Handle(), stride);
#elif IGAME_OPENGL_VERSION_460
    glVertexArrayVertexBuffer(m_Handle, vbo_binding_index, buffer->Handle(),
                              offset, stride);
#endif
}

void GLVertexArray::ElementBuffer(SmartPointer<GLBuffer> buffer) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindVertexArray(m_Handle);
    buffer->Target(GL_ELEMENT_ARRAY_BUFFER);
    buffer->Bind();
    glBindVertexArray(0);
#elif IGAME_OPENGL_VERSION_460
    glVertexArrayElementBuffer(m_Handle, buffer->Handle());
#endif
}

void GLVertexArray::EnableAttrib(const GLVertexAttribute& attribute) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindVertexArray(m_Handle);
    glEnableVertexAttribArray(attribute.Index());
    glBindVertexArray(0);
#elif IGAME_OPENGL_VERSION_460
    glEnableVertexArrayAttrib(m_Handle, attribute.Index());
#endif
}

void GLVertexArray::AttribBindingFormat(const GLVertexAttribute& attribute,
                                        unsigned int vbo_binding_index,
                                        int size, GLenum type, bool normalized,
                                        unsigned int relative_offset) {
#ifdef IGAME_OPENGL_VERSION_330
    auto buffer = GLVertexArrayManager::Instance().GetBuffer(m_Handle,
                                                             vbo_binding_index);
    auto stride = GLVertexArrayManager::Instance().GetStride(m_Handle,
                                                             vbo_binding_index);
    GLintptr offset = static_cast<uintptr_t>(relative_offset);

    glBindVertexArray(m_Handle);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(attribute.Index(), size, type, normalized, stride,
                          reinterpret_cast<void*>(offset));
    glBindVertexArray(0);
#elif IGAME_OPENGL_VERSION_460
    glVertexArrayAttribBinding(m_Handle, attribute.Index(), vbo_binding_index);
    glVertexArrayAttribFormat(m_Handle, attribute.Index(), size, type,
                              normalized, relative_offset);
#endif
}

void GLVertexArray::CreateHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glGenVertexArrays(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glCreateVertexArrays(count, handles);
#endif
}

void GLVertexArray::DestroyHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    GLVertexArrayManager& manager = GLVertexArrayManager::Instance();
    for (GLsizei i = 0; i < count; ++i) {
        manager.UnRegisterVertexArray(handles[i]);
    }
    glDeleteVertexArrays(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glDeleteVertexArrays(count, handles);
#endif
}

void GLSetVertexAttrib(SmartPointer<GLVertexArray> VAO,
                       const GLVertexAttribute& attribute,
                       GLuint vbo_binding_index, int size, GLenum type,
                       GLboolean normalized, unsigned int offset) {
    VAO->EnableAttrib(attribute);
    VAO->AttribBindingFormat(attribute, vbo_binding_index, size, type,
                             normalized, offset);
}

IGAME_NAMESPACE_END
