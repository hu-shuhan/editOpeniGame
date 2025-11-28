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


    struct Gradient {
        float gx, gy, gz;
    };
    
    struct VectorGrad {
        Gradient x, y, z;
    };

    void SetAttributeByIndex(int index) { curIndex = index; }
    void SetAttributeByName(const std::string& name) { this->name = name; }

    bool Execute() override;

private:
    float ComputeCellVolume(Cell* cell);

    // 三角形和四边形和多边形
    std::array<float, 3> ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);
    std::array<float, 3> ComputePointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    std::array<float, 3> ComputeCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    bool ComputeVorticityWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeVorticityWithSurfaceMesh2(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeVorticityWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeVorticityWithUnstructuredMesh(UnstructuredMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeVorticityWithVolumeMesh2(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);

    VectorGrad ComputeVectorGradByPlane(Cell* cell, ArrayObject* data);
    Vector3f ComputeCenter(Cell* cell);

    VectorGrad ComputeVectorGradByTetra(Cell* cell, ArrayObject* data);

    void InterpolationDerivs(float derivs[12]) {
        // r-derivatives
        derivs[0] = -1.0;
        derivs[1] = 1.0;
        derivs[2] = 0.0;
        derivs[3] = 0.0;

        // s-derivatives
        derivs[4] = -1.0;
        derivs[5] = 0.0;
        derivs[6] = 1.0;
        derivs[7] = 0.0;

        // t-derivatives
        derivs[8] = -1.0;
        derivs[9] = 0.0;
        derivs[10] = 0.0;
        derivs[11] = 1.0;
    }

    // =========================== 表面网格 Point ===========================
    bool ComputePointVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 表面网格 Cell ===========================
    bool ComputeCellVorticityForMesh(SurfaceMesh::Pointer surface_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 体网格 Point ===========================
    bool ComputePointVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex);

    // =========================== 体网格 Cell  ===========================
    bool ComputeCellVorticityForVol(VolumeMesh::Pointer volume_Mesh, AttributeSet* attributeSet, int curIndex);
    VectorGrad ComputeVectorGradByHex(Cell* cell, ArrayObject* data);
    VectorGrad ComputeVectorGradByPolyhedron(Cell* cell, ArrayObject* data);

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


    //void quadricByAttributes(Quadric& Q, QuadricGrad* G, const Vector3f& p0, const Vector3f& p1, const Vector3f& p2,
    //                         const float* va0, const float* va1, const float* va2, size_t attribute_count) {

    //    // 我们使用下面这个线性插值函数计算新位置 pos 处的属性值
    //    //      eval(pos) = pos.x * gx + pos.y * gy + pos.z * gz + gw
    //    // 其中，gx/gy/gz 是属性梯度，gw是基准常数值
    //    // 使用插值处的属性值与真实值的差的平方作为属性误差
    //    //      Δ(pos) = (eval(pos) - attr)^2

    //    Vector3f p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
    //    Vector3f p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

    //    Vector3f normal = cross(p10, p20);
    //    float area = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z) * 0.5f;

    //    // quadric 使用三角形面积进行加权
    //    float w = area;

    //    // 我们使用重心坐标计算梯度，重心坐标的计算方法如下：
    //    // v = (d11 * d20 - d01 * d21) / denom
    //    // w = (d00 * d21 - d01 * d20) / denom
    //    // u = 1 - v - w
    //    // here v0, v1 are triangle edge vectors, v2 is a vector from point to triangle corner, and dij = dot(vi, vj)
    //    // note: v2 and d20/d21 can not be evaluated here as v2 is effectively an unknown variable; we need these only as variables for derivation of gradients
    //    const Vector3f& v0 = p10;
    //    const Vector3f& v1 = p20;
    //    float d00 = v0.x * v0.x + v0.y * v0.y + v0.z * v0.z;
    //    float d01 = v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
    //    float d11 = v1.x * v1.x + v1.y * v1.y + v1.z * v1.z;
    //    float denom = d00 * d11 - d01 * d01;
    //    float denomr = denom == 0 ? 0.f : 1.f / denom;

    //    // precompute gradient factors
    //    // these are derived by directly computing derivative of eval(pos) = a0 * u + a1 * v + a2 * w and factoring out expressions that are shared between attributes
    //    float gx1 = (d11 * v0.x - d01 * v1.x) * denomr;
    //    float gx2 = (d00 * v1.x - d01 * v0.x) * denomr;
    //    float gy1 = (d11 * v0.y - d01 * v1.y) * denomr;
    //    float gy2 = (d00 * v1.y - d01 * v0.y) * denomr;
    //    float gz1 = (d11 * v0.z - d01 * v1.z) * denomr;
    //    float gz2 = (d00 * v1.z - d01 * v0.z) * denomr;

    //    memset(&Q, 0, sizeof(Quadric));

    //    Q.w = w;

    //    for (size_t k = 0; k < attribute_count; ++k) {
    //        float a0 = va0[k], a1 = va1[k], a2 = va2[k];

    //        // compute gradient of eval(pos) for x/y/z/w
    //        // the formulas below are obtained by directly computing derivative of eval(pos) = a0 * u + a1 * v + a2 * w
    //        float gx = gx1 * (a1 - a0) + gx2 * (a2 - a0);
    //        float gy = gy1 * (a1 - a0) + gy2 * (a2 - a0);
    //        float gz = gz1 * (a1 - a0) + gz2 * (a2 - a0);
    //        float gw = a0 - p0.x * gx - p0.y * gy - p0.z * gz;

    //        // quadric encodes (eval(pos)-attr)^2; this means that the resulting expansion needs to compute, for example, pos.x * pos.y * K
    //        // since quadrics already encode factors for pos.x * pos.y, we can accumulate almost everything in basic quadric fields
    //        // note: for simplicity we scale all factors by weight here instead of outside the loop
    //        Q.a00 += w * (gx * gx);
    //        Q.a11 += w * (gy * gy);
    //        Q.a22 += w * (gz * gz);

    //        Q.a10 += w * (gy * gx);
    //        Q.a20 += w * (gz * gx);
    //        Q.a21 += w * (gz * gy);

    //        Q.b0 += w * (gx * gw);
    //        Q.b1 += w * (gy * gw);
    //        Q.b2 += w * (gz * gw);

    //        Q.c += w * (gw * gw);

    //        // the only remaining sum components are ones that depend on attr; these will be addded during error evaluation, see quadricError
    //        G[k].gx = w * gx;
    //        G[k].gy = w * gy;
    //        G[k].gz = w * gz;
    //        G[k].gw = w * gw;
    //    }
    //}

protected:
    VortexFilter();
    ~VortexFilter() override = default;

    VolumeMesh::Pointer volume_Mesh{};
    SurfaceMesh::Pointer surface_Mesh{};
    AttributeSet::Pointer attributeSet{};

    int curIndex{-1};
    std::string name;
};

IGAME_NAMESPACE_END
#endif