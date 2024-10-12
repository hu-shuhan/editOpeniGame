#pragma once

#include "GLBuffer.h"
#include "GLObject.h"
#include "GLVertexArrayManager.h"

IGAME_NAMESPACE_BEGIN

class GLVertexAttribute : public Object {
private:
    GLuint m_index;

public:
    explicit GLVertexAttribute(unsigned int location) : m_index{location} {}

    unsigned int index() const { return m_index; }
};

class GLVertexArray : public GLObject<GLVertexArray> {
private:
    friend class GLObject<GLVertexArray>;
    static void createHandle(GLsizei count, GLuint* handles) {
        glGenVertexArrays(count, handles);
    }
    static void destroyHandle(GLsizei count, GLuint* handles) {
        GLVertexArrayManager& manager = GLVertexArrayManager::Instance();
        for (GLsizei i = 0; i < count; ++i) {
            manager.UnRegisterVertexArray(handles[i]);
        }
        glDeleteVertexArrays(count, handles);
    }

public:
    void bind() const { glBindVertexArray(handle); }
    void release() const { glBindVertexArray(0); }

    void vertexBuffer(unsigned int vbo_binding_index, GLBuffer& buffer,
                      ptrdiff_t offset, size_t stride) {
        if (offset != 0) {
            throw std::runtime_error(
                    "You are trying to offset the VBO in the opengl330 "
                    "version, which is illegal. Please check your code.");
        }
        GLVertexArrayManager& manager = GLVertexArrayManager::Instance();
        manager.RegisterBufferToVertexArray(handle, vbo_binding_index, buffer,
                                            stride);
    }

    void elementBuffer(GLBuffer& buffer) {
        glBindVertexArray(handle);
        buffer.target(GL_ELEMENT_ARRAY_BUFFER);
        buffer.bind();
        glBindVertexArray(0);
    }

    void enableAttrib(const GLVertexAttribute& attribute) {
        glBindVertexArray(handle);
        glEnableVertexAttribArray(attribute.index());
        glBindVertexArray(0);
    }

    void attribBindingFormat(const GLVertexAttribute& attribute,
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
        glVertexAttribPointer(attribute.index(), size, type, normalized, stride,
                              reinterpret_cast<void*>(offset));
        glBindVertexArray(0);
    }
};

//Per-vertex attributes binding indices
inline const GLVertexAttribute GL_LOCATION_IDX_0{0};
inline const GLVertexAttribute GL_LOCATION_IDX_1{1};
inline const GLVertexAttribute GL_LOCATION_IDX_2{2};
inline const GLVertexAttribute GL_LOCATION_IDX_3{3};

inline void GLAllocateGLBuffer(GLBuffer& vbo, size_t size, const void* data) {
    vbo.allocate(size, data, GL_STATIC_DRAW);
}

inline void GLSetVertexAttrib(GLVertexArray& VAO,
                              const GLVertexAttribute& attribute,
                              GLuint vbo_binding_index, int size, GLenum type,
                              GLboolean normalized, unsigned int offset) {
    VAO.enableAttrib(attribute);
    VAO.attribBindingFormat(attribute, vbo_binding_index, size, type,
                            normalized, offset);
}

IGAME_NAMESPACE_END