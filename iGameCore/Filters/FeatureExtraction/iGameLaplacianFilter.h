#ifndef LaplacianFilter_h
#define LaplacianFilter_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include <cmath>


IGAME_NAMESPACE_BEGIN

class LaplacianFilter : public Filter {
public:
    I_OBJECT(LaplacianFilter);
    static Pointer New() { return new LaplacianFilter; }

    void SetAttributeByIndex(int index);
    void SetAttributeByName(const std::string& name);

    bool Execute() override;
    
private:
    
    bool GetPointLaplacian(int type, Points::Pointer points, int pointNum, SurfaceMesh::Pointer surface_Mesh);

    std::array<float, 3> GetPosition_volume(Volume* v, int num);
    std::array<float, 3> GetPosition_face(Face* f, int num);

    bool GetOtherLaplacian(int type, int Num);

protected:
    LaplacianFilter();
    ~LaplacianFilter() override = default;

    SurfaceMesh::Pointer surface_Mesh{};
    VolumeMesh::Pointer volume_Mesh{};
    AttributeSet* attributeSet{nullptr};

    int curIndex{-1};
    std::string name;
};

IGAME_NAMESPACE_END
#endif
