#ifndef IGAMEVIS_GLTEXTURE3D_H
#define IGAMEVIS_GLTEXTURE3D_H

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

// 3D texture wrapper used by the GPU volume ray-caster.
// It mirrors GLTexture2d's style but targets GL_TEXTURE_3D.
class GLTexture3d : public GLObject<GLTexture3d> {
public:
    I_OBJECT(GLTexture3d);
    static Pointer New() { return new GLTexture3d; }

    // Allocate immutable storage for the 3D texture.
    // internal_format(Sized Internal Format): GL_R8, GL_R16F, GL_R32F, GL_RGBA8, ...
    void Storage(unsigned mip_levels, GLenum internal_format, unsigned width,
                 unsigned height, unsigned depth) const;

    // Upload a sub-region. format(Base Internal Format): GL_RED, GL_RGBA, ...
    // type: GL_FLOAT, GL_UNSIGNED_BYTE, ...
    void SubImage(unsigned mip_level, unsigned xoffset, unsigned yoffset,
                  unsigned zoffset, unsigned width, unsigned height,
                  unsigned depth, GLenum format, GLenum type,
                  const void* pixels);

    // GLenum pname: GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R
    // GLenum pname: GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER
    void Parameteri(GLenum pname, GLint param);
    void Parameterfv(GLenum pname, const GLfloat* params);

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15 (GL_TEXTURE0 reserved)
    void Active(GLenum texture);
    void Bind() const;
    void Release() const;

protected:
    GLTexture3d();
    explicit GLTexture3d(GLuint handle);
    ~GLTexture3d() override;

    friend class GLObject<GLTexture3d>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);
};

IGAME_NAMESPACE_END

#endif // IGAMEVIS_GLTEXTURE3D_H
