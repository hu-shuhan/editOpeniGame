#ifndef iGameMeshTriangulationFilter_h
#define iGameMeshTriangulationFilter_h

#include "iGameFilter.h"
#include "iGameModel.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class MeshTriangulationFilter : public Filter {
public:
    I_OBJECT(MeshTriangulationFilter);
    static Pointer New() { return new MeshTriangulationFilter; }

    bool Execute() override;

protected:
    MeshTriangulationFilter();
    ~MeshTriangulationFilter() override = default;

    double GetArea(Vector3d a, Vector3d b, Vector3d c);

    SurfaceMesh::Pointer mesh{};
};
IGAME_NAMESPACE_END
#endif