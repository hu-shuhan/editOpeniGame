#ifndef GLTEXTUREBUFFER_H
#define GLTEXTUREBUFFER_H

#include "GLBuffer.h"
#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLTextureBuffer : public GLObject<GLTextureBuffer> {
public:
    I_OBJECT(GLTextureBuffer);
    static Pointer New() { return new GLTextureBuffer; }

    // GLenum internal_format: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    // GLenum internal_format(Sized Internal Format): GL_R8, GL_RG8, GL_RGB8, GL_RGBA8
    // GLenum internal_format(Sized Internal Format): GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24
    // GLenum internal_format(Sized Internal Format): GL_DEPTH32F_STENCIL8, GL_DEPTH24_STENCIL8
    // GLenum internal_format(Sized Internal Format): GL_STENCIL_INDEX8
    void Buffer(GLenum internalformat, GLBuffer::Pointer buffer);

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15
    // GL_TEXTURE0 is reserved to prevent other binding operations from being performed after a texture unit is activated.
    void Active(GLenum texture);

    void Bind() const;
    void Release() const;
    void BindImage(unsigned int binding_index, unsigned int mip_level,
                   bool layered, int layer, GLenum access, GLenum format);

protected:
    GLTextureBuffer();
    explicit GLTextureBuffer(GLuint handle);
    ~GLTextureBuffer() override;

    friend class GLObject<GLTextureBuffer>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);
};

IGAME_NAMESPACE_END

#endif // GLTEXTUREBUFFER_H