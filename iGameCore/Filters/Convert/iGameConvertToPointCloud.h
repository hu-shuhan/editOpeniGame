#ifndef iGameConvertToPointCloud_h
#define iGameConvertToPointCloud_h

#include "iGameFilter.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToPointCloud : public Filter {
public:
    I_OBJECT(ConvertToPointCloud);
    static Pointer New() { return new ConvertToPointCloud; }

	bool Execute() override;

	static bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer OldMesh, PointSet::Pointer NewMesh);

	static bool ExecuteWithVolumeMesh(VolumeMesh::Pointer OldMesh, PointSet::Pointer NewMesh);

    static bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer OldMesh, PointSet::Pointer NewMesh);

protected:
    ConvertToPointCloud();
    ~ConvertToPointCloud() override = default;
};
IGAME_NAMESPACE_END
#endif