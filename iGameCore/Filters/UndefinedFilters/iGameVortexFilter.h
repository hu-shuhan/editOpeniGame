#ifndef VortexFilter_h
#define VortexFilter_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <cmath>


IGAME_NAMESPACE_BEGIN
class VortexFilter : public Filter {
public:
    I_OBJECT(VortexFilter);
    static Pointer New() { return new VortexFilter; }

    bool Execute() override;

    float ComputeCellVolume(Cell* cell);

    // 三角形和四边形和多边形
    std::array<float, 3> ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    std::array<float, 3> ComputeCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    // =========================== 表面网格 Point ===========================
    bool ComputePointVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 表面网格 Cell ===========================
    bool ComputeCellVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 体网格 Point ===========================
    bool ComputePointVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 体网格 Cell  ===========================
    bool ComputeCellVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex);

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

    float ComputeTriangleArea(Cell* cell);

    float ComputeAverageEdgeLength(Cell* cell);

    float ComputeTetVolume(Cell* cell);

    bool InverseMatrix4x4(const float in[4][4], float out[4][4]);

protected:
    VortexFilter();
    ~VortexFilter() override = default;

    VolumeMesh::Pointer volume_Mesh{};
    SurfaceMesh::Pointer surface_Mesh{};
    AttributeSet* attributeSet{nullptr};

    int curIndex{-1};
    int curDim{-1};
};

IGAME_NAMESPACE_END
#endif