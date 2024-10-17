#pragma once

#include "GLObject.h"
#include "GLTextureHandle.h"

IGAME_NAMESPACE_BEGIN

class GLTexture2dArray : public GLObject<GLTexture2dArray> {
public:
    I_OBJECT(GLTexture2dArray);
    static Pointer New() { return new GLTexture2dArray; }

    // GLenum internal_format: GL_RGBA8
    void Storage(unsigned mip_levels, GLenum internal_format, unsigned width,
                 unsigned height, unsigned depth) const {
        glTextureStorage3D(handle, mip_levels, internal_format, width, height,
                           depth);
    }

    // GLenum format: GL_RGBA8
    // GLenum type:GL_UNSIGNED_BYTE
    void SubImage(unsigned mip_level, unsigned xoffset, unsigned yoffset,
                  unsigned zoffset, unsigned width, unsigned height,
                  unsigned depth, GLenum format, GLenum type,
                  const void* pixels) {
        glTextureSubImage3D(handle, mip_level, xoffset, yoffset, zoffset, width,
                            height, depth, format, type, pixels);
    }

    void GenerateMipmap() { glGenerateTextureMipmap(handle); }

    GLTextureHandle GetTextureHandle() const {
        return GLTextureHandle(glGetTextureHandleARB(handle));
    }

    void Bind() const { glBindTexture(GL_TEXTURE_2D_ARRAY, handle); }

protected:
    GLTexture2dArray() = default;
    //GLTexture2dArray(GLuint handle) : GLObject<GLTexture2dArray>{handle} {}
    ~GLTexture2dArray() override = default;

    friend class GLObject<GLTexture2dArray>;
    static void CreateHandle(GLsizei count, GLuint* handles) {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, count, handles);
    }
    static void DestroyHandle(GLsizei count, GLuint* handles) {
        glDeleteTextures(count, handles);
    }
};

IGAME_NAMESPACE_END