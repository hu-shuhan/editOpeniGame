#ifndef GLRENDERBUFFER_H
#define GLRENDERBUFFER_H

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLRenderBuffer : public GLObject<GLRenderBuffer> {
public:
    I_OBJECT(GLRenderBuffer);
    static Pointer New() { return new GLRenderBuffer; }

    // GLenum internal_format: GL_DEPTH_COMPONENT32,  GL_DEPTH24_STENCIL8
    void Storage(GLenum internal_format, unsigned width, unsigned height);

protected:
    GLRenderBuffer();
    ~GLRenderBuffer() override;

    friend class GLObject<GLRenderBuffer>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);
};

IGAME_NAMESPACE_END

#endif // GLRENDERBUFFER_H
