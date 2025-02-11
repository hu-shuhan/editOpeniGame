#ifndef GLBUFFER_H
#define GLBUFFER_H

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLBuffer : public GLObject<GLBuffer> {
public:
    I_OBJECT(GLBuffer);
    static Pointer New() { return new GLBuffer; }

    static void CopySubData(SmartPointer<GLBuffer> source,
                            SmartPointer<GLBuffer> destination,
                            size_t read_offset, size_t write_offset,
                            size_t size);

    // GLenum usage: GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW
    // GLenum usage: GL_STATIC_READ, GL_DYNAMIC_READ, GL_STREAM_READ
    // GLenum usage: GL_STATIC_COPY, GL_DYNAMIC_COPY, GL_STREAM_COPY
    void Allocate(size_t size, const void* data, GLenum usage);

    // GLbitfield flags: GL_DYNAMIC_STORAGE_BIT
    // GLbitfield flags: GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
    // GLbitfield flags: GL_MAP_PERSISTENT_BIT, GL_MAP_COHERENT_BIT
    // GLbitfield flags: GL_CLIENT_STORAGE_BIT
    void Storage(size_t size, const void* data, GLbitfield flags);

    // GLbitfield access: GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
    // GLbitfield access: GL_MAP_INVALIDATE_RANGE_BIT, GL_MAP_INVALIDATE_BUFFER_BIT
    // GLbitfield access: GL_MAP_FLUSH_EXPLICIT_BIT, GL_MAP_UNSYNCHRONIZED_BIT
    void* MapRange(size_t offset, size_t length, GLbitfield access) const;

    void SubData(size_t offset, size_t size, const void* data) const;

    void GetSubData(size_t offset, size_t size, void* data) const;

    void Unmap();

    // What is target this buffer for
    // GLenum target: GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_UNIFORM_BUFFER
    // GLenum target: GL_SHADER_STORAGE_BUFFER, GL_DRAW_INDIRECT_BUFFER
    // GLenum target: GL_DISPATCH_INDIRECT_BUFFER
    // GLenum target: GL_ATOMIC_COUNTER_BUFFER, GL_TEXTURE_BUFFER
    void Target(GLenum target);

    void Bind() const;
    void Release() const;
    void BindBase(unsigned index) const;

protected:
    GLBuffer();
    ~GLBuffer() override;

    friend class GLObject<GLBuffer>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);

    GLenum m_Target;
};

static const unsigned int GL_VBO_IDX_0{0};
static const unsigned int GL_VBO_IDX_1{1};
static const unsigned int GL_VBO_IDX_2{2};
static const unsigned int GL_VBO_IDX_3{3};

void GLAllocateGLBuffer(SmartPointer<GLBuffer> vbo, size_t size,
                        const void* data);

IGAME_NAMESPACE_END

#endif // GLBUFFER_H
