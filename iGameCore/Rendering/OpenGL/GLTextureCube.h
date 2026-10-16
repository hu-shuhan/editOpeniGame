#pragma once
#ifndef __EMSCRIPTEN__
#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

// Floating-point cubemap; callers must have a current desktop GL context.
class GLTextureCube : public GLObject<GLTextureCube> {
public:
    I_OBJECT(GLTextureCube);
    static Pointer New() { return new GLTextureCube; }
    void Storage(unsigned levels, unsigned size);
    void GenerateMipmap();
    void Bind(unsigned unit) const;

protected:
    GLTextureCube() = default;
    ~GLTextureCube() override = default;
    friend class GLObject<GLTextureCube>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);
};

IGAME_NAMESPACE_END
#endif
