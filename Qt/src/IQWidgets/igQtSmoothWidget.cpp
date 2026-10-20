#include "IQWidgets/igQtSmoothWidget.h"
#include "ui_igQtSmooth.h"

#include <Smooth/iGameSmoothFilter.h>
#include <iGameModel.h>

#include <QCheckBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

using namespace iGame;

igQtSmoothWidget::igQtSmoothWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtSmooth) {
    ui->setupUi(this);
    resetForm();
    ui->pushButton_Apply->setEnabled(false);
    connect(ui->pushButton_Apply, &QPushButton::clicked, this, &igQtSmoothWidget::apply);
    connect(ui->pushButton_Cancel, &QPushButton::clicked, this, &igQtSmoothWidget::cancel);
}

igQtSmoothWidget::~igQtSmoothWidget() {
    delete ui;
}

// 创建右侧参数面板
QDockWidget* igQtSmoothWidget::createDockWidget(QWidget* parent) {
    auto* dockWidget = new QDockWidget(QStringLiteral("表面平滑 (Smooth)"), parent);
    dockWidget->setObjectName(QStringLiteral("dockWidget_Smooth"));
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    dockWidget->setWidget(new igQtSmoothWidget(dockWidget));
    return dockWidget;
}

void igQtSmoothWidget::setCurrentModel(iGame::Model* model) {
    m_currentInput = model ? model->GetDataObject() : nullptr;
    ui->pushButton_Apply->setEnabled(m_currentInput != nullptr);
}

void igQtSmoothWidget::resetForm() {
    auto filter = SmoothFilter::New();
    ui->spinBox_Iterations->setValue(filter->GetNumberOfIterations());
    ui->doubleSpinBox_Convergence->setValue(filter->GetConvergence());
    ui->doubleSpinBox_RelaxationFactor->setValue(filter->GetRelaxationFactor());
    ui->checkBox_PreserveBoundary->setChecked(filter->GetPreserveBoundary());
}

// 根据页面参数执行平滑
void igQtSmoothWidget::apply() {
    if (!m_currentInput) {
        QMessageBox::warning(this, QStringLiteral("表面平滑"), QStringLiteral("请先选择一个模型。"));
        return;
    }

    auto filter = SmoothFilter::New();
    filter->SetInput(m_currentInput);
    filter->SetNumberOfIterations(ui->spinBox_Iterations->value());
    filter->SetConvergence(ui->doubleSpinBox_Convergence->value());
    filter->SetRelaxationFactor(ui->doubleSpinBox_RelaxationFactor->value());
    filter->SetPreserveBoundary(ui->checkBox_PreserveBoundary->isChecked());
    if (!filter->Execute()) {
        QMessageBox::warning(this, QStringLiteral("平滑失败"), QString::fromStdString(filter->GetMessage()));
        return;
    }
    emit smoothGenerated(filter->GetOutput());
}

void igQtSmoothWidget::cancel() {
    resetForm();
    emit cancelRequested();
}
