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
        throw std::runtime_error("failed to open file");
    }

    void CheckCompileErrors() {
        int success;
        std::string infoLog;
        infoLog.resize(BUFSIZ);

        glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

        if (!success) {
            glGetShaderInfoLog(handle, BUFSIZ, NULL, infoLog.data());
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR\n"
                      << infoLog << std::endl;
            throw std::runtime_error("Shader compilation failed");
        }
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

    void SetUniform(const GLUniform::Pointer uniform, int value) const {
        glProgramUniform1i(handle, uniform->Index(), value);
    }

    void SetUniform(const GLUniform::Pointer uniform,
                    unsigned int value) const {
        glProgramUniform1ui(handle, uniform->Index(), value);
    }

    void GetUniformValue(const GLUniform::Pointer uniform,
                         unsigned int& value) const {
        glGetUniformuiv(handle, uniform->Index(), &value);
    }

    void SetUniform(const GLUniform::Pointer uniform, float value) const {
        glProgramUniform1f(handle, uniform->Index(), value);
    }

    void SetUniform(const GLUniform::Pointer uniform,
                    const igm::uvec2& vec2) const {
        glProgramUniform2uiv(handle, uniform->Index(), 1, vec2.data());
    }

    void SetUniform(const GLUniform::Pointer uniform,
                    const igm::vec3& vec3) const {
        glProgramUniform3fv(handle, uniform->Index(), 1, vec3.data());
    }

    void SetUniform(const GLUniform::Pointer uniform,
                    const igm::vec4& vec4) const {
        glProgramUniform4fv(handle, uniform->Index(), 1, vec4.data());
    }

    void SetUniform(const GLUniform::Pointer uniform,
                    const igm::mat4& mat4) const {
        glProgramUniformMatrix4fv(handle, uniform->Index(), 1, false,
                                  mat4.data());
    }

    void SetUniform(const GLUniform::Pointer uniform, bool transpose,
                    const igm::mat4& mat4) const {
        glProgramUniformMatrix4fv(handle, uniform->Index(), 1, transpose,
                                  mat4.data());
    }

    void MapUniformBlock(const char* uniformBlockName,
                         uint32_t uniformBlockBinding,
                         GLBuffer::Pointer m_UBOBlock) {
        GLuint blockIndex = glGetUniformBlockIndex(handle, uniformBlockName);
        assert(blockIndex != GL_INVALID_INDEX);

        glUniformBlockBinding(handle, blockIndex, uniformBlockBinding);
        m_UBOBlock->Target(GL_UNIFORM_BUFFER);
        m_UBOBlock->BindBase(uniformBlockBinding);
    }

    GLVertexAttribute GetAttribLocation(const char* const name) {
        int location = glGetAttribLocation(handle, name);
        assert(location != -1);
        return GLVertexAttribute{static_cast<unsigned int>(location)};
    }

    GLUniform::Pointer GetUniformLocation(const char* const name) {
        int location = glGetUniformLocation(handle, name);
        assert(location != -1);

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
            throw std::runtime_error("shader program linkage failed: " +
                                     infoLog);
        }
    }

    GLuint handle;
};

IGAME_NAMESPACE_END