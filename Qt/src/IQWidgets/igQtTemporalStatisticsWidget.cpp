#include "IQWidgets/igQtTemporalStatisticsWidget.h"

#include "ui_igQtTemporalStatistics.h"

#include <Animation/iGameTemporalStatistics.h>
#include <iGameModel.h>

#include <QCheckBox>
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>

igQtTemporalStatisticsWidget::igQtTemporalStatisticsWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtTemporalStatistics) {
    ui->setupUi(this);
    initUI();
    initConnections();
}

igQtTemporalStatisticsWidget::~igQtTemporalStatisticsWidget() { delete ui; }

QDockWidget* igQtTemporalStatisticsWidget::createDockWidget(QWidget* parent) {
    auto* dockWidget = new QDockWidget(QStringLiteral("时域统计"), parent);
    dockWidget->setObjectName(QStringLiteral("dockWidget_TemporalStatistics"));
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    dockWidget->setWidget(new igQtTemporalStatisticsWidget(dockWidget));
    return dockWidget;
}

void igQtTemporalStatisticsWidget::initUI() { refreshSummary(); }

void igQtTemporalStatisticsWidget::initConnections() {
    connect(ui->pushButton_Run, &QPushButton::clicked, this, &igQtTemporalStatisticsWidget::onRunClicked);
    connect(ui->pushButton_Close, &QPushButton::clicked, this, [this]() { emit closeRequested(); });
}

void igQtTemporalStatisticsWidget::setCurrentModel(iGame::Model* model) {
    m_currentModel = model;
    m_input = (model != nullptr) ? model->GetDataObject() : nullptr;
    m_lastDiagnostic.clear();
    refreshSummary();
}

bool igQtTemporalStatisticsWidget::hasAnyStatisticChecked() const {
    return ui->checkBox_Average->isChecked() || ui->checkBox_Minimum->isChecked() ||
           ui->checkBox_Maximum->isChecked() || ui->checkBox_StandardDeviation->isChecked();
}

void igQtTemporalStatisticsWidget::refreshSummary() {
    QStringList lines;
    if (m_input == nullptr) {
        lines << QStringLiteral("请先在模型树中选择一个模型。");
    } else {
        auto frames = m_input->PeekTimeFrames();
        const int frameNum = frames.IsNull() ? 0 : static_cast<int>(frames->GetTimeNum());
        int arrayCount = 0;
        if (auto* attributeSet = m_input->GetAttributeSet()) {
            arrayCount = static_cast<int>(attributeSet->GetNumberOfAttributes());
        }
        lines << QStringLiteral("模型：%1").arg(QString::fromStdString(m_input->GetName()));
        lines << QStringLiteral("时间帧数：%1").arg(frameNum);
        lines << QStringLiteral("数组数：%1（全部数组都会参与统计）").arg(arrayCount);
        if (frameNum == 0) {
            lines << QStringLiteral("提示：当前模型没有时间序列，无法统计。");
        } else if (frameNum == 1) {
            lines << QStringLiteral("提示：只有 1 帧 —— 平均值 = 原值，最小值 = 最大值 = 原值，标准差 = 0。");
        }
    }
    if (!m_lastDiagnostic.isEmpty()) {
        lines << QString();
        lines << m_lastDiagnostic;
    }
    ui->label_Info->setText(lines.join(QStringLiteral("\n")));
}

void igQtTemporalStatisticsWidget::onRunClicked() {
    if (m_input == nullptr) {
        m_lastDiagnostic = QStringLiteral("请先在模型树中选择一个模型。");
        refreshSummary();
        return;
    }
    if (!hasAnyStatisticChecked()) {
        m_lastDiagnostic = QStringLiteral("请至少勾选一个统计量。");
        refreshSummary();
        return;
    }

    auto filter = iGame::iGameTemporalStatistics::New();
    filter->SetInput(m_input);
    filter->SetComputeAverage(ui->checkBox_Average->isChecked());
    filter->SetComputeMinimum(ui->checkBox_Minimum->isChecked());
    filter->SetComputeMaximum(ui->checkBox_Maximum->isChecked());
    filter->SetComputeStandardDeviation(ui->checkBox_StandardDeviation->isChecked());

    if (!filter->Execute()) {
        const std::string message = filter->GetMessage();
        m_lastDiagnostic = QStringLiteral("执行失败：%1")
                                   .arg(message.empty() ? QStringLiteral("未知原因")
                                                        : QString::fromStdString(message));
        refreshSummary();
        return;
    }

    auto output = filter->GetOutput(0);
    if (output == nullptr) {
        m_lastDiagnostic = QStringLiteral("执行失败：算法未产生有效结果。");
        refreshSummary();
        return;
    }

    const std::string warnings = filter->GetWarningsText();
    m_lastDiagnostic = warnings.empty()
                               ? QStringLiteral("执行完成。")
                               : QStringLiteral("执行完成，存在以下提示：\n%1")
                                         .arg(QString::fromStdString(warnings));
    refreshSummary();
    emit resultReady(output);
}
