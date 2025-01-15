//
// Created by Sumzeek on 1/4/2025.
//

#pragma once

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameDataObject.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

class Model;

class Meshleter : public Object {
public:
    I_OBJECT(Meshleter);
    static Pointer New() { return new Meshleter; }

    struct MeshletDescriptor {
        igm::vec4 spherebounds;
        igm::vec4 extents; // not use now
    };

    struct Meshlet {
        unsigned int vertex_offset;
        unsigned int triangle_offset;

        unsigned int vertex_count;
        unsigned int triangle_count;
    };

    void SetInput(DataObject::Pointer obj);
    void Update();

protected:
    Meshleter();
    ~Meshleter() override;

    virtual void Build();

    const size_t m_MaxVertices = 64;
    const size_t m_MaxTriangles = 124;
    const float m_ConeWeight = 0.0f;

    DataObject::Pointer m_DataObject;
    size_t m_MeshletCount;

#ifdef GL_SUPPORTS_MESH_SHADER
    GLBuffer::Pointer m_MeshletBuffer;
    GLBuffer::Pointer m_MeshletVertexBuffer;
    GLBuffer::Pointer m_MeshletTriangleBuffer;

    GLBuffer::Pointer m_MeshletDescriptorBuffer;
    GLBuffer::Pointer m_InvisibleMeshletBuffer;

    GLBuffer::Pointer m_PositionBuffer;
    GLBuffer::Pointer m_ColorBuffer;
    GLBuffer::Pointer m_NormalBuffer;
    GLBuffer::Pointer m_UVBuffer;
#else
    GLVertexArray::Pointer m_TriangleVAO;
    GLBuffer::Pointer m_TriangleEBO;

    GLBuffer::Pointer m_PositionVBO;
    GLBuffer::Pointer m_ColorVBO;
    GLBuffer::Pointer m_NormalVBO;
    GLBuffer::Pointer m_UVVBO;

    GLBuffer::Pointer m_MeshletDescriptorBuffer;
    GLBuffer::Pointer m_DrawCommandBuffer;
    GLBuffer::Pointer m_VisibleMeshletBuffer;
    GLBuffer::Pointer m_FinalDrawCommandBuffer;
#endif

    friend class Model;
};
IGAME_NAMESPACE_END
