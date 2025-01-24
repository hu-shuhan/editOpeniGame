//
// Created by Sumzeek on 12/9/2024.
//
#include "GLTexture2dMultisample.h"

IGAME_NAMESPACE_BEGIN

GLTexture2dMultisample::GLTexture2dMultisample() {}

GLTexture2dMultisample::~GLTexture2dMultisample() {}

void GLTexture2dMultisample::CopyImageSubData(
        const GLTexture2dMultisample::Pointer source, GLint srcLevel,
        GLint srcX, GLint srcY, GLint srcZ,
        const GLTexture2dMultisample::Pointer destination, GLint dstLevel,
        GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight,
        GLsizei srcDepth) {
#ifdef IGAME_OPENGL_VERSION_330
    Logger::LogError(
            "[GLTexture2dMultisample::CopyImageSubData] Error: This function "
            "is not supported in OpenGL 3.3. Please use OpenGL 4.6 or higher.");
#elif IGAME_OPENGL_VERSION_460
    glCopyImageSubData(source->Handle(), GL_TEXTURE_2D_MULTISAMPLE, srcLevel,
                       srcX, srcY, srcZ, destination->Handle(),
                       GL_TEXTURE_2D_MULTISAMPLE, dstLevel, dstX, dstY, dstZ,
                       srcWidth, srcHeight, srcDepth);
#endif
}

void GLTexture2dMultisample::Storage(unsigned samples, GLenum internal_format,
                                     unsigned width, unsigned height,
                                     bool fixedsamplelocations) const {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_Handle);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internal_format,
                            width, height, fixedsamplelocations);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureStorage2DMultisample(m_Handle, samples, internal_format, width,
                                  height, fixedsamplelocations);
#endif
}

void GLTexture2dMultisample::Active(GLenum texture) {
    if (texture == GL_TEXTURE0) {
        Logger::LogError("[GLTexture2dMultisample::Active] Error: GL_TEXTURE0 "
                         "is reserved and cannot be used for binding.");
        return;
    }
    glActiveTexture(texture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_Handle);
    glActiveTexture(GL_TEXTURE0);
}

void GLTexture2dMultisample::Parameteri(GLenum pname, GLint param) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_Handle);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, pname, param);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureParameteri(m_Handle, pname, param);
#endif
}

void GLTexture2dMultisample::Bind() const {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_Handle);
}

void GLTexture2dMultisample::Release() const {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
}

void GLTexture2dMultisample::CreateHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glGenTextures(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, count, handles);
#endif
}

void GLTexture2dMultisample::DestroyHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glDeleteTextures(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glDeleteTextures(count, handles);
#endif
}

IGAME_NAMESPACE_END