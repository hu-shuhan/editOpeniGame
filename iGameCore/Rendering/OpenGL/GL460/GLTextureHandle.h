#pragma once

#include "GLVendor.h"

IGAME_NAMESPACE_BEGIN

class GLTextureHandle : public Object {
private:
    GLuint64 handle;

public:
    GLTextureHandle() : handle(0) {}

    explicit GLTextureHandle(GLuint64 handle) : handle(handle) {}

    operator igm::uvec2() const {
        igm::uvec2 result;
        *reinterpret_cast<GLuint64*>(&result) = handle;
        return result;
    }

    void makeResident() { glMakeTextureHandleResidentARB(handle); }
    void makeNonResident() { glMakeTextureHandleNonResidentARB(handle); }
    GLuint64 raw() const { return handle; }
};

IGAME_NAMESPACE_END