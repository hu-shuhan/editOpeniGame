#ifndef iGameExportEdgesFilter_h
#define iGameExportEdgesFilter_h

#include "iGameFilter.h"

// 框架现成的 VTK 写入器。
// 【注意】IO 子目录的头文件要带目录前缀，这是项目惯例：
// 编译器通过 -I iGameCore/IO 找头文件，所以写 "VTK/iGameVTKWriter.h"。
#include "VTK/iGameVTKWriter.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class ExportEdgesFilter
 * @brief 配套功能：把网格（通常是 ExtractEdges 提取出的边网格）导出为 .vtk 文件。
 *
 * 【设计思想：组合复用，不重复造轮子】
 *   框架本身就有现成的 VTKWriter（VTKWriter -> FileWriter -> Filter，
 *   Writer 在 iGame 里也是 Filter！），它支持写 UnstructuredMesh 的所有
 *   单元类型（包括 LINE）。所以我们**不需要自己写文件格式代码**，
 *   只需在 Execute 里调 writer->WriteToFile(...) 一行即可完成导出。
 *   这正是老师说的"纯组合实现"。
 *
 * 【典型用法】（与 GUI 面板的「导出边为 VTK」按钮对应）
 *   auto exporter = ExportEdgesFilter::New();
 *   exporter->SetInput(0, edgesMesh);       // 要导出的边网格
 *   exporter->SetFilePath("edges.vtk");     // 保存路径
 *   exporter->Execute();                    // 执行导出
 *
 * 【输出约定】
 *   导出是"副作用"操作（成果是磁盘上的文件），数据本身不变，
 *   所以输入网格原样透传为输出，方便继续串联下游 Filter。
 */
class ExportEdgesFilter : public Filter {

public:
    // 对象系统宏 + 工厂方法（iGame Filter 标准三件套）
    I_OBJECT(ExportEdgesFilter);
    static Pointer New() { return new ExportEdgesFilter; }

    // 管道核心：执行导出
    bool Execute() override;

    /// 设置导出文件路径（*.vtk）
    void SetFilePath(const std::string& filePath) { m_FilePath = filePath; }

    /// 获取当前导出文件路径（便于调试/检查）
    const std::string& GetFilePath() const { return m_FilePath; }

protected:
    ExportEdgesFilter();

    /// 目标文件路径（由调用方通过 SetFilePath 设置）
    std::string m_FilePath;
};

IGAME_NAMESPACE_END
#endif
