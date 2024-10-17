#pragma once

#include "GLObject.h"
#include "GLRenderbuffer.h"
#include "GLTexture2d.h"
#include "GLTexture2dArray.h"
#include "GLTexture2dMultisample.h"

IGAME_NAMESPACE_BEGIN
class GLFramebuffer : public GLObject<GLFramebuffer> {
public:
    I_OBJECT(GLFramebuffer);
    static Pointer New() { return new GLFramebuffer; }

    // GLbitfield mask: GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT, GL_STENCIL_BUFFER_BIT
    // GLenum filter: GL_NEAREST, GL_LINEAR
    static void Blit(const GLFramebuffer::Pointer source,
                     const GLFramebuffer::Pointer destination, GLint srcX0,
                     GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0,
                     GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                     GLenum filter) {
        glBlitNamedFramebuffer(source->handle, destination->handle, srcX0,
                               srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                               mask, filter);
    }

public:
    // GLenum target: GL_FRAMEBUFFER, GL_READ_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER
    void Target(GLenum target) { m_Target = target; }

    void Bind() const { glBindFramebuffer(m_Target, handle); }
    void Release() const { glBindFramebuffer(m_Target, 0); }

    void DrawBuffers(size_t count, GLenum* buffers) {
        glNamedFramebufferDrawBuffers(handle, count, buffers);
    }

    // GLenum attachment: GL_COLOR_ATTACHMENT0(texture need GL_RGB)
    // GLenum attachment: GL_DEPTH_ATTACHMENT(texture need GL_DEPTH_COMPONENT)
    // GLenum attachment: GL_STENCIL_ATTACHMENT(texture need GL_STENCIL_INDEX)
    void Texture(GLenum attachment, const GLTexture2d::Pointer texture,
                 unsigned mip_level) {
        glNamedFramebufferTexture(handle, attachment, texture->Handle(),
                                  mip_level);
    }
    void Texture(GLenum attachment,
                 const GLTexture2dMultisample::Pointer texture,
                 unsigned mip_level) {
        glNamedFramebufferTexture(handle, attachment, texture->Handle(),
                                  mip_level);
    }

    void TextureLayer(GLenum attachment,
                      const GLTexture2dArray::Pointer texture,
                      unsigned mip_level, unsigned layer) {
        glNamedFramebufferTextureLayer(handle, attachment, texture->Handle(),
                                       mip_level, layer);
    }

    void Renderbuffer(GLenum attachment, GLenum renderbuffer_target,
                      const GLRenderbuffer::Pointer rbo) {
        glNamedFramebufferRenderbuffer(handle, attachment, renderbuffer_target,
                                       rbo->Handle());
    }

    GLenum CheckStatus() {
        return glCheckNamedFramebufferStatus(handle, m_Target);
    }

protected:
    GLFramebuffer() = default;
    ~GLFramebuffer() override = default;

    friend class GLObject<GLFramebuffer>;
    static void CreateHandle(GLsizei count, GLuint* handles) {
        glCreateFramebuffers(count, handles);
    }
    static void DestroyHandle(GLsizei count, GLuint* handles) {
        glDeleteFramebuffers(count, handles);
    }

    GLenum m_Target;
};
IGAME_NAMESPACE_END