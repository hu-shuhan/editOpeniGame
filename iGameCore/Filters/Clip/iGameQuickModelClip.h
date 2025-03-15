/**
* @class    iGameQuickModelClip
* @brief   基于打表的模型切割算法
* 注意能够打表的cell只有 IG_TETRA，IG_PYRAMID，IG_PRISM，IG_HEXAHEDRO，IG_TRIANGLE，
* IG_QUAD，IG_LINE，IG_VERTEX，其余cell类型的网格仍旧采用ModelClip算法
* 打表的具体方式参考VTK开源库 vtkTableBasedClipCases.cxx
 */
#ifndef iGameQuickModelClip_h
#define iGameQuickModelClip_h

#include "iGameModelClip.h"


IGAME_NAMESPACE_BEGIN
class QuickModelClip : public ModelClip {

public:
    I_OBJECT(QuickModelClip);
    static Pointer New() { return new QuickModelClip; }
    ~QuickModelClip();


protected:
    QuickModelClip();


    ///@{ 重写具体的特定网格的执行算法
    bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um) override;
    bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm) override;
    bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm) override;
    ///@}

private:
};
IGAME_NAMESPACE_END
#endif