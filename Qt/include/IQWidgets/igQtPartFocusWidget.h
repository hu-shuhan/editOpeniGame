#pragma once

#include <IQCore/igQtExportModule.h>
#include <iGameModel.h>

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QVector>
#include <QPair>
#include <QWidget>
#include <vector>

/**
 * igQtPartFocusWidget
 * 零件聚焦面板：基于 part_id（BlockMapping）列出零件列表，
 * 支持多选后聚焦视角或将选中零件的包围盒设置为 SelectionBox。
 *
 * 使用方式：
 *   widget->SetScene(scene, rendererWidget);
 *   widget->RefreshPartList();
 */
class IG_QT_MODULE_EXPORT igQtPartFocusWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtPartFocusWidget(QWidget* parent = nullptr);

    // 设置场景和渲染窗口（必须在使用前调用）
    void SetScene(iGame::Scene* scene, QWidget* rendererWidget);

    // 返回当前勾选的 part id 列表
    std::vector<int> GetSelectedPartIds() const;

public slots:
    // 刷新零件列表（读取当前模型的 BlockMapping）
    void RefreshPartList();

signals:
    void SIGNAL_FocusApplied();
    void SIGNAL_SelectedPartsChanged(const QVector<int>& partIds);

private slots:
    void onFocusCamera();
    void onSetSelectionBox();
    void onFocusBoth();

private:
    // 计算选中 part 的合并包围盒，返回 false 表示无有效 part
    bool computeBoundingBoxForSelected(iGame::BoundingBox& outBBox) const;

    bool applyFocusCamera(const iGame::BoundingBox& bbox);
    bool applySelectionBox(const iGame::BoundingBox& bbox);

    void setupUI();
    void setStatus(const QString& msg);

    iGame::Scene*  m_scene{nullptr};
    QWidget*       m_rendererWidget{nullptr};

    QLabel*       m_statusLabel{nullptr};
    QScrollArea*  m_partScrollArea{nullptr};
    QWidget*      m_partContainer{nullptr};
    QVector<QPair<int, QCheckBox*>> m_partCheckBoxes;
    QPushButton*  m_btnRefresh{nullptr};
    QPushButton*  m_btnFocusCamera{nullptr};
    QPushButton*  m_btnSetBox{nullptr};
    QPushButton*  m_btnFocusBoth{nullptr};
};
