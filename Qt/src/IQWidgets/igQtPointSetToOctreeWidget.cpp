#include "IQWidgets/igQtPointSetToOctreeWidget.h"

#include "ui_igQtPointSetToOctree.h"

#include <Convert/iGamePointSetToOctreeFilter.h>
#include <iGameAttributeSet.h>
#include <iGameModel.h>
#include <iGamePointSet.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

#include <string>

namespace {

// 大尺寸提醒阈值：预估体素数超过该值时执行前二次确认。
constexpr igIndex64 kLargeCellCountWarning = 50000000ll;

/// 汇总 6 个统计函数复选框的状态。
struct FunctionFlags {
    bool lastValue{false};
    bool min{true};
    bool max{true};
    bool count{true};
    bool sum{false};
    bool mean{true};

    bool any() const { return lastValue || min || max || count || sum || mean; }
    int count_() const {
        int n = 0;
        n += (lastValue ? 1 : 0) + (min ? 1 : 0) + (max ? 1 : 0);
        n += ((count || mean) ? 1 : 0) + ((sum || mean) ? 1 : 0) + (mean ? 1 : 0);
        return n;
    }
};

} // namespace

igQtPointSetToOctreeWidget::igQtPointSetToOctreeWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtPointSetToOctree) {
    ui->setupUi(this);
    initUI();
    initConnections();
}

igQtPointSetToOctreeWidget::~igQtPointSetToOctreeWidget() { delete ui; }

QDockWidget* igQtPointSetToOctreeWidget::createDockWidget(QWidget* parent) {
    auto* dockWidget = new QDockWidget(QStringLiteral("点集转八叉树"), parent);
    dockWidget->setObjectName(QStringLiteral("dockWidget_PointSetToOctree"));
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    dockWidget->setWidget(new igQtPointSetToOctreeWidget(dockWidget));
    return dockWidget;
}

void igQtPointSetToOctreeWidget::initUI() {
    // 未勾选「处理点属性数组」时，属性相关控件保持禁用
    const bool process = ui->checkBox_ProcessInputPointArray->isChecked();
    ui->comboBox_PointArray->setEnabled(process);
    ui->checkBox_LastValue->setEnabled(process);
    ui->checkBox_Min->setEnabled(process);
    ui->checkBox_Max->setEnabled(process);
    ui->checkBox_Count->setEnabled(process);
    ui->checkBox_Sum->setEnabled(process);
    ui->checkBox_Mean->setEnabled(process);
    refreshInfo();
}

void igQtPointSetToOctreeWidget::initConnections() {
    connect(ui->pushButton_Run, &QPushButton::clicked, this,
            &igQtPointSetToOctreeWidget::onRunClicked);
    connect(ui->pushButton_Close, &QPushButton::clicked, this, [this]() { emit closeRequested(); });

    connect(ui->spinBox_PointsPerCell, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtPointSetToOctreeWidget::refreshInfo);

    // 是否处理点属性数组 → 联动属性下拉框与统计函数复选框的可用状态
    connect(ui->checkBox_ProcessInputPointArray, &QCheckBox::toggled, this, [this](bool process) {
        ui->comboBox_PointArray->setEnabled(process);
        ui->checkBox_LastValue->setEnabled(process);
        ui->checkBox_Min->setEnabled(process);
        ui->checkBox_Max->setEnabled(process);
        ui->checkBox_Count->setEnabled(process);
        ui->checkBox_Sum->setEnabled(process);
        ui->checkBox_Mean->setEnabled(process);
        refreshInfo();
    });
}

void igQtPointSetToOctreeWidget::setCurrentModel(iGame::Model* model) {
    m_currentModel = model;
    m_input = (model != nullptr) ? model->GetDataObject() : nullptr;
    refreshPointArrayList();
    refreshInfo();
}

void igQtPointSetToOctreeWidget::refreshPointArrayList() {
    ui->comboBox_PointArray->clear();

    auto mesh = iGame::DynamicCast<iGame::PointSet>(m_input);
    if (mesh == nullptr) return;

    const IGsize numberOfPoints = mesh->GetNumberOfPoints();
    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    if (attrs == nullptr) return;

    for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
        auto& a = attrs->GetElement(i);
        if (a.isDeleted || a.pointer == nullptr) continue;
        if (a.attachmentType != IG_POINT) continue;
        if (a.pointer->GetNumberOfElements() != numberOfPoints) continue;
        // 本过滤器只支持单分量点属性（等价 VTK 的 SCALARS 输入约定）
        if (a.pointer->GetDimension() != 1) continue;
        ui->comboBox_PointArray->addItem(QString::fromStdString(a.pointer->GetName()));
    }
}

void igQtPointSetToOctreeWidget::refreshInfo() {
    if (m_input == nullptr) {
        ui->label_Info->setText(QStringLiteral("请先选择一个模型。"));
        ui->pushButton_Run->setEnabled(false);
        return;
    }

    auto mesh = iGame::DynamicCast<iGame::PointSet>(m_input);
    if (mesh == nullptr) {
        ui->label_Info->setText(QStringLiteral("当前数据不是点集/网格（PointSet），无法转换。"));
        ui->pushButton_Run->setEnabled(false);
        return;
    }
    ui->pushButton_Run->setEnabled(true);

    const IGsize numberOfPoints = mesh->GetNumberOfPoints();
    const igIndex64 perCell = static_cast<igIndex64>(ui->spinBox_PointsPerCell->value());
    const igIndex64 estimatedCells =
            (perCell > 0) ? static_cast<igIndex64>(numberOfPoints / static_cast<IGsize>(perCell)) : 0;

    QStringList lines;
    lines << QStringLiteral("输入点数：%1").arg(static_cast<qlonglong>(numberOfPoints));
    lines << QStringLiteral("预估体素数：%1（= 点数 / 每体素平均点数；输出维度按输入包围盒"
                            "各向尺寸比例自动分布）")
                     .arg(static_cast<qlonglong>(estimatedCells));

    if (ui->checkBox_ProcessInputPointArray->isChecked()) {
        if (ui->comboBox_PointArray->count() == 0) {
            lines << QStringLiteral("⚠ 未找到可用的单分量点属性数组，请先关闭“处理点属性数组”或更换数据。");
        } else {
            FunctionFlags flags;
            flags.lastValue = ui->checkBox_LastValue->isChecked();
            flags.min = ui->checkBox_Min->isChecked();
            flags.max = ui->checkBox_Max->isChecked();
            flags.count = ui->checkBox_Count->isChecked();
            flags.sum = ui->checkBox_Sum->isChecked();
            flags.mean = ui->checkBox_Mean->isChecked();
            lines << QStringLiteral("点属性数组：%1（输出 %2 个分量）")
                             .arg(ui->comboBox_PointArray->currentText())
                             .arg(flags.count_());
            if (!flags.any()) {
                lines << QStringLiteral("⚠ 未勾选任何统计函数，执行将报错。");
            } else if (flags.mean && !(flags.count && flags.sum)) {
                lines << QStringLiteral("提示：Mean 已开启，Count 与 Sum 会被自动一并计算。");
            }
        }
    } else {
        lines << QStringLiteral("未处理点属性：输出仅包含 octree 占用位编码（等价 VTK 的 octree 数组）。");
    }

    if (estimatedCells > kLargeCellCountWarning) {
        lines << QStringLiteral("⚠ 预估体素数较大，执行可能耗时并占用较多内存，执行前会再次确认。");
    }

    if (!m_lastDiagnostic.isEmpty()) {
        lines << QStringLiteral("---- 上次执行 ----");
        lines << m_lastDiagnostic;
    }
    ui->label_Info->setText(lines.join(QStringLiteral("\n")));
}

bool igQtPointSetToOctreeWidget::buildFilter(iGame::PointSetToOctreeFilter::Pointer& filter,
                                             QString& error) {
    if (m_input == nullptr) {
        error = QStringLiteral("请先选择一个模型。");
        return false;
    }

    filter = iGame::PointSetToOctreeFilter::New();
    filter->SetInput(m_input);
    filter->SetNumberOfPointsPerCell(
            static_cast<igIndex64>(ui->spinBox_PointsPerCell->value()));

    const bool process = ui->checkBox_ProcessInputPointArray->isChecked();
    filter->SetProcessInputPointArray(process);
    if (!process) return true;

    if (ui->comboBox_PointArray->count() == 0) {
        error = QStringLiteral("未找到可用的单分量点属性数组，请关闭“处理点属性数组”或更换数据。");
        return false;
    }
    filter->SetInputPointArrayName(ui->comboBox_PointArray->currentText().toStdString());

    FunctionFlags flags;
    flags.lastValue = ui->checkBox_LastValue->isChecked();
    flags.min = ui->checkBox_Min->isChecked();
    flags.max = ui->checkBox_Max->isChecked();
    flags.count = ui->checkBox_Count->isChecked();
    flags.sum = ui->checkBox_Sum->isChecked();
    flags.mean = ui->checkBox_Mean->isChecked();
    if (!flags.any()) {
        error = QStringLiteral("至少需要勾选一个统计函数（Last / Min / Max / Count / Sum / Mean）。");
        return false;
    }

    filter->SetComputeLastValue(flags.lastValue);
    filter->SetComputeMin(flags.min);
    filter->SetComputeMax(flags.max);
    filter->SetComputeCount(flags.count);
    filter->SetComputeSum(flags.sum);
    filter->SetComputeMean(flags.mean);
    return true;
}

void igQtPointSetToOctreeWidget::onRunClicked() {
    iGame::PointSetToOctreeFilter::Pointer filter;
    QString error;
    if (!buildFilter(filter, error)) {
        QMessageBox::warning(this, QStringLiteral("点集转八叉树"), error);
        return;
    }

    // 执行前规模提醒
    auto mesh = iGame::DynamicCast<iGame::PointSet>(m_input);
    if (mesh != nullptr) {
        const IGsize numberOfPoints = mesh->GetNumberOfPoints();
        const igIndex64 perCell = static_cast<igIndex64>(ui->spinBox_PointsPerCell->value());
        const igIndex64 estimatedCells =
                (perCell > 0) ? static_cast<igIndex64>(numberOfPoints / static_cast<IGsize>(perCell)) : 0;
        if (estimatedCells > kLargeCellCountWarning) {
            const auto answer = QMessageBox::question(
                    this, QStringLiteral("点集转八叉树"),
                    QStringLiteral("预估体素数较大：约 %1 个。\n\n是否继续执行？")
                            .arg(static_cast<qlonglong>(estimatedCells)),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes) return;
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = filter->Execute();
    QApplication::restoreOverrideCursor();

    const QString message = QString::fromStdString(filter->GetMessage()).trimmed();
    m_lastDiagnostic = message;
    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("点集转八叉树"),
                             message.isEmpty() ? QStringLiteral("执行失败。") : message);
        if (message.isEmpty()) m_lastDiagnostic.clear();
        refreshInfo();
        return;
    }

    iGame::DataObject::Pointer result = filter->GetOutput(0);
    if (result == nullptr) {
        QMessageBox::warning(this, QStringLiteral("点集转八叉树"),
                             QStringLiteral("执行成功但没有输出。"));
        refreshInfo();
        return;
    }
    result->SetName(m_input->GetName() + std::string("_octree"));
    emit resultReady(result);

    if (m_lastDiagnostic.isEmpty()) m_lastDiagnostic = QStringLiteral("执行完成。");
    refreshInfo();
}
