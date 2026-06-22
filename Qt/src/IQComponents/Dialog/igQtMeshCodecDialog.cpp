#include "IQComponents/Dialog/igQtMeshCodecDialog.h"
#include <QBrush>
#include <QColor>
#include <QPen>
#include <QTimer>
#include <QScreen>
#include <limits>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace {
constexpr QColor kChartPlotBg(0x25, 0x25, 0x26);
constexpr QColor kChartLabelColor(0xD8, 0xD8, 0xD8);
constexpr QColor kChartGridColor(0x3A, 0x3A, 0x3A);
constexpr QColor kChartAxisLine(0x5A, 0x5A, 0x5A);
constexpr QColor kChartSeriesFill(0xC0, 0xC0, 0xC0);
constexpr QColor kChartSeriesBorder(0xA0, 0xA0, 0xA0);

void StyleChartAxes(QValueAxis* axisX, QValueAxis* axisY)
{
    if (axisX) {
        axisX->setLabelsColor(kChartLabelColor);
        axisX->setGridLineColor(kChartGridColor);
        axisX->setLinePenColor(kChartAxisLine);
    }
    if (axisY) {
        axisY->setLabelsColor(kChartLabelColor);
        axisY->setGridLineColor(kChartGridColor);
        axisY->setLinePenColor(kChartAxisLine);
    }
}

void ApplyDarkChartShell(QChart* chart)
{
    if (!chart) return;
    chart->setBackgroundVisible(true);
    chart->setBackgroundBrush(QBrush(Qt::transparent));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(kChartPlotBg));
    chart->setPlotAreaBackgroundPen(QPen(kChartGridColor, 1));
    if (chart->legend()) chart->legend()->setVisible(false);
}

void ApplyKeyAreaDarkStyle(QGroupBox* groupBox, QChartView* chartView, QWidget* checkBoxContainer)
{
    if (groupBox) {
        groupBox->setAttribute(Qt::WA_StyledBackground, true);
        groupBox->setStyleSheet(
            "QGroupBox#groupbox_dataDistGroup {"
            "  background-color: #252526;"
            "  border: 1px solid #3A3A3A;"
            "  border-radius: 4px;"
            "  color: #D8D8D8;"
            "  margin-top: 8px;"
            "  padding-top: 8px;"
            "}"
            "QGroupBox#groupbox_dataDistGroup::title {"
            "  subcontrol-origin: margin;"
            "  left: 8px;"
            "  padding: 0 4px;"
            "  color: #D8D8D8;"
            "}");
    }
    if (chartView) {
        chartView->setAttribute(Qt::WA_StyledBackground, true);
        chartView->setStyleSheet("background-color: #252526; border: none;");
    }
    if (checkBoxContainer) {
        checkBoxContainer->setAttribute(Qt::WA_StyledBackground, true);
        checkBoxContainer->setStyleSheet(
            "QWidget#checkBoxContainer { background-color: transparent; }"
            "QCheckBox { color: #D8D8D8; spacing: 4px; }"
            "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 2px; }"
            "QCheckBox::indicator:unchecked {"
            "  background-color: #2A2A2A;"
            "  border: 1px solid #5A5A5A;"
            "}"
            "QCheckBox::indicator:checked {"
            "  background-color: #569CD6;"
            "  border: 1px solid #569CD6;"
            "}");
    }
}
} // namespace

static iGame::PointSet::Pointer FindFirstLeafPointSet(iGame::DataObject::Pointer obj) {
    if (!obj) {
        return nullptr;
    }
    if (!obj->HasSubDataObject()) {
        return iGame::DynamicCast<iGame::PointSet>(obj);
    }
    for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); ++it) {
        auto found = FindFirstLeafPointSet(it->second);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

igQtMeshCodecDialog::igQtMeshCodecDialog(QWidget* parent, iGame::DataObject::Pointer obj) :
    igQtChromeFramelessDialog(parent),
    ui(new Ui::MeshCodecDialog),
    m_uiSampleLeafObj(nullptr),
    m_exportSourceObj(obj)
{
    setDialogTitle(QStringLiteral("压缩"));
    setModal(true);
    resize(1100, 760);
    setMinimumSize(900, 620);

    m_bodyWidget = new QWidget(contentHost());
    m_bodyWidget->setObjectName(QStringLiteral("MeshCodecDialogBody"));
    m_bodyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_bodyWidget->setAttribute(Qt::WA_StyledBackground, true);
    m_bodyWidget->setStyleSheet(
        "QWidget#MeshCodecDialogBody { background-color: transparent; color: #EAEAEA; }"
        "QLabel { color: #D8D8D8; }"
        "QGroupBox { color: #D8D8D8; border: 1px solid #3A3A3A; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QLineEdit, QComboBox { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 3px 6px; border-radius: 3px; }"
        "QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3A3A3A; }"
        "QPushButton:pressed { background-color: #252526; }"
        "QCheckBox, QRadioButton { color: #D8D8D8; }"
        "QGroupBox#groupbox_dataDistGroup { background-color: #252526; border: 1px solid #3A3A3A; }"
        "QTableWidget { background-color: #2A2A2A; color: #EAEAEA; gridline-color: #3A3A3A; }"
        "QHeaderView::section { background-color: #333333; color: #EAEAEA; border: 1px solid #3A3A3A; }");

    ui->setupUi(m_bodyWidget);
    setContentWidget(m_bodyWidget);
    QMetaObject::connectSlotsByName(this);

    m_isMultiBlock = (m_exportSourceObj && m_exportSourceObj->HasSubDataObject());

    // 多块数据：UI 仅用于生成编码参数，实际写文件时需要使用根对象
    if (m_isMultiBlock) {
        m_uiSampleLeafObj = FindFirstLeafPointSet(m_exportSourceObj);
        if (!m_uiSampleLeafObj) {
            QMessageBox::critical(this, "错误", "多块数据中未找到可压缩的叶子块（PointSet）！");
            QTimer::singleShot(0, this, &igQtMeshCodecDialog::reject);
            return;
        }
    } else {
        m_uiSampleLeafObj = m_exportSourceObj;
    }

    // 多块数据不支持直方图/关键区域相关 UI，直接隐藏对应选项
    if (m_isMultiBlock) {
        if (ui->radio_areaModel) {
            ui->radio_areaModel->setChecked(false);
            ui->radio_areaModel->setVisible(false);
        }
        if (ui->stack_keyArea) {
            ui->stack_keyArea->setVisible(false);
        }
        if (ui->groupbox_dataDistGroup) {
            ui->groupbox_dataDistGroup->setVisible(false);
        }
        if (ui->comboBox_criticalLevel) ui->comboBox_criticalLevel->setVisible(false);
        if (ui->label_critical) ui->label_critical->setVisible(false);
        if (ui->comboBox_normalLevel) ui->comboBox_normalLevel->setVisible(false);
        if (ui->label_normal) ui->label_normal->setVisible(false);
    }

    // 显示压缩报告选项
    ui->checkbox_showReport->setVisible(true);

    // 内容区随窗口伸缩，仅保证最小高度满足控件
    if (m_bodyWidget->layout()) m_bodyWidget->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    if (ui->rightContainer) ui->rightContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (ui->rightPanel) ui->rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (ui->verticalLayout_main) ui->verticalLayout_main->setStretch(1, 1);
    if (ui->verticalLayout_4) ui->verticalLayout_4->setStretch(1, 1);
    if (ui->label_intro) ui->label_intro->setWordWrap(true);

    InitDataItems();
    InitIntro();
    // 初始化radio的前缀标记
    auto startsWithAny = [&](const QString& s) -> bool {
        for (const auto& m : m_modeMark) { if (s.startsWith(m + " ")) return true; }
        return false;
    };
    auto setWithMark = [&](QRadioButton* r, const QString& mark){ if(!r) return; QString t=r->text(); if(startsWithAny(t)){ for(const auto& m: m_modeMark){ if(t.startsWith(m+" ")){ t=t.mid(m.size()+1); break; } } } r->setText(mark+" "+t); };
    setWithMark(ui->radio_losslessMode, m_modeMark[0]);
    setWithMark(ui->radio_globalMode,   m_modeMark[1]);
    setWithMark(ui->radio_areaModel,    m_modeMark[2]);
    InitAttrFeatureDatas();
    if (!m_isMultiBlock) {
        InitHistogramView();
    }
    InitAttributeList();

    for (int i = 1; i < m_quantizeLevel.size(); i++) {
        ui->comboBox_globalLevel->addItem(m_quantizeLevel[i]);
    }

    // 初始化量化等级ComboBox选项
    for (const auto& level : m_quantizeLevel) {
        ui->comboBox_criticalLevel->addItem(level);
        ui->comboBox_normalLevel->addItem(level);
    }

    // 默认无损模式，禁用所有量化等级ComboBox和Label
    ui->comboBox_globalLevel->setEnabled(false);
    ui->label_global->setEnabled(false);
    ui->label_global->setStyleSheet("QLabel { color: gray; }");
    ui->comboBox_criticalLevel->setEnabled(false);
    ui->label_critical->setEnabled(false);
    ui->label_critical->setStyleSheet("QLabel { color: gray; }");
    ui->comboBox_normalLevel->setEnabled(false);
    ui->label_normal->setEnabled(false);
    ui->label_normal->setStyleSheet("QLabel { color: gray; }");
    
    // 初始隐藏关键区域选择板块并压缩高度
    ui->groupbox_dataDistGroup->setEnabled(false);
    UpdateKeyAreaVisibility(false);

    // 初始化压缩等级选择（zstd支持1-22），默认选择等级12
    ui->comboBox_compressLevel->blockSignals(true);
    for (int i = 1; i <= 22; ++i) {
        ui->comboBox_compressLevel->addItem(QString::number(i));
    }
    ui->comboBox_compressLevel->setCurrentIndex(m_compressLevel);
    ui->comboBox_compressLevel->blockSignals(false);
}

void igQtMeshCodecDialog::InitIntro()
{
    ui->label_intro->setText(
        "1. 通过选择数据来为每种数据单独地设置压缩参数\n"
        "2. 当选择数据类型为\"全部数据\"时，不能设置分区量化模式\n"
        "3. 仅当选择\"全部数据\"并修改参数时，才会将当前设置应用到所有数据；仅切换到\"全部数据\"不会影响现有各项参数\n"
        "4. 当选择\"分区量化模式\"但未产生直方图时，将采用无损压缩模式处理\n"
    );
}

void igQtMeshCodecDialog::InitDataItems()
{
    m_uiDataItems.clear();

    // AllData（虚拟项，用于批量设置）
    {
        UIDataItem item;
        item.category = UIDataCategory::AllData;
        item.displayName = QString::fromUtf8("全部数据");
        item.dimension = 0;
        item.elementCount = 0;
        m_uiDataItems.append(item);
    }

    // Geom（几何数据）
    {
        UIDataItem item;
        item.category = UIDataCategory::Geom;
        item.displayName = QString::fromUtf8("顶点坐标");
        item.dimension = 3;
        item.elementCount = iGame::DynamicCast<iGame::PointSet>(m_uiSampleLeafObj)->GetNumberOfPoints();
        item.isKeyElement = std::vector<bool>(item.elementCount, false);
        m_uiDataItems.append(item);
    }

    // Attributes（属性数据）
    const int attrCount = m_uiSampleLeafObj->GetAttributeSet()->GetNumberOfAttributes();
    for (int i = 0; i < attrCount; i++)
    {
        auto attr = m_uiSampleLeafObj->GetAttributeSet()->GetAttribute(i);
        UIDataItem item;
        item.category = UIDataCategory::Attr;
        item.attrIndex = i;
        item.displayName = QString::fromStdString(attr.pointer->GetName());
        item.dimension = attr.pointer->GetDimension();
        item.elementCount = attr.pointer->GetNumberOfElements();
        item.isKeyElement = std::vector<bool>(item.elementCount, false);
        m_uiDataItems.append(item);
    }
}

void igQtMeshCodecDialog::InitAttributeList()
{
    // 清除属性列表
    ui->combo_boxFloatSelect->clear();

    for (int i = 0; i < m_uiDataItems.size(); ++i)
    {
        UIDataItem& item = m_uiDataItems[i];
        // 初始按参数的 errorMode 加前缀；默认均为无损
        QString mark = GetModeMark(item.errorMode);
        QString displayText = mark + " " + item.displayName;

        // 将 UIDataItem 指针绑定到 ComboBox 项
        ui->combo_boxFloatSelect->addItem(displayText, QVariant::fromValue(&item));
    }

    if (!m_uiDataItems.empty()) {
        ui->combo_boxFloatSelect->setCurrentIndex(0);
    }
}

UIDataItem* igQtMeshCodecDialog::GetCurrentDataItem()
{
    QVariant data = ui->combo_boxFloatSelect->currentData();
    if (data.isValid() && data.canConvert<UIDataItem*>()) {
        return data.value<UIDataItem*>();
    }
    return nullptr;
}

const UIDataItem* igQtMeshCodecDialog::GetCurrentDataItem() const
{
    QVariant data = ui->combo_boxFloatSelect->currentData();
    if (data.isValid() && data.canConvert<UIDataItem*>()) {
        return data.value<UIDataItem*>();
    }
    return nullptr;
}

void igQtMeshCodecDialog::on_combo_boxFloatSelect_currentIndexChanged(int)
{
    UIDataItem* item = GetCurrentDataItem();
    if (!item)
        return;

    // 阻塞信号以避免在加载时触发change事件
    ui->comboBox_globalLevel->blockSignals(true);
    ui->comboBox_criticalLevel->blockSignals(true);
    ui->comboBox_normalLevel->blockSignals(true);

    // 设置三个量化等级ComboBox的当前值
    // 全局量化等级：ComboBox从索引1开始（跳过"无损"），所以需要-1映射
    if (item->globalQuantizeLevel > 0) {
        ui->comboBox_globalLevel->setCurrentIndex(item->globalQuantizeLevel - 1);
    } else {
        ui->comboBox_globalLevel->setCurrentIndex(0); // 默认FP24
    }
    ui->comboBox_criticalLevel->setCurrentIndex(item->criticalQuantizeLevel);
    ui->comboBox_normalLevel->setCurrentIndex(item->normalQuantizeLevel);

    // 恢复信号
    ui->comboBox_globalLevel->blockSignals(false);
    ui->comboBox_criticalLevel->blockSignals(false);
    ui->comboBox_normalLevel->blockSignals(false);

    SetRadiosFromErrorMode(item->errorMode);

    // 多块数据：不显示直方图/关键区域相关选项
    if (m_isMultiBlock) {
        ui->radio_areaModel->setEnabled(false);
        if (ui->radio_areaModel->isChecked()) {
            ui->radio_losslessMode->setChecked(true);
        }
        UpdateKeyAreaVisibility(false);
        HideRefreshButton();
        return;
    }

    // 当选择"全部数据"时，禁用分区量化选项
    if (item->isAllData()) {
        ui->radio_areaModel->setEnabled(false);
        // 如果当前选中的是分区量化，切换到全局量化模式
        if (ui->radio_areaModel->isChecked()) {
            ui->radio_globalMode->setChecked(true);
        }
        // 隐藏直方图相关功能并收缩（内部已清理图表与复选框）
        UpdateKeyAreaVisibility(false);
        RecomputeDialogSize();
    } else {
        // 选择其他数据项时，启用分区量化选项
        ui->radio_areaModel->setEnabled(true);
        LoadAttrFeatureWidget();
        // 根据当前单选状态决定是否显示关键区域分组
        UpdateKeyAreaVisibility(ui->radio_areaModel->isChecked());
    }
    UpdateRefreshButtonStateForCurrent();
}

QString igQtMeshCodecDialog::GetModeMark(iGame::QuantizeMode mode) const
{
    switch (mode) {
    case iGame::QuantizeMode::None:    return m_modeMark[0];
    case iGame::QuantizeMode::Default: return m_modeMark[1];
    case iGame::QuantizeMode::KeyArea: return m_modeMark[2];
    default:                        return m_modeMark[0];
    }
}

void igQtMeshCodecDialog::RefreshComboItemMark(int dataIndex)
{
    if (dataIndex < 0 || dataIndex >= m_uiDataItems.size()) return;
    if (!ui->combo_boxFloatSelect) return;
    const UIDataItem& item = m_uiDataItems[dataIndex];
    QString mark = GetModeMark(item.errorMode);
    ui->combo_boxFloatSelect->setItemText(dataIndex, mark + " " + item.displayName);
}

void igQtMeshCodecDialog::RefreshAllComboItemMarks()
{
    if (!ui->combo_boxFloatSelect) return;
    for (int i = 0; i < m_uiDataItems.size(); ++i) {
        RefreshComboItemMark(i);
    }
}

void igQtMeshCodecDialog::LoadAttrFeatureWidget()
{
    if (m_isMultiBlock) {
        return;
    }

    int idx = ui->combo_boxFloatSelect->currentIndex();
    if (idx < 0 || idx >= m_uiDataItems.size())
        return;

    UIDataItem& item = m_uiDataItems[idx];

    // 检查是否有直方图数据
    if (m_attrFeatureDatas[idx].genStatus == FeatureHistoGenStatus::Yes) {
        HideRefreshButton();
        // 使用缓存的 x/y 数据重绘
        ClearCurrentHistogram();
        const auto& x = m_attrFeatureDatas[idx].xAxis;
        const auto& y = m_attrFeatureDatas[idx].yAxis;
        if (!x.empty() && !y.empty()) {
            QChart* chart = new QChart();
            DrawFeatureHistogramFromData(chart, x, y);
            if (m_chartView) {
                m_chartView->setChart(chart);
            }
        }
        LoadAllCheckBoxes();
        // 同步复选状态到关键元素掩码
        ApplyCheckStatusToKeyElements(idx);
        
        // 已有直方图数据，启用关键/非关键区域量化等级
        SetComboAndLabelEnabled(ui->comboBox_criticalLevel, ui->label_critical, true);
        SetComboAndLabelEnabled(ui->comboBox_normalLevel, ui->label_normal, true);
    }
    else {
        // 没有数据，清空直方图并隐藏复选框
        ClearCurrentHistogram();
        HideAllCheckBoxes();
        
        // 没有直方图数据，禁用关键/非关键区域量化等级
        SetComboAndLabelEnabled(ui->comboBox_criticalLevel, ui->label_critical, false);
        SetComboAndLabelEnabled(ui->comboBox_normalLevel, ui->label_normal, false);
        
        // 如果当前是分区量化模式，显示刷新按钮
        if (item.errorMode == iGame::QuantizeMode::KeyArea) {
            ShowRefreshButton();
        } else {
            HideRefreshButton();
        }
    }
}

/**
 * @brief 初始化梯度直方图视图（不再使用选项卡）
 */
void igQtMeshCodecDialog::InitHistogramView()
{
    // 获取直方图显示区域的父容器（假设UI中有一个容器用于显示直方图）
    // 这里假设UI文件中 groupbox_dataDistGroup 内部有一个布局可以添加控件
    QLayout* parentLayout = ui->groupbox_dataDistGroup->layout();
    if (!parentLayout) {
        parentLayout = new QVBoxLayout(ui->groupbox_dataDistGroup);
        ui->groupbox_dataDistGroup->setLayout(parentLayout);
    }

    // 创建图表视图
    m_chartView = new QChartView(ui->groupbox_dataDistGroup);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(220);
    m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (auto* vlay = qobject_cast<QVBoxLayout*>(parentLayout)) {
        vlay->addWidget(m_chartView, 1);
    } else {
        parentLayout->addWidget(m_chartView);
    }

    // 创建悬浮的产生直方图按钮（作为chartView的子控件）
    m_refreshButton = new QPushButton(m_chartView);
    m_refreshButton->setText("产生量化影响程度直方图");
    {
        QFontMetrics fm(m_refreshButton->font());
        int calcW = fm.horizontalAdvance(m_refreshButton->text()) + 36;
        int minW = 180;
        m_refreshButton->setFixedSize(std::max(minW, calcW), 40);
    }
    m_refreshButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 5px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #888888;"
        "}"
    );
    
    // 连接刷新按钮的点击信号
    connect(m_refreshButton, &QPushButton::clicked, this, &igQtMeshCodecDialog::GenerateHistogram);
    
    // 初始时禁用并隐藏按钮（位置将在showEvent中设置）
    m_refreshButton->setEnabled(false);
    m_refreshButton->setVisible(false);

    // 创建复选框容器和布局
    m_checkBoxContainer = new QWidget(ui->groupbox_dataDistGroup);
    m_checkBoxContainer->setObjectName("checkBoxContainer");
    m_checkBoxContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QGridLayout* checkBoxLayout = new QGridLayout(m_checkBoxContainer);
    checkBoxLayout->setContentsMargins(75, 5, 40, 5);
    checkBoxLayout->setHorizontalSpacing(0);  // 水平间距设为0
    checkBoxLayout->setVerticalSpacing(0);    // 垂直间距设为0

    for (int i = 0; i < m_binNum; i++) {
        checkBoxLayout->setColumnStretch(i, 1);
        QCheckBox* checkBox = new QCheckBox(m_checkBoxContainer);
        checkBox->setText("");
        checkBoxLayout->addWidget(checkBox, 0, i, Qt::AlignCenter);
        m_checkBoxes.append(checkBox);

        connect(checkBox, &QCheckBox::stateChanged, this, [this, i](int state) {
            onCheckBoxStateChanged(i, state);  // 传递索引和状态
        });
    }

    parentLayout->addWidget(m_checkBoxContainer);

    ApplyKeyAreaDarkStyle(ui->groupbox_dataDistGroup, m_chartView, m_checkBoxContainer);

    auto* emptyChart = new QChart();
    ApplyDarkChartShell(emptyChart);
    m_chartView->setChart(emptyChart);

    // 初始时隐藏复选框容器
    m_checkBoxContainer->setVisible(false);

    // 直接绑定到 UI 中的堆叠控件
    m_keyAreaStack = ui->stack_keyArea;
    if (m_keyAreaStack) {
        // page 0 是空页；page 1 是 groupbox_dataDistGroup
        m_keyAreaEmptyPage = ui->page_empty;
        m_keyAreaStack->setCurrentIndex(0);
    }
}

void igQtMeshCodecDialog::InitAttrFeatureDatas()
{
    // 为每个属性数据初始化梯度特征数据
    for (int i = 0; i < m_uiDataItems.size(); i++)
    {
        AttrFeatureData data;
        data.checkStatus = std::vector<bool>(m_binNum, false);
        data.genStatus = FeatureHistoGenStatus::No;

        m_attrFeatureDatas.push_back(data);
    }
}

void igQtMeshCodecDialog::onCheckBoxStateChanged(int binIndex, int state)
{
    int idx = ui->combo_boxFloatSelect->currentIndex();
    
    if (idx < 0 || idx >= m_uiDataItems.size())
        return;

    // 防御：若分箱尚未准备好或索引超界，直接返回
    if (binIndex < 0 || binIndex >= m_binNum)
        return;
    if (m_attrFeatureDatas.size() <= idx)
        return;
    if (m_attrFeatureDatas[idx].idInBins.size() < static_cast<size_t>(m_binNum))
        return;

    bool check = m_checkBoxes[binIndex]->isChecked();
    m_attrFeatureDatas[idx].checkStatus[binIndex] = check;

    // 更新关键元素标记
    const auto &binVec = m_attrFeatureDatas[idx].idInBins[binIndex];
    auto& keyMask = m_uiDataItems[idx].isKeyElement;
    
    for (auto i : binVec)
    {
        if (i >= 0 && i < static_cast<igIndex>(keyMask.size())) {
            keyMask[i] = check;
        }
    }
}

void igQtMeshCodecDialog::ClearCurrentHistogram() {
    if (m_chartView) {
        auto* chart = new QChart();
        ApplyDarkChartShell(chart);
        m_chartView->setChart(chart);
    }
}

void igQtMeshCodecDialog::HideAllCheckBoxes() {
    if (m_checkBoxContainer) {
        m_checkBoxContainer->setVisible(false);
    }
}

void igQtMeshCodecDialog::DisableAllCheckBoxes()
{
    if (m_checkBoxContainer) {
        m_checkBoxContainer->setEnabled(false);
    }
}

void igQtMeshCodecDialog::EnableAllCheckBoxes()
{
    if (m_checkBoxContainer) {
        m_checkBoxContainer->setEnabled(true);
    }
}

void igQtMeshCodecDialog::LoadAllCheckBoxes() {
    int idx = ui->combo_boxFloatSelect->currentIndex();
    if (idx < 0 || idx >= m_uiDataItems.size())
        return;

    AttrFeatureData& data = m_attrFeatureDatas[idx];
    if (m_checkBoxContainer) {
        m_checkBoxContainer->setVisible(true);
    }
    // 均匀分箱：复选框列采用统一伸展
    for (int c = 0; c < m_binNum; ++c) {
        if (auto grid = qobject_cast<QGridLayout*>(m_checkBoxContainer->layout())) {
            grid->setColumnStretch(c, 1);
        }
    }

    for (int i = 0; i < data.checkStatus.size() && i < m_checkBoxes.size(); i++)
    {
        m_checkBoxes[i]->blockSignals(true);
        m_checkBoxes[i]->setChecked(data.checkStatus[i]);
        m_checkBoxes[i]->blockSignals(false);
    }
}

// 旧的输入验证相关代码已移除

void igQtMeshCodecDialog::GenerateHistogram()
{
    if (m_isMultiBlock) {
        return;
    }

    int idx = ui->combo_boxFloatSelect->currentIndex();
    if (idx < 0 || idx >= m_uiDataItems.size())
        return;

    // 隐藏刷新按钮
    HideRefreshButton();

    // 若未生成过则计算并缓存 x/y，之后按需重绘
    if (m_attrFeatureDatas[idx].genStatus != FeatureHistoGenStatus::Yes) {
        QChart* chart = new QChart();
        // 生成并绘制；在 DrawFeatureHistogram 内部会通过 CalFeatureHistogram 计算 x/y
        DrawFeatureHistogram(chart);
        if (m_chartView) m_chartView->setChart(chart);
        m_attrFeatureDatas[idx].genStatus = FeatureHistoGenStatus::Yes;
    } else {
        // 已经有缓存 x/y，直接重绘
        const auto& x = m_attrFeatureDatas[idx].xAxis;
        const auto& y = m_attrFeatureDatas[idx].yAxis;
        QChart* chart = new QChart();
        DrawFeatureHistogramFromData(chart, x, y);
        if (m_chartView) m_chartView->setChart(chart);
    }
    LoadAllCheckBoxes();
    // 同步复选状态到关键元素掩码
    ApplyCheckStatusToKeyElements(idx);
    
    // 直方图绘制完成后，启用关键/非关键区域量化等级
    SetComboAndLabelEnabled(ui->comboBox_criticalLevel, ui->label_critical, true);
    SetComboAndLabelEnabled(ui->comboBox_normalLevel, ui->label_normal, true);
}

void igQtMeshCodecDialog::ShowRefreshButton()
{
    if (m_refreshButton && m_chartView) {
        PositionRefreshButton();
        m_refreshButton->setEnabled(true);
        m_refreshButton->setVisible(true);
        m_refreshButton->raise(); // 确保按钮在最上层
    }
}

void igQtMeshCodecDialog::HideRefreshButton()
{
    if (m_refreshButton) {
        m_refreshButton->setEnabled(false);
        m_refreshButton->setVisible(false);
    }
}

void igQtMeshCodecDialog::PositionRefreshButton()
{
    if (m_refreshButton && m_chartView) {
        // 将按钮居中放置在chartView中
        int x = (m_chartView->width() - m_refreshButton->width()) / 2;
        int y = (m_chartView->height() - m_refreshButton->height()) / 2;
        m_refreshButton->move(x, y);
    }
}

void igQtMeshCodecDialog::resizeEvent(QResizeEvent* event)
{
    igQtChromeFramelessDialog::resizeEvent(event);
    // 窗口大小改变时重新定位按钮
    PositionRefreshButton();
}

void igQtMeshCodecDialog::showEvent(QShowEvent* event)
{
    igQtChromeFramelessDialog::showEvent(event);
    // 窗口首次显示时定位按钮
    PositionRefreshButton();
}

// 应用设置到全体数据或当前数据的辅助函数
template<typename Func>
void igQtMeshCodecDialog::ApplySettingToData(Func setter)
{
    int idx = ui->combo_boxFloatSelect->currentIndex();
    if (idx < 0 || idx >= m_uiDataItems.size())
        return;
    
    UIDataItem* currentItem = &m_uiDataItems[idx];
    
    // 如果当前选择的是"全体数据"，则应用到所有数据项
    if (currentItem->isAllData()) {
        for (int i = 0; i < m_uiDataItems.size(); i++) {
            setter(m_uiDataItems[i]);
        }
    } else {
        // 否则只修改当前数据项
        setter(*currentItem);
    }
}

void igQtMeshCodecDialog::on_radio_losslessMode_toggled(bool checked)
{
    if (checked) {
        // 无损：全部禁用
        ApplyModeUI(true, false, false);

        // 禁用并隐藏关键区域选择板块（内部已隐藏刷新按钮/复选框/图表）
        ui->groupbox_dataDistGroup->setEnabled(false);
        UpdateKeyAreaVisibility(false);

        ApplySettingToData([&](UIDataItem& item) {
            item.errorMode = iGame::QuantizeMode::None;
        });
        // 更新Combo前缀
        UIDataItem* item = GetCurrentDataItem();
        if (item && item->isAllData()) RefreshAllComboItemMarks(); else RefreshComboItemMark(ui->combo_boxFloatSelect->currentIndex());
        RecomputeDialogSize();
    }
}

void igQtMeshCodecDialog::on_radio_globalMode_toggled(bool checked)
{
    if (checked) {
        // 全局：只启用全局量化
        ApplyModeUI(false, true, false);

        // 禁用并隐藏关键区域选择板块（内部已隐藏刷新按钮/复选框/图表）
        ui->groupbox_dataDistGroup->setEnabled(false);
        UpdateKeyAreaVisibility(false);

        ApplySettingToData([&](UIDataItem& item) {
            item.errorMode = iGame::QuantizeMode::Default;
        });
        // 更新Combo前缀
        UIDataItem* item = GetCurrentDataItem();
        if (item && item->isAllData()) RefreshAllComboItemMarks(); else RefreshComboItemMark(ui->combo_boxFloatSelect->currentIndex());
        RecomputeDialogSize();
    }
}

void igQtMeshCodecDialog::on_radio_areaModel_toggled(bool checked)
{
    if (m_isMultiBlock) {
        if (checked) {
            ui->radio_areaModel->setChecked(false);
            ui->radio_losslessMode->setChecked(true);
        }
        return;
    }

    if (checked) {
        // 分区：启用关键/非关键两个量化
        ApplyModeUI(false, false, true);

        // 先写入模式参数，再显示区域并恢复直方图
        ApplySettingToData([&](UIDataItem& item) {
            item.errorMode = iGame::QuantizeMode::KeyArea;
        });
        // 更新Combo前缀
        UIDataItem* item = GetCurrentDataItem();
        if (item && item->isAllData()) RefreshAllComboItemMarks(); else RefreshComboItemMark(ui->combo_boxFloatSelect->currentIndex());

        // 启用并显示关键区域选择板块
        ui->groupbox_dataDistGroup->setEnabled(true);
        UpdateKeyAreaVisibility(true);

        // 若已有直方图，恢复显示；否则根据状态显示“产生直方图”按钮
        LoadAttrFeatureWidget();

        EnableAllCheckBoxes();
        UpdateRefreshButtonStateForCurrent();
    }
}

// 旧的文本框事件处理已废弃

// ComboBox量化等级选择事件处理函数
void igQtMeshCodecDialog::on_comboBox_globalLevel_currentIndexChanged(int index)
{
    ApplySettingToData([index](UIDataItem& item) {
        item.globalQuantizeLevel = index + 1;
    });
}

void igQtMeshCodecDialog::on_comboBox_criticalLevel_currentIndexChanged(int index)
{
    ApplySettingToData([index](UIDataItem& item) {
        item.criticalQuantizeLevel = index;
    });
}

void igQtMeshCodecDialog::on_comboBox_normalLevel_currentIndexChanged(int index)
{
    ApplySettingToData([index](UIDataItem& item) {
        item.normalQuantizeLevel = index;
    });
}

void igQtMeshCodecDialog::on_checkbox_showReport_stateChanged(int state)
{
    m_showReport = ui->checkbox_showReport->isChecked();
}

void igQtMeshCodecDialog::on_comboBox_compressLevel_currentIndexChanged(int index)
{
    m_compressLevel = index;
}

void igQtMeshCodecDialog::DrawFeatureHistogram(QChart* chart)
{
    std::vector<float> xAxis;
    std::vector<int> yAxis;

    CalFeatureHistogram(xAxis, yAxis);

    // 缓存 x/y 数据供切换复用
    int idx = ui->combo_boxFloatSelect->currentIndex();
    if (idx >= 0 && idx < m_uiDataItems.size()) {
        m_attrFeatureDatas[idx].xAxis = xAxis;
        m_attrFeatureDatas[idx].yAxis = yAxis;
    }

    // 若为空数据，直接清空并返回，避免 front()/back() 越界
    if (xAxis.empty() || yAxis.empty()) {
        chart->removeAllSeries();
        foreach(QAbstractAxis * axis, chart->axes()) {
            chart->removeAxis(axis);
        }
        ApplyDarkChartShell(chart);
        return;
    }

    chart->removeAllSeries();
    foreach(QAbstractAxis * axis, chart->axes()) {
        chart->removeAxis(axis);
    }

    QValueAxis* axisX = new QValueAxis();
    QValueAxis* axisY = new QValueAxis();

    // 轴范围健壮性：若前后相等，增加微小扩展
    float minX = xAxis.front();
    float maxX = xAxis.back();
    if (minX == maxX) { minX -= 1e-6f; maxX += 1e-6f; }
    const float scale = 100.0f;
    axisX->setRange(minX * scale, maxX * scale);
    axisX->setTickCount(static_cast<int>(xAxis.size()));
    axisX->setLabelFormat("%.0f%%");
    axisX->setLabelsAngle(70);
    chart->addAxis(axisX, Qt::AlignBottom);

    int maxY = *std::max_element(yAxis.begin(), yAxis.end());
    double upperY = (maxY > 0) ? (maxY * 1.05) : 1.0; // 避免 Y 轴上下界相等
    axisY->setRange(0, upperY);
    axisY->setTickCount(5);
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);
    StyleChartAxes(axisX, axisY);

    for (size_t i = 0; i < yAxis.size(); i++) {
        QLineSeries* lowerLine = new QLineSeries();
        QLineSeries* upperLine = new QLineSeries();
        QAreaSeries* barSeries = new QAreaSeries();

        float x1 = xAxis[i] * scale;
        float x2 = xAxis[i + 1] * scale;

        *lowerLine << QPointF(x1, 0) << QPointF(x2, 0);
        *upperLine << QPointF(x1, yAxis[i]) << QPointF(x2, yAxis[i]);

        barSeries->setLowerSeries(lowerLine);
        barSeries->setUpperSeries(upperLine);

        barSeries->setColor(kChartSeriesFill);
        barSeries->setBorderColor(kChartSeriesBorder);

        chart->addSeries(barSeries);
        barSeries->attachAxis(axisX);
        barSeries->attachAxis(axisY);
    }

    ApplyDarkChartShell(chart);
}

void igQtMeshCodecDialog::DrawFeatureHistogramFromData(QChart* chart, const std::vector<float>& xAxis, const std::vector<int>& yAxis)
{
    if (!chart || xAxis.empty() || yAxis.empty()) return;

    chart->removeAllSeries();
    foreach(QAbstractAxis * axis, chart->axes()) {
        chart->removeAxis(axis);
    }

    QValueAxis* axisX = new QValueAxis();
    QValueAxis* axisY = new QValueAxis();

    float minX = xAxis.front();
    float maxX = xAxis.back();
    if (minX == maxX) { minX -= 1e-6f; maxX += 1e-6f; }
    const float scale = 100.0f;
    axisX->setRange(minX * scale, maxX * scale);
    axisX->setTickCount(static_cast<int>(xAxis.size()));
    axisX->setLabelFormat("%.0f%%");
    axisX->setLabelsAngle(70);
    chart->addAxis(axisX, Qt::AlignBottom);

    int maxY = *std::max_element(yAxis.begin(), yAxis.end());
    double upperY = (maxY > 0) ? (maxY * 1.05) : 1.0;
    axisY->setRange(0, upperY);
    axisY->setTickCount(5);
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);
    StyleChartAxes(axisX, axisY);

    for (size_t i = 0; i < yAxis.size(); i++) {
        QLineSeries* lowerLine = new QLineSeries();
        QLineSeries* upperLine = new QLineSeries();
        QAreaSeries* barSeries = new QAreaSeries();

        float x1 = xAxis[i] * scale;
        float x2 = xAxis[i + 1] * scale;

        *lowerLine << QPointF(x1, 0) << QPointF(x2, 0);
        *upperLine << QPointF(x1, yAxis[i]) << QPointF(x2, yAxis[i]);

        barSeries->setLowerSeries(lowerLine);
        barSeries->setUpperSeries(upperLine);

        barSeries->setColor(kChartSeriesFill);
        barSeries->setBorderColor(kChartSeriesBorder);

        chart->addSeries(barSeries);
        barSeries->attachAxis(axisX);
        barSeries->attachAxis(axisY);
    }

    ApplyDarkChartShell(chart);
}

// 旧实现：均匀分箱，无需按 bin 宽度动态伸展


void igQtMeshCodecDialog::CalFeatureHistogram(std::vector<float>& xAxis, std::vector<int>& yAxis)
{
    int idx = ui->combo_boxFloatSelect->currentIndex();

    if (idx < 0 || idx >= m_uiDataItems.size())
        return;

    const UIDataItem& item = m_uiDataItems[idx];

    // "全部数据"不参与特征直方图计算，防御性短路
    if (item.isAllData()) {
        xAxis.clear();
        yAxis.clear();
        return;
    }

    // MeshCodecFeature 索引与 UI 索引一致：1=几何，2+=属性
    // UI索引：0=全部数据，1=几何，2+=属性
    iGame::MeshCodecFeature featureExtractor(this->m_uiSampleLeafObj, idx);
    std::vector<float> norms;

    // 旧逻辑：使用梯度 Frobenius 范数并进行对数缩放
    // std::vector<std::vector<std::array<float, 3>>> result = featureExtractor->GetDataPointGradient();
    // FrobeniusNorm(result, norms);
    // for (auto& v : norms) {
    //     if (v > 0.0f) v = std::log10(v);
    //     else v = std::numeric_limits<float>::lowest();
    // }

    // 新逻辑：使用 Verificarlo 归一化后的拉普拉斯平均误差（容差）作为特征
    norms = featureExtractor.GetDataPointTolerance(3);
    // 防御性截断到 [0,1]
    for (auto& v : norms) {
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
    }

    // 空数据防御：无元素则返回空轴，避免后续 min/max 未定义
    if (norms.empty()) {
        xAxis.clear();
        yAxis.clear();
        return;
    }

    // Find min and max values
    float minVal = *std::min_element(norms.begin(), norms.end());
    float maxVal = *std::max_element(norms.begin(), norms.end());

    // 对归一化误差，强制直方图覆盖 [0,1]，使得 bin 对应 0.0~0.1, 0.1~0.2 等区间
    minVal = 0.0f;
    maxVal = 1.0f;

    // Handle case where all values are the same（单柱方案，避免越界）
    if (minVal == maxVal) {
        // 初始化为 m_binNum 等宽的极小范围，放入第0个 bin
        const float eps = 1e-6f;
        xAxis.resize(m_binNum + 1);
        float start = minVal - (m_binNum / 2.0f) * (eps / std::max(1, m_binNum));
        for (int i = 0; i <= m_binNum; ++i) {
            xAxis[i] = start + i * (eps / std::max(1, m_binNum));
        }
        yAxis.assign(m_binNum, 0);
        yAxis[0] = static_cast<int>(norms.size());

        // 分箱索引：全部元素归入第0个 bin，避免勾选越界
        m_attrFeatureDatas[idx].idInBins.assign(m_binNum, {});
        for (igIndex id = 0; id < static_cast<igIndex>(norms.size()); ++id) {
            m_attrFeatureDatas[idx].idInBins[0].push_back(id);
        }
        // 根据当前复选状态应用到关键元素掩码
        ApplyCheckStatusToKeyElements(idx);
    }
    else
    {
        // 均匀分箱
        float binWidth = (maxVal - minVal) / m_binNum;
        xAxis.resize(m_binNum + 1);
        yAxis.assign(m_binNum, 0);
        for (int i = 0; i < m_binNum; ++i) {
            xAxis[i] = minVal + i * binWidth;
        }
        xAxis[m_binNum] = maxVal;

        m_attrFeatureDatas[idx].idInBins.assign(m_binNum, {});
        for (igIndex id = 0; id < norms.size(); id++) {
            float norm = norms[id];
            if (norm < minVal || norm > maxVal) continue;
            int binIndex = std::min(static_cast<int>((norm - minVal) / binWidth), m_binNum - 1);
            yAxis[binIndex]++;
            m_attrFeatureDatas[idx].idInBins[binIndex].push_back(id);
        }
        // 根据当前复选状态应用到关键元素掩码
        ApplyCheckStatusToKeyElements(idx);
    }
}



// for gradient
void igQtMeshCodecDialog::FrobeniusNorm(
    const std::vector<std::vector<std::array<float, 3>>>& gradient, std::vector<float>& result) {
    for (const auto& jacobian : gradient)
    {
        float sumSquared = 0.0f;
        for (const auto& row : jacobian) {
            for (float val : row) {
                sumSquared += val * val;
            }
        }
        result.push_back(std::sqrt(sumSquared));
    }
}

// 保留谱范数的实现仅在需要时使用

// for laplacian & vortex
void igQtMeshCodecDialog::L2Norm(
    const std::vector<std::vector<float>>& datas, std::vector<float>& result)
{
    int pointNum = datas.size();

    for (int i = 0; i < pointNum; ++i) {
        float sumSquared = 0.0f;
        for (float val : datas[i]) {
            sumSquared += val * val;
        }
        result.push_back(std::sqrt(sumSquared));
    }
}

void igQtMeshCodecDialog::on_btnStartCompress_clicked()
{
    if (!m_exportSourceObj) {
        QMessageBox::critical(this, "错误", "无可用数据对象，无法压缩！");
        return;
    }

    const bool isMultiBlock = m_exportSourceObj->HasSubDataObject();
    const QString filter = isMultiBlock ? "Compression Manifest(*.igcm)" : "Compress Mesh(*.igc)";

    std::string saveFilePath =
        QFileDialog::getSaveFileName(nullptr, "Compress file as ", "", filter)
        .toStdString();
    if (saveFilePath.empty()) {
        igDebug("Could not save file with error file path\n");
        return;
    }

    {
        const std::string expectExt = isMultiBlock ? ".igcm" : ".igc";
        std::filesystem::path outPath(saveFilePath);
        if (outPath.extension().string() != expectExt) {
            outPath.replace_extension(expectExt);
            saveFilePath = outPath.string();
        }
    }

    // 使用桥接器将 UI 模型转换为编码器参数
    iGame::CodecControlParams codecParams = BuildCodecParams();

    bool result = false;
    if (isMultiBlock) {
        // 注意：多帧序列在当前帧也会表现为“多块”（subdataobj），不能仅凭 HasSubDataObject() 判断。
        iGame::DataObject::Pointer rootObj = m_exportSourceObj;
        if (m_exportSourceObj) {
            auto* parent = m_exportSourceObj->FindParent();
            if (parent && parent != m_exportSourceObj.get()) {
                rootObj = iGame::DataObject::Pointer(parent);
            }
        }
        auto timeFrames = rootObj ? rootObj->PeekTimeFrames() : nullptr;
        const bool isTimeSeries = (timeFrames && timeFrames->GetTimeNum() > 1);

        std::vector<std::pair<std::string, std::string>> report;
        if (isTimeSeries) {
            auto writer = iGame::IGCMTimeSeriesWriter::New();
            writer->SetCodecControlParams(codecParams);
            result = writer->WriteToFile(rootObj, saveFilePath);
            if (result) {
                report = writer->GetReport();
            }
        } else {
            auto writer = iGame::IGCMWriter::New();
            writer->SetCodecControlParams(codecParams);
            result = writer->WriteToFile(m_exportSourceObj, saveFilePath);
            if (result) {
                report = writer->GetReport();
            }
        }

        if (!result) {
            QMessageBox::critical(this, "错误", "压缩失败！");
            return;
        }

        if (m_showReport) {
            ShowReportDialog(report);
        }
        accept();
        return;
    }
    
    auto writer = iGame::IGCWriter::New();
    writer->SetCodecControlParams(codecParams);
    result = writer->WriteToFile(m_exportSourceObj, saveFilePath);
    
    if (!result) {
        QMessageBox::critical(this, "错误", "压缩失败！");
        return;
    }

    if (m_showReport)
    {
        ShowReportDialog(writer->GetReport());
    }

    accept();
}

void igQtMeshCodecDialog::on_button_cancel_clicked()
{
    // 关闭对话框并返回拒绝结果
    reject();
}

void igQtMeshCodecDialog::ShowReportDialog(const std::vector<std::pair<std::string, std::string>>& report)
{
    // 创建对话框
    QDialog* dialog = new QDialog;
    dialog->setWindowTitle("压缩报告");
    dialog->setMinimumSize(600, 400);

    // 创建表格
    QTableWidget* tableWidget = new QTableWidget(dialog);
    tableWidget->setColumnCount(2);
    tableWidget->setRowCount(report.size());

    // 隐藏表头
    tableWidget->horizontalHeader()->setVisible(false);
    tableWidget->verticalHeader()->setVisible(false);

    // 设置表格调整策略
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    // 填充数据
    int row = 0;
    for (const auto& pair : report)
    {
        tableWidget->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(pair.first)));
        tableWidget->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(pair.second)));
        row++;
    }

    // 布局
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->addWidget(tableWidget);

    dialog->setLayout(layout);

    // 显示对话框（模态）
    dialog->exec();

    // 对话框关闭后自动删除
    dialog->deleteLater();
}

void igQtMeshCodecDialog::UpdateKeyAreaVisibility(bool show)
{
    if (!ui || !ui->groupbox_dataDistGroup) return;

    if (m_isMultiBlock) {
        show = false;
    }

    // 使用 QStackedWidget 切换空页/真实内容，空页高度为0
    if (m_keyAreaStack) {
        if (show) {
            m_keyAreaStack->setCurrentIndex(1);
            m_keyAreaStack->setMaximumHeight(QWIDGETSIZE_MAX);
            m_keyAreaStack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
            ui->groupbox_dataDistGroup->show();
        } else {
            m_keyAreaStack->setCurrentIndex(0);
            // 关键：把堆叠控件本身设为 Fixed 并限制最大高度为0，防止占据剩余空间
            m_keyAreaStack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            m_keyAreaStack->setMaximumHeight(0);
            ui->groupbox_dataDistGroup->hide();
            HideRefreshButton();
            HideAllCheckBoxes();
            ClearCurrentHistogram();
            // 防御性：仅禁用复选框容器，保留缓存与分箱索引
            DisableAllCheckBoxes();
        }
        m_keyAreaStack->updateGeometry();
    } else {
        // 兜底
        ui->groupbox_dataDistGroup->setVisible(show);
        ui->groupbox_dataDistGroup->setMaximumHeight(show ? QWIDGETSIZE_MAX : 0);
        ui->groupbox_dataDistGroup->setMinimumHeight(show ? 0 : 0);
    }

    RecomputeDialogSize();
}

void igQtMeshCodecDialog::RecomputeDialogSize()
{
    if (!m_bodyWidget) return;
    if (auto lay = m_bodyWidget->layout()) {
        lay->setSizeConstraint(QLayout::SetMinimumSize);
        lay->invalidate();
        lay->activate();
    }

    QTimer::singleShot(0, this, [this]() {
        const int needH = sizeHint().height();
        setMinimumHeight(qMax(620, needH));
        // 仅在当前高度不足以容纳内容时自动增高，不强制缩小用户已放大的窗口
        if (height() < needH) {
            resize(width(), needH);
        }
    });
}

void igQtMeshCodecDialog::SetComboAndLabelEnabled(QComboBox* combo, QLabel* label, bool enabled)
{
    if (!combo || !label) return;
    combo->setEnabled(enabled);
    label->setEnabled(enabled);
    label->setStyleSheet(enabled ? "" : "QLabel { color: gray; }");
}

void igQtMeshCodecDialog::ApplyModeUI(bool lossless, bool global, bool area)
{
    // lossless: 全部禁用
    if (lossless) {
        SetComboAndLabelEnabled(ui->comboBox_globalLevel, ui->label_global, false);
        SetComboAndLabelEnabled(ui->comboBox_criticalLevel, ui->label_critical, false);
        SetComboAndLabelEnabled(ui->comboBox_normalLevel, ui->label_normal, false);
        return;
    }

    // global: 只启用全局
    if (global) {
        SetComboAndLabelEnabled(ui->comboBox_globalLevel, ui->label_global, true);
        SetComboAndLabelEnabled(ui->comboBox_criticalLevel, ui->label_critical, false);
        SetComboAndLabelEnabled(ui->comboBox_normalLevel, ui->label_normal, false);
        return;
    }

    // area: 关键/非关键初始时禁用，仅在直方图绘制后启用
    if (area) {
        SetComboAndLabelEnabled(ui->comboBox_globalLevel, ui->label_global, false);
        SetComboAndLabelEnabled(ui->comboBox_criticalLevel, ui->label_critical, false);
        SetComboAndLabelEnabled(ui->comboBox_normalLevel, ui->label_normal, false);
    }
}

void igQtMeshCodecDialog::UpdateRefreshButtonStateForCurrent()
{
    if (m_isMultiBlock) {
        HideRefreshButton();
        return;
    }

    int idx = ui->combo_boxFloatSelect->currentIndex();
    if (idx < 0 || idx >= m_uiDataItems.size()) { HideRefreshButton(); return; }

    // 仅在分区模式且当前数据未生成直方图时显示按钮
    if (ui->radio_areaModel->isChecked()) {
        if (m_attrFeatureDatas[idx].genStatus == FeatureHistoGenStatus::No) {
            ShowRefreshButton();
        } else {
            HideRefreshButton();
        }
    } else {
        HideRefreshButton();
    }
}

void igQtMeshCodecDialog::SetRadiosFromErrorMode(iGame::QuantizeMode mode)
{
    // 阻断信号，避免反复触发 UI 逻辑
    QSignalBlocker b1(ui->radio_losslessMode);
    QSignalBlocker b2(ui->radio_globalMode);
    QSignalBlocker b3(ui->radio_areaModel);
    ui->radio_losslessMode->setChecked(mode == iGame::QuantizeMode::None);
    ui->radio_globalMode->setChecked(mode == iGame::QuantizeMode::Default);
    ui->radio_areaModel->setChecked(mode == iGame::QuantizeMode::KeyArea);
}

// 将当前属性的分箱勾选状态应用到关键元素掩码
void igQtMeshCodecDialog::ApplyCheckStatusToKeyElements(int dataIndex)
{
    if (dataIndex < 0 || dataIndex >= m_uiDataItems.size()) return;
    if (m_attrFeatureDatas.size() <= dataIndex) return;

    const auto& bins = m_attrFeatureDatas[dataIndex].idInBins;
    const auto& checks = m_attrFeatureDatas[dataIndex].checkStatus;
    if (bins.size() != checks.size()) return; // 尚未完成分箱

    auto& keyMask = m_uiDataItems[dataIndex].isKeyElement;
    if (keyMask.empty()) return;

    std::fill(keyMask.begin(), keyMask.end(), false);
    for (int b = 0; b < (int)bins.size(); ++b) {
        if (!checks[b]) continue;
        for (auto idx : bins[b]) {
            if (idx >= 0 && idx < (int)keyMask.size()) keyMask[idx] = true;
        }
    }
}

// 桥接器：将 UI 数据模型转换为编码器参数
iGame::CodecControlParams igQtMeshCodecDialog::BuildCodecParams() const
{
    iGame::CodecControlParams params;
    params.showReport = m_showReport;
    params.compressLevel = m_compressLevel + 1;

    // 辅助函数：从 UIDataItem 构建 FloatControlParams
    auto buildControlParams = [](const UIDataItem& item) -> iGame::FloatControlParams {
        iGame::FloatControlParams p;
        p.errorMode = item.errorMode;
        p.globalQuantizeLevel = item.globalQuantizeLevel;
        p.criticalQuantizeLevel = item.criticalQuantizeLevel;
        p.normalQuantizeLevel = item.normalQuantizeLevel;
        p.isKeyElement = item.isKeyElement;
        return p;
    };

    // 遍历 UI 数据项，跳过 AllData（虚拟项）
    for (int i = 0; i < m_uiDataItems.size(); ++i)
    {
        const UIDataItem& item = m_uiDataItems[i];
        
        switch (item.category) {
        case UIDataCategory::AllData:
            // 虚拟项，跳过
            break;
        case UIDataCategory::Geom:
            params.geomControl = buildControlParams(item);
            break;
        case UIDataCategory::Attr:
            params.attrControl.push_back(buildControlParams(item));
            break;
        }
    }

    return params;
}
