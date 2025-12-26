#include "iGameBuildAdjacencyRelationFilter.h"
IGAME_NAMESPACE_BEGIN

bool BuildAdjacencyRelationFilter::Execute() {
    m_Mesh = GetInput(0);
    if (m_Mesh.IsNull()) return false;
    auto meshType = m_Mesh->GetDataObjectType();
    switch (meshType) {
        case IG_SURFACE_MESH: {
            auto mesh = DynamicCast<SurfaceMesh>(m_Mesh);
            Run(mesh);
        } break;
        case IG_VOLUME_MESH: {
            auto mesh = DynamicCast<VolumeMesh>(m_Mesh);
            Run(mesh);
        } break;
        case IG_UNSTRUCTURED_MESH: {
            return false;
        } break;
        case IG_STRUCTURED_MESH: {
            auto mesh = DynamicCast<StructuredMesh>(m_Mesh);
            Run(mesh);
        } break;
    }
    SetOutput(0, m_Mesh);
    return true;
}

void BuildAdjacencyRelationFilter::Run(UnstructuredMesh::Pointer mesh) { return; }
void BuildAdjacencyRelationFilter::Run(SurfaceMesh::Pointer mesh) {
    if (mesh->InEditStatus()) { return; }
    this->UpdateProgress(0.2);
    mesh->RequestPointStatus();
    this->UpdateProgress(0.4);
    mesh->RequestEdgeStatus();
    this->UpdateProgress(0.6);
    mesh->RequestFaceStatus();
    this->UpdateProgress(0.8);
    mesh->MakeEditStatusOn();
    this->UpdateProgress(1.0);
}
void BuildAdjacencyRelationFilter::Run(VolumeMesh::Pointer mesh) {
    if (mesh->InEditStatus()) { return; }
    if (mesh->IsPolyhedronType) {
        this->UpdateProgress(0.2);
        mesh->InitPolyhedronVertices();
        this->UpdateProgress(0.8);
    } else {
        this->UpdateProgress(0.2);
        mesh->RequestPointStatus();
        this->UpdateProgress(0.4);
        mesh->RequestFaceStatus();
        this->UpdateProgress(0.6);
        mesh->RequestVolumeStatus();
        this->UpdateProgress(0.8);
    }
    mesh->MakeEditStatusOn();
    this->UpdateProgress(1.0);
}
void BuildAdjacencyRelationFilter::Run(StructuredMesh::Pointer mesh) {
    if (mesh->InEditStatus()) { return; }
    if (mesh->IsPolyhedronType) {
        this->UpdateProgress(0.2);
        mesh->InitPolyhedronVertices();
        this->UpdateProgress(0.8);
    } else {
        this->UpdateProgress(0.2);
        mesh->RequestPointStatus();
        this->UpdateProgress(0.4);
        mesh->RequestFaceStatus();
        this->UpdateProgress(0.6);
        mesh->RequestVolumeStatus();
        this->UpdateProgress(0.8);
    }
    mesh->MakeEditStatusOn();
    this->UpdateProgress(1.0);
}


BuildAdjacencyRelationFilter::BuildAdjacencyRelationFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

IGAME_NAMESPACE_END