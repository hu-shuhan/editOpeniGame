/**
 * @class    iGameIsoVolumeFilter
 * @brief    等值面之间的体提取
 * 根据标量场的范围 [Lower, Upper] 提取体网格中数值在该区间内的部分。
 * 完全在内的单元直接保留；与边界相交的单元会被裁剪，生成新的非结构化网格。
 */

#ifndef iGameIsoVolumeFilter_h
#define iGameIsoVolumeFilter_h

#include "../Clip/iGameCellClip.h"
#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class IsoVolumeFilter : public Filter {
public:
    I_OBJECT(IsoVolumeFilter);
    static Pointer New() { return new IsoVolumeFilter; }
    ~IsoVolumeFilter();

    bool Execute() override;

    // 返回提取后的非结构化网格
    UnstructuredMesh::Pointer GetOutputMesh() { return DynamicCast<UnstructuredMesh>(this->GetOutput()); }

    // 设置用于区间判断的标量数组以及 [lower, upper] 范围
    // dimension 表示使用数组的第几个分量（默认 0）
    void SetIsoScalarData(ArrayObject::Pointer array, double lower, double upper, int dimension = 0);

protected:
    IsoVolumeFilter();

    ArrayObject::Pointer m_SelectedScalar{nullptr};
    std::string m_SelectedScalarName;
    double m_LowerValue{0.0};
    double m_UpperValue{0.0};
    double m_SelectDimension{0.0};

    bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
    bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm);
    bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm);
    bool ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm);

    // 计算每个顶点相对于给定等值面的带符号距离，以及每个 cell 的在内/在外/相交状态
    void ComputePointValueAndCellVisible(Points::Pointer inPoints, CellArray::Pointer inCells,
                                         DoubleArray::Pointer PointIsoArray, CharArray::Pointer CellVisible,
                                         ArrayObject::Pointer scalarArray, double isoValue, bool keepAbove);

    // 对输入网格按单个标量阈值进行裁剪
    // keepAbove = true 保留 scalar >= isoValue 的部分
    // keepAbove = false 保留 scalar <= isoValue 的部分
    bool ClipMeshByScalar(UnstructuredMesh::Pointer input, ArrayObject::Pointer scalarArray, double isoValue,
                          bool keepAbove, UnstructuredMesh::Pointer output);

    // 复制属性数据，对裁剪产生的新点进行插值
    void CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData,
                              AttributeSet::Pointer outData, std::vector<CellClip::InterpolateEdge> OriginEdge,
                              std::vector<igIndex> OriginCell);
};

IGAME_NAMESPACE_END
#endif
