#include "iGameMeshleter.h"
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
    if (m_DataObject && m_DataObject->GetMTime() > this->GetMTime()) {
        Build();
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
