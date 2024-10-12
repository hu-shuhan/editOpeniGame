#pragma once

#include "GLTexture2d.h"
#include "GLVertexArray.h"
#include <fstream>
#include <sstream>

IGAME_NAMESPACE_BEGIN

class GLShader : public Object {
private:
    GLuint handle;
    friend class GLShaderProgram;

    void checkCompileErrors() {
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

public:
    GLShader(const char* const file_path, GLenum type)
        : handle(glCreateShader(type)) {
        std::string sourceStr = read_file(file_path);
        const char* source = sourceStr.c_str();

        auto src_cpy = source;
        glShaderSource(handle, 1, &src_cpy, 0);
        glCompileShader(handle);

        checkCompileErrors();
    }

    std::string read_file(const char* file_path) {
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

    ~GLShader() { glDeleteShader(handle); }
};

class GLUniform : public Object {
private:
    GLuint m_index;

public:
    explicit GLUniform(unsigned int location) : m_index{location} {}

    unsigned int index() const { return m_index; }
};

class GLShaderProgram : public Object {
private:
    GLuint handle;

    void checkCompileErrors() {
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

public:
    GLShaderProgram() {}

    template<typename... Shaders>
    void addShaders(Shaders&&... shaders) {
        handle = glCreateProgram();

        (glAttachShader(handle, shaders.handle), ...);

        glLinkProgram(handle);
        checkCompileErrors();
    }

    void use() const { glUseProgram(handle); }

    GLuint programID() const { return handle; }

    void setUniform(const GLUniform& uniform, int value) const {
        glUniform1i(uniform.index(), value);
    }

    void setUniform(const GLUniform& uniform, unsigned int value) const {
        glUniform1ui(uniform.index(), value);
    }

    void setUniform(const GLUniform& uniform, float value) const {
        glUniform1f(uniform.index(), value);
    }

    void setUniform(const GLUniform& uniform, const igm::uvec2& vec2) const {
        glUniform2ui(uniform.index(), vec2.x, vec2.y);
    }

    void setUniform(const GLUniform& uniform, const igm::vec3& vec3) const {
        glUniform3f(uniform.index(), vec3.x, vec3.y, vec3.z);
    }

    void setUniform(const GLUniform& uniform, const igm::vec4& vec4) const {
        glUniform4f(uniform.index(), vec4.x, vec4.y, vec4.z, vec4.w);
    }

    void setUniform(const GLUniform& uniform, const igm::mat4& mat4) const {
        glUniformMatrix4fv(uniform.index(), 1, GL_FALSE, mat4.data());
    }

    void setUniform(const GLUniform& uniform, bool transpose,
                    const igm::mat4& mat4) const {
        glUniformMatrix4fv(uniform.index(), 1, transpose ? GL_TRUE : GL_FALSE,
                           mat4.data());
    }

    void mapUniformBlock(const char* uniformBlockName,
                         uint32_t uniformBlockBinding, GLBuffer& m_UBOBlock) {
        GLuint blockIndex = glGetUniformBlockIndex(handle, uniformBlockName);
        glUniformBlockBinding(handle, blockIndex, uniformBlockBinding);
        m_UBOBlock.target(GL_UNIFORM_BUFFER);
        m_UBOBlock.bindBase(uniformBlockBinding);
    }

    GLVertexAttribute getAttribLocation(const char* const name) {
        int location = glGetAttribLocation(handle, name);
        assert(location != -1);
        return GLVertexAttribute{static_cast<unsigned int>(location)};
    }

    GLUniform getUniformLocation(const char* const name) {
        int location = glGetUniformLocation(handle, name);
        assert(location != -1);
        return GLUniform{static_cast<unsigned int>(location)};
    }

    ~GLShaderProgram() { glDeleteProgram(handle); }
};

IGAME_NAMESPACE_END