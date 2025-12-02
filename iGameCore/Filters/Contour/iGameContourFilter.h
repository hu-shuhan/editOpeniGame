/**
* @class    iGameContourFilter
 * @brief   模型轮廓提取算法，可以继承重写
 * 继承后需要重写Execute()函数或者针对于特定网格的执行算法（该基类的Execute函数会判断网格的类型并采用对应的执行接口）
 * 会遍历所有的cell，针对于不同的cell进行轮廓提取
 */

#ifndef iGameContourFilter_h
#define iGameContourFilter_h

#include "iGameCellContour.h"
#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include <vector>
IGAME_NAMESPACE_BEGIN

class ContourFilter : public Filter {

public:
    I_OBJECT(ContourFilter);
    static Pointer New() { return new ContourFilter; }
    ~ContourFilter();

    bool Execute() override;

    //Returns the converted output mesh, resulting in an unstructured mesh
    UnstructuredMesh::Pointer GetContourMesh() { return DynamicCast<UnstructuredMesh>(this->GetOutput()); };

    // To set equivalent data, you need to enter the corresponding equivalent data and data objects
    // the dimension means extract the dimension of datas, dimension = 0 means the first dimension
    void SetIsoScalarData(ArrayObject::Pointer array, double value, int dimension = 0);
    // Overloaded version to support multiple contour values
    void SetIsoScalarData(ArrayObject::Pointer array, const std::vector<double>& values, int dimension = 0);

protected:
    ContourFilter();



    DoubleArray::Pointer m_PointValues{nullptr};
    ArrayObject::Pointer m_SelectedScalar{nullptr};
    double m_SeletectDimension{0.0};
    double m_ContourValue{0.0};
    std::vector<double> m_ContourValues;  // Store multiple contour values

    //对非结构化网格进行轮廓提取
    virtual bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
    //对普通体网格进行轮廓提取
    virtual bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm);
    //对多面体网格进行轮廓提取
    virtual bool ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm);
    //对表面网格进行轮廓提取
    virtual bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm);
    /**
    * 计算顶点的value值和cell的状态，顶点的values值为点到切割面的带符号距离，
    * cell的状态总共有三种，分别为0：被切割，1：在内部，2：在外部
    * 需要输入points数据和cells数据
    */
    void ComputePointValueAndCellVisible(Points::Pointer, CellArray::Pointer, DoubleArray::Pointer, CharArray::Pointer);
    /**
     * copy数据场，包括顶点数据场以及cell数据场
     * @param outPointNum 顶点数量
     * @param outCellNum  cell数量
     * @param inData 原始场数据
     * @param outData 输出场数据
     * @param OriginEdge 输出点的来源edge的顶点信息，如果是原始点，则edge的另一个点是-1
     * @param OriginCell 输出cell的原始cell
     */
    void CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData,
                              AttributeSet::Pointer outData, std::vector<CellContour::InterpolateEdge> OriginEdge,
                              std::vector<igIndex> OriginCell);

private:
};
IGAME_NAMESPACE_END
#endif