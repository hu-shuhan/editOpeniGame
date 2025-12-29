#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameUnstructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameVolumeMesh.h>
#include <iGameStructuredMesh.h>
IGAME_NAMESPACE_BEGIN
class BuildAdjacencyRelationFilter : public Filter {
public:
    I_OBJECT(BuildAdjacencyRelationFilter);
    static Pointer New() { return new BuildAdjacencyRelationFilter(); }
    bool Execute() override;

private:
    void Run(UnstructuredMesh::Pointer mesh);
    void Run(SurfaceMesh::Pointer mesh);
    void Run(VolumeMesh::Pointer mesh);
    void Run(StructuredMesh::Pointer mesh);

protected:
    BuildAdjacencyRelationFilter();
    ~BuildAdjacencyRelationFilter() override = default;

private:
    DataObject::Pointer m_Mesh;
};
IGAME_NAMESPACE_END