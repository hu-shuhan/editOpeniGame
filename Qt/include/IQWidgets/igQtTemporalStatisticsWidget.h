#pragma once

// ============================================================================
// igQtTemporalStatisticsWidget —「时域统计 (Temporal Statistics)」参数面板
//
// 对应 ParaView / VTK 的 vtkTemporalStatistics：
//   - 参数：要输出哪些统计量（平均值 / 最小值 / 最大值 / 标准差），默认全开，
//     与 VTK 9.4 的 vtkTemporalStatistics 默认值一致；
//   - 对当前时序模型的**全部数组**逐元素统计，无需（也不能）选择数组；
//   - 执行后把 filter->GetWarnings() 的非致命提示（某帧缺数组、数组规模随帧变化
//     被丢弃、单帧输入等）显示在面板上，避免静默的不完整结果；
//   - 成功时通过 resultReady 把新数据集交给主窗口挂成新模型。
//
// 该面板由主窗口挂在**左侧** Dock（与「重采样到图像」同一种面板形态）。
// ============================================================================

#include <IQCore/igQtExportModule.h>

#include <QWidget>

#include <iGameDataObject.h>

class QDockWidget;

namespace iGame {
class Model;
}

namespace Ui {
class igQtTemporalStatistics;
}

class IG_QT_MODULE_EXPORT igQtTemporalStatisticsWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtTemporalStatisticsWidget(QWidget* parent = nullptr);
    ~igQtTemporalStatisticsWidget() override;

    static QDockWidget* createDockWidget(QWidget* parent);

    /// 打开面板 / 切换模型时调用：刷新输入信息（帧数、数组数）。
    void setCurrentModel(iGame::Model* model);

signals:
    void closeRequested();
    void resultReady(iGame::DataObject::Pointer result);

private slots:
    void onRunClicked();
    void refreshSummary();

private:
    void initUI();
    void initConnections();
    /// 面板上勾选的统计量是否至少有一个
    bool hasAnyStatisticChecked() const;

    Ui::igQtTemporalStatistics* ui{nullptr};
    iGame::Model* m_currentModel{nullptr};
    iGame::DataObject::Pointer m_input;
    QString m_lastDiagnostic; ///< 上次执行的诊断信息，与输入信息一起显示
};
