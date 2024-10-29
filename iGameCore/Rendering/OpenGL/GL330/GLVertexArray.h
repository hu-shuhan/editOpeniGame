#pragma once

#include "GLBuffer.h"
#include "GLObject.h"
#include "GLVertexArrayManager.h"

IGAME_NAMESPACE_BEGIN

class GLVertexAttribute : public Object {
public:
    explicit GLVertexAttribute(unsigned int location) : m_index{location} {}

    unsigned int Index() const { return m_index; }

protected:
    GLuint m_index;
};

class GLVertexArray : public GLObject<GLVertexArray> {
public:
    I_OBJECT(GLVertexArray);
    static Pointer New() { return new GLVertexArray; }

    void Bind() const { glBindVertexArray(handle); }
    void Release() const { glBindVertexArray(0); }

    void VertexBuffer(unsigned int vbo_binding_index, GLBuffer::Pointer buffer,
                      ptrdiff_t offset, size_t stride) {
        if (offset != 0) {
            igError("You are trying to offset the VBO in the opengl330 "
                    "version, which is illegal. Please check your code.");
            throw std::runtime_error(
                    "You are trying to offset the VBO in the opengl330 "
                    "version, which is illegal. Please check your code.");
        }
        GLVertexArrayManager& manager = GLVertexArrayManager::Instance();
        manager.RegisterBufferToVertexArray(handle, vbo_binding_index,
                                            buffer->Handle(), stride);
    }

    void ElementBuffer(GLBuffer::Pointer buffer) {
        glBindVertexArray(handle);
        buffer->Target(GL_ELEMENT_ARRAY_BUFFER);
        buffer->Bind();
        glBindVertexArray(0);
    }

    void EnableAttrib(const GLVertexAttribute& attribute) {
        glBindVertexArray(handle);
        glEnableVertexAttribArray(attribute.Index());
        glBindVertexArray(0);
    }

    void AttribBindingFormat(const GLVertexAttribute& attribute,
                             unsigned int vbo_binding_index, int size,
                             GLenum type, bool normalized,
                             unsigned int relative_offset) {
        auto buffer = GLVertexArrayManager::Instance().GetBuffer(
                handle, vbo_binding_index);
        auto stride = GLVertexArrayManager::Instance().GetStride(
                handle, vbo_binding_index);
        GLintptr offset = static_cast<uintptr_t>(relative_offset);

        glBindVertexArray(handle);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glVertexAttribPointer(attribute.Index(), size, type, normalized, stride,
                              reinterpret_cast<void*>(offset));
        glBindVertexArray(0);
    }

protected:
    GLVertexArray() = default;
    ~GLVertexArray() override = default;

    friend class GLObject<GLVertexArray>;
    static void CreateHandle(GLsizei count, GLuint* handles) {
        glGenVertexArrays(count, handles);
    }
    static void DestroyHandle(GLsizei count, GLuint* handles) {
        GLVertexArrayManager& manager = GLVertexArrayManager::Instance();
        for (GLsizei i = 0; i < count; ++i) {
            manager.UnRegisterVertexArray(handles[i]);
        }
        glDeleteVertexArrays(count, handles);
    }
};

//Per-vertex attributes binding indices
inline const GLVertexAttribute GL_LOCATION_IDX_0{0};
inline const GLVertexAttribute GL_LOCATION_IDX_1{1};
inline const GLVertexAttribute GL_LOCATION_IDX_2{2};
inline const GLVertexAttribute GL_LOCATION_IDX_3{3};

inline void GLAllocateGLBuffer(GLBuffer::Pointer vbo, size_t size,
                               const void* data) {
    vbo->Allocate(size, data, GL_STATIC_DRAW);
}

inline void GLSetVertexAttrib(GLVertexArray::Pointer VAO,
                              const GLVertexAttribute& attribute,
                              GLuint vbo_binding_index, int size, GLenum type,
                              GLboolean normalized, unsigned int offset) {
    VAO->EnableAttrib(attribute);
    VAO->AttribBindingFormat(attribute, vbo_binding_index, size, type,
                             normalized, offset);
}

IGAME_NAMESPACE_END