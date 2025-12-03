
/**
 * @class   iGameSliceFilter
 * @brief   最上层的切割算法接口，需要输入切割的网格和切割方式以及具体信息
 * 会选择不同的切割接口进行切割算法的实现，比如最基础的ModelClip算法以及较快的QuickModelClip算法
 * 使用自定义的切割算法时，需要用户自己维护正确性
 */
#ifndef iGameSliceFilter_h
#define iGameSliceFilter_h

#include "Contour/iGameContourFilter.h"
#include "Clip/iGameModelClip.h"
IGAME_NAMESPACE_BEGIN
class SliceFilter : public Filter {
public:
    I_OBJECT(SliceFilter);
    static Pointer New() { return new SliceFilter; }
    ~SliceFilter();

    bool Execute() override;
    //Returns the slice output mesh, resulting in an unstructured mesh
    UnstructuredMesh::Pointer GetSliceMesh() { 
        return DynamicCast<UnstructuredMesh>(this->GetOutput()); 
    };


    ///@{ 设置或得到切割平面信息
    void SetPlane(double o[3], double n[3]) { this->SetPlane(o[0], o[1], o[2], n[0], n[1], n[2]); };
    void SetPlane(float o[3], float n[3]) { this->SetPlane(o[0], o[1], o[2], n[0], n[1], n[2]); };
    void SetPlane(double ox, double oy, double oz, double nx, double ny, double nz);

    ///@}

    ///@{ 设置或获取是否启用锯齿模式（保留原始网格切割，不补全平面）
    void SetCrinkle(bool crinkle);
    bool GetCrinkle() const { return m_Crinkle; }
    ///@}


protected:
    SliceFilter();

    iGame::ContourFilter::Pointer m_Contourer = nullptr;
    iGame::ModelClip::Pointer m_Clipper = nullptr;
    double m_PlaneOrigin[3];
    double m_PlaneNormal[3];
    bool m_Crinkle = false;  // 是否启用锯齿模式（保留原始网格切割，不补全平面）

    /**
     * crinkle 模式：只保留与平面相交的 cell
     */
    virtual bool ExecuteCrinkle(DataObject::Pointer input);
    /**
     * contour 模式：使用 ContourFilter 提取等值面
     */
    virtual bool ExecuteContour(DataObject::Pointer input);
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
                              AttributeSet::Pointer outData, std::vector<CellClip::InterpolateEdge> OriginEdge,
                              std::vector<igIndex> OriginCell);

private:
    ///@{ 根据坐标获得与切割面的带符号距离
    double GetPointValue(igIndex pId, Points::Pointer points);
    ///@}

};
IGAME_NAMESPACE_END
#endif