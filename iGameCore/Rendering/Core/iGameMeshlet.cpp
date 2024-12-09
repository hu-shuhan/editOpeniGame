#include "iGameMeshlet.h"

IGAME_NAMESPACE_BEGIN

#ifdef IGAME_OPENGL_VERSION_460

Meshlet::Meshlet() {
    m_MeshletsCount = 0;
    m_MeshletsBuffer = GLBuffer::New();
    m_DrawCommandBuffer = GLBuffer::New();
    m_VisibleMeshletBuffer = GLBuffer::New();
    m_FinalDrawCommandBuffer = GLBuffer::New();
}

Meshlet::~Meshlet() {}

void Meshlet::CreateBuffer() {
    m_MeshletsBuffer->Create();
    m_DrawCommandBuffer->Create();
    m_VisibleMeshletBuffer->Create();
    m_FinalDrawCommandBuffer->Create();
}

void Meshlet::BuildMeshlet(const float* vertex_positions, size_t vertex_count,
                           const unsigned int* indices, size_t index_count,
                           UnsignedIntArray::Pointer afterBuildIndices) {
    Timer::Pointer timer = Timer::New();
    timer->Reset();

    // use for mesh shader
    std::vector<unsigned int> meshletVertices;
    std::vector<unsigned char> meshletTriangles;

    // Preprocessing: Vertex Cache Optimization
    std::vector<unsigned int> optimized_indices(index_count);
    meshopt_optimizeVertexCache(optimized_indices.data(), indices, index_count,
                                vertex_count);

    // Compute the upper bound of the meshlet
    size_t max_meshlets = meshopt_buildMeshletsBound(index_count, m_MaxVertices,
                                                     m_MaxTriangles);

    // Allocate meshlet data structure
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    // Triangle index
    meshletVertices.resize(max_meshlets * m_MaxVertices);
    // Index of triangle index
    meshletTriangles.resize(max_meshlets * m_MaxTriangles * 3);

    // Generate meshlet data
    size_t meshlet_count = meshopt_buildMeshlets(
            meshlets.data(), meshletVertices.data(), meshletTriangles.data(),
            optimized_indices.data(), index_count, vertex_positions,
            vertex_count, sizeof(float) * 3, m_MaxVertices, m_MaxTriangles,
            m_ConeWeight);

    // Resize the vector to fit the actual generated meshlet data
    const meshopt_Meshlet& last = meshlets[meshlet_count - 1];
    meshletVertices.resize(last.vertex_offset + last.vertex_count);
    meshletTriangles.resize(last.triangle_offset +
                            ((last.triangle_count * 3 + 3) & ~3));
    meshlets.resize(meshlet_count);

    // Optimize vertex and index data for each meshlet
    for (size_t i = 0; i < meshlet_count; ++i) {
        const meshopt_Meshlet& m = meshlets[i];
        meshopt_optimizeMeshlet(&meshletVertices[m.vertex_offset],
                                &meshletTriangles[m.triangle_offset],
                                m.triangle_count, m.vertex_count);
    }

    // Calculate the boundary data of meshlet for cluster culling
    std::vector<meshopt_Bounds> meshlet_bounds(meshlet_count);
    for (size_t i = 0; i < meshlet_count; ++i) {
        const meshopt_Meshlet& m = meshlets[i];
        meshlet_bounds[i] = meshopt_computeMeshletBounds(
                &meshletVertices[m.vertex_offset],
                &meshletTriangles[m.triangle_offset], m.triangle_count,
                vertex_positions, vertex_count, sizeof(float) * 3);
    }

    // Record indirect Command
    std::vector<unsigned int> meshletIndices;
    std::vector<MeshletData> meshletDatas(meshlet_count);
    std::vector<DrawElementsIndirectCommand> drawCommands(meshlet_count);
    meshletIndices.resize(meshletTriangles.size());
    for (size_t i = 0; i < meshlet_count; ++i) {
        const meshopt_Meshlet& m = meshlets[i];
        const meshopt_Bounds& b = meshlet_bounds[i];

        meshletDatas[i] = {
                igm::vec4{b.center[0], b.center[1], b.center[2], b.radius},
                igm::vec4{0.0f}};
        drawCommands[i] = {m.triangle_count * 3, 0, m.triangle_offset, 0, 0};

        for (auto j = m.triangle_offset;
             j < m.triangle_offset + m.triangle_count * 3; j++) {
            meshletIndices[j] =
                    meshletVertices[m.vertex_offset + meshletTriangles[j]];
        }
    }

    afterBuildIndices->Reset();
    afterBuildIndices->SetDimension(3);
    for (size_t i = 0; i < meshletIndices.size(); i++) {
        afterBuildIndices->AddValue(meshletIndices[i]);
    }
    afterBuildIndices->Modified();
    //GLAllocateGLBuffer(EBO, meshletIndices.size() * sizeof(igIndex),
    //                   meshletIndices.data());

    m_MeshletsCount = meshlet_count;
    m_MeshletsBuffer->Target(GL_SHADER_STORAGE_BUFFER);
    m_MeshletsBuffer->Allocate(meshletDatas.size() * sizeof(MeshletData),
                               meshletDatas.data(), GL_STATIC_DRAW);

    m_DrawCommandBuffer->Target(GL_SHADER_STORAGE_BUFFER);
    m_DrawCommandBuffer->Allocate(drawCommands.size() *
                                          sizeof(DrawElementsIndirectCommand),
                                  drawCommands.data(), GL_STATIC_DRAW);

    m_VisibleMeshletBuffer->Target(GL_SHADER_STORAGE_BUFFER);
    m_VisibleMeshletBuffer->Allocate(drawCommands.size() * sizeof(unsigned int),
                                     nullptr, GL_DYNAMIC_DRAW);

    m_FinalDrawCommandBuffer->Target(GL_DRAW_INDIRECT_BUFFER);
    m_FinalDrawCommandBuffer->Allocate(
            drawCommands.size() * sizeof(DrawElementsIndirectCommand), nullptr,
            GL_DYNAMIC_DRAW);

    std::cout << std::format("Build meshlets [count: {}, time: {}]",
                             meshlet_count,
                             FormatTime(timer->ElapsedMilliseconds()))
              << std::endl;
}

size_t Meshlet::MeshletsCount() { return m_MeshletsCount; };

GLBuffer::Pointer Meshlet::MeshletsBuffer() { return m_MeshletsBuffer; };

GLBuffer::Pointer Meshlet::VisibleMeshletBuffer() {
    return m_VisibleMeshletBuffer;
}

GLBuffer::Pointer Meshlet::DrawCommandBuffer() { return m_DrawCommandBuffer; };

GLBuffer::Pointer Meshlet::FinalDrawCommandBuffer() {
    return m_FinalDrawCommandBuffer;
};

#endif

IGAME_NAMESPACE_END