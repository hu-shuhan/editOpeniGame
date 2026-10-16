#pragma once

// —— 两个任务 Filter 的头文件 ——
#include "ExtractEdges/iGameExtractEdgesFilter.h"  // 中等任务#28：边提取 Filter
#include "ExportEdges/iGameExportEdgesFilter.h"    // 配套：边导出为 VTK 的 Filter
#include "iGameUnstructuredMesh.h"                 // UnstructuredMesh 类型

#include <ui_ExtractEdges.h>  // Qt Designer 生成的界面类（对应 ExtractEdges.ui）

/**
 * @class igQtExtractEdgesWidget
 * @brief "边提取" 面板控件（GUI 集成层）。
 *
 * 【职责】
 *   把中等任务 #28 的 ExtractEdgesFilter 包装成可视化的操作面板：
 *   用户在左侧 Dock 面板里点「执行」→ 调 Filter 提取边 →
 *   通过信号把结果发给主窗口 → 主窗口把结果模型加入场景树并渲染。
 *
 * 【数据流】
 *   主窗口(选中模型) ──SetOriginDataObject──> 本面板
 *   用户点「执行」──> ExtractEdges() ──> 调 ExtractEdgesFilter
 *   ──DrawEdgesModel/UpdateEdgesModel 信号──> 主窗口加模型到场景
 *
 * 【信号说明】
 *   - DrawEdgesModel：第一次执行时发出（把新模型加入场景树）
 *   - UpdateEdgesModel：之后每次重新执行时发出（只刷新已有模型，不重复加树）
 */
class igQtExtractEdgesWidget : public QWidget {

    Q_OBJECT  // Qt 元对象宏：启用信号/槽机制

public:
    // 构造函数：加载 UI（ExtractEdges.ui）
    igQtExtractEdgesWidget(QWidget* parent = nullptr);

public slots:
    // 槽函数 = 被按钮等触发时执行的函数

    /// 「执行」按钮：运行 ExtractEdgesFilter，把提取出的边模型发给主窗口
    void ExtractEdges();

    /// 「导出边为 VTK」按钮：把提取结果写为 .vtk 文件（调 ExportEdgesFilter）
    void ExportEdges();

    /// 由主窗口调用：把当前选中的模型喂给本面板（记录输入，供提取用）
    void SetOriginDataObject(iGame::DataObject::Pointer m_d);

signals:
    // 信号 = 只声明不实现，由 Qt 在运行时自动生成调用代码

    /// 第一次提取成功时发出：告诉主窗口"把新边模型加入场景树"
    void DrawEdgesModel(iGame::DataObject::Pointer);

    /// 重复提取时发出：告诉主窗口"刷新已有边模型"
    void UpdateEdgesModel(iGame::DataObject::Pointer);

private:
    Ui::ExtractEdges* ui;  // Designer 生成的界面指针（按钮等控件都挂在它下面）

    iGame::DataObject::Pointer m_OriginDataObject{ nullptr };   // 用户选中的输入模型
    iGame::UnstructuredMesh::Pointer m_ResultMesh{ nullptr };   // 提取结果（边网格）
    iGame::ExtractEdgesFilter::Pointer m_Extracter{ nullptr };  // 边提取 Filter 实例
    bool m_Generated = false;  // 是否已经成功提取过一次（决定发哪个信号）
};
