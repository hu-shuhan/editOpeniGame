/**
* @class    ConvertToSurfaceMesh
 * @brief   将各种类型的网格转换为表面网格的过滤器
 * 支持从体网格、非结构化网格等转换为表面网格
 * 继承后需要重写Execute()函数或者针对于特定网格的执行算法
 */
#ifndef ConvertToSurfaceMesh_h
#define ConvertToSurfaceMesh_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameSurfaceMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToSurfaceMesh : public Filter {

public:
    I_OBJECT(ConvertToSurfaceMesh);
    static Pointer New() { return new ConvertToSurfaceMesh; }
    ~ConvertToSurfaceMesh();

    bool Execute() override;
    //返回转换后的输出网格，结果为表面网格
    SurfaceMesh::Pointer GetSurfaceMesh() { return DynamicCast<SurfaceMesh>(this->GetOutput()); };

    //转换模式
    enum ConvertMethod {
        IG_CONVERT_SURFACE_MESH, //转化为表面，需要原始模型就是表面模型，比如纯表面的非结构化网格
        IG_EXTRACT_SURFACE_CELL,     //提取表面单元，专门用于非结构化网格，用于获取非结构化网格中的表面单元
        IG_EXTRACT_SURFACE_MESH,         //提取模型的表面网格，抽壳
    };
    //设置转换模式
    void SetConvertMethod(ConvertMethod CM);
    //获取转换模式
    ConvertMethod GetConvertMethod() const { return m_ConvertMethod; }


protected:
    ConvertToSurfaceMesh();

    ConvertMethod m_ConvertMethod = IG_CONVERT_SURFACE_MESH;

    /**
     * 对非结构化网格进行转换
     */
    virtual bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
    
    /**
     * 对体网格进行转换
     */
    virtual bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm);
    


private:
};
IGAME_NAMESPACE_END
#endif