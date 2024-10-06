#pragma once

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLBuffer : public GLObject<GLBuffer> {
private:
    GLenum m_Target;

private:
    friend class GLObject<GLBuffer>;
    static void createHandle(GLsizei count, GLuint* handles) {
        glCreateBuffers(count, handles);
    }
    static void destroyHandle(GLsizei count, GLuint* handles) {
        glDeleteBuffers(count, handles);
    }

public:
    static void copySubData(const GLBuffer& source, const GLBuffer& destination,
                            size_t read_offset, size_t write_offset,
                            size_t size) {
        glCopyNamedBufferSubData(source.handle, destination.handle, read_offset,
                                 write_offset, size);
    }

public:
    // GLenum usage: GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW
    // GLenum usage: GL_STATIC_READ, GL_DYNAMIC_READ, GL_STREAM_READ
    // GLenum usage: GL_STATIC_COPY, GL_DYNAMIC_COPY, GL_STREAM_COPY
    void allocate(size_t size, const void* data, GLenum usage) {
        glNamedBufferData(handle, size, data, usage);
    }

    // GLbitfield flags: GL_DYNAMIC_STORAGE_BIT
    // GLbitfield flags: GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
    // GLbitfield flags: GL_MAP_PERSISTENT_BIT, GL_MAP_COHERENT_BIT
    // GLbitfield flags: GL_CLIENT_STORAGE_BIT
    void storage(size_t size, const void* data, GLbitfield flags) {
        glNamedBufferStorage(handle, size, data, flags);
    }

    void subData(size_t offset, size_t size, const void* data) const {
        glNamedBufferSubData(handle, offset, size, data);
    }

    void getSubData(size_t offset, size_t size, void* data) const {
        glGetNamedBufferSubData(handle, offset, size, data);
    }

    // GLbitfield access: GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
    // GLbitfield access: GL_MAP_INVALIDATE_RANGE_BIT, GL_MAP_INVALIDATE_BUFFER_BIT
    // GLbitfield access: GL_MAP_FLUSH_EXPLICIT_BIT, GL_MAP_UNSYNCHRONIZED_BIT
    void* mapRange(size_t offset, size_t length, GLbitfield access) const {
        void* ptr = glMapNamedBufferRange(handle, offset, length, access);
        if (ptr == nullptr) {
            throw std::runtime_error("Map buffer range is nullptr.");
        }
        return ptr;
    }

    void unmap() {
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
    void target(GLenum target) { m_Target = target; }

    void bind() const { glBindBuffer(m_Target, handle); }
    void release() const { glBindBuffer(m_Target, 0); }
    void bindBase(unsigned index) const {
        glBindBufferBase(m_Target, index, handle);
    }
};

static inline const unsigned int GL_VBO_IDX_0{0};
static inline const unsigned int GL_VBO_IDX_1{1};
static inline const unsigned int GL_VBO_IDX_2{2};
static inline const unsigned int GL_VBO_IDX_3{3};

inline void GLAllocateGLBuffer(GLBuffer& vbo, size_t size, const void* data) {
    vbo.allocate(size, data, GL_STATIC_DRAW);
}

IGAME_NAMESPACE_END