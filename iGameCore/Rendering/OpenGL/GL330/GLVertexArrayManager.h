//
// Created by Sumzeek on 8/16/2024.
//

#pragma once

#include "GLVendor.h"
#include <unordered_map>

IGAME_NAMESPACE_BEGIN

struct GLVertexArrayManagerTupleHash {
    template<typename T1, typename T2>
    std::size_t operator()(const std::tuple<T1, T2>& t) const {
        auto hash1 = std::hash<T1>{}(std::get<0>(t));
        auto hash2 = std::hash<T2>{}(std::get<1>(t));
        return hash1 ^ (hash2 << 1);
    }
};

class GLVertexArrayManager : public Object {
private:
    GLVertexArrayManager() = default;
    ~GLVertexArrayManager() = default;

public:
    static GLVertexArrayManager& Instance() {
        static GLVertexArrayManager instance;
        return instance;
    }

    void RegisterBufferToVertexArray(unsigned int vao,
                                     unsigned int vbo_binding_index,
                                     unsigned int buffer, size_t stride) {
        auto key = std::make_tuple(vao, vbo_binding_index);
        auto value = std::make_tuple(buffer, stride);
        bufferMap[key] = value;
    }

    void UnRegisterVertexArray(unsigned int vao) {
        for (auto it = bufferMap.begin(); it != bufferMap.end();) {
            if (std::get<0>(it->first) == vao) {
                it = bufferMap.erase(it);
            } else {
                ++it;
            }
        }
    }

    unsigned int GetBuffer(unsigned int vao,
                           unsigned int vbo_binding_index) const {
        auto key = std::make_tuple(vao, vbo_binding_index);
        auto it = bufferMap.find(key);
        if (it != bufferMap.end()) {
            return std::get<0>(it->second);
        } else {
            throw std::runtime_error(
                    "Buffer not found for given VAO and binding index.");
        }
    }

    size_t GetStride(unsigned int vao, unsigned int vbo_binding_index) const {
        auto key = std::make_tuple(vao, vbo_binding_index);
        auto it = bufferMap.find(key);
        if (it != bufferMap.end()) {
            return std::get<1>(it->second);
        } else {
            throw std::runtime_error(
                    "Buffer not found for given VAO and binding index.");
        }
    }

private:
    std::unordered_map<std::tuple<unsigned int, unsigned int>,
                       std::tuple<unsigned int, size_t>,
                       GLVertexArrayManagerTupleHash>
            bufferMap;
};

IGAME_NAMESPACE_END