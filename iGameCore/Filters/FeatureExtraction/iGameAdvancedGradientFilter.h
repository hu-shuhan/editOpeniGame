
#ifndef AdvancedGradientFilter_h
#define AdvancedGradientFilter_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <cmath>


IGAME_NAMESPACE_BEGIN
class AdvancedGradientFilter : public Filter {
public:
    I_OBJECT(AdvancedGradientFilter);
    static Pointer New() { return new AdvancedGradientFilter; }

    std::string GetMessage() const { return m_Message; }

    struct Gradient {
        float gx, gy, gz;
    };

    struct VectorGrad {
        Gradient x, y, z;
    };

    void SetAttributeByIndex(int index) { curIndex = index; }
    void SetAttributeByName(const std::string& name) { this->name = name; }
    void SetCurrentAttributeDimension(int dimension) { m_currentAttributeDimension = dimension; }
    // true = 输出点属性（IG_POINT）；false = 输出单元属性（IG_CELL，默认）
    void SetOutputToPointData(bool flag) { m_OutputToPointData = flag; }
    // true = 向量输入时输出 3×3 张量（9 分量）；false = 只输出选中分量的 3 分量梯度
    void SetComputeGradientTensor(bool flag) { m_ComputeGradientTensor = flag; }

    bool Execute() override;

 private:
    float ComputeCellVolume(Cell* cell);

    // 三角形和四边形和多边形
    std::array<float, 3> ComputePointGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);
    std::array<float, 3> ComputePointGradient(Cell* cell, ArrayObject::Pointer data, int dim);

    std::array<float, 3> ComputeCellGradient(int type, Cell* cell, ArrayObject::Pointer data, int dim);

    bool ComputeVorticityWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    std::array<float, 3> ComputeSurfaceCellGradient(Cell* cell, ArrayObject::Pointer data, int dim, int component);
    std::array<float, 3> ComputeSurfacePointGradient(Cell* cell, ArrayObject::Pointer data, int dim, int component, int localPointIndex);
    std::array<float, 3> ComputeQuadraticQuadGradient(Cell* cell, ArrayObject::Pointer data, int dim, int component, double xi, double eta);
    void AddGradientAttributeToSet(AttributeSet::Pointer attrs, IGenum attachmentType, FloatArray::Pointer arr, int outDim);
    bool ComputeGradientWithSurfaceMesh(SurfaceMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeGradientWithVolumeMesh(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeGradientWithMixedMesh(UnstructuredMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);
    bool ComputeGradientWithVolumeMesh2(VolumeMesh::Pointer Mesh, AttributeSet::Pointer Attributes, int Index);

    // =========================== 体网格 Cell 梯度 ===========================
    std::array<float, 3> ComputeVolumeCellGradient(Cell* cell, ArrayObject::Pointer data, int dim, int component);
    // 四面体：线性形函数精确梯度
    std::array<float, 3> ComputeTetCellGradientExact(Cell* cell, ArrayObject::Pointer data, int dim, int component);
    // 六面体：三线性形函数导数（在参数坐标 r,s,t 处）
    std::array<float, 3> ComputeHexGradient(Cell* cell, ArrayObject::Pointer data, int dim, int component, double r, double s, double t);
    // 体网格点梯度：在局部节点处求形函数导数（与 ParaView 对齐）
    std::array<float, 3> ComputeVolumePointGradient(Cell* cell, ArrayObject::Pointer data, int dim, int component, int localPointIndex);
    // 六面体/棱柱/棱锥/多面体：Green-Gauss 面积分近似（备用）
    std::array<float, 3> ComputeGreenGaussCellGradient(Cell* cell, ArrayObject::Pointer data, int dim, int component);
    // 六面体/棱柱/棱锥/多面体：最小二乘线性拟合梯度（推荐）
    std::array<float, 3> ComputeLeastSquaresCellGradient(Cell* cell, ArrayObject::Pointer data, int dim, int component);
    // Green-Gauss 辅助函数
    Vector3f ComputeFaceAreaVector(Cell* face);
    Vector3f ComputeFaceCenter(Cell* face);
    float ComputeVolumeByGreenGauss(Cell* cell);

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
    AdvancedGradientFilter();
    ~AdvancedGradientFilter() override = default;

    VolumeMesh::Pointer volume_Mesh{};
    SurfaceMesh::Pointer surface_Mesh{};
    AttributeSet::Pointer attributeSet{};
    int dim{-1};

    int curIndex{-1};
    int m_currentAttributeDimension{-1};
    bool m_OutputToPointData{false};
    bool m_ComputeGradientTensor{false};
    std::string name;

    std::string m_Message{"Not Supported Mesh !"};
};

IGAME_NAMESPACE_END
#endif