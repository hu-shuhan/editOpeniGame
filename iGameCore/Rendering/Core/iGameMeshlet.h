#pragma once

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLIndirectCommand.h"
#include "iGameFlatArray.h"
#include "iGameObject.h"
#include "iGameTimer.h"
#include "meshoptimizer.h"

IGAME_NAMESPACE_BEGIN

#ifdef IGAME_OPENGL_VERSION_460
class Meshlet : public Object {
public:
    I_OBJECT(Meshlet);
    static Pointer New() { return new Meshlet; }

    struct MeshletData {
        igm::vec4 spherebounds;
        igm::vec4 extents;
    };

    void CreateBuffer();
    void BuildMeshlet(const float* vertex_positions, size_t vertex_count,
                      const unsigned int* indices, size_t index_count,
                      UnsignedIntArray::Pointer afterBuildIndices);

    size_t MeshletsCount();
    GLBuffer::Pointer MeshletsBuffer();
    GLBuffer::Pointer DrawCommandBuffer();
    GLBuffer::Pointer VisibleMeshletBuffer();
    GLBuffer::Pointer FinalDrawCommandBuffer();

protected:
    Meshlet();
    ~Meshlet() override;

    const size_t m_MaxVertices = 64;
    const size_t m_MaxTriangles = 124;
    const float m_ConeWeight = 0.0f;

    // use for indirect draw
    size_t m_MeshletsCount;
    GLBuffer::Pointer m_MeshletsBuffer;
    GLBuffer::Pointer m_DrawCommandBuffer;
    GLBuffer::Pointer m_VisibleMeshletBuffer;
    GLBuffer::Pointer m_FinalDrawCommandBuffer;
};
#endif

IGAME_NAMESPACE_END