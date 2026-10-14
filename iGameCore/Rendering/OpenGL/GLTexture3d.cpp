//
// 3D 纹理封装（体绘制用）
//
#include "GLTexture3d.h"

IGAME_NAMESPACE_BEGIN

GLTexture3d::GLTexture3d() {}

GLTexture3d::~GLTexture3d() {}

void GLTexture3d::Storage(unsigned mip_levels, GLenum internal_format,
                          unsigned width, unsigned height,
                          unsigned depth) const {
#ifdef IGAME_OPENGL_VERSION_330
    IGAME_RENDERING_ERROR(
            "[GLTexture3d::Storage] Error: Function not implemented for "
            "OpenGL 3.3 / GLES2. Volume ray casting requires OpenGL 4.6.");
    (void) mip_levels;
    (void) internal_format;
    (void) width;
    (void) height;
    (void) depth;
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
    IGAME_RENDERING_ERROR(
            "[GLTexture3d::SubImage] Error: Function not implemented for "
            "OpenGL 3.3 / GLES2. Volume ray casting requires OpenGL 4.6.");
    (void) mip_level;
    (void) xoffset;
    (void) yoffset;
    (void) zoffset;
    (void) width;
    (void) height;
    (void) depth;
    (void) format;
    (void) type;
    (void) pixels;
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

void GLTexture3d::GenerateMipmap() {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_3D, m_Handle);
    glGenerateMipmap(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);
#elif IGAME_OPENGL_VERSION_460
    glGenerateTextureMipmap(m_Handle);
#endif
}

void GLTexture3d::Active(GLenum texture) {
    if (texture == GL_TEXTURE0) {
        IGAME_RENDERING_ERROR(
                "[GLTexture3d::Active] Error: GL_TEXTURE0 is reserved "
                "and cannot be used.");
        return;
    }
    glActiveTexture(texture);
    glBindTexture(GL_TEXTURE_3D, m_Handle);
    glActiveTexture(GL_TEXTURE0);
}

void GLTexture3d::Bind() const { glBindTexture(GL_TEXTURE_3D, m_Handle); }

void GLTexture3d::Release() const { glBindTexture(GL_TEXTURE_3D, 0); }

void GLTexture3d::BindImage(unsigned int binding_index, unsigned int mip_level,
                            bool layered, int layer, GLenum access,
                            GLenum format) {
#ifdef IGAME_OPENGL_VERSION_330
    IGAME_RENDERING_ERROR(
            "[GLTexture3d::BindImage] Error: Function not implemented for "
            "OpenGL 3.3 / GLES2. Volume ray casting requires OpenGL 4.6.");
    (void) binding_index;
    (void) mip_level;
    (void) layered;
    (void) layer;
    (void) access;
    (void) format;
#elif IGAME_OPENGL_VERSION_460
    glBindImageTexture(binding_index, m_Handle, mip_level, layered, layer,
                       access, format);
#endif
}

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
