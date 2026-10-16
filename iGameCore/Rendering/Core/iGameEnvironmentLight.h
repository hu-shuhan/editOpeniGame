#pragma once
#ifndef __EMSCRIPTEN__
#include "GLShader.h"
#include "GLTextureCube.h"

IGAME_NAMESPACE_BEGIN

class EnvironmentLight : public Object {
public:
    I_OBJECT(EnvironmentLight);
    static Pointer New() { return new EnvironmentLight; }

    // Lazy, one attempt per lifetime. Release permits retry in a new context.
    bool Initialize(const std::string& hdrPath =
                            "./Resources/Assests/Environments/studio_small_08_2k.hdr",
                    const std::string& shaderDirectory = "./Resources/Shaders");
    void Release();
    bool IsReady() const { return m_Ready; }
    void Bind(const SmartPointer<GLShaderProgram>& shader, bool enabled,
              float intensity, float rotationDegrees) const;

protected:
    EnvironmentLight() = default;
    ~EnvironmentLight() override = default;

private:
    bool m_Attempted = false;
    bool m_Ready = false;
    SmartPointer<GLTextureCube> m_Diffuse;
    SmartPointer<GLTextureCube> m_Specular;
};

IGAME_NAMESPACE_END
#endif
