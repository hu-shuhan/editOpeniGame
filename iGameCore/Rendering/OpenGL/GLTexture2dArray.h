#ifndef IGAMEVIS_GLTEXTURE2DARRAY_H
#define IGAMEVIS_GLTEXTURE2DARRAY_H

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLTexture2dArray : public GLObject<GLTexture2dArray> {
public:
    I_OBJECT(GLTexture2dArray);
    static Pointer New() { return new GLTexture2dArray; }

    // GLenum internal_format: GL_RGBA8
    void Storage(unsigned mip_levels, GLenum internal_format, unsigned width,
                 unsigned height, unsigned depth) const;

    // GLenum format: GL_RGBA8
    // GLenum type:GL_UNSIGNED_BYTE
    void SubImage(unsigned mip_level, unsigned xoffset, unsigned yoffset,
                  unsigned zoffset, unsigned width, unsigned height,
                  unsigned depth, GLenum format, GLenum type,
                  const void* pixels);

    void GenerateMipmap();

    void Bind() const;

protected:
    GLTexture2dArray();
    ~GLTexture2dArray() override;

    friend class GLObject<GLTexture2dArray>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);
};

IGAME_NAMESPACE_END

#endif // IGAMEVIS_GLTEXTURE2DARRAY_H
