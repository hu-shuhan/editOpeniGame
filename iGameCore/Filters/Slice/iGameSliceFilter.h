
/**
 * @class   iGameSliceFilter
 * @brief   最上层的切割算法接口，需要输入切割的网格和切割方式以及具体信息
 * 会选择不同的切割接口进行切割算法的实现，比如最基础的ModelClip算法以及较快的QuickModelClip算法
 * 使用自定义的切割算法时，需要用户自己维护正确性
 */
#ifndef iGameSliceFilter_h
#define iGameSliceFilter_h

#include "Contour/iGameContourFilter.h"
IGAME_NAMESPACE_BEGIN
class SliceFilter : public Filter {
public:
    I_OBJECT(SliceFilter);
    static Pointer New() { return new SliceFilter; }
    ~SliceFilter();

    bool Execute() override;
    //Returns the slice output mesh, resulting in an unstructured mesh
    UnstructuredMesh::Pointer GetSliceMesh() { return DynamicCast<UnstructuredMesh>(m_Contourer->GetOutput()); };


    ///@{ 设置或得到切割平面信息
    void SetPlane(double o[3], double n[3]) { this->SetPlane(o[0], o[1], o[2], n[0], n[1], n[2]); };
    void SetPlane(float o[3], float n[3]) { this->SetPlane(o[0], o[1], o[2], n[0], n[1], n[2]); };
    void SetPlane(double ox, double oy, double oz, double nx, double ny, double nz);

    ///@}


protected:
    SliceFilter();

    iGame::ContourFilter::Pointer m_Contourer = nullptr;
    double m_PlaneOrigin[3];
    double m_PlaneNormal[3];


private:

};
IGAME_NAMESPACE_END
#endif