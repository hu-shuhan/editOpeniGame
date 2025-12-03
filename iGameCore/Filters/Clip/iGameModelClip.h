/**
* @class    iGameModelClip
 * @brief   最基本的模型切割算法，是所有模型切割的基类，其余的模型切割都需要继承该类
 * 继承后需要重写Execute()函数或者针对于特定网格的执行算法（该基类的Execute函数会判断网格的类型并采用对应的执行接口）
 * 会遍历所有的cell，针对于不同的cell进行切割
 */
#ifndef iGameModelClip_h
#define iGameModelClip_h

#include "iGameCellClip.h"
#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class ModelClip : public Filter {

public:
    I_OBJECT(ModelClip);
    static Pointer New() { return new ModelClip; }
    ~ModelClip();

    bool Execute() override;
    //Returns the converted output mesh, resulting in an unstructured mesh
    UnstructuredMesh::Pointer GetClipMesh() { return DynamicCast<UnstructuredMesh>(this->GetOutput()); };

    //切割模式
    enum ClipMethod {
        IG_PLANE,
        IG_BOX,
    };
    //设置切割模式
    void SetClipMethod(ClipMethod CM);
    ///@{ 设置或得到切割平面信息
    void SetPlane(float o[3], float n[3]);
    void SetPlane(double o[3], double n[3]);
    void GetPlane(float o[3], float n[3]);
    void GetPlane(double o[3], double n[3]);
    ///@}

    ///@{ 设置或获取是否启用翻转模式
    void SetInvert(bool _in);
    bool GetInvert() const { return m_Invert; }
    ///@}


    ///@{ 设置或获取是否启用锯齿模式（保留原始网格切割，不补全平面）
    void SetCrinkle(bool crinkle);
    bool GetCrinkle() const { return m_Crinkle; }
    ///@}

protected:
    ModelClip();

    ClipMethod m_ClipMethod = IG_PLANE;
    double m_CutPlane[4];
    double m_Normal[3];
    double m_Origin[3];


    bool m_Invert = true;
    bool m_Crinkle = false;  // 是否启用锯齿模式（保留原始网格切割，不补全平面）

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

    //对非结构化网格进行切割
    virtual bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
    //对普通体网格进行切割
    virtual bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm);
    //对多面体网格进行切割
    virtual bool ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm);
    //对表面网格进行切割
    virtual bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm);

private:
    ///@{ 根据坐标获得与切割面的带符号距离
    double GetCutValue(float x[3]);
    double GetCutValue(double x[3]);
    double GetCutValue(float x0, float x1, float x2);
    double GetCutValue(double x0, double x1, double x2);
    double GetCutValue(Point x);
    double GetPointValue(igIndex pId, Points::Pointer points);
    ///@}
};
IGAME_NAMESPACE_END
#endif