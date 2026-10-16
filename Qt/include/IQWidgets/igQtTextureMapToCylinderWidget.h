#pragma once

// —— 本面板对应的 Filter ——
#include "TextureMapToCylinder/iGameTextureMapToCylinderFilter.h"
#include "iGameUnstructuredMesh.h"

#include <ui_TextureMapToCylinder.h>  // Qt Designer 生成的界面类（对应 TextureMapToCylinder.ui）

/**
 * @class igQtTextureMapToCylinderWidget
 * @brief "圆柱面纹理坐标" 面板控件（GUI 集成层）。
 *
 * 【职责】
 *   把 TextureMapToCylinderFilter 包装成可视化面板：用户在左侧 Dock 里设置轴与选项、
 *   点「执行」→ 调 Filter 生成纹理坐标 → 通过信号通知主窗口加模型/刷新渲染。
 *
 * 【数据流】
 *   主窗口(选中模型) ──SetOriginDataObject──> 本面板
 *   用户点「执行」──> Execute() ──> 调 TextureMapToCylinderFilter
 *   ──DrawResultModel/UpdateResultModel 信号──> 主窗口加模型到场景
 *
 * 【信号】
 *   - DrawResultModel：第一次执行时发出（把结果加入场景树）
 *   - UpdateResultModel：之后每次重新执行时发出（只刷新已有模型）
 */
class igQtTextureMapToCylinderWidget : public QWidget {

    Q_OBJECT

public:
    igQtTextureMapToCylinderWidget(QWidget* parent = nullptr);

public slots:
    /// 「执行」按钮：按当前面板参数生成纹理坐标，并把结果发给主窗口
    void Execute();

    /// 由主窗口调用：把当前选中模型喂给本面板
    void SetOriginDataObject(iGame::DataObject::Pointer m_d);

signals:
    /// 第一次成功时发出：告诉主窗口"把结果模型加入场景树"
    void DrawResultModel(iGame::DataObject::Pointer);

    /// 重复执行时发出：告诉主窗口"刷新已有结果模型"
    void UpdateResultModel(iGame::DataObject::Pointer);

private:
    /// 勾选/取消「自动求轴」时，启用或禁用轴端点输入框
    void UpdateAxisControlsEnabled();

    /// 从面板读参数写入 Filter
    void ApplyParametersToFilter();

    /// 执行单个输入对象，成功返回结果网格（失败返回空）
    iGame::UnstructuredMesh::Pointer RunFilterOn(iGame::DataObject::Pointer input);

    Ui::TextureMapToCylinder* ui;

    iGame::DataObject::Pointer m_OriginDataObject{nullptr};               // 用户选中的输入模型
    iGame::UnstructuredMesh::Pointer m_ResultMesh{nullptr};               // 结果容器
    iGame::TextureMapToCylinderFilter::Pointer m_Filter{nullptr};         // Filter 实例
    bool m_Generated = false;  // 是否已成功执行过（决定发哪个信号）
};
