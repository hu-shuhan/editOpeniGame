#pragma once

// —— 本面板对应的 Filter ——
#include "TensorPrincipalInvariants/iGameTensorPrincipalInvariantsFilter.h"
#include "iGameUnstructuredMesh.h"

#include <ui_TensorPrincipalInvariants.h>  // Designer 生成的界面类

/**
 * @class igQtTensorPrincipalInvariantsWidget
 * @brief "张量主值与主方向" 面板控件（GUI 集成层）。
 *
 * 【职责】
 *   把 TensorPrincipalInvariantsFilter 包装成可视化面板：
 *   下拉框里只列出**能当张量用**的数组（6 分量，或标记为 IG_TENSOR 的 3 分量），
 *   用户选中后点「执行」→ 调 Filter 求主值/主方向 → 信号通知主窗口显示结果。
 *
 * 【数据流】
 *   主窗口(选中模型) ──SetOriginDataObject──> 本面板（顺带刷新张量数组下拉框）
 *   用户点「执行」──> Execute() ──> 调 TensorPrincipalInvariantsFilter
 *   ──DrawResultModel/UpdateResultModel 信号──> 主窗口加模型到场景
 */
class igQtTensorPrincipalInvariantsWidget : public QWidget {

    Q_OBJECT

public:
    igQtTensorPrincipalInvariantsWidget(QWidget* parent = nullptr);

public slots:
    /// 「执行」按钮：按选中的张量数组求主值与主方向
    void Execute();

    /// 由主窗口调用：把当前选中模型喂给本面板，并刷新张量数组下拉框
    void SetOriginDataObject(iGame::DataObject::Pointer m_d);

signals:
    /// 第一次成功时发出：告诉主窗口"把结果模型加入场景树"
    void DrawResultModel(iGame::DataObject::Pointer);

    /// 重复执行时发出：告诉主窗口"刷新已有结果模型"
    void UpdateResultModel(iGame::DataObject::Pointer);

private:
    /// 扫描输入模型的属性集，把可作为张量的数组填进下拉框
    void ReloadTensorArrays();

    /// 从面板读参数写入 Filter（数组名 + 挂载位置 + ScaleVectors）
    void ApplyParametersToFilter();

    /// 对单个输入跑一遍 Filter
    iGame::UnstructuredMesh::Pointer RunFilterOn(iGame::DataObject::Pointer input);

    Ui::TensorPrincipalInvariants* ui;

    iGame::DataObject::Pointer m_OriginDataObject{nullptr};                  // 用户选中的输入模型
    iGame::UnstructuredMesh::Pointer m_ResultMesh{nullptr};                  // 结果容器
    iGame::TensorPrincipalInvariantsFilter::Pointer m_Filter{nullptr};       // Filter 实例
    bool m_Generated = false;  // 是否已成功执行过（决定发哪个信号）
};
