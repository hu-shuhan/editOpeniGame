//
// Created by Sumzeek on 12/9/2024.
//
#include "GLShader.h"
#include <filesystem>

IGAME_NAMESPACE_BEGIN

GLShader::GLShader() {}

GLShader::~GLShader() {
    if (m_Handle != 0) {
        glDeleteShader(m_Handle);
        m_Handle = 0;
    }
}

SmartPointer<GLShader> GLShader::CreateShader(const std::string& path,
                                              GLenum shaderType) {
    auto shader = GLShader::New();
    shader->SetName(std::filesystem::path(path).filename().string());
    shader->Compile(path.c_str(), shaderType);
    return shader;
}

void GLShader::Compile(const char* const file_path, GLenum type) {
    m_Handle = glCreateShader(type);

    std::string sourceStr = ReadFile(file_path);
    const char* source = sourceStr.c_str();

    auto src_cpy = source;
    glShaderSource(m_Handle, 1, &src_cpy, 0);
    glCompileShader(m_Handle);

    CheckCompileErrors();
}

void GLShader::CheckCompileErrors() {
    int success;
    std::string infoLog;
    infoLog.resize(BUFSIZ);

    glGetShaderiv(m_Handle, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(m_Handle, BUFSIZ, NULL, infoLog.data());
        IGAME_RENDERING_ERROR("[GLShader::CheckCompileErrors] Shader Name: '{}', "
                         "Error: SHADER_COMPILATION_FAILED, Details: {}",
                         this->GetName(), infoLog);
    }
}

std::string GLShader::ReadFile(const char* file_path) {
    std::ifstream file(file_path, std::ios::in | std::ios::binary);
    if (file) {
        std::string contents;
        file.seekg(0, std::ios::end);
        contents.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(contents.data(), contents.size());
        file.close();
        return contents;
    } else {
        IGAME_RENDERING_ERROR("[GLShader::ReadFile] Failed to open file '{}', "
                         "Shader Name: '{}'",
                         file_path, this->GetName());
        return std::string();
    }
}

GLUniform::GLUniform(){};

GLUniform::~GLUniform(){};

unsigned int GLUniform::Index() const { return m_Index; }

GLShaderProgram::GLShaderProgram() {}

GLShaderProgram::~GLShaderProgram() {
    if (m_Handle != 0) {
        glDeleteProgram(m_Handle);
        m_Handle = 0;
    }
}

void GLShaderProgram::Use() const { glUseProgram(m_Handle); }

GLuint GLShaderProgram::ProgramID() const { return m_Handle; }

// SetUniform1
void GLShaderProgram::SetUniformi(const char* name, int value) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform1i(uniform->Index(), value);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform1i(m_Handle, uniform->Index(), value);
#endif
}

void GLShaderProgram::SetUniformf(const char* name, float value) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform1f(uniform->Index(), value);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform1f(m_Handle, uniform->Index(), value);
#endif
}

void GLShaderProgram::SetUniformui(const char* name, unsigned int value) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform1ui(uniform->Index(), value);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform1ui(m_Handle, uniform->Index(), value);
#endif
}

// SetUniform2
void GLShaderProgram::SetUniform2i(const char* name,
                                   const igm::ivec2& vec2) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform2i(uniform->Index(), vec2.x, vec2.y);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform2iv(m_Handle, uniform->Index(), 1, vec2.data());
#endif
}

void GLShaderProgram::SetUniform2f(const char* name,
                                   const igm::vec2& vec2) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform2f(uniform->Index(), vec2.x, vec2.y);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform2fv(m_Handle, uniform->Index(), 1, vec2.data());
#endif
}

void GLShaderProgram::SetUniform2ui(const char* name,
                                    const igm::uvec2& vec2) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform2ui(uniform->Index(), vec2.x, vec2.y);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform2uiv(m_Handle, uniform->Index(), 1, vec2.data());
#endif
}

// SetUniform3
void GLShaderProgram::SetUniform3i(const char* name,
                                   const igm::ivec3& vec3) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform3i(uniform->Index(), vec3.x, vec3.y, vec3.z);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform3iv(m_Handle, uniform->Index(), 1, vec3.data());
#endif
}

void GLShaderProgram::SetUniform3f(const char* name,
                                   const igm::vec3& vec3) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform3f(uniform->Index(), vec3.x, vec3.y, vec3.z);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform3fv(m_Handle, uniform->Index(), 1, vec3.data());
#endif
}

void GLShaderProgram::SetUniform3ui(const char* name,
                                    const igm::uvec3& vec3) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform3ui(uniform->Index(), vec3.x, vec3.y, vec3.z);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform3uiv(m_Handle, uniform->Index(), 1, vec3.data());
#endif
}

// SetUniform4i
void GLShaderProgram::SetUniform4i(const char* name,
                                   const igm::ivec4& vec4) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform4i(uniform->Index(), vec4.x, vec4.y, vec4.z, vec4.w);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform4iv(m_Handle, uniform->Index(), 1, vec4.data());
#endif
}

void GLShaderProgram::SetUniform4f(const char* name,
                                   const igm::vec4& vec4) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform4f(uniform->Index(), vec4.x, vec4.y, vec4.z, vec4.w);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform4fv(m_Handle, uniform->Index(), 1, vec4.data());
#endif
}

void GLShaderProgram::SetUniform4ui(const char* name,
                                    const igm::uvec4& vec4) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniform4ui(uniform->Index(), vec4.x, vec4.y, vec4.z, vec4.w);
#elif IGAME_OPENGL_VERSION_460
    glProgramUniform4uiv(m_Handle, uniform->Index(), 1, vec4.data());
#endif
}

// SetUniformMatrix
void GLShaderProgram::SetUniformMatrix3x3(const char* name,
                                          const igm::mat3& mat3) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniformMatrix3fv(uniform->Index(), 1, GL_FALSE, mat3.data());
#elif IGAME_OPENGL_VERSION_460
    glProgramUniformMatrix3fv(m_Handle, uniform->Index(), 1, false,
                              mat3.data());
#endif
}

void GLShaderProgram::SetUniformMatrix4x4(const char* name,
                                          const igm::mat4& mat4) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniformMatrix4fv(uniform->Index(), 1, GL_FALSE, mat4.data());
#elif IGAME_OPENGL_VERSION_460
    glProgramUniformMatrix4fv(m_Handle, uniform->Index(), 1, false,
                              mat4.data());
#endif
}

void GLShaderProgram::SetUniformMatrix4x4(const char* name, bool transpose,
                                          const igm::mat4& mat4) const {
    SmartPointer<GLUniform> uniform = GetUniformLocation(name);

#ifdef IGAME_OPENGL_VERSION_330
    glUniformMatrix4fv(uniform->Index(), 1, transpose ? GL_TRUE : GL_FALSE,
                       mat4.data());
#elif IGAME_OPENGL_VERSION_460
    glProgramUniformMatrix4fv(m_Handle, uniform->Index(), 1,
                              transpose ? GL_TRUE : GL_FALSE, mat4.data());
#endif
}

void GLShaderProgram::MapUniformBlock(const char* uniformBlockName,
                                      uint32_t uniformBlockBinding,
                                      SmartPointer<GLBuffer> m_UBOBlock) {
    GLuint blockIndex = glGetUniformBlockIndex(m_Handle, uniformBlockName);
    if (blockIndex == GL_INVALID_INDEX) {
        IGAME_RENDERING_ERROR("[GLShaderProgram::MapUniformBlock] Shader '{}' does "
                         "not contain the Uniform Block '{}'.",
                         this->GetName(), uniformBlockName);
        return;
    }
    glUniformBlockBinding(m_Handle, blockIndex, uniformBlockBinding);
    m_UBOBlock->Target(GL_UNIFORM_BUFFER);
    m_UBOBlock->BindBase(uniformBlockBinding);
}


GLVertexAttribute GLShaderProgram::GetAttribLocation(const char* name) {
    int location = glGetAttribLocation(m_Handle, name);
    if (location == -1) {
        IGAME_RENDERING_ERROR("[GLShaderProgram::GetAttribLocation] Shader '{}' "
                         "does not contain the attribute '{}' (location: -1).",
                         this->GetName(), name);
    }

    return GLVertexAttribute{static_cast<unsigned int>(location)};
}

SmartPointer<GLUniform>
GLShaderProgram::GetUniformLocation(const char* name) const {
    int location = glGetUniformLocation(m_Handle, name);
    if (location == -1) {
        IGAME_RENDERING_ERROR("[GLShaderProgram::GetUniformLocation] Shader '{}' "
                         "does not contain the uniform '{}' (location: -1).",
                         this->GetName(), name);
    }

    SmartPointer<GLUniform> uniform = GLUniform::New();
    uniform->m_Index = static_cast<unsigned int>(location);
    return uniform;
}

void GLShaderProgram::CheckCompileErrors() {
    int success;
    std::string infoLog;
    infoLog.resize(BUFSIZ);

    glGetProgramiv(m_Handle, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(m_Handle, BUFSIZ, NULL, infoLog.data());
        IGAME_RENDERING_ERROR("[GLShaderProgram::CheckCompileErrors] Shader program "
                         "'{}' linkage failed. Error: {}",
                         this->GetName(), infoLog);
    }
}

IGAME_NAMESPACE_END
