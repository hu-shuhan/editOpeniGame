#include "iGameMeshleter.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnStructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

Meshleter::Meshleter() {
    m_MeshletBuffer = GLBuffer::New();
    m_MeshletVertexBuffer = GLBuffer::New();
    m_MeshletTriangleBuffer = GLBuffer::New();

    m_MeshletDescriptor = GLBuffer::New();
    m_VisibleMeshletCounter = GLBuffer::New();

    m_PositionBuffer = GLBuffer::New();
    m_ColorBuffer = GLBuffer::New();
    m_NormalBuffer = GLBuffer::New();
    m_UVBuffer = GLBuffer::New();
}

Meshleter::~Meshleter() {}

void Meshleter::Build(DataObject::Pointer dataObject) {
    if (DynamicCast<UnstructuredMesh>(dataObject) ||
        DynamicCast<StructuredMesh>(dataObject) ||
        DynamicCast<VolumeMesh>(dataObject)) {
        igError("Now only support surface mesh.");
    }

    Timer::Pointer timer = Timer::New();

    FloatArray::Pointer positions = FloatArray::New();
    UnsignedIntArray::Pointer triangleIndices = UnsignedIntArray::New();

    // convert to draw date
    timer->Reset();
    {
        auto mesh = DynamicCast<SurfaceMesh>(dataObject);

        // positions
        positions = mesh->GetPoints()->ConvertToArray();
        // indices
        triangleIndices->SetDimension(3);

        int i, ncell;
        igIndex cell[32]{};
        for (i = 0; i < mesh->GetNumberOfFaces(); i++) {
            ncell = mesh->GetFacePointIds(i, cell);
            for (int j = 1; j < ncell - 1; j++) {
                triangleIndices->AddElement3(cell[0], cell[j], cell[j + 1]);
                // add edge mask
                //int mask = ncell == 3 ? 7 : j == 1 ? 3 : j == ncell - 2 ? 6 : 2;
                //triangleEdgeMasks->AddValue(mask);
            }
        }

        std::cout << std::format("Convert to draw data [time: {}]",
                                 FormatTime(timer->ElapsedMilliseconds()))
                  << std::endl;
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
        m_MeshletCount = meshlet_count;

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

        // Prepare MeshletDescriptors array to store meshlet descriptors
        std::vector<MeshletDescriptor> MeshletDescriptors(meshlet_count);
        for (size_t i = 0; i < meshlet_count; ++i) {
            const meshopt_Bounds& b = meshlet_bounds[i];
            MeshletDescriptors[i] = {
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

            m_MeshletDescriptor->Create();
            m_MeshletDescriptor->Target(GL_SHADER_STORAGE_BUFFER);
            m_MeshletDescriptor->Allocate(
                    MeshletDescriptors.size() * sizeof(MeshletDescriptor),
                    MeshletDescriptors.data(), GL_STATIC_DRAW);

            m_VisibleMeshletCounter->Create();
            m_VisibleMeshletCounter->Target(GL_SHADER_STORAGE_BUFFER);
            m_VisibleMeshletCounter->Allocate(sizeof(unsigned int), nullptr,
                                              GL_DYNAMIC_DRAW);

            m_PositionBuffer->Create();
            m_PositionBuffer->Target(GL_SHADER_STORAGE_BUFFER);
            m_PositionBuffer->Allocate(vertex_count * 3 * sizeof(float),
                                       vertex_positions, GL_STATIC_DRAW);
        }

        std::cout << std::format("Build meshlets [count: {}, time: {}]",
                                 meshlet_count,
                                 FormatTime(timer->ElapsedMilliseconds()))
                  << std::endl;
    }
}

IGAME_NAMESPACE_END
