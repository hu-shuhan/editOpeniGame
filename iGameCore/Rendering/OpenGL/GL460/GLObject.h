#pragma once

#include "GLVendor.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
template<typename Helper>
class GLObject : public Object {
protected:
    GLuint handle = 0;
    GLObject(GLuint _handle) : handle{_handle} {}

public:
    GLObject() = default;
    ~GLObject() { destroy(); }

    void create() {
        if (handle == 0) { Helper::createHandle(1, &handle); }
    }
    void destroy() {
        if (handle != 0) {
            Helper::destroyHandle(1, &handle);
            handle = 0;
        }
    }

    GLObject(const GLObject&) = delete;
    GLObject(GLObject&& other) noexcept : handle{other.handle} {
        other.handle = 0;
    }

    GLObject& operator=(const GLObject&) = delete;
    GLObject& operator=(GLObject&& other) noexcept {
        destroy();
        handle = other.handle;
        other.handle = 0;
        return *this;
    };

    operator GLuint() const noexcept { return handle; }

    operator bool() const noexcept { return handle != 0; }
};

IGAME_NAMESPACE_END