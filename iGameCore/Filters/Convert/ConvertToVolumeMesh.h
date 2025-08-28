/**
* @class    ConvertToVolumeMesh
 * @brief   将各种类型的网格转换为体网格的过滤器
 * 支持从表面网格、非结构化网格等转换为体网格
 * 继承后需要重写Execute()函数或者针对于特定网格的执行算法
 */
#ifndef ConvertToVolumeMesh_h
#define ConvertToVolumeMesh_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameSurfaceMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToVolumeMesh : public Filter {

public:
    I_OBJECT(ConvertToVolumeMesh);
    static Pointer New() { return new ConvertToVolumeMesh; }
    ~ConvertToVolumeMesh();

    bool Execute() override;
    //返回转换后的输出网格，结果为体网格
    VolumeMesh::Pointer GetVolumeMesh() { return DynamicCast<VolumeMesh>(this->GetOutput()); };

    //转换模式
    enum ConvertMethod {
        IG_CONVERT_VOLUME_MESH,        //转化为体网格，需要原始模型就是体网格模型，比如纯体的非结构化网格
        IG_EXTRACT_VOLUME_CELL,     //提取体单元，专门用于非结构化网格，用于获取非结构化网格中的体单元
    };
    //设置转换模式
    void SetConvertMethod(ConvertMethod CM);
    //获取转换模式
    ConvertMethod GetConvertMethod() const { return m_ConvertMethod; }


protected:
    ConvertToVolumeMesh();

    ConvertMethod m_ConvertMethod = IG_CONVERT_VOLUME_MESH;

    /**
     * 对非结构化网格进行转换
     */
    virtual bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
    
    /**
     * 对表面网格进行转换
     */
    virtual bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm);
    


private:
    //私有辅助函数可以在这里添加
};
IGAME_NAMESPACE_END
#endif
