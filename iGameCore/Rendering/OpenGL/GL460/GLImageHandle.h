#pragma once

#include "GLVendor.h"

IGAME_NAMESPACE_BEGIN

/*
 * need extension GL_ARB_bindless_texture
 */

class GLImageHandle : public Object {
private:
    GLuint64 handle;

public:
    GLImageHandle() : handle(0) {}

    explicit GLImageHandle(GLuint64 handle) : handle(handle) {}

    operator igm::uvec2() const {
        igm::uvec2 result;
        *reinterpret_cast<GLuint64*>(&result) = handle;
        return result;
    }
    operator GLuint64() const { return handle; }

    // GLenum access: GL_WRITE_ONLY
    void makeResident(GLenum access) {
        glMakeImageHandleResidentARB(handle, access);
    }
    void makeNonResident() { glMakeImageHandleNonResidentARB(handle); }
};

IGAME_NAMESPACE_END