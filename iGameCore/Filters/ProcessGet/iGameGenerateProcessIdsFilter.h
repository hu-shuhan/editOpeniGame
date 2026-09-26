#pragma once
#ifndef EOIGAME_IGAMECORE_PROCESSGET_IGAMEGENERATEPROCESSIDSFILTER_H
#define EOIGAME_IGAMECORE_PROCESSGET_IGAMEGENERATEPROCESSIDSFILTER_H

#include <iGameFilter.h>

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class   GenerateProcessIdsFilter
 * @brief   为网格生成进程号数组（PointProcessIds / CellProcessIds）
 *
 * 语义对齐 VTK vtkGenerateProcessIds：所有点 / 单元的进程号都写当前进程号，不读取输入上
 * 已有的任何数组；点进程号默认生成，单元进程号默认不生成。
 *
 * 输出为新的数据对象：几何（点、单元）与输入共享，属性集为输入的拷贝加上新生成的进程号
 * 数组，输入对象本身不被修改；GUI 会把结果作为独立节点加入模型树。
 *
 * iGame 目前没有多进程 / 分区机制，进程号由 SetProcessId() 给出（默认 0）；派生类可重写
 * GetPointProcessId / GetCellProcessId 自定义分区策略（如按 index 交替分区）。
 */
class GenerateProcessIdsFilter : public Filter {
public:
    I_OBJECT(GenerateProcessIdsFilter)

    static Pointer New() { return new GenerateProcessIdsFilter; }

    bool Execute() override;

    void SetGeneratePointData(bool b) { m_GeneratePointData = b; }
    bool GetGeneratePointData() const { return m_GeneratePointData; }

    void SetGenerateCellData(bool b) { m_GenerateCellData = b; }
    bool GetGenerateCellData() const { return m_GenerateCellData; }

    // 当前进程号，写入所有点 / 单元（默认 0）
    void SetProcessId(long long pid) { m_ProcessId = pid; }
    long long GetProcessId() const { return m_ProcessId; }

    const std::string& GetMessage() const { return m_Message; }

protected:
    GenerateProcessIdsFilter();
    ~GenerateProcessIdsFilter() override = default;

    // 进程号取值：默认全部为当前进程号，派生类可重写以实现自定义分区策略。
    virtual long long GetPointProcessId(IGsize index);
    virtual long long GetCellProcessId(IGsize index);

    long long m_ProcessId{0};
    bool m_GeneratePointData{true};
    bool m_GenerateCellData{false}; // 与 VTK 一致：单元进程号默认不生成

    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif
