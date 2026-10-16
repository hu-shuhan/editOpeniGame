#include "iGameExportEdgesFilter.h"

IGAME_NAMESPACE_BEGIN

// ------------------------------------------------------------------
// 构造函数：声明管道端口（1 输入 1 输出）
// ------------------------------------------------------------------
ExportEdgesFilter::ExportEdgesFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

// ------------------------------------------------------------------
// Execute：执行导出
// ------------------------------------------------------------------
bool ExportEdgesFilter::Execute() {
    UpdateProgress(0);

    // 取输入：要导出的网格（通常是 ExtractEdges 的边网格）
    // m_Inputs：Filter 基类的输入容器；GetNumberOfElements() 返回已喂入的数据个数
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }
    auto input = m_Inputs->GetElement(0);  // GetElement(0)：取第 0 个输入数据对象
    if (!input) { return false; }

    // 路径检查：没设置保存路径就报错退出
    if (m_FilePath.empty()) {
        // IGAME_CORE_ERROR：框架的日志宏，带 [iGameVis_Core][error] 前缀输出到控制台
        IGAME_CORE_ERROR("ExportEdgesFilter: file path is empty!");
        return false;
    }

    // 【核心一行】组合复用框架现成的 VTKWriter。
    // VTKWriter::New()：工厂创建写入器实例（VTKWriter 是 FileWriter 的子类，
    //                   而 FileWriter 本身又是 Filter，所以 Writer 也是管道单元）
    // WriteToFile(dataObject, filePath)：一步完成"写文件"——
    //   内部封装：SetInput(dataObject) → SetFilePath(filePath) → Execute()（生成缓冲区并写盘）
    // 它对 UnstructuredMesh 的 LINE 单元原生支持（VTK 格式中 LINE 是标准类型），
    // 所以边网格可以直接导出，无需任何额外处理。
    auto writer = VTKWriter::New();
    if (!writer->WriteToFile(input, m_FilePath)) {
        IGAME_CORE_ERROR("ExportEdgesFilter: failed to write file: " + m_FilePath);
        return false;
    }

    // 导出是副作用操作：数据没变，输入原样透传为输出（保持管道可继续串联）
    SetOutput(0, input);
    UpdateProgress(1);
    return true;
}

IGAME_NAMESPACE_END
