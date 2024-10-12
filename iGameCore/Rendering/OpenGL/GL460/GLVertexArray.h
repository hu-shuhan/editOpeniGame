#pragma once

#include "GLBuffer.h"
#include "GLObject.h"

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
        glCreateVertexArrays(count, handles);
    }
    static void destroyHandle(GLsizei count, GLuint* handles) {
        glDeleteVertexArrays(count, handles);
    }

public:
    void bind() const { glBindVertexArray(handle); }
    void release() const { glBindVertexArray(0); }

    void vertexBuffer(unsigned int binding_index, const GLBuffer& buffer,
                      ptrdiff_t offset, size_t stride) {
        glVertexArrayVertexBuffer(handle, binding_index, buffer, offset,
                                  stride);
    }

    void elementBuffer(const GLBuffer& buffer) {
        glVertexArrayElementBuffer(handle, buffer);
    }

    void enableAttrib(const GLVertexAttribute& attribute) {
        glEnableVertexArrayAttrib(handle, attribute.index());
    }

    // GLVertexAttribute is one attribute in binding_index of VAO
    void attribBinding(const GLVertexAttribute& attribute,
                       unsigned int binding_index) {
        glVertexArrayAttribBinding(handle, attribute.index(), binding_index);
    }

    void attribFormat(const GLVertexAttribute& attribute, int size, GLenum type,
                      bool normalized, unsigned int relative_offset) {
        glVertexArrayAttribFormat(handle, attribute.index(), size, type,
                                  normalized, relative_offset);
    }

    void attribFormati(GLVertexAttribute& attribute, int size, GLenum type,
                       unsigned int relative_offset) {
        glVertexArrayAttribIFormat(handle, attribute.index(), size, type,
                                   relative_offset);
    }

    void bindingDivisor(unsigned int binding_index, int divisor) const {
        glVertexArrayBindingDivisor(handle, binding_index, divisor);
    }
};

//Per-vertex attributes binding indices
inline const GLVertexAttribute GL_LOCATION_IDX_0{0};
inline const GLVertexAttribute GL_LOCATION_IDX_1{1};
inline const GLVertexAttribute GL_LOCATION_IDX_2{2};
inline const GLVertexAttribute GL_LOCATION_IDX_3{3};

inline static void GLSetVertexAttrib(GLVertexArray& VAO,
                                     const GLVertexAttribute& attribute,
                                     GLuint vbo_binding_index, int size,
                                     GLenum type, GLboolean normalized,
                                     unsigned int offset) {
    VAO.enableAttrib(attribute);
    VAO.attribBinding(attribute, vbo_binding_index);
    VAO.attribFormat(attribute, size, type, normalized, offset);
}

IGAME_NAMESPACE_END