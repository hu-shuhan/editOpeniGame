
/**
 * @class   iGameClipFilter
 * @brief   最上层的切割算法接口，需要输入切割的网格和切割方式以及具体信息
 * 会选择不同的切割接口进行切割算法的实现，比如最基础的ModelClip算法以及较快的QuickModelClip算法
 * 使用自定义的切割算法时，需要用户自己维护正确性
 */
#ifndef iGameClipFilter_h
#define iGameClipFilter_h

#include "iGameQuickModelClip.h"
IGAME_NAMESPACE_BEGIN
class ClipFilter : public Filter {

public:
    I_OBJECT(ClipFilter);
    static Pointer New() { return new ClipFilter; }
    ~ClipFilter();

    bool Execute() override;
    //Returns the converted output mesh, resulting in an unstructured mesh
    UnstructuredMesh::Pointer GetClipMesh() { return DynamicCast<UnstructuredMesh>(m_Clipper->GetOutput()); };

    //设置切割模式
    void SetClipMethod(ModelClip::ClipMethod CM) { this->m_Clipper->SetClipMethod(CM); };
    ///@{ 设置或得到切割平面信息
    void SetPlane(float o[3], float n[3]) { this->m_Clipper->SetPlane(o, n); };
    void SetPlane(double o[3], double n[3]) { this->m_Clipper->SetPlane(o, n); };
    void GetPlane(float o[3], float n[3]) { this->m_Clipper->GetPlane(o, n); };
    void GetPlane(double o[3], double n[3]) { this->m_Clipper->GetPlane(o, n); };
    ///@}
    void SetInvert(bool _in) { this->m_Clipper->SetInvert(_in); };
    void SetCrinkle(bool _cr) { this->m_Clipper->SetCrinkle(_cr); };

    //设置clipper，如果用户有自定义的切割算法， 可以使用这个设置
    void SetClipper(ModelClip::Pointer CM) { this->m_Clipper = CM; };

    //返回clipper
    ModelClip::Pointer GetClipper() { return this->m_Clipper; };

protected:
    ClipFilter();


private:
    ModelClip::Pointer m_Clipper{nullptr};
};
IGAME_NAMESPACE_END
#endif