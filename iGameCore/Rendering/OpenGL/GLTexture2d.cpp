//
// Created by Sumzeek on 12/9/2024.
//
#include "GLTexture2d.h"

IGAME_NAMESPACE_BEGIN

GLTexture2d::GLTexture2d() {}

GLTexture2d::GLTexture2d(GLuint handle) : GLObject<GLTexture2d>{handle} {}

GLTexture2d::~GLTexture2d() {}

void GLTexture2d::CopyImageSubData(const SmartPointer<GLTexture2d> source,
                                   GLenum srcTarget, GLint srcLevel, GLint srcX,
                                   GLint srcY, GLint srcZ,
                                   const SmartPointer<GLTexture2d> destination,
                                   GLenum dstTarget, GLint dstLevel, GLint dstX,
                                   GLint dstY, GLint dstZ, GLsizei srcWidth,
                                   GLsizei srcHeight, GLsizei srcDepth) {
#ifdef IGAME_OPENGL_VERSION_330
    IGAME_RENDERING_ERROR("[GLTexture2d::CopyImageSubData] Error: Function not "
                     "supported on OpenGL 3.3.");
#elif IGAME_OPENGL_VERSION_460
    glCopyImageSubData(source->Handle(), GL_TEXTURE_2D, srcLevel, srcX, srcY,
                       srcZ, destination->Handle(), GL_TEXTURE_2D, dstLevel,
                       dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
#endif
}

GLTexture2d GLTexture2d::View(GLenum target,
                              const SmartPointer<GLTexture2d> original,
                              GLenum internal_format, unsigned first_mip_level,
                              unsigned mip_level_count, unsigned first_layer,
                              unsigned layer_count) {
#ifdef IGAME_OPENGL_VERSION_330
    IGAME_RENDERING_ERROR(
            "[GLTexture2d::View] Error: Function not supported on OpenGL 3.3.");
    return GLTexture2d(0);
#elif IGAME_OPENGL_VERSION_460
    GLuint handle;
    glGenTextures(1, &handle);
    glTextureView(handle, target, original->Handle(), internal_format,
                  first_mip_level, mip_level_count, first_layer, layer_count);
    return GLTexture2d(handle);
#endif
}

void GLTexture2d::Storage(unsigned mip_levels, GLenum internal_format,
                          unsigned width, unsigned height) const {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_2D, m_Handle);

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
            IGAME_RENDERING_ERROR("[GLTexture2d::Storage] Error: Unsupported "
                             "internal format.");
            return;
    }

    for (unsigned int level = 0; level < mip_levels; ++level) {
        glTexImage2D(GL_TEXTURE_2D, level, internal_format, width >> level,
                     height >> level, 0, format, type, nullptr);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureStorage2D(m_Handle, mip_levels, internal_format, width, height);
#endif
}

void GLTexture2d::SubImage(unsigned mip_level, unsigned xoffset,
                           unsigned yoffset, unsigned width, unsigned height,
                           GLenum format, GLenum type, const void* pixels) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_2D, m_Handle);
    glTexSubImage2D(GL_TEXTURE_2D, mip_level, xoffset, yoffset, width, height,
                    format, type, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureSubImage2D(m_Handle, mip_level, xoffset, yoffset, width, height,
                        format, type, pixels);
#endif
}

void GLTexture2d::Parameteri(GLenum pname, GLint param) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_2D, m_Handle);
    glTexParameteri(GL_TEXTURE_2D, pname, param);
    glBindTexture(GL_TEXTURE_2D, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureParameteri(m_Handle, pname, param);
#endif
}

void GLTexture2d::Parameterfv(GLenum pname, const GLfloat* params) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_2D, m_Handle);
    glTexParameterfv(GL_TEXTURE_2D, pname, params);
    glBindTexture(GL_TEXTURE_2D, 0);
#elif IGAME_OPENGL_VERSION_460
    glTextureParameterfv(m_Handle, pname, params);
#endif
}

void GLTexture2d::GenerateMipmap() {
#ifdef IGAME_OPENGL_VERSION_330
    glBindTexture(GL_TEXTURE_2D, m_Handle);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
#elif IGAME_OPENGL_VERSION_460
    glGenerateTextureMipmap(m_Handle);
#endif
}

void GLTexture2d::Active(GLenum texture) {
    if (texture == GL_TEXTURE0) {
        IGAME_RENDERING_ERROR("[GLTexture2d::Active] Error: GL_TEXTURE0 is reserved "
                         "and cannot be used.");
        return;
    }
    glActiveTexture(texture);
    glBindTexture(GL_TEXTURE_2D, m_Handle);
    glActiveTexture(GL_TEXTURE0);
}

void GLTexture2d::Bind() const { glBindTexture(GL_TEXTURE_2D, m_Handle); }

void GLTexture2d::Release() const { glBindTexture(GL_TEXTURE_2D, 0); }

void GLTexture2d::BindImage(unsigned int binding_index, unsigned int mip_level,
                            bool layered, int layer, GLenum access,
                            GLenum format) {
#ifdef IGAME_OPENGL_VERSION_330
    IGAME_RENDERING_ERROR("You called the GLTexture2d::BindImage function on the "
                     "opengl330. This function is currently not supported.");
#elif IGAME_OPENGL_VERSION_460
    glBindImageTexture(binding_index, m_Handle, mip_level, layered, layer,
                       access, format);
#endif
}

void GLTexture2d::CreateHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glGenTextures(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glCreateTextures(GL_TEXTURE_2D, count, handles);
#endif
}

void GLTexture2d::DestroyHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glDeleteTextures(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glDeleteTextures(count, handles);
#endif
}

IGAME_NAMESPACE_END
