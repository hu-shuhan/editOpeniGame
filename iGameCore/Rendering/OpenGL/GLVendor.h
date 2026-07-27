//
// Created by Sumzeek on 6/29/2024.
//

#ifndef IGAMEVIS_GLVENDOR_H
#define IGAMEVIS_GLVENDOR_H

#ifdef __EMSCRIPTEN__
    #include <GLES2/gl2.h>
    #ifndef GL_GLEXT_PROTOTYPES
        #define GL_GLEXT_PROTOTYPES 1
    #endif
    #include <GLES2/gl2ext.h>

    #ifndef GL_INVALID_INDEX
        #define GL_INVALID_INDEX 0xFFFFFFFFu
    #endif

    #ifndef GL_UNIFORM_BUFFER
        #define GL_UNIFORM_BUFFER 0
    #endif

    #ifndef GL_COPY_READ_BUFFER
        #define GL_COPY_READ_BUFFER GL_ARRAY_BUFFER
    #endif

    #ifndef GL_COPY_WRITE_BUFFER
        #define GL_COPY_WRITE_BUFFER GL_ARRAY_BUFFER
    #endif

    #ifndef GL_TEXTURE_BUFFER
        #define GL_TEXTURE_BUFFER 0
    #endif

    #ifndef GL_DRAW_INDIRECT_BUFFER
        #define GL_DRAW_INDIRECT_BUFFER 0
    #endif

    #ifndef GL_SHADER_STORAGE_BUFFER
        #define GL_SHADER_STORAGE_BUFFER 0
    #endif

    #ifndef GL_GEOMETRY_SHADER
        #define GL_GEOMETRY_SHADER 0
    #endif

    #ifndef GL_TIME_ELAPSED
        #define GL_TIME_ELAPSED 0
    #endif

    #ifndef GL_QUERY_RESULT_AVAILABLE
        #define GL_QUERY_RESULT_AVAILABLE 0
    #endif

    #ifndef GL_QUERY_RESULT
        #define GL_QUERY_RESULT 0
    #endif

    #ifndef GL_R8
        #define GL_R8 GL_LUMINANCE
    #endif

    #ifndef GL_RED
        #define GL_RED GL_LUMINANCE
    #endif

    #ifndef GL_R32F
        #define GL_R32F GL_ALPHA
    #endif

    #ifndef GL_RGB8
        #define GL_RGB8 GL_RGB
    #endif

    #ifndef GL_DEPTH_COMPONENT32F
        #define GL_DEPTH_COMPONENT32F GL_DEPTH_COMPONENT
    #endif

    #ifndef GL_DEPTH24_STENCIL8
        #define GL_DEPTH24_STENCIL8 GL_DEPTH_COMPONENT
    #endif

    #ifndef GL_DEPTH_STENCIL
        #define GL_DEPTH_STENCIL GL_DEPTH_COMPONENT
    #endif

    #ifndef GL_UNSIGNED_INT_24_8
        #define GL_UNSIGNED_INT_24_8 GL_UNSIGNED_INT
    #endif

    #ifndef GL_RGBA8
        #define GL_RGBA8 GL_RGBA
    #endif

    #ifndef GL_DEPTH_COMPONENT24
        #define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT
    #endif

    #ifndef GL_COLOR_ATTACHMENT1
        #define GL_COLOR_ATTACHMENT1 GL_COLOR_ATTACHMENT0
    #endif

    #ifndef GL_CLAMP_TO_BORDER
        #define GL_CLAMP_TO_BORDER GL_CLAMP_TO_EDGE
    #endif

    #ifndef GL_READ_FRAMEBUFFER
        #define GL_READ_FRAMEBUFFER GL_FRAMEBUFFER
    #endif

    #ifndef GL_DRAW_FRAMEBUFFER
        #define GL_DRAW_FRAMEBUFFER GL_FRAMEBUFFER
    #endif

    #ifndef GL_TEXTURE_2D_ARRAY
        #define GL_TEXTURE_2D_ARRAY GL_TEXTURE_2D
    #endif

    #ifndef GL_TEXTURE_2D_MULTISAMPLE
        #define GL_TEXTURE_2D_MULTISAMPLE GL_TEXTURE_2D
    #endif

    #ifndef GL_FILL
        #define GL_FILL 0
    #endif

inline void glBindBufferBase(GLenum, GLuint, GLuint) {}
inline void glCopyBufferSubData(GLenum, GLenum, GLintptr, GLintptr,
                                GLsizeiptr) {}
inline void* glMapBufferRange(GLenum, GLintptr, GLsizeiptr, GLbitfield) {
    return nullptr;
}
inline void glGetBufferSubData(GLenum, GLintptr, GLsizeiptr, void*) {}
inline GLboolean glUnmapBuffer(GLenum) { return GL_TRUE; }
inline GLuint glGetUniformBlockIndex(GLuint, const GLchar*) {
    return GL_INVALID_INDEX;
}
inline void glUniformBlockBinding(GLuint, GLuint, GLuint) {}
inline void glUniform1ui(GLint location, GLuint v0) {
    glUniform1i(location, static_cast<GLint>(v0));
}
inline void glUniform2ui(GLint location, GLuint v0, GLuint v1) {
    glUniform2i(location, static_cast<GLint>(v0), static_cast<GLint>(v1));
}
inline void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    glUniform3i(location, static_cast<GLint>(v0), static_cast<GLint>(v1),
                static_cast<GLint>(v2));
}
inline void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2,
                         GLuint v3) {
    glUniform4i(location, static_cast<GLint>(v0), static_cast<GLint>(v1),
                static_cast<GLint>(v2), static_cast<GLint>(v3));
}
inline void glDeleteQueries(GLsizei, const GLuint*) {}
inline void glGenQueries(GLsizei n, GLuint* ids) {
    if (!ids) return;
    for (GLsizei i = 0; i < n; ++i) ids[i] = 0;
}
inline void glBeginQuery(GLenum, GLuint) {}
inline void glEndQuery(GLenum) {}
inline void glGetQueryObjectiv(GLuint, GLenum, GLint* params) {
    if (params) *params = 1;
}
inline void glGetQueryObjectui64v(GLuint, GLenum, GLuint64* params) {
    if (params) *params = 0;
}
inline void glPolygonMode(GLenum, GLenum) {}
inline void glClearDepth(GLdouble depth) {
    glClearDepthf(static_cast<GLfloat>(depth));
}
inline void glBlitFramebuffer(GLint, GLint, GLint, GLint, GLint, GLint, GLint,
                              GLint, GLbitfield, GLenum) {}
inline void glDrawBuffers(GLsizei, const GLenum*) {}
inline void glFramebufferTexture(GLenum target, GLenum attachment,
                                 GLuint texture, GLint level) {
    glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, texture, level);
}
inline void glFramebufferTextureLayer(GLenum, GLenum, GLuint, GLint, GLint) {}
inline void glTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLsizei,
                            GLsizei, GLsizei, GLenum, GLenum, const void*) {}
inline void glTexImage2DMultisample(GLenum target, GLsizei,
                                    GLenum internalformat, GLsizei width,
                                    GLsizei height, GLboolean) {
    glTexImage2D(target, 0, internalformat, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
}
inline void glTexBuffer(GLenum, GLenum, GLuint) {}

    #if defined(GL_OES_vertex_array_object)
inline void glBindVertexArray(GLuint array) { glBindVertexArrayOES(array); }
inline void glGenVertexArrays(GLsizei n, GLuint* arrays) {
    glGenVertexArraysOES(n, arrays);
}
inline void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
    glDeleteVertexArraysOES(n, arrays);
}
    #else
inline void glBindVertexArray(GLuint) {}
inline void glGenVertexArrays(GLsizei n, GLuint* arrays) {
    if (!arrays) return;
    for (GLsizei i = 0; i < n; ++i) arrays[i] = 0;
}
inline void glDeleteVertexArrays(GLsizei, const GLuint*) {}
    #endif
inline void glDrawRangeElements(GLenum mode, GLuint, GLuint, GLsizei count,
                                GLenum type, const void* indices) {
    glDrawElements(mode, count, type, indices);
}

inline void glPointSize(GLfloat) {}

    #ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
        #define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0
    #endif

    #ifndef GL_TEXTURE_FETCH_BARRIER_BIT
        #define GL_TEXTURE_FETCH_BARRIER_BIT 0
    #endif

    #ifndef GL_FRAMEBUFFER_BARRIER_BIT
        #define GL_FRAMEBUFFER_BARRIER_BIT 0
    #endif

inline void glMemoryBarrier(GLbitfield) {}
#else
    #include "glad/glad.h"
#endif
#include "iGameMacro.h"
#include "iGameObject.h"
#include "iGameRenderingLogger.h"
#include "iGameRenderingMacro.h"
#include "igm/igm.h"
#include "igm/transform.h"

#endif //IGAMEVIS_GLVENDOR_H
