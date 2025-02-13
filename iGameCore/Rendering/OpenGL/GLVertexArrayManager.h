//
// Created by Sumzeek on 8/16/2024.
//
#ifndef IGAMEVIS_GLVERTEXARRAYMANAGER_H
#define IGAMEVIS_GLVERTEXARRAYMANAGER_H

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
public:
    static GLVertexArrayManager& Instance();

    void RegisterBufferToVertexArray(unsigned int vao,
                                     unsigned int vbo_binding_index,
                                     unsigned int buffer, size_t stride);

    void UnRegisterVertexArray(unsigned int vao);

    unsigned int GetBuffer(unsigned int vao,
                           unsigned int vbo_binding_index) const;

    size_t GetStride(unsigned int vao, unsigned int vbo_binding_index) const;

protected:
    GLVertexArrayManager();
    ~GLVertexArrayManager() override;

    std::unordered_map<std::tuple<unsigned int, unsigned int>,
                       std::tuple<unsigned int, size_t>,
                       GLVertexArrayManagerTupleHash>
            m_BufferMapper;
};

IGAME_NAMESPACE_END

#endif // IGAMEVIS_GLVERTEXARRAYMANAGER_H