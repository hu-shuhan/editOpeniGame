#ifndef GLTEXTURE2DMULTISAMPLE_H
#define GLTEXTURE2DMULTISAMPLE_H

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLTexture2dMultisample : public GLObject<GLTexture2dMultisample> {
public:
    I_OBJECT(GLTexture2dMultisample);
    static Pointer New() { return new GLTexture2dMultisample; }

    static void
    CopyImageSubData(SmartPointer<GLTexture2dMultisample> source,
                     GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ,
                     SmartPointer<GLTexture2dMultisample> destination,
                     GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ,
                     GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);

    // GLenum internal_format: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    // Sized Internal Format: GL_R8, GL_RG8, GL_RGB8, GL_RGBA8
    // Sized Internal Format: GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24
    // Sized Internal Format: GL_DEPTH32F_STENCIL8, GL_DEPTH24_STENCIL8
    // Sized Internal Format: GL_STENCIL_INDEX8
    void Storage(unsigned samples, GLenum internal_format, unsigned width,
                 unsigned height, bool fixedsamplelocations) const;

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15
    // GL_TEXTURE0 is reserved to prevent other binding operations from being performed after a texture unit is activated.
    void Active(GLenum texture);

    // GLenum pname: GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER
    // GLint param: GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, GL_NEAREST, GL_LINEAR
    void Parameteri(GLenum pname, GLint param);

    void Bind() const;
    void Release() const;

protected:
    GLTexture2dMultisample();
    ~GLTexture2dMultisample() override;

    friend class GLObject<GLTexture2dMultisample>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);
};

IGAME_NAMESPACE_END

#endif // GLTEXTURE2DMULTISAMPLE_H