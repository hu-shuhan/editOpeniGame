#include "iGameSurfaceMeshMeshleter.h"
#include "iGameRenderingLogger.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameTimer.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

SurfaceMeshMeshleter::SurfaceMeshMeshleter() {}

SurfaceMeshMeshleter::~SurfaceMeshMeshleter() {}

void SurfaceMeshMeshleter::Build() {
    if (m_DataObject->GetDataObjectType() != IG_SURFACE_MESH) {
        IGAME_RENDERING_ERROR(
                "{} is not a SurfaceMesh, but it will be processed using "
                "SurfaceMeshMeshleter for meshleting.",
                m_DataObject->GetName());
    }

    SmartPointer<Timer> timer = Timer::New();

    SmartPointer<FloatArray> positions = FloatArray::New();
    SmartPointer<UnsignedIntArray> triangleIndices = UnsignedIntArray::New();
    struct KeyHash {
        size_t operator()(const std::array<unsigned int, 3>& k) const noexcept {
            // simple combine
            return (static_cast<size_t>(k[0]) * 73856093u) ^
                   (static_cast<size_t>(k[1]) * 19349663u) ^
                   (static_cast<size_t>(k[2]) * 83492791u);
        }
    };
    std::unordered_map<std::array<unsigned int, 3>, unsigned int, KeyHash>
            triToFace;

    // convert to draw date
    timer->Reset();
    {
        auto mesh = DynamicCast<SurfaceMesh>(m_DataObject);

        // positions
        positions = mesh->GetPoints()->ConvertToArray();
        // indices
        triangleIndices->SetDimension(3);
        // triangle to face map
        triToFace.reserve(mesh->GetNumberOfFaces() * 2);

        igIndex cell[32]{};
        for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
            int ncell = mesh->GetFacePointIds(i, cell);
            for (int j = 1; j < ncell - 1; j++) {
                triangleIndices->AddElement3(cell[0], cell[j], cell[j + 1]);
                triToFace[{static_cast<unsigned int>(cell[0]),
                           static_cast<unsigned int>(cell[j]),
                           static_cast<unsigned int>(cell[j + 1])}] =
                        static_cast<unsigned int>(i);
                // add edge mask
                //int mask = ncell == 3 ? 7 : j == 1 ? 3 : j == ncell - 2 ? 6 : 2;
                //triangleEdgeMasks->AddValue(mask);
            }
        }

        IGAME_RENDERING_TRACE(
                "DataObject {}, convert to rendering data [time: {}]",
                m_DataObject->GetName(),
                FormatTime(timer->ElapsedMilliseconds()));
    }

    // build meshlet
    timer->Reset();
    {
        const float* vertex_positions = positions->RawPointer();
        size_t vertex_count = positions->GetNumberOfElements();
        const unsigned int* indices = triangleIndices->RawPointer();
        size_t index_count = triangleIndices->GetNumberOfValues();

        // use for mesh shader
        std::vector<unsigned int> meshletVertices;
        std::vector<unsigned char> meshletTriangles;

        // Preprocessing: Vertex Cache Optimization
        std::vector<unsigned int> optimized_indices(index_count);
        meshopt_optimizeVertexCache(optimized_indices.data(), indices,
                                    index_count, vertex_count);

        // Compute the upper bound of the meshlet
        size_t max_meshlets = meshopt_buildMeshletsBound(
                index_count, m_MaxVertices, m_MaxTriangles);

        // Allocate meshlet data structure
        std::vector<meshopt_Meshlet> meshlets(max_meshlets);
        // Triangle index
        meshletVertices.resize(max_meshlets * m_MaxVertices);
        // Index of triangle index
        meshletTriangles.resize(max_meshlets * m_MaxTriangles * 3);

        // Generate meshlet data
        size_t meshlet_count = meshopt_buildMeshlets(
                meshlets.data(), meshletVertices.data(),
                meshletTriangles.data(), optimized_indices.data(), index_count,
                vertex_positions, vertex_count, sizeof(float) * 3,
                m_MaxVertices, m_MaxTriangles, m_ConeWeight);

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

        m_MeshletCount = meshlet_count;

#ifdef GL_SUPPORTS_MESH_SHADER
        // Prepare MeshletDescriptors array to store meshlet descriptors
        std::vector<MeshletDescriptor> meshletDescriptors(meshlet_count);
        for (size_t i = 0; i < meshlet_count; ++i) {
            const meshopt_Bounds& b = meshlet_bounds[i];
            meshletDescriptors[i] = {
                    igm::vec4{b.center[0], b.center[1], b.center[2], b.radius},
                    igm::vec4{0.0f}};
        }

        // create buffer
        {
            m_MeshletBuffer->Create();
            m_MeshletBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_MeshletBuffer->Allocate(meshlets.size() * sizeof(Meshlet),
                                      meshlets.data(), GL_STATIC_DRAW);

            m_MeshletVertexBuffer->Create();
            m_MeshletVertexBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_MeshletVertexBuffer->Allocate(
                    meshletVertices.size() * sizeof(unsigned int),
                    meshletVertices.data(), GL_STATIC_DRAW);

            m_MeshletTriangleBuffer->Create();
            m_MeshletTriangleBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_MeshletTriangleBuffer->Allocate(
                    meshletTriangles.size() * sizeof(unsigned char),
                    meshletTriangles.data(), GL_STATIC_DRAW);

            m_MeshletDescriptorBuffer->Create();
            m_MeshletDescriptorBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_MeshletDescriptorBuffer->Allocate(
                    meshletDescriptors.size() * sizeof(MeshletDescriptor),
                    meshletDescriptors.data(), GL_STATIC_DRAW);

            m_InvisibleMeshletBuffer->Create();
            m_InvisibleMeshletBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_InvisibleMeshletBuffer->Allocate((1 + meshlets.size()) *
                                                       sizeof(unsigned int),
                                               nullptr, GL_DYNAMIC_DRAW);

            m_PositionBuffer->Create();
            m_PositionBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_PositionBuffer->Allocate(vertex_count * 3 * sizeof(float),
                                       vertex_positions, GL_STATIC_DRAW);
        }
#else
        // Record indirect Command
        m_MeshletIndices.resize(meshletTriangles.size());
        m_TriangleToFace.resize(meshletTriangles.size() / 3);
        m_MeshletDescriptors.resize(meshlet_count);
        m_ElementsDrawCommands.resize(meshlet_count);
        m_ArraysDrawCommands.resize(meshlet_count);

        // store cell positions for array draws
        SmartPointer<FloatArray> cellPositions = FloatArray::New();
        cellPositions->Reserve(meshletTriangles.size() * 3);

        unsigned int cellVertexOffset = 0u;
        for (size_t i = 0; i < meshlet_count; ++i) {
            const meshopt_Meshlet& m = meshlets[i];
            const meshopt_Bounds& b = meshlet_bounds[i];

            for (auto j = m.triangle_offset;
                 j < m.triangle_offset + m.triangle_count * 3; j += 3) {
                for (auto k = 0; k < 3; ++k) {
                    auto id = meshletVertices[m.vertex_offset +
                                              meshletTriangles[j + k]];
                    m_MeshletIndices[j + k] = id;
                    cellPositions->AddValue(positions->GetValue(id * 3 + 0));
                    cellPositions->AddValue(positions->GetValue(id * 3 + 1));
                    cellPositions->AddValue(positions->GetValue(id * 3 + 2));
                }
                m_TriangleToFace[j / 3] =
                        triToFace[{m_MeshletIndices[j], m_MeshletIndices[j + 1],
                                   m_MeshletIndices[j + 2]}];
            }

            for (auto j = m.triangle_offset;
                 j < m.triangle_offset + m.triangle_count * 3; j++) {
                m_MeshletIndices[j] =
                        meshletVertices[m.vertex_offset + meshletTriangles[j]];
            }

            m_MeshletDescriptors[i] = {
                    igm::vec4{b.center[0], b.center[1], b.center[2], b.radius},
                    igm::vec4{0.0f}};

            m_ElementsDrawCommands[i] = DrawElementsIndirectCommand{
                    m.triangle_count * 3, 0, m.triangle_offset, 0, 0};

            // DrawArraysIndirectCommand.first is the starting vertex index.
            m_ArraysDrawCommands[i] = DrawArraysIndirectCommand{
                    m.triangle_count * 3, 0, cellVertexOffset, 0};
            cellVertexOffset += static_cast<unsigned int>(m.triangle_count * 3);
        }

        // create draw command buffers
        {
            m_MeshletDescriptorBuffer->Create();
            m_MeshletDescriptorBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_MeshletDescriptorBuffer->Allocate(
                    meshlet_count * sizeof(MeshletDescriptor),
                    m_MeshletDescriptors.data(), GL_STATIC_DRAW);

            m_VisibleMeshletBuffer->Create();
            m_VisibleMeshletBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_VisibleMeshletBuffer->Allocate(sizeof(unsigned int), nullptr,
                                             GL_DYNAMIC_DRAW);
            // m_VisibleMeshletBuffer->Allocate((1 + meshlet_count) *
            //                                          sizeof(unsigned int),
            //                                  nullptr, GL_DYNAMIC_DRAW);

            m_DrawCommandBuffer->Create();
            m_DrawCommandBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_DrawCommandBuffer->Allocate(
                    meshlet_count * sizeof(DrawElementsIndirectCommand),
                    m_ElementsDrawCommands.data(), GL_STATIC_DRAW);

            m_FinalDrawCommandBuffer->Create();
            m_FinalDrawCommandBuffer->Target(GL_DRAW_INDIRECT_BUFFER);
            m_FinalDrawCommandBuffer->Allocate(
                    meshlet_count * sizeof(DrawElementsIndirectCommand),
                    nullptr, GL_DYNAMIC_DRAW);

            m_CellDrawCommandBuffer->Create();
            m_CellDrawCommandBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_CellDrawCommandBuffer->Allocate(
                    meshlet_count * sizeof(DrawArraysIndirectCommand),
                    m_ArraysDrawCommands.data(), GL_STATIC_DRAW);

            m_CellFinalDrawCommandBuffer->Create();
            m_CellFinalDrawCommandBuffer->Target(GL_DRAW_INDIRECT_BUFFER);
            m_CellFinalDrawCommandBuffer->Allocate(
                    meshlet_count * sizeof(DrawArraysIndirectCommand), nullptr,
                    GL_DYNAMIC_DRAW);
        }

        // create position buffer
        {
            // element draw
            m_PositionVBO->Create();
            m_PositionVBO->Target(GL_ARRAY_BUFFER);
            m_PositionVBO->Allocate(positions->GetNumberOfValues() *
                                            sizeof(float),
                                    positions->RawPointer(), GL_STATIC_DRAW);
            m_PositionVBO->Modified();

            m_TriangleEBO->Create();
            m_TriangleEBO->Target(GL_ELEMENT_ARRAY_BUFFER);
            m_TriangleEBO->Allocate(m_MeshletIndices.size() *
                                            sizeof(unsigned int),
                                    m_MeshletIndices.data(), GL_STATIC_DRAW);

            m_TriangleVAO->Create();
            m_TriangleVAO->VertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
                                        3 * sizeof(float));
            GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                              GL_FLOAT, GL_FALSE, 0);
            m_TriangleVAO->ElementBuffer(m_TriangleEBO);

            // array draw
            m_CellPositionVBO->Create();
            m_CellPositionVBO->Target(GL_ARRAY_BUFFER);
            m_CellPositionVBO->Allocate(
                    cellPositions->GetNumberOfValues() * sizeof(float),
                    cellPositions->RawPointer(), GL_STATIC_DRAW);
            m_CellPositionVBO->Modified();

            m_CellTriangleVAO->Create();
            m_CellTriangleVAO->VertexBuffer(GL_VBO_IDX_0, m_CellPositionVBO, 0,
                                            3 * sizeof(float));
            GLSetVertexAttrib(m_CellTriangleVAO, GL_LOCATION_IDX_0,
                              GL_VBO_IDX_0, 3, GL_FLOAT, GL_FALSE, 0);
        }
#endif

        IGAME_RENDERING_TRACE(
                "DataObject {}, build meshlets [count: {}, time: {}]",
                m_DataObject->GetName(), meshlet_count,
                FormatTime(timer->ElapsedMilliseconds()));
    }

    this->Modified();
}

IGAME_NAMESPACE_END
