//
// Created for iGameVis parallel volume rendering (Phase 1).
//
#include "GLTexture3d.h"

IGAME_NAMESPACE_BEGIN

GLTexture3d::GLTexture3d() {}

GLTexture3d::GLTexture3d(GLuint handle) : GLObject<GLTexture3d>{handle} {}

GLTexture3d::~GLTexture3d() {}

void GLTexture3d::Storage(unsigned mip_levels, GLenum internal_format,
                          unsigned width, unsigned height,
                          unsigned depth) const {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_3D, m_Handle);

    GLenum format;
    GLenum type;
    GLenum storageInternalFormat = internal_format;

    switch (internal_format) {
        case GL_R8:
            format = GL_RED;
            type = GL_UNSIGNED_BYTE;
            break;
        case GL_R16F:
            format = GL_RED;
            type = GL_HALF_FLOAT;
            break;
        case GL_R32F:
            format = GL_RED;
            type = GL_FLOAT;
            break;
        case GL_RGBA8:
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case GL_RGBA16F:
            format = GL_RGBA;
            type = GL_HALF_FLOAT;
            break;
        default:
            IGAME_RENDERING_ERROR(
                    "[GLTexture3d::Storage] Error: Unsupported internal format.");
            glBindTexture(GL_TEXTURE_3D, 0);
            return;
    }

    for (unsigned int level = 0; level < mip_levels; ++level) {
        glTexImage3D(GL_TEXTURE_3D, level, storageInternalFormat, width >> level,
                     height >> level, depth >> level, 0, format, type, nullptr);
    }

    glBindTexture(GL_TEXTURE_3D, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureStorage3D(m_Handle, mip_levels, internal_format, width, height,
                       depth);
#endif
}

void GLTexture3d::SubImage(unsigned mip_level, unsigned xoffset,
                           unsigned yoffset, unsigned zoffset, unsigned width,
                           unsigned height, unsigned depth, GLenum format,
                           GLenum type, const void* pixels) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_3D, m_Handle);
    glTexSubImage3D(GL_TEXTURE_3D, mip_level, xoffset, yoffset, zoffset, width,
                    height, depth, format, type, pixels);
    glBindTexture(GL_TEXTURE_3D, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureSubImage3D(m_Handle, mip_level, xoffset, yoffset, zoffset, width,
                        height, depth, format, type, pixels);
#endif
}

void GLTexture3d::Parameteri(GLenum pname, GLint param) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_3D, m_Handle);
    glTexParameteri(GL_TEXTURE_3D, pname, param);
    glBindTexture(GL_TEXTURE_3D, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureParameteri(m_Handle, pname, param);
#endif
}

void GLTexture3d::Parameterfv(GLenum pname, const GLfloat* params) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_3D, m_Handle);
    glTexParameterfv(GL_TEXTURE_3D, pname, params);
    glBindTexture(GL_TEXTURE_3D, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureParameterfv(m_Handle, pname, params);
#endif
}

void GLTexture3d::Active(GLenum texture) {
    if (texture == GL_TEXTURE0) {
        IGAME_RENDERING_ERROR("[GLTexture3d::Active] Error: GL_TEXTURE0 is "
                              "reserved and cannot be used.");
        return;
    }
    glActiveTexture(texture);
    glBindTexture(GL_TEXTURE_3D, m_Handle);
    glActiveTexture(GL_TEXTURE0);
}

void GLTexture3d::Bind() const { glBindTexture(GL_TEXTURE_3D, m_Handle); }

void GLTexture3d::Release() const { glBindTexture(GL_TEXTURE_3D, 0); }

void GLTexture3d::CreateHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glGenTextures(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glCreateTextures(GL_TEXTURE_3D, count, handles);
#endif
}

void GLTexture3d::DestroyHandle(GLsizei count, GLuint* handles) {
    glDeleteTextures(count, handles);
}

IGAME_NAMESPACE_END
