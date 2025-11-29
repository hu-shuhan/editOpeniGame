#ifndef iGameConvertToPointCloud_h
#define iGameConvertToPointCloud_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToPointCloudFilter : public Filter {
public:
    I_OBJECT(ConvertToPointCloudFilter);
    static Pointer New() { return new ConvertToPointCloudFilter; }

    bool Execute() override;

    static bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer OldMesh, PointSet::Pointer NewMesh);

    static bool ExecuteWithVolumeMesh(VolumeMesh::Pointer OldMesh, PointSet::Pointer NewMesh);

    static bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer OldMesh, PointSet::Pointer NewMesh);

protected:
    ConvertToPointCloudFilter();
    ~ConvertToPointCloudFilter() override = default;
};
IGAME_NAMESPACE_END
#endif
