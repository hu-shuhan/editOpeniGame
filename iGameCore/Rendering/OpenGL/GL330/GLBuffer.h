#pragma once

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLBuffer : public GLObject<GLBuffer> {
public:
    I_OBJECT(GLBuffer);
    static Pointer New() { return new GLBuffer; }

    static void copySubData(const GLBuffer& source, const GLBuffer& destination,
                            size_t read_offset, size_t write_offset,
                            size_t size) {
        glBindBuffer(GL_COPY_READ_BUFFER, source.handle);
        glBindBuffer(GL_COPY_WRITE_BUFFER, destination.handle);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
                            read_offset, write_offset, size);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    }

public:
    //bool isAllocated() const { return bufferAllocated; }

    // GLenum usage: GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW
    // GLenum usage: GL_STATIC_READ, GL_DYNAMIC_READ, GL_STREAM_READ
    // GLenum usage: GL_STATIC_COPY, GL_DYNAMIC_COPY, GL_STREAM_COPY
    void Allocate(size_t size, const void* data, GLenum usage) {
        glBindBuffer(m_Target, handle);
        glBufferData(m_Target, size, data, usage);
        glBindBuffer(m_Target, 0);
    }

    void Storage(size_t size, const void* data, GLbitfield flags) {
        throw std::runtime_error(
                "You called the GLBuffer::storage function on the opengl330. "
                "This function is currently not supported.");
        //glNamedBufferStorage(handle, size, data, flags);
    }

    // GLbitfield access: GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
    // GLbitfield access: GL_MAP_INVALIDATE_RANGE_BIT, GL_MAP_INVALIDATE_BUFFER_BIT
    // GLbitfield access: GL_MAP_FLUSH_EXPLICIT_BIT, GL_MAP_UNSYNCHRONIZED_BIT
    void* MapRange(size_t offset, size_t length, GLbitfield access) const {
        glBindBuffer(m_Target, handle);
        void* ptr = glMapBufferRange(m_Target, offset, length, access);
        glBindBuffer(m_Target, 0);

        return ptr;
    }

    void SubData(size_t offset, size_t size, const void* data) const {
        glBindBuffer(m_Target, handle);
        glBufferSubData(m_Target, offset, size, data);
        glBindBuffer(m_Target, 0);
    }

    void GetSubData(size_t offset, size_t size, void* data) const {
        glBindBuffer(m_Target, handle);
        glGetBufferSubData(m_Target, offset, size, data);
        glBindBuffer(m_Target, 0);
    }

    void Unmap() {
        glBindBuffer(m_Target, handle);
        if (!glUnmapBuffer(m_Target)) {
            glBindBuffer(m_Target, 0);
            throw std::runtime_error(
                    "data store contents have become corrupt during the time "
                    "the data store was mapped");
        }
        glBindBuffer(m_Target, 0);
    }

    // What is target this buffer for
    // GLenum target: GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_UNIFORM_BUFFER
    // GLenum target: GL_SHADER_STORAGE_BUFFER, GL_DRAW_INDIRECT_BUFFER
    // GLenum target: GL_DISPATCH_INDIRECT_BUFFER
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
        glGenBuffers(count, handles);
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

IGAME_NAMESPACE_END