#pragma once

#include "GLTexture2d.h"
#include "GLVertexArray.h"
#include <fstream>
#include <sstream>

IGAME_NAMESPACE_BEGIN

class GLShader : public Object {
public:
    I_OBJECT(GLShader);
    static Pointer New() { return new GLShader; }

    void Compile(const char* const file_path, GLenum type) {
        handle = glCreateShader(type);

        std::string sourceStr = ReadFile(file_path);
        const char* source = sourceStr.c_str();

        auto src_cpy = source;
        glShaderSource(handle, 1, &src_cpy, 0);
        glCompileShader(handle);

        CheckCompileErrors();
    }

protected:
    GLShader() = default;
    ~GLShader() {
        if (handle != 0) {
            glDeleteShader(handle);
            handle = 0;
        }
    }

    void CheckCompileErrors() {
        int success;
        std::string infoLog;
        infoLog.resize(BUFSIZ);

        glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

        if (!success) {
            glGetShaderInfoLog(handle, BUFSIZ, NULL, infoLog.data());
            igError("ERROR::SHADER_COMPILATION_ERROR\n" + infoLog);
            throw std::runtime_error("Shader compilation failed");
        }
    }

    std::string ReadFile(const char* file_path) {
        std::ifstream file(file_path, std::ios::in | std::ios::binary);
        if (file) {
            std::string contents;
            file.seekg(0, std::ios::end);
            contents.resize(file.tellg());
            file.seekg(0, std::ios::beg);
            file.read(contents.data(), contents.size());
            file.close();
            return contents;
        }
        igError("failed to open file");
        throw std::runtime_error("failed to open file");
    }

    GLuint handle;

    friend class GLShaderProgram;
};

inline GLShader::Pointer CreateShader(const std::string& path,
                                      GLenum shaderType) {
    auto shader = GLShader::New();
    shader->Compile(path.c_str(), shaderType);
    return shader;
}

class GLUniform : public Object {
public:
    I_OBJECT(GLUniform);
    static Pointer New() { return new GLUniform; }

    unsigned int Index() const { return m_index; }

protected:
    GLUniform() = default;
    //explicit GLUniform(unsigned int location) : m_index{location} {}
    ~GLUniform() override = default;

    GLuint m_index;

    friend class GLShaderProgram;
};

class GLShaderProgram : public Object {
public:
    I_OBJECT(GLShaderProgram);
    static Pointer New() { return new GLShaderProgram; }

    template<typename... Shaders>
    void AddShaders(Shaders&&... shaders) {
        handle = glCreateProgram();

        // 解引用 smart pointers 以访问 handle
        (glAttachShader(handle, shaders->handle), ...);

        glLinkProgram(handle);
        CheckCompileErrors();
    }

    void Use() const { glUseProgram(handle); }

    GLuint ProgramID() const { return handle; }

    // SetUniform1
    void SetUniformi(const char* const name, int value) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform1i(uniform->Index(), value);
    }

    void SetUniformf(const char* const name, float value) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform1f(uniform->Index(), value);
    }

    void SetUniformui(const char* const name, unsigned int value) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform1ui(uniform->Index(), value);
    }


    // SetUniform2
    void SetUniform2i(const char* const name, const igm::ivec2& vec2) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform2i(uniform->Index(), vec2.x, vec2.y);
    }

    void SetUniform2f(const char* const name, const igm::uvec2& vec2) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform2f(uniform->Index(), vec2.x, vec2.y);
    }

    void SetUniform2ui(const char* const name, const igm::uvec2& vec2) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform2ui(uniform->Index(), vec2.x, vec2.y);
    }

    // SetUniform3
    void SetUniform3i(const char* const name, const igm::ivec3& vec3) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform3i(uniform->Index(), vec3.x, vec3.y, vec3.z);
    }

    void SetUniform3f(const char* const name, const igm::vec3& vec3) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform3f(uniform->Index(), vec3.x, vec3.y, vec3.z);
    }

    void SetUniform3ui(const char* const name, const igm::uvec3& vec3) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform3ui(uniform->Index(), vec3.x, vec3.y, vec3.z);
    }

    // SetUniform4i
    void SetUniform4i(const char* const name, const igm::ivec4& vec4) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform4i(uniform->Index(), vec4.x, vec4.y, vec4.z, vec4.w);
    }

    void SetUniform4f(const char* const name, const igm::vec4& vec4) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform4f(uniform->Index(), vec4.x, vec4.y, vec4.z, vec4.w);
    }

    void SetUniform4ui(const char* const name, const igm::uvec4& vec4) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniform4ui(uniform->Index(), vec4.x, vec4.y, vec4.z, vec4.w);
    }

    // SetUniformMatrix
    void SetUniformMatrix3x3(const char* const name,
                             const igm::mat3& mat3) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniformMatrix3fv(uniform->Index(), 1, GL_FALSE, mat3.data());
    }

    void SetUniformMatrix4x4(const char* const name,
                             const igm::mat4& mat4) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniformMatrix4fv(uniform->Index(), 1, GL_FALSE, mat4.data());
    }

    void SetUniformMatrix4x4(const char* const name, bool transpose,
                             const igm::mat4& mat4) const {
        GLUniform::Pointer uniform = GetUniformLocation(name);
        glUniformMatrix4fv(uniform->Index(), 1, transpose ? GL_TRUE : GL_FALSE,
                           mat4.data());
    }

    void MapUniformBlock(const char* uniformBlockName,
                         uint32_t uniformBlockBinding,
                         GLBuffer::Pointer m_UBOBlock) {
        GLuint blockIndex = glGetUniformBlockIndex(handle, uniformBlockName);
        if (blockIndex == GL_INVALID_INDEX) {
            igError("Uniform block does not exist: " << uniformBlockName);
        }

        glUniformBlockBinding(handle, blockIndex, uniformBlockBinding);
        m_UBOBlock->Target(GL_UNIFORM_BUFFER);
        m_UBOBlock->BindBase(uniformBlockBinding);
    }

    GLVertexAttribute GetAttribLocation(const char* const name) {
        int location = glGetAttribLocation(handle, name);
        if (location == -1) {
            igError("Could not get attribute (does not exist) " << name);
        }

        return GLVertexAttribute{static_cast<unsigned int>(location)};
    }

    GLUniform::Pointer GetUniformLocation(const char* const name) const {
        int location = glGetUniformLocation(handle, name);
        if (location == -1) {
            igError("Could not get uniform (does not exist) " << name);
        }

        GLUniform::Pointer uniform = GLUniform::New();
        uniform->m_index = static_cast<unsigned int>(location);
        return uniform;
    }

protected:
    GLShaderProgram() {}
    ~GLShaderProgram() {
        if (handle != 0) {
            glDeleteProgram(handle);
            handle = 0;
        }
    }

    void CheckCompileErrors() {
        int success;
        std::string infoLog;
        infoLog.resize(BUFSIZ);

        glGetProgramiv(handle, GL_LINK_STATUS, &success);

        if (!success) {
            glGetProgramInfoLog(handle, BUFSIZ, NULL, infoLog.data());
            igError("shader program linkage failed: " + infoLog);
            throw std::runtime_error("shader program linkage failed: " +
                                     infoLog);
        }
    }

    GLuint handle;
};

IGAME_NAMESPACE_END