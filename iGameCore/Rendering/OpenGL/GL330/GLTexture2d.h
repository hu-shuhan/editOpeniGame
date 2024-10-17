#pragma once

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

class GLTexture2d : public GLObject<GLTexture2d> {
public:
    I_OBJECT(GLTexture2d);
    static Pointer New() { return new GLTexture2d; }

    static void CopyImageSubData(const GLTexture2d& source, GLenum srcTarget,
                                 GLint srcLevel, GLint srcX, GLint srcY,
                                 GLint srcZ, const GLTexture2d& destination,
                                 GLenum dstTarget, GLint dstLevel, GLint dstX,
                                 GLint dstY, GLint dstZ, GLsizei srcWidth,
                                 GLsizei srcHeight, GLsizei srcDepth) {
        throw std::runtime_error(
                "You called the GLTexture2d::copyImageSubData function on the "
                "opengl330. This function is currently not supported.");
        //glCopyImageSubData(source.handle, srcTarget, srcLevel, srcX, srcY, srcZ,
        //                   destination.handle, dstTarget, dstLevel, dstX, dstY,
        //                   dstZ, srcWidth, srcHeight, srcDepth);
    }

public:
    static GLTexture2d View(GLenum target, const GLTexture2d& original,
                            GLenum internal_format, unsigned first_mip_level,
                            unsigned mip_level_count, unsigned first_layer,
                            unsigned layer_count) {
        throw std::runtime_error(
                "You called the GLTexture2d::view function on the opengl330. "
                "This function is currently not supported.");
        //GLuint handle;
        //glGenTextures(1, &handle);
        //glTextureView(handle, target, original, internal_format,
        //              first_mip_level, mip_level_count, first_layer,
        //              layer_count);
        //return GLTexture2d(handle);
    }

public:
    // GLenum internal_format: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    // GLenum internal_format(Sized Internal Format): GL_R8, GL_RG8, GL_RGB8, GL_RGBA8
    // GLenum internal_format(Sized Internal Format): GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24
    // GLenum internal_format(Sized Internal Format): GL_DEPTH32F_STENCIL8, GL_DEPTH24_STENCIL8
    // GLenum internal_format(Sized Internal Format): GL_STENCIL_INDEX8
    void Storage(unsigned mip_levels, GLenum internal_format, unsigned width,
                 unsigned height) const {
        glBindTexture(GL_TEXTURE_2D, handle);

        GLenum format;
        GLenum type;

        // Select the appropriate format and type based on internal_format
        switch (internal_format) {
            case GL_R8:
                format = GL_RED;
                type = GL_UNSIGNED_BYTE;
                break;
            case GL_RGB8:
                format = GL_RGB;
                type = GL_UNSIGNED_BYTE;
                break;
            case GL_RGBA8:
                format = GL_RGBA;
                type = GL_UNSIGNED_BYTE;
                break;
            case GL_R32F:
                format = GL_RED;
                type = GL_FLOAT;
                break;
            case GL_DEPTH_COMPONENT24:
                format = GL_DEPTH_COMPONENT;
                type = GL_FLOAT;
                break;
            case GL_DEPTH_COMPONENT32F:
                format = GL_DEPTH_COMPONENT;
                type = GL_FLOAT;
                break;
            case GL_DEPTH24_STENCIL8:
                format = GL_DEPTH_STENCIL;
                type = GL_UNSIGNED_INT_24_8;
                break;
            default:
                throw std::runtime_error("You called the GLTexture2d::storage "
                                         "function on the opengl330. "
                                         "but the internal_format you provided "
                                         "was not enumrated.");
        }

        for (unsigned int level = 0; level < mip_levels; ++level) {
            glTexImage2D(GL_TEXTURE_2D, level, internal_format, width >> level,
                         height >> level, 0, format, type, nullptr);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }


    // GLenum format(Base Internal Format): GL_RED, GL_RG, GL_RGB, GL_RGBA
    // GLenum type:GL_UNSIGNED_BYTE, GL_FLOAT
    void SubImage(unsigned mip_level, unsigned xoffset, unsigned yoffset,
                  unsigned width, unsigned height, GLenum format, GLenum type,
                  const void* pixels) {
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexSubImage2D(GL_TEXTURE_2D, mip_level, xoffset, yoffset, width,
                        height, format, type, pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // GLenum pname: GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER
    // GLint param: GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, GL_NEAREST, GL_LINEAR
    void Parameteri(GLenum pname, GLint param) {
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexParameteri(GL_TEXTURE_2D, pname, param);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GenerateMipmap() {
        glBindTexture(GL_TEXTURE_2D, handle);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15
    // GL_TEXTURE0 is reserved to prevent other binding operations from being performed after a texture unit is activated.
    void Active(GLenum texture) {
        if (texture == GL_TEXTURE0) {
            throw std::runtime_error("GL_TEXTURE0 is reserved.");
        }
        glActiveTexture(texture);
        glBindTexture(GL_TEXTURE_2D, handle);
        glActiveTexture(GL_TEXTURE0);
    };

    void Bind() const { glBindTexture(GL_TEXTURE_2D, handle); }
    void Release() const { glBindTexture(GL_TEXTURE_2D, 0); }

    void BindImage(unsigned int binding_index, unsigned int mip_level,
                   bool layered, int layer, GLenum access, GLenum format) {
        throw std::runtime_error("You called the GLTexture2d::bindImage "
                                 "function on the opengl330. "
                                 "This function is currently not supported.");
        //glBindImageTexture(binding_index, handle, mip_level, layered, layer,
        //                   access, format);
    }

protected:
    GLTexture2d() = default;
    GLTexture2d(GLuint handle) : GLObject<GLTexture2d>{handle} {}
    ~GLTexture2d() override = default;

    friend class GLObject<GLTexture2d>;
    static void CreateHandle(GLsizei count, GLuint* handles) {
        glGenTextures(count, handles);
    }
    static void DestroyHandle(GLsizei count, GLuint* handles) {
        glDeleteTextures(count, handles);
    }
};

IGAME_NAMESPACE_END