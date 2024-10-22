#pragma once

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLBuffer : public GLObject<GLBuffer> {
public:
    I_OBJECT(GLBuffer);
    static Pointer New() { return new GLBuffer; }

    static void CopySubData(const GLBuffer::Pointer source,
                            const GLBuffer::Pointer destination,
                            size_t read_offset, size_t write_offset,
                            size_t size) {
        glCopyNamedBufferSubData(source->handle, destination->handle,
                                 read_offset, write_offset, size);
    }

public:
    // GLenum usage: GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW
    // GLenum usage: GL_STATIC_READ, GL_DYNAMIC_READ, GL_STREAM_READ
    // GLenum usage: GL_STATIC_COPY, GL_DYNAMIC_COPY, GL_STREAM_COPY
    void Allocate(size_t size, const void* data, GLenum usage) {
        glNamedBufferData(handle, size, data, usage);
    }

    // GLbitfield flags: GL_DYNAMIC_STORAGE_BIT
    // GLbitfield flags: GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
    // GLbitfield flags: GL_MAP_PERSISTENT_BIT, GL_MAP_COHERENT_BIT
    // GLbitfield flags: GL_CLIENT_STORAGE_BIT
    void Storage(size_t size, const void* data, GLbitfield flags) {
        glNamedBufferStorage(handle, size, data, flags);
    }

    void SubData(size_t offset, size_t size, const void* data) const {
        glNamedBufferSubData(handle, offset, size, data);
    }

    void GetSubData(size_t offset, size_t size, void* data) const {
        glGetNamedBufferSubData(handle, offset, size, data);
    }

    // GLbitfield access: GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
    // GLbitfield access: GL_MAP_INVALIDATE_RANGE_BIT, GL_MAP_INVALIDATE_BUFFER_BIT
    // GLbitfield access: GL_MAP_FLUSH_EXPLICIT_BIT, GL_MAP_UNSYNCHRONIZED_BIT
    void* MapRange(size_t offset, size_t length, GLbitfield access) const {
        void* ptr = glMapNamedBufferRange(handle, offset, length, access);
        if (ptr == nullptr) {
            throw std::runtime_error("Map buffer range is nullptr.");
        }
        return ptr;
    }

    void Unmap() {
        if (!glUnmapNamedBuffer(handle))
            throw std::runtime_error(
                    "data store contents have become corrupt during the time "
                    "the data store was mapped");
    }

    // What is target this buffer for
    // GLenum target: GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_UNIFORM_BUFFER
    // GLenum target: GL_SHADER_STORAGE_BUFFER, GL_DRAW_INDIRECT_BUFFER
    // GLenum target: GL_DISPATCH_INDIRECT_BUFFER
    // GLenum target: GL_ATOMIC_COUNTER_BUFFER, GL_TEXTURE_BUFFER
    void Target(GLenum target) { m_Target = target; }

    void Bind() const { glBindBuffer(m_Target, handle); }
    void Release() const { glBindBuffer(m_Target, 0); }
    void BindBase(unsigned index) const {
        glBindBufferBase(m_Target, index, handle);
    }

protected:
    GLBuffer() = default;
    ~GLBuffer() override = default;

    friend class GLObject<GLBuffer>;
    static void CreateHandle(GLsizei count, GLuint* handles) {
        glCreateBuffers(count, handles);
    }
    static void DestroyHandle(GLsizei count, GLuint* handles) {
        glDeleteBuffers(count, handles);
    }

    GLenum m_Target{GL_NONE};
};

static inline const unsigned int GL_VBO_IDX_0{0};
static inline const unsigned int GL_VBO_IDX_1{1};
static inline const unsigned int GL_VBO_IDX_2{2};
static inline const unsigned int GL_VBO_IDX_3{3};

inline void GLAllocateGLBuffer(GLBuffer::Pointer vbo, size_t size,
                               const void* data) {
    vbo->Allocate(size, data, GL_STATIC_DRAW);
}

IGAME_NAMESPACE_END