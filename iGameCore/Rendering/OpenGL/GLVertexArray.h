#ifndef GLVERTEXARRAY_H
#define GLVERTEXARRAY_H

#include "GLBuffer.h"
#include "GLObject.h"
#include "GLVertexArrayManager.h"

IGAME_NAMESPACE_BEGIN

class GLVertexAttribute : public Object {
public:
    explicit GLVertexAttribute(unsigned int location);
    ~GLVertexAttribute() override;

    unsigned int Index() const;

protected:
    GLuint m_Index;
};

class GLVertexArray : public GLObject<GLVertexArray> {
public:
    I_OBJECT(GLVertexArray);
    static Pointer New() { return new GLVertexArray; }

    void VertexBuffer(unsigned int vbo_binding_index,
                      SmartPointer<GLBuffer> buffer, ptrdiff_t offset,
                      size_t stride);

    void ElementBuffer(SmartPointer<GLBuffer> buffer);

    void EnableAttrib(const GLVertexAttribute& attribute);

    void AttribBindingFormat(const GLVertexAttribute& attribute,
                             unsigned int vbo_binding_index, int size,
                             GLenum type, bool normalized,
                             unsigned int relative_offset);

    void Bind() const;
    void Release() const;

    void DrawArrays(GLenum mode, GLint first, GLsizei count);
    void DrawElements(GLenum mode, int elementCount, GLenum type,
                      const void* indices = 0);
    void DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                           GLenum type, const void* indices = 0);

protected:
    GLVertexArray();
    ~GLVertexArray() override;

    friend class GLObject<GLVertexArray>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);
};

//Per-vertex attributes binding indices
static const GLVertexAttribute GL_LOCATION_IDX_0{0};
static const GLVertexAttribute GL_LOCATION_IDX_1{1};
static const GLVertexAttribute GL_LOCATION_IDX_2{2};
static const GLVertexAttribute GL_LOCATION_IDX_3{3};

void GLSetVertexAttrib(SmartPointer<GLVertexArray> VAO,
                       const GLVertexAttribute& attribute,
                       GLuint vbo_binding_index, int size, GLenum type,
                       GLboolean normalized, unsigned int offset);

IGAME_NAMESPACE_END

#endif // GLVERTEXARRAY_H
