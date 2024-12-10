#ifndef GLTEXTURE2D_H
#define GLTEXTURE2D_H

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLTexture2d : public GLObject<GLTexture2d> {
public:
    I_OBJECT(GLTexture2d);
    static Pointer New() { return new GLTexture2d; }

    static void CopyImageSubData(GLTexture2d::Pointer source, GLenum srcTarget,
                                 GLint srcLevel, GLint srcX, GLint srcY,
                                 GLint srcZ, GLTexture2d::Pointer destination,
                                 GLenum dstTarget, GLint dstLevel, GLint dstX,
                                 GLint dstY, GLint dstZ, GLsizei srcWidth,
                                 GLsizei srcHeight, GLsizei srcDepth);

    static GLTexture2d View(GLenum target, GLTexture2d::Pointer original,
                            GLenum internal_format, unsigned first_mip_level,
                            unsigned mip_level_count, unsigned first_layer,
                            unsigned layer_count);

    // GLenum internal_format: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    // GLenum internal_format(Sized Internal Format): GL_R8, GL_RG8, GL_RGB8, GL_RGBA8
    // GLenum internal_format(Sized Internal Format): GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24
    // GLenum internal_format(Sized Internal Format): GL_DEPTH32F_STENCIL8, GL_DEPTH24_STENCIL8
    // GLenum internal_format(Sized Internal Format): GL_STENCIL_INDEX8
    void Storage(unsigned mip_levels, GLenum internal_format, unsigned width,
                 unsigned height) const;

    // GLenum format(Base Internal Format): GL_RED, GL_RG, GL_RGB, GL_RGBA
    // GLenum type:GL_UNSIGNED_BYTE, GL_FLOAT
    void SubImage(unsigned mip_level, unsigned xoffset, unsigned yoffset,
                  unsigned width, unsigned height, GLenum format, GLenum type,
                  const void* pixels);

    // GLenum pname: GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T
    // GLint param: GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    // -------------------------------------------------------------------------
    // GLenum pname: GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER
    // GLint param: GL_NEAREST, GL_LINEAR
    // GLint param(GL_TEXTURE_MIN_FILTER): GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST
    // GLint param(GL_TEXTURE_MIN_FILTER): GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
    void Parameteri(GLenum pname, GLint param);

    // GLenum pname: GL_TEXTURE_BORDER_COLOR
    void Parameterfv(GLenum pname, const GLfloat* params);

    void GenerateMipmap();

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15
    // GL_TEXTURE0 is reserved to prevent other binding operations from being performed after a texture unit is activated.
    void Active(GLenum texture);

    void Bind() const;
    void Release() const;
    void BindImage(unsigned int binding_index, unsigned int mip_level,
                   bool layered, int layer, GLenum access, GLenum format);

protected:
    GLTexture2d();
    explicit GLTexture2d(GLuint handle);
    ~GLTexture2d() override;

    friend class GLObject<GLTexture2d>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);
};

IGAME_NAMESPACE_END

#endif // GLTEXTURE2D_H