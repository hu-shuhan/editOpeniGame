/**
* @class    ConvertToLagrangeUnstructuredMesh
 * @brief   将各种类型的网格转换为拉格朗日非结构网格的过滤器
 * 支持从体网格、非结构化网格等转换为表面网格
 * 继承后需要重写Execute()函数或者针对于特定网格的执行算法
 */
#ifndef ConvertToLagrangeUnstructuredMesh_h
#define ConvertToLagrangeUnstructuredMesh_h

#include "iGameFilter.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToLagrangeUnstructuredMeshFilter : public Filter {

public:
    I_OBJECT(ConvertToLagrangeUnstructuredMeshFilter);
    static Pointer New() { return new ConvertToLagrangeUnstructuredMeshFilter; }
    ~ConvertToLagrangeUnstructuredMeshFilter();

    bool Execute() override;
    //返回转换后的输出网格，结果为表面网格
    LagrangeUnstructuredMesh::Pointer GetLagrangeUnstructuredMesh() {
        return DynamicCast<LagrangeUnstructuredMesh>(this->GetOutput());
    };

    //转换模式
    enum ConvertMethod {
        IG_CONVERT_LAGRANGE_UNSTRUCTURED_MESH, // 非结构网格转化成拉格朗日高阶网格
    };
    //设置转换模式
    void SetConvertMethod(ConvertMethod CM);
    //获取转换模式
    ConvertMethod GetConvertMethod() const { return m_ConvertMethod; }


protected:
    ConvertToLagrangeUnstructuredMeshFilter();

    ConvertMethod m_ConvertMethod = IG_CONVERT_LAGRANGE_UNSTRUCTURED_MESH;

    /**
     * 对非结构化网格进行转换
     */
    virtual bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);


    /**
    * 网格阶数计算
    */
    virtual int GetOrderFromNodes_Quad(int num_nodes);
    virtual int GetOrderFromNodes_Hex(int num_nodes);
    virtual int GetOrderFromNodes_Prism(int num_nodes);
    virtual int GetOrderFromNodes_Pyramid(int num_nodes);
    virtual int GetOrderFromNodes_Tri(int num_nodes);
    virtual int GetOrderFromNodes_Tetra(int num_nodes);

private:
};
IGAME_NAMESPACE_END
#endif
