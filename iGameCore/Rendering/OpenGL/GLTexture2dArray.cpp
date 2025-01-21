//
// Created by Sumzeek on 12/9/2024.
//
#include "GLTexture2dArray.h"

IGAME_NAMESPACE_BEGIN

GLTexture2dArray::GLTexture2dArray() {}

GLTexture2dArray::~GLTexture2dArray() {}

void GLTexture2dArray::Storage(unsigned mip_levels, GLenum internal_format,
                               unsigned width, unsigned height,
                               unsigned depth) const {
#ifdef IGAME_OPENGL_VERSION_330
    Logger::LogError("GLTexture2dArray::Storage function is not implemented.");
#elif IGAME_OPENGL_VERSION_460
    glTextureStorage3D(m_Handle, mip_levels, internal_format, width, height,
                       depth);
#endif
}

void GLTexture2dArray::SubImage(unsigned mip_level, unsigned xoffset,
                                unsigned yoffset, unsigned zoffset,
                                unsigned width, unsigned height, unsigned depth,
                                GLenum format, GLenum type,
                                const void* pixels) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_Handle);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mip_level, xoffset, yoffset, zoffset,
                    width, height, depth, format, type, pixels);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureSubImage3D(m_Handle, mip_level, xoffset, yoffset, zoffset, width,
                        height, depth, format, type, pixels);
#endif
}

void GLTexture2dArray::GenerateMipmap() {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_Handle);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
#elif IGAME_OPENGL_VERSION_460
    glGenerateTextureMipmap(m_Handle);
#endif
}

void GLTexture2dArray::Bind() const {
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_Handle);
}

void GLTexture2dArray::CreateHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glGenTextures(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glCreateTextures(GL_TEXTURE_2D_ARRAY, count, handles);
#endif
}

void GLTexture2dArray::DestroyHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glDeleteTextures(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glDeleteTextures(count, handles);
#endif
}

IGAME_NAMESPACE_END