#pragma once

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN
class GLRenderbuffer : public GLObject<GLRenderbuffer> {
public:
    I_OBJECT(GLRenderbuffer);
    static Pointer New() { return new GLRenderbuffer; }

    // GLenum internal_format: GL_DEPTH_COMPONENT32,  GL_DEPTH24_STENCIL8
    void Storage(GLenum internal_format, unsigned width, unsigned height) {
        glNamedRenderbufferStorage(handle, internal_format, width, height);
    }

protected:
    GLRenderbuffer() = default;
    ~GLRenderbuffer() override = default;

    friend class GLObject<GLRenderbuffer>;
    static void CreateHandle(GLsizei count, GLuint* handles) {
        glCreateRenderbuffers(count, handles);
    }
    static void DestroyHandle(GLsizei count, GLuint* handles) {
        glDeleteRenderbuffers(count, handles);
    }
};
IGAME_NAMESPACE_END