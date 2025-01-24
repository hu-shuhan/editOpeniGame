//
// Created by Sumzeek on 12/9/2024.
//
#include "GLVertexArrayManager.h"

IGAME_NAMESPACE_BEGIN

GLVertexArrayManager::GLVertexArrayManager() {}

GLVertexArrayManager::~GLVertexArrayManager() {}

GLVertexArrayManager& GLVertexArrayManager::Instance() {
    static GLVertexArrayManager instance;
    return instance;
}

void GLVertexArrayManager::RegisterBufferToVertexArray(
        unsigned int vao, unsigned int vbo_binding_index, unsigned int buffer,
        size_t stride) {
    auto key = std::make_tuple(vao, vbo_binding_index);
    auto value = std::make_tuple(buffer, stride);
    m_BufferMapper[key] = value;
}

void GLVertexArrayManager::UnRegisterVertexArray(unsigned int vao) {
    for (auto it = m_BufferMapper.begin(); it != m_BufferMapper.end();) {
        if (std::get<0>(it->first) == vao) {
            it = m_BufferMapper.erase(it);
        } else {
            ++it;
        }
    }
}

unsigned int
GLVertexArrayManager::GetBuffer(unsigned int vao,
                                unsigned int vbo_binding_index) const {
    auto key = std::make_tuple(vao, vbo_binding_index);
    auto it = m_BufferMapper.find(key);
    if (it != m_BufferMapper.end()) {
        return std::get<0>(it->second);
    } else {
        Logger::LogWarn("Buffer not found for given VAO and binding index.");
        return 0;
    }
}

size_t GLVertexArrayManager::GetStride(unsigned int vao,
                                       unsigned int vbo_binding_index) const {
    auto key = std::make_tuple(vao, vbo_binding_index);
    auto it = m_BufferMapper.find(key);
    if (it != m_BufferMapper.end()) {
        return std::get<1>(it->second);
    } else {
        Logger::LogWarn("Buffer not found for given VAO and binding index.");
        return 0;
    }
}

IGAME_NAMESPACE_END
