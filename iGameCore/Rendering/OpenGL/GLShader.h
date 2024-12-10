#ifndef GLSHADER_H
#define GLSHADER_H

#include "GLTexture2d.h"
#include "GLVertexArray.h"
#include <fstream>
#include <sstream>

IGAME_NAMESPACE_BEGIN

class GLShader : public Object {
public:
    I_OBJECT(GLShader);
    static Pointer New() { return new GLShader; }

    void Compile(const char* const file_path, GLenum type);

protected:
    GLShader();
    ~GLShader() override;

    void CheckCompileErrors();

    std::string ReadFile(const char* file_path);

    GLuint m_Handle;

    friend class GLShaderProgram;
};

GLShader::Pointer CreateShader(const std::string& path, GLenum shaderType);

class GLUniform : public Object {
public:
    I_OBJECT(GLUniform);
    static Pointer New() { return new GLUniform; }

    unsigned int Index() const;

protected:
    GLUniform();
    ~GLUniform() override;

    GLuint m_Index;

    friend class GLShaderProgram;
};

class GLShaderProgram : public Object {
public:
    I_OBJECT(GLShaderProgram);
    static Pointer New() { return new GLShaderProgram; }

    template<typename... Shaders>
    void AddShaders(Shaders&&... shaders);

    void Use() const;

    GLuint ProgramID() const;

    // SetUniform1
    void SetUniformi(const char* name, int value) const;
    void SetUniformf(const char* name, float value) const;
    void SetUniformui(const char* name, unsigned int value) const;

    // SetUniform2
    void SetUniform2i(const char* name, const igm::ivec2& vec2) const;
    void SetUniform2f(const char* name, const igm::vec2& vec2) const;
    void SetUniform2ui(const char* name, const igm::uvec2& vec2) const;

    // SetUniform3
    void SetUniform3i(const char* name, const igm::ivec3& vec3) const;
    void SetUniform3f(const char* name, const igm::vec3& vec3) const;
    void SetUniform3ui(const char* name, const igm::uvec3& vec3) const;

    // SetUniform4
    void SetUniform4i(const char* name, const igm::ivec4& vec4) const;
    void SetUniform4f(const char* name, const igm::vec4& vec4) const;
    void SetUniform4ui(const char* name, const igm::uvec4& vec4) const;

    // SetUniformMatrix
    void SetUniformMatrix3x3(const char* name, const igm::mat3& mat3) const;
    void SetUniformMatrix4x4(const char* name, const igm::mat4& mat4) const;
    void SetUniformMatrix4x4(const char* name, bool transpose,
                             const igm::mat4& mat4) const;

    void MapUniformBlock(const char* uniformBlockName,
                         uint32_t uniformBlockBinding,
                         GLBuffer::Pointer m_UBOBlock);

    GLVertexAttribute GetAttribLocation(const char* name);

    GLUniform::Pointer GetUniformLocation(const char* name) const;

protected:
    GLShaderProgram();
    ~GLShaderProgram() override;

    void CheckCompileErrors();

    GLuint m_Handle;
};

template<typename... Shaders>
void GLShaderProgram::AddShaders(Shaders&&... shaders) {
    m_Handle = glCreateProgram();

    (glAttachShader(m_Handle, shaders->m_Handle), ...);

    glLinkProgram(m_Handle);
    CheckCompileErrors();
}

IGAME_NAMESPACE_END

#endif // GLSHADER_H
