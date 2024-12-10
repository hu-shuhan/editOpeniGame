//
// Created by Sumzeek on 12/9/2024.
//
#include "GLFramebuffer.h"

IGAME_NAMESPACE_BEGIN

GLFramebuffer::GLFramebuffer() { m_Target = GL_NONE; }

GLFramebuffer::~GLFramebuffer() {}

void GLFramebuffer::Blit(const GLFramebuffer::Pointer source,
                         const GLFramebuffer::Pointer destination, GLint srcX0,
                         GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0,
                         GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                         GLenum filter) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindFramebuffer(GL_READ_FRAMEBUFFER, source->Handle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination->Handle());
    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                      mask, filter);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#elif IGAME_OPENGL_VERSION_460
    glBlitNamedFramebuffer(source->Handle(), destination->Handle(), srcX0,
                           srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                           mask, filter);
#endif
}

void GLFramebuffer::DrawBuffers(size_t count, GLenum* buffers) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_Handle);
    glDrawBuffers(count, buffers);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#elif IGAME_OPENGL_VERSION_460
    glNamedFramebufferDrawBuffers(m_Handle, count, buffers);
#endif
}

void GLFramebuffer::Texture(GLenum attachment,
                            const GLTexture2d::Pointer texture,
                            unsigned mip_level) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);
    glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture->Handle(),
                         mip_level);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif IGAME_OPENGL_VERSION_460
    glNamedFramebufferTexture(m_Handle, attachment, texture->Handle(),
                              mip_level);
#endif
}

void GLFramebuffer::Texture(GLenum attachment,
                            const GLTexture2dMultisample::Pointer texture,
                            unsigned mip_level) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);
    glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture->Handle(),
                         mip_level);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif IGAME_OPENGL_VERSION_460
    glNamedFramebufferTexture(m_Handle, attachment, texture->Handle(),
                              mip_level);
#endif
}

void GLFramebuffer::TextureLayer(GLenum attachment,
                                 const GLTexture2dArray::Pointer texture,
                                 unsigned mip_level, unsigned layer) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, texture->Handle(),
                              mip_level, layer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif IGAME_OPENGL_VERSION_460
    glNamedFramebufferTextureLayer(m_Handle, attachment, texture->Handle(),
                                   mip_level, layer);
#endif
}

void GLFramebuffer::Renderbuffer(GLenum attachment, GLenum renderbuffer_target,
                                 const GLRenderBuffer::Pointer rbo) {
#ifdef IGAME_OPENGL_VERSION_330
    glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, renderbuffer_target,
                              rbo->Handle());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif IGAME_OPENGL_VERSION_460
    glNamedFramebufferRenderbuffer(m_Handle, attachment, renderbuffer_target,
                                   rbo->Handle());
#endif
}

GLenum GLFramebuffer::CheckStatus() {
#ifdef IGAME_OPENGL_VERSION_330
    glBindFramebuffer(m_Target, m_Handle);
    GLenum status = glCheckFramebufferStatus(m_Target);
    glBindFramebuffer(m_Target, 0);
    return status;
#elif IGAME_OPENGL_VERSION_460
    return glCheckNamedFramebufferStatus(m_Handle, m_Target);
#endif
}

void GLFramebuffer::Bind() const { glBindFramebuffer(m_Target, m_Handle); }

void GLFramebuffer::Release() const { glBindFramebuffer(m_Target, 0); }

void GLFramebuffer::Target(GLenum target) { m_Target = target; }


void GLFramebuffer::CreateHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glGenFramebuffers(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glCreateFramebuffers(count, handles);
#endif
}

void GLFramebuffer::DestroyHandle(GLsizei count, GLuint* handles) {
#ifdef IGAME_OPENGL_VERSION_330
    glDeleteFramebuffers(count, handles);
#elif IGAME_OPENGL_VERSION_460
    glDeleteFramebuffers(count, handles);
#endif
}

IGAME_NAMESPACE_END
