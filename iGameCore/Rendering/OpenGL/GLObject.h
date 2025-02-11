#ifndef IGAMEVIS_GLOBJECT_H
#define IGAMEVIS_GLOBJECT_H

#include "GLVendor.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

template<typename Helper>
class GLObject : public Object {
public:
    I_OBJECT(GLObject);
    //static Pointer New() { return new GLObject; }

    void Create();

    void Destroy();

    GLuint Handle();

    explicit operator GLuint() const noexcept;

    operator bool() const noexcept;

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
    GLObject();
    explicit GLObject(GLuint handle);
    ~GLObject() override;

    GLuint m_Handle;
};

template<typename Helper>
GLObject<Helper>::GLObject() {
    m_Handle = 0;
};

template<typename Helper>
GLObject<Helper>::GLObject(GLuint handle) {
    m_Handle = handle;
}

template<typename Helper>
GLObject<Helper>::~GLObject() {
    Destroy();
}

template<typename Helper>
void GLObject<Helper>::Create() {
    if (m_Handle == 0) { Helper::CreateHandle(1, &m_Handle); }
}

template<typename Helper>
void GLObject<Helper>::Destroy() {
    if (m_Handle != 0) {
        Helper::DestroyHandle(1, &m_Handle);
        m_Handle = 0;
    }
}

template<typename Helper>
GLuint GLObject<Helper>::Handle() {
    return m_Handle;
}

template<typename Helper>
GLObject<Helper>::operator GLuint() const noexcept {
    return m_Handle;
}

template<typename Helper>
GLObject<Helper>::operator bool() const noexcept {
    return m_Handle != 0;
}

IGAME_NAMESPACE_END

#endif // IGAMEVIS_GLOBJECT_H
