#pragma once

#include "GLVendor.h"

IGAME_NAMESPACE_BEGIN

class GLTextureHandle : public Object {
public:
    I_OBJECT(GLTextureHandle);
    //static Pointer New() { return new GLTextureHandle; }

    GLuint64 Raw() const { return handle; }

    void MakeResident() { glMakeTextureHandleResidentARB(handle); }

    void MakeNonResident() { glMakeTextureHandleNonResidentARB(handle); }

protected:
    GLTextureHandle() : handle(0) {}
    explicit GLTextureHandle(GLuint64 handle) : handle(handle) {}
    ~GLTextureHandle() override = default;

    GLuint64 handle;

    friend class GLTexture2d;
    friend class GLTexture2dArray;
    friend class GLTexture2dMultisample;

private:
    operator igm::uvec2() const {
        igm::uvec2 result;
        *reinterpret_cast<GLuint64*>(&result) = handle;
        return result;
    }
};

IGAME_NAMESPACE_END