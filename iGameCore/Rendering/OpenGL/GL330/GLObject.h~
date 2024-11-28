#pragma once

#include "GLVendor.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
template<typename Helper>
class GLObject : public Object {
public:
    I_OBJECT(GLObject);
    //static Pointer New() { return new GLObject; }

    void Create() {
        if (handle == 0) { Helper::CreateHandle(1, &handle); }
    }
    void Destroy() {
        if (handle != 0) {
            Helper::DestroyHandle(1, &handle);
            handle = 0;
        }
    }

    GLuint Handle() { return handle; }

    operator GLuint() const noexcept { return handle; }

    operator bool() const noexcept { return handle != 0; }

    //GLObject(const GLObject&) = delete;
    //GLObject(GLObject&& other) noexcept : handle{other.handle} {
    //    other.handle = 0;
    //}
    //
    //GLObject& operator=(const GLObject&) = delete;
    //GLObject& operator=(GLObject&& other) noexcept {
    //    destroy();
    //    handle = other.handle;
    //    other.handle = 0;
    //    return *this;
    //};

protected:
    GLObject() = default;
    explicit GLObject(GLuint _handle) : handle{_handle} {}
    ~GLObject() override { Destroy(); }

    GLuint handle = 0;
};

IGAME_NAMESPACE_END