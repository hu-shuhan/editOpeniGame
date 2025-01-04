//
// Created by Sumzeek on 1/4/2025.
//

#pragma once

#include "OpenGL/GLBuffer.h"
#include "iGameDataObject.h"
#include "iGameObject.h"
#include "igm/igm.h"
#include "meshoptimizer.h"

IGAME_NAMESPACE_BEGIN
class Meshleter : public Object {
public:
    I_OBJECT(Meshleter);
    static Pointer New() { return new Meshleter; }

    struct MeshletData {
        igm::vec4 spherebounds;
        igm::vec4 extents;
    };

    struct Meshlet {
        unsigned int vertex_offset;
        unsigned int triangle_offset;

        unsigned int vertex_count;
        unsigned int triangle_count;
    };

    void Build(DataObject::Pointer dataObject);

protected:
    Meshleter();
    ~Meshleter() override;

    const size_t m_MaxVertices = 64;
    const size_t m_MaxTriangles = 124;
    const float m_ConeWeight = 0.0f;

    GLBuffer::Pointer m_MeshletBuffer;
    GLBuffer::Pointer m_MeshletVertexBuffer;
    GLBuffer::Pointer m_MeshletTriangleBuffer;

    GLBuffer::Pointer m_PositionBuffer;
    GLBuffer::Pointer m_ColorBuffer;
    GLBuffer::Pointer m_NormalBuffer;
    GLBuffer::Pointer m_UVBuffer;
};
IGAME_NAMESPACE_END
