#include "iGameMeshleter.h"

#include "iGameDrawObject.h"
//#include <format>

IGAME_NAMESPACE_BEGIN

Meshleter::Meshleter() {
    m_DataObject = nullptr;

#ifdef GL_SUPPORTS_MESH_SHADER
    m_MeshletBuffer = GLBuffer::New();
    m_MeshletVertexBuffer = GLBuffer::New();
    m_MeshletTriangleBuffer = GLBuffer::New();

    m_MeshletDescriptorBuffer = GLBuffer::New();
    m_InvisibleMeshletBuffer = GLBuffer::New();

    m_PositionBuffer = GLBuffer::New();
    m_ColorBuffer = GLBuffer::New();
    m_NormalBuffer = GLBuffer::New();
    m_UVBuffer = GLBuffer::New();
#else
    m_TriangleVAO = GLVertexArray::New();
    m_TriangleEBO = GLBuffer::New();

    m_PositionVBO = GLBuffer::New();
    m_ColorVBO = GLBuffer::New();
    m_NormalVBO = GLBuffer::New();
    m_UVVBO = GLBuffer::New();

    m_MeshletDescriptorBuffer = GLBuffer::New();
    m_DrawCommandBuffer = GLBuffer::New();
    m_VisibleMeshletBuffer = GLBuffer::New();
    m_FinalDrawCommandBuffer = GLBuffer::New();

    m_CellTriangleVAO = GLVertexArray::New();
    m_CellPositionVBO = GLBuffer::New();
    m_CellColorVBO = GLBuffer::New();
    m_CellDrawCommandBuffer = GLBuffer::New();
    m_CellFinalDrawCommandBuffer = GLBuffer::New();
#endif
}

Meshleter::~Meshleter() {}

void Meshleter::SetInput(SmartPointer<DataObject> obj) {
    m_DataObject = obj;
    // this->SetName(std::format("{}'s Meshleter", m_DataObject->GetName()));
    this->SetName(m_DataObject->GetName());
}

SmartPointer<DataObject> Meshleter::GetInput() const { return m_DataObject; }

void Meshleter::SyncGpuBuffers() {
#ifndef IGAME_OPENGL_VERSION_460
    IGAME_RENDERING_ERROR("The OpenGL330 version does not support meshleter "
                          "accelerated rendering function");
#else
    if (!m_DataObject) { return; }

    auto oldTime = this->GetMTime();
    auto drawObject = DynamicCast<DrawObject>(m_DataObject);

    auto positions = drawObject->m_Positions;
    if (positions->GetMTime() > m_PositionVBO->GetMTime()) { Build(); }

    auto colors = drawObject->m_Colors;
    if (colors->GetMTime() > m_ColorVBO->GetMTime()) {
        m_ColorVBO->Create();
        m_ColorVBO->Target(GL_ARRAY_BUFFER);
        GLAllocateGLBuffer(m_ColorVBO,
                           colors->GetNumberOfValues() * sizeof(float),
                           colors->RawPointer());
        m_ColorVBO->Modified();

        m_TriangleVAO->VertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
                                    3 * sizeof(float));
        GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);
    }

    auto cellColors = drawObject->m_CellColors;
    if (cellColors->GetMTime() > m_CellColorVBO->GetMTime()) {
        auto& attr = drawObject->GetAttributeSet()->GetAttribute(
                drawObject->m_AttributeIndex);
        SmartPointer<FloatArray> cellColorMapper =
                drawObject->m_ColorMapper->MapScalars(
                        attr.pointer, drawObject->m_AttributeDimension);

        float color[3]{};
        SmartPointer<FloatArray> ces = FloatArray::New();
        ces->SetDimension(3);
        for (auto i = 0; i < m_TriangleToFace.size(); i++) {
            cellColorMapper->GetElement(m_TriangleToFace[i], color);
            ces->AddElement3(color[0], color[1], color[2]);
            ces->AddElement3(color[0], color[1], color[2]);
            ces->AddElement3(color[0], color[1], color[2]);
        }

        m_CellColorVBO->Create();
        m_CellColorVBO->Target(GL_ARRAY_BUFFER);
        GLAllocateGLBuffer(m_CellColorVBO,
                           ces->GetNumberOfValues() * sizeof(float),
                           ces->RawPointer());
        m_CellColorVBO->Modified();

        m_CellTriangleVAO->Create();
        m_CellTriangleVAO->VertexBuffer(GL_VBO_IDX_1, m_CellColorVBO, 0,
                                        3 * sizeof(float));
        GLSetVertexAttrib(m_CellTriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);
    }
#endif
}

void Meshleter::ReleaseGpuBuffers() {
#ifdef GL_SUPPORTS_MESH_SHADER
    m_MeshletBuffer = GLBuffer::New();
    m_MeshletVertexBuffer = GLBuffer::New();
    m_MeshletTriangleBuffer = GLBuffer::New();

    m_MeshletDescriptorBuffer = GLBuffer::New();
    m_InvisibleMeshletBuffer = GLBuffer::New();

    m_PositionBuffer = GLBuffer::New();
    m_ColorBuffer = GLBuffer::New();
    m_NormalBuffer = GLBuffer::New();
    m_UVBuffer = GLBuffer::New();
#else
    m_TriangleVAO = GLVertexArray::New();
    m_TriangleEBO = GLBuffer::New();

    m_PositionVBO = GLBuffer::New();
    m_ColorVBO = GLBuffer::New();
    m_NormalVBO = GLBuffer::New();
    m_UVVBO = GLBuffer::New();

    m_MeshletDescriptorBuffer = GLBuffer::New();
    m_DrawCommandBuffer = GLBuffer::New();
    m_VisibleMeshletBuffer = GLBuffer::New();
    m_FinalDrawCommandBuffer = GLBuffer::New();
#endif
}

void Meshleter::Build() {}

IGAME_NAMESPACE_END
