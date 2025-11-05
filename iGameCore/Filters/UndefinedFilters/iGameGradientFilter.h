
#ifndef GradientFilter_h
#define GradientFilter_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <cmath>


IGAME_NAMESPACE_BEGIN
class GradientFilter : public Filter {
public:
    I_OBJECT(GradientFilter);
    static Pointer New() { return new GradientFilter; }


    struct Gradient {
        float gx, gy, gz;
    };

    struct VectorGrad {
        Gradient x, y, z;
    };

    bool Execute() override;

    float ComputeCellVolume(Cell* cell);

    // 三角形和四边形和多边形
    std::array<float, 3> ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);
    std::array<float, 3> ComputePointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    std::array<float, 3> ComputeCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    bool ComputeVorticityWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeGradientWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeGradientWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeGradientWithVolumeMesh2(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);

    VectorGrad ComputeVectorGradByPlane(Cell* cell, ArrayObject* data);
    Vector3f ComputeCenter(Cell* cell);

    // =========================== 表面网格 Point ===========================
    bool ComputePointVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 表面网格 Cell ===========================
    bool ComputeCellVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 体网格 Point ===========================
    bool ComputePointVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 体网格 Cell  ===========================
    bool ComputeCellVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex);

    std::array<float, 3> ComputePointGradientWithSurfaceMesh(Cell* cell, ArrayObject::Pointer data, int dim);
    std::array<float, 3> ComputePointGradientWithVolumeMesh(Cell* cell, ArrayObject::Pointer data, int dim);

    // 三角形
    std::array<float, 3> ComputeTriPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 四边形
    std::array<float, 3> ComputeQuadPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 多边形
    std::array<float, 3> ComputePolygonPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 四面体线性插值 point
    std::array<float, 3> ComputeTetPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 四面体线性插值 cell
    std::array<float, 3> ComputeTetCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    // 六面体中心差分
    std::array<float, 3> ComputeHexPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 六面体中心差分 cell
    std::array<float, 3> ComputeHexCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    // 多面体中心差分
    std::array<float, 3> ComputePolyPointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    // 多面体中心差分 cell
    std::array<float, 3> ComputePolyCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    ArrayObject::Pointer AttributeCell2Point(CellArray::Pointer Cell, ArrayObject::Pointer OriArray, size_t PointNum);

    float ComputeSurfaceArea(Cell* cell);
    float ComputeTriangleArea(Cell* cell);

    float ComputeVolumeAverageEdgeLength(Cell* cell);
    float ComputeAverageEdgeLength(Cell* cell);

    float ComputeTetVolume(Cell* cell);

    bool InverseMatrix4x4(const float in[4][4], float out[4][4]);

protected:
    GradientFilter();
    ~GradientFilter() override = default;

    VolumeMesh::Pointer volume_Mesh{};
    SurfaceMesh::Pointer surface_Mesh{};
    AttributeSet::Pointer attributeSet{};

    int curIndex{-1};
    int curDim{-1};
};

IGAME_NAMESPACE_END
#endif