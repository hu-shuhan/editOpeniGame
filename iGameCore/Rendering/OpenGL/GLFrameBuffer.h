#ifndef GLFRAMEBUFFER_H
#define GLFRAMEBUFFER_H

#include "GLObject.h"
#include "GLRenderBuffer.h"
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
    static void Blit(SmartPointer<GLFramebuffer> source,
                     SmartPointer<GLFramebuffer> destination, GLint srcX0,
                     GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0,
                     GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                     GLenum filter);

    void DrawBuffers(size_t count, GLenum* buffers);

    // GLenum attachment: GL_COLOR_ATTACHMENT0(texture need GL_RGB)
    // GLenum attachment: GL_DEPTH_ATTACHMENT(texture need GL_DEPTH_COMPONENT)
    // GLenum attachment: GL_STENCIL_ATTACHMENT(texture need GL_STENCIL_INDEX)
    // GLenum attachment: GL_DEPTH_STENCIL_ATTACHMENT(texture need GL_DEPTH24_STENCIL8)
    void Texture(GLenum attachment, SmartPointer<GLTexture2d> texture,
                 unsigned mip_level);
    void Texture(GLenum attachment,
                 SmartPointer<GLTexture2dMultisample> texture,
                 unsigned mip_level);

    void TextureLayer(GLenum attachment, SmartPointer<GLTexture2dArray> texture,
                      unsigned mip_level, unsigned layer);

    void Renderbuffer(GLenum attachment, GLenum renderbuffer_target,
                      SmartPointer<GLRenderBuffer> rbo);

    GLenum CheckStatus();

    void Bind() const;
    void Release() const;
    // GLenum target: GL_FRAMEBUFFER, GL_READ_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER
    void Target(GLenum target);

private:
    GLFramebuffer();
    ~GLFramebuffer() override;

    friend class GLObject<GLFramebuffer>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);

    GLenum m_Target;
};

IGAME_NAMESPACE_END

#endif // GLFRAMEBUFFER_H
