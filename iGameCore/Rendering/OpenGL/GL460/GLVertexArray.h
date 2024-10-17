#pragma once

#include "GLBuffer.h"
#include "GLObject.h"

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

    void VertexBuffer(unsigned int binding_index,
                      const GLBuffer::Pointer buffer, ptrdiff_t offset,
                      size_t stride) {
        glVertexArrayVertexBuffer(handle, binding_index, buffer->Handle(),
                                  offset, stride);
    }

    void ElementBuffer(const GLBuffer::Pointer buffer) {
        glVertexArrayElementBuffer(handle, buffer->Handle());
    }

    void EnableAttrib(const GLVertexAttribute& attribute) {
        glEnableVertexArrayAttrib(handle, attribute.Index());
    }

    // GLVertexAttribute is one attribute in binding_index of VAO
    void AttribBinding(const GLVertexAttribute& attribute,
                       unsigned int binding_index) {
        glVertexArrayAttribBinding(handle, attribute.Index(), binding_index);
    }

    void AttribFormat(const GLVertexAttribute& attribute, int size, GLenum type,
                      bool normalized, unsigned int relative_offset) {
        glVertexArrayAttribFormat(handle, attribute.Index(), size, type,
                                  normalized, relative_offset);
    }

    void AttribFormati(GLVertexAttribute& attribute, int size, GLenum type,
                       unsigned int relative_offset) {
        glVertexArrayAttribIFormat(handle, attribute.Index(), size, type,
                                   relative_offset);
    }

    void BindingDivisor(unsigned int binding_index, int divisor) const {
        glVertexArrayBindingDivisor(handle, binding_index, divisor);
    }

protected:
    GLVertexArray() = default;
    ~GLVertexArray() override = default;

    friend class GLObject<GLVertexArray>;
    static void CreateHandle(GLsizei count, GLuint* handles) {
        glCreateVertexArrays(count, handles);
    }
    static void DestroyHandle(GLsizei count, GLuint* handles) {
        glDeleteVertexArrays(count, handles);
    }
};

//Per-vertex attributes binding indices
inline const GLVertexAttribute GL_LOCATION_IDX_0{0};
inline const GLVertexAttribute GL_LOCATION_IDX_1{1};
inline const GLVertexAttribute GL_LOCATION_IDX_2{2};
inline const GLVertexAttribute GL_LOCATION_IDX_3{3};

inline static void GLSetVertexAttrib(GLVertexArray::Pointer VAO,
                                     const GLVertexAttribute& attribute,
                                     GLuint vbo_binding_index, int size,
                                     GLenum type, GLboolean normalized,
                                     unsigned int offset) {
    VAO->EnableAttrib(attribute);
    VAO->AttribBinding(attribute, vbo_binding_index);
    VAO->AttribFormat(attribute, size, type, normalized, offset);
}

IGAME_NAMESPACE_END