#include "IQWidgets/igQtPointAndCellIdsWidget.h"
#include "ui_igQtPointAndCellIds.h"

#include <PointAndCellIds/iGamePointAndCellIdsFilter.h>
#include <iGameModel.h>

#include <QCheckBox>
#include <QDockWidget>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

using namespace iGame;

igQtPointAndCellIdsWidget::igQtPointAndCellIdsWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtPointAndCellIds) {
    ui->setupUi(this);
    resetForm();
    initConnections();
}

igQtPointAndCellIdsWidget::~igQtPointAndCellIdsWidget() {
    delete ui;
}

// 创建右侧参数面板
QDockWidget* igQtPointAndCellIdsWidget::createDockWidget(QWidget* parent) {
    auto* dockWidget = new QDockWidget(QStringLiteral("点与单元ID"), parent);
    dockWidget->setObjectName(QStringLiteral("dockWidget_PointAndCellIds"));
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    dockWidget->setWidget(new igQtPointAndCellIdsWidget(dockWidget));
    return dockWidget;
}

// 设置当前待处理模型
void igQtPointAndCellIdsWidget::setCurrentModel(iGame::Model* model) {
    m_currentModel = model;
    resetForm();
}

// 恢复 ParaView/VTK 默认参数
void igQtPointAndCellIdsWidget::resetForm() {
    ui->checkBox_GeneratePointIds->setChecked(true);
    ui->checkBox_GenerateCellIds->setChecked(true);
    ui->lineEdit_PointIdsName->setText(QStringLiteral("vtkPointIds"));
    ui->lineEdit_CellIdsName->setText(QStringLiteral("vtkCellIds"));
    ui->lineEdit_PointIdsName->setEnabled(true);
    ui->lineEdit_CellIdsName->setEnabled(true);
}

// 连接页面控件
void igQtPointAndCellIdsWidget::initConnections() {
    connect(ui->pushButton_Apply, &QPushButton::clicked,
            this, &igQtPointAndCellIdsWidget::apply);

    connect(ui->pushButton_Cancel, &QPushButton::clicked,
            this, &igQtPointAndCellIdsWidget::cancel);

    connect(ui->checkBox_GeneratePointIds, &QCheckBox::toggled,
            ui->lineEdit_PointIdsName, &QLineEdit::setEnabled);

    connect(ui->checkBox_GenerateCellIds, &QCheckBox::toggled,
            ui->lineEdit_CellIdsName, &QLineEdit::setEnabled);
}

// 根据页面参数执行 Filter
void igQtPointAndCellIdsWidget::apply() {
    if (!m_currentModel) {
        QMessageBox::warning(this, QStringLiteral("点与单元ID"),
                             QStringLiteral("请先选择一个模型。"));
        return;
    }

    auto dataObject = m_currentModel->GetDataObject();
    if (!dataObject) {
        QMessageBox::warning(this, QStringLiteral("点与单元ID"),
                             QStringLiteral("当前模型没有可处理的数据。"));
        return;
    }

    auto filter = PointAndCellIdsFilter::New();
    filter->SetInput(dataObject);
    filter->SetGeneratePointIds(ui->checkBox_GeneratePointIds->isChecked());
    filter->SetGenerateCellIds(ui->checkBox_GenerateCellIds->isChecked());
    filter->SetPointIdsArrayName(ui->lineEdit_PointIdsName->text().trimmed().toStdString());
    filter->SetCellIdsArrayName(ui->lineEdit_CellIdsName->text().trimmed().toStdString());

    if (!filter->Execute()) {
        QMessageBox::warning(this, QStringLiteral("生成ID失败"),
                             QString::fromStdString(filter->GetMessage()));
        return;
    }

    emit idsGenerated(filter->GetOutput());
}

// 取消并关闭参数页面
void igQtPointAndCellIdsWidget::cancel() {
    resetForm();
    emit cancelRequested();
}
