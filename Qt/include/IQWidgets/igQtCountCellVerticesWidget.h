#pragma once

// —— 简单任务 Filter 头文件 ——
#include "CountCellVertices/iGameCountCellVerticesFilter.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"

#include <ui_CountCellVertices.h>

/**
 * @class igQtCountCellVerticesWidget
 * @brief "统计单元顶点数" 面板控件（简单任务 #5 的 GUI 集成）。
 *
 * 【职责】
 *   把 CountCellVerticesFilter 包装成可视化面板：
 *   点「执行」→ 跑 Filter（生成**独立输出节点**，数据与原模型隔离）→
 *   把每个单元的顶点数以表格形式列出来（模仿 ParaView SpreadSheet View 的体验），
 *   并把结果网格作为一个独立节点加入模型树（可按 cell_vertex_count 着色）。
 *
 * 【对标 ParaView 的显示方式】
 *   与 ParaView 一致：执行后**结果替换输入显示**——原模型自动隐藏、结果节点成为当前显示
 *   （见主窗口对 DrawCountModel 信号的处理）。场景里始终只渲染一份网格，不产生额外负担。
 *
 * 【表格分页（对标框架 igQtSearchInfoWidget）】
 *   大模型不一次性把所有单元塞进 QTableWidget，而是**分页**展示：
 *   每页最多 kPageSize 行，提供「上一页 / 下一页」翻页与页码提示；
 *   完整数据仍可「导出CSV」。
 */
class igQtCountCellVerticesWidget : public QWidget {

    Q_OBJECT

public:
    igQtCountCellVerticesWidget(QWidget* parent = nullptr);

public slots:
    /// 「执行」按钮：运行 CountCellVerticesFilter、填表格、并把结果加入模型树
    void ExecuteCount();

    /// 「导出CSV」按钮：把完整统计数据保存为 .csv 文件（不截断）
    void ExportCSV();

    /// 翻页：上一页
    void PrevPage();

    /// 翻页：下一页
    void NextPage();

    /// 由主窗口调用：记录当前选中的输入模型
    void SetOriginDataObject(iGame::DataObject::Pointer obj);

signals:
    /// 第一次执行成功：通知主窗口把结果网格作为独立节点加入模型树（并隐藏原模型）
    void DrawCountModel(iGame::DataObject::Pointer);

    /// 重复执行：通知主窗口刷新已有结果节点
    void UpdateCountModel(iGame::DataObject::Pointer);

private:
    /// 从结果属性集里按名字 + 挂载位置找 cell_vertex_count 数组（找不到返回空）
    iGame::ArrayObject::Pointer FindCountArray(iGame::DataObject::Pointer obj);

    /// 按当前页填充表格、更新页码与摘要
    void ShowPage();

    /// 每页最多显示的单元行数
    static constexpr int kPageSize = 1000;

    Ui::CountCellVertices* ui;

    iGame::DataObject::Pointer m_OriginDataObject{ nullptr };   // 选中的输入模型
    iGame::CountCellVerticesFilter::Pointer m_Filter{ nullptr }; // 简单任务 Filter 实例
    iGame::UnstructuredMesh::Pointer m_ResultMesh{ nullptr };    // 独立输出节点（最近一次结果）
    iGame::ArrayObject::Pointer m_Counts{ nullptr };             // 最近一次统计结果（供导出用）
    bool m_Generated = false;                                    // 是否已成功执行过一次（决定发哪个信号）
    int m_currentPage = 0;                                       // 当前页码（0 起）
};
