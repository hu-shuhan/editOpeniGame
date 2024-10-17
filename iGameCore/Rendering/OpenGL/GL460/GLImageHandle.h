#pragma once

#include "GLVendor.h"

IGAME_NAMESPACE_BEGIN

/*
 * need extension GL_ARB_bindless_texture
 */

class GLImageHandle : public Object {
public:
    I_OBJECT(GLImageHandle);
    //static Pointer New() { return new GLImageHandle; }

    GLuint64 Raw() const { return handle; }

    // GLenum access: GL_WRITE_ONLY
    void MakeResident(GLenum access) {
        glMakeImageHandleResidentARB(handle, access);
    }
    void MakeNonResident() { glMakeImageHandleNonResidentARB(handle); }

protected:
    GLImageHandle() : handle(0) {}
    explicit GLImageHandle(GLuint64 handle) : handle(handle) {}

    GLuint64 handle;

    friend class GLTexture2d;
    friend class GLTexture2dMultisample;

private:
    operator igm::uvec2() const {
        igm::uvec2 result;
        *reinterpret_cast<GLuint64*>(&result) = handle;
        return result;
    }
    operator GLuint64() const { return handle; }
};

IGAME_NAMESPACE_END