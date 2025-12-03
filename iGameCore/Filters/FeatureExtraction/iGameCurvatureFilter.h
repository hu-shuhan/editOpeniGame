#ifndef CurvatureFilter_h
#define CurvatureFilter_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include <cmath>
#include <unordered_set>
#include <algorithm>

IGAME_NAMESPACE_BEGIN

class CurvatureFilter : public Filter {
public:
    I_OBJECT(CurvatureFilter);
    static Pointer New() { return new CurvatureFilter; }

    void SetAttributeByIndex(int index) { curIndex = index; }
    void SetAttributeByName(const std::string& name) {  this->name = name; }

    bool Execute() override;

private:
    static auto constexpr PI = 3.14159265358979323846;
    bool ComputeSurfaceCurvatureCotangent(SurfaceMesh::Pointer surface_Mesh, AttributeSet::Pointer Attributes,
                                          int Index);

    double GetArea(Vector3d a, Vector3d b, Vector3d c);

    SurfaceMesh::Pointer TriangulateSurfaceMesh(SurfaceMesh::Pointer mesh);

    bool GetPointCurvature(int type, Points::Pointer Points, int PointNum);

    std::array<float, 3> GetPosition_volume(Volume* v, int num);
    std::array<float, 3> GetPosition_face(Face* f, int num);

    bool GetOtherCurvature(int type, int Num);

    std::vector<std::array<float, 3>> GetPointGradient(int type, Points::Pointer Points, int PointNum);

    std::vector<std::array<float, 3>> GetOtherGradient(int type, int Num);

protected:
    CurvatureFilter()
    {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~CurvatureFilter() override = default;

    SurfaceMesh::Pointer surface_Mesh{};
    VolumeMesh::Pointer volume_Mesh{};
    AttributeSet* attributeSet{nullptr};

    int curIndex{-1};
    std::string name;
};

IGAME_NAMESPACE_END
#endif