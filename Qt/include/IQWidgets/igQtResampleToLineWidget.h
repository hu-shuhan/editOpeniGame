#pragma once
#include "ResampleToLine/iGameResampleToLine.h"

#include "IQCore/igQtMainWindow.h"
#include "IQComponents/igQtModelTreeWidget.h"
#include "Core/Interactor/iGameResampleToLineStyle.h"
#include "iGameSelection.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <ui_ResampleToLine.h>

#include <vector>

class igQtModelDialogWidget;

/**
 * @brief 重采样至直线面板
 *
 * - 起点/终点可由文本框输入，也可以在场景中直接拖动（交互风格 ResampleToLineStyle）；
 * - 提供 X / Y / Z 轴快捷按钮，按模型包围盒设置端点方向；
 * - 首次打开面板（或切换模型）时，默认端点由模型包围盒自动给出；
 * - 执行后输出真正的折线数据（SurfaceMesh：点 + 边）。
 */
class igQtResampleToLine : public QWidget {
    Q_OBJECT

public:
    igQtResampleToLine(igQtModelDialogWidget* modelTreeWidget, QWidget* parent = nullptr);
    ~igQtResampleToLine() override;

    /** 显式指定模型树控件（为空时自动从顶层窗口查找） */
    void SetModelTreeWidget(igQtModelDialogWidget* modelTreeWidget);

public slots:
    // 交互传过来（LineSelection 的 IG_CHANGE 回调）
    void SetLine(float o[3], float t[3]);
    void SetLine(iGame::Vector3d orig, iGame::Vector3d target);

    // Widget 输入
    void UpdateLine();
    // 绘制预览线段（仅在没有激活交互风格时使用）
    void DrawLine(float o[3], float t[3]);

    void ResampleToLine();

    void SetOriginDataObject(iGame::DataObject::Pointer m_d);
    void UpdateOriginDataObject(iGame::DataObject::Pointer _origin_ptr);

    /** 绑定当前场景中的模型，并按需激活可拖动端点的交互风格（供主窗口打开面板时调用） */
    void BindCurrentModel();

    iGame::LineSelection::Pointer GetSelection();

    /** X / Y / Z 轴快捷设置：axis = 0/1/2 */
    void SetAxis(int axis);
    /** 根据模型包围盒重置默认端点 */
    void ResetLineFromBoundingBox();

signals:
    void DrawLine(iGame::DrawObject::Pointer);
    void UpdateLineModel(iGame::DrawObject::Pointer);
    void ResetInteractor();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    /** 查找主窗口中的模型树控件（避免使用构造时可能失效的指针） */
    igQtModelDialogWidget* ResolveModelTreeWidget();

    /** 绑定当前模型并激活可拖动端点的交互风格，成功返回 true */
    bool ActivateInteractorStyle();

    /** 刷新选择对象/交互风格中的线段 */
    void RefreshLine();
    /** 把内部端点同步到输入框（阻塞信号，避免递归） */
    void SyncLineWidgets();
    /** 清理控件自己绘制的预览线段 */
    void ClearPreviewHandles();

    Ui::ResampleToLineWidget* ui;

    iGame::LineSelection::Pointer m_Selection;
    float m_orig[3] = {-1, 0, 0};
    float m_target[3] = {1, 0, 0};
    int resolution = 40;
    double m_Tolerance = 0.0; // 0 表示自动容差（包围盒对角线 × 1e-6）

    iGame::DataObject::Pointer m_OriginDataObject{nullptr};
    iGame::SurfaceMesh::Pointer m_ResultMesh{nullptr};
    igQtModelDialogWidget* m_ModelTreeWidget{nullptr};

    unsigned long m_OriginObserverTag{0};
    unsigned long m_ResultObserverTag{0};

    bool m_StyleActive{false};            // 交互风格是否已交给交互器
    bool m_Activating{false};             // 激活流程的重入保护
    std::vector<IGuint> m_PreviewHandles; // 控件自己绘制的预览句柄
};
