#include "GLTextureCube.h"
#ifndef __EMSCRIPTEN__
#include <algorithm>
IGAME_NAMESPACE_BEGIN

void GLTextureCube::CreateHandle(GLsizei count, GLuint* handles) {
    glGenTextures(count, handles);
}
void GLTextureCube::DestroyHandle(GLsizei count, GLuint* handles) {
    glDeleteTextures(count, handles);
}
void GLTextureCube::Storage(unsigned levels, unsigned size) {
    GLint previous = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previous);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle);
    for (unsigned level = 0; level < levels; ++level) {
        const unsigned extent = std::max(1u, size >> level);
        for (unsigned face = 0; face < 6; ++face) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, GL_RGB16F,
                         extent, extent, 0, GL_RGB, GL_FLOAT, nullptr);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, levels - 1);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, previous);
}
void GLTextureCube::GenerateMipmap() {
    GLint previous = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previous);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, previous);
}
void GLTextureCube::Bind(unsigned unit) const {
    GLint active = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle);
    glActiveTexture(active);
}

IGAME_NAMESPACE_END
#endif
