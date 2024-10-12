#pragma once
#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLIndirectCommand.h"
#include "iGameObject.h"
#include "iGameTimer.h"
#include "meshoptimizer.h"

IGAME_NAMESPACE_BEGIN

#ifdef IGAME_OPENGL_VERSION_460
class Meshlet : public Object {
public:
    I_OBJECT(Meshlet);
    static Pointer New() { return new Meshlet; }

protected:
    Meshlet() {}
    ~Meshlet() override {}

    struct MeshletData {
        igm::vec4 spherebounds;
        igm::vec4 extents;
    };

public:
    void CreateBuffer();
    void BuildMeshlet(const float* vertex_positions, size_t vertex_count,
                      const unsigned int* indices, size_t index_count,
                      GLBuffer& EBO);

    size_t MeshletsCount();
    GLBuffer& MeshletsBuffer();
    GLBuffer& DrawCommandBuffer();
    GLBuffer& VisibleMeshletBuffer();
    GLBuffer& FinalDrawCommandBuffer();

private:
    const size_t m_MaxVertices = 64;
    const size_t m_MaxTriangles = 124;
    const float m_ConeWeight = 0.0f;

    // use for indirect draw
    size_t m_MeshletsCount = 0;
    GLBuffer m_MeshletsBuffer;
    GLBuffer m_DrawCommandBuffer;
    GLBuffer m_VisibleMeshletBuffer;
    GLBuffer m_FinalDrawCommandBuffer;
};
#endif

IGAME_NAMESPACE_END