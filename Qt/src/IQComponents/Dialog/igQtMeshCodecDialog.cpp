#include "IQComponents/Dialog/igQtMeshCodecDialog.h"
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QRadioButton>
#include <QComboBox>
#include <QFontMetrics>
#include <limits>
#include <cmath>

igQtMeshCodecDialog::igQtMeshCodecDialog(QWidget* parent, iGame::DataObject::Pointer obj) :
    QDialog(parent),
    ui(new Ui::MeshCodecDialog),
    m_dataObj(obj)
{
    ui->setupUi(this);

    // 显示压缩报告选项
    ui->checkbox_showReport->setVisible(true);

    // 让对话框可根据内容自由收缩/展开
    if (this->layout()) this->layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);

    InitUIControlParams();
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
    InitHistogramView();
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
}

bool igQtMeshCodecDialog::IsValidAttrIndex(int dataIndex)
{
    return (dataIndex >= 0 && dataIndex < m_DataNum);
}

void igQtMeshCodecDialog::InitIntro()
{
    ui->label_intro->setText(
        "1. 通过选择数据来为每种数据单独地设置压缩参数\n"
        "2. 当选择数据类型为\"全部数据\"时，不能设置分区量化模式\n"
        "3. 仅当选择\"全部数据\"并修改参数时，才会将当前设置应用到所有数据；仅切换到\"全部数据\"不会影响现有各项参数\n"
        "4. 当量化等级 ≥ 浮点精度时，浮点数将不会被量化"
    );
}

void igQtMeshCodecDialog::InitUIControlParams()
{
    // +2: 一个"全部数据"选项 + 一个"顶点坐标"选项
    for (int i = 0; i < m_dataObj->GetAttributeSet()->GetNumberOfAttributes() + ATTRIBUTE_OFFSET; i++)
    {
        iGame::FloatErrorControlParameters p;
       
        if (i == 0)
        {
            // 第一项：全部数据（特殊选项，用于全局设置）
            p.dataName = m_AllDataName;
            p.dimension = 0;  // 特殊标记，表示这是全局设置
            p.elementCount = 0;
            p.isKeyElement = std::vector<bool>();
        }
        else if (i == 1)
        {
            // 第二项：顶点坐标
            p.dataName = m_GeomName;
            p.dimension = 3;
            p.elementCount = iGame::DynamicCast<iGame::PointSet>(m_dataObj)->GetNumberOfPoints();
            p.isKeyElement = std::vector<bool>(p.elementCount, false);
        }
        else
        {
            // 第三项开始：实际属性数据
            auto attr = m_dataObj->GetAttributeSet()->GetAttribute(i - ATTRIBUTE_OFFSET);
            p.dataName = attr.pointer->GetName();
            p.dimension = attr.pointer->GetDimension();
            p.elementCount = attr.pointer->GetNumberOfElements();
            p.isKeyElement = std::vector<bool>(p.elementCount, false);
        }
        
        p.lossyMode = iGame::LossyMode::MantissaTruncation;
        p.errorMode = iGame::ErrorMode::None;
        p.globalQuantizeLevel = 0;
        p.criticalQuantizeLevel = 0;
        p.normalQuantizeLevel = 0;
        m_params.errorBoundSetting.push_back(p);
    }

    m_DataNum = m_params.errorBoundSetting.size();
    m_params.showReport = false;
}

void igQtMeshCodecDialog::InitAttributeList()
{
    // 清除属性列表
    ui->combo_boxFloatSelect->clear();

    for (int i = 0; i < m_params.errorBoundSetting.size(); ++i)
    {
        auto& info = m_params.errorBoundSetting[i];
        // 初始按参数的 errorMode 加前缀；默认均为无损
        QString mark = m_modeMark[0];
        if (info.errorMode == iGame::ErrorMode::Default) mark = m_modeMark[1];
        else if (info.errorMode == iGame::ErrorMode::KeyArea) mark = m_modeMark[2];
        ui->combo_boxFloatSelect->addItem(mark + " " + QString::fromStdString(info.dataName));
    }
    if (!m_params.errorBoundSetting.empty()) {
        ui->combo_boxFloatSelect->setCurrentIndex(0);
    }
}

int igQtMeshCodecDialog::GetCurrentDataIndex() const
{
    return ui->combo_boxFloatSelect->currentIndex();
}

void igQtMeshCodecDialog::on_combo_boxFloatSelect_currentIndexChanged(int dataIndex)
{
    if (!IsValidAttrIndex(dataIndex))
        return;

    // 更新参数中的属性名称
    const auto& errorBoundSetting = m_params.errorBoundSetting[dataIndex];

    // 阻塞信号以避免在加载时触发change事件
    ui->comboBox_globalLevel->blockSignals(true);
    ui->comboBox_criticalLevel->blockSignals(true);
    ui->comboBox_normalLevel->blockSignals(true);

    // 设置三个量化等级ComboBox的当前值
    // 全局量化等级：ComboBox从索引1开始（跳过"无损"），所以需要-1映射
    if (errorBoundSetting.globalQuantizeLevel > 0) {
        ui->comboBox_globalLevel->setCurrentIndex(errorBoundSetting.globalQuantizeLevel - 1);
    } else {
        ui->comboBox_globalLevel->setCurrentIndex(0); // 默认FP24
    }
    ui->comboBox_criticalLevel->setCurrentIndex(errorBoundSetting.criticalQuantizeLevel);
    ui->comboBox_normalLevel->setCurrentIndex(errorBoundSetting.normalQuantizeLevel);

    // 恢复信号
    ui->comboBox_globalLevel->blockSignals(false);
    ui->comboBox_criticalLevel->blockSignals(false);
    ui->comboBox_normalLevel->blockSignals(false);

    SetRadiosFromErrorMode(errorBoundSetting.errorMode);

    // 当选择"全部数据"（索引0）时，禁用分区量化选项
    if (dataIndex == 0) {
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

QString igQtMeshCodecDialog::GetModeMark(iGame::ErrorMode mode) const
{
    switch (mode) {
    case iGame::ErrorMode::None:    return m_modeMark[0];
    case iGame::ErrorMode::Default: return m_modeMark[1];
    case iGame::ErrorMode::KeyArea: return m_modeMark[2];
    default:                        return m_modeMark[0];
    }
}

void igQtMeshCodecDialog::RefreshComboItemMark(int dataIndex)
{
    if (!IsValidAttrIndex(dataIndex)) return;
    if (!ui->combo_boxFloatSelect) return;
    auto mode = m_params.errorBoundSetting[dataIndex].errorMode;
    QString mark = GetModeMark(mode);
    QString name = QString::fromStdString(m_params.errorBoundSetting[dataIndex].dataName);
    ui->combo_boxFloatSelect->setItemText(dataIndex, mark + " " + name);
}

void igQtMeshCodecDialog::RefreshAllComboItemMarks()
{
    if (!ui->combo_boxFloatSelect) return;
    for (int i = 0; i < m_params.errorBoundSetting.size(); ++i) {
        RefreshComboItemMark(i);
    }
}

void igQtMeshCodecDialog::LoadAttrFeatureWidget()
{
    int dataIndex = GetCurrentDataIndex();

    if (!IsValidAttrIndex(dataIndex))
        return;

    // 检查是否有直方图数据
    if (m_attrFeatureDatas[dataIndex].genStatus == FeatureHistoGenStatus::Yes) {
        HideRefreshButton();
        // 使用缓存的 x/y 数据重绘
        ClearCurrentHistogram();
        const auto& x = m_attrFeatureDatas[dataIndex].xAxis;
        const auto& y = m_attrFeatureDatas[dataIndex].yAxis;
        if (!x.empty() && !y.empty()) {
            QChart* chart = new QChart();
            DrawFeatureHistogramFromData(chart, x, y);
            if (m_chartView) {
                m_chartView->setChart(chart);
            }
        }
        LoadAllCheckBoxes();
        // 同步复选状态到关键元素掩码
        ApplyCheckStatusToKeyElements(dataIndex);
        
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
        if (m_params.errorBoundSetting[dataIndex].errorMode == iGame::ErrorMode::KeyArea) {
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
    parentLayout->addWidget(m_chartView);

    // 创建悬浮的产生直方图按钮（作为chartView的子控件）
    m_refreshButton = new QPushButton(m_chartView);
    m_refreshButton->setText("产生梯度范数直方图");
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
    // 放大复选框指示器尺寸，使勾选更易于交互
    m_checkBoxContainer->setStyleSheet(
        "QCheckBox::indicator { width: 18px; height: 18px; }"
    );

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
    for (int i = 0; i < m_DataNum; i++)
    {
        AttrFeatureData data;
        data.checkStatus = std::vector<bool>(m_binNum, false);
        data.genStatus = FeatureHistoGenStatus::No;

        m_attrFeatureDatas.push_back(data);
    }
}

void igQtMeshCodecDialog::onCheckBoxStateChanged(int binIndex, int state)
{
    int dataIndex = GetCurrentDataIndex();
    
    if (!IsValidAttrIndex(dataIndex))
    {
        return;
    }

    // 防御：若分箱尚未准备好或索引超界，直接返回
    if (binIndex < 0 || binIndex >= m_binNum) {
        return;
    }
    if (m_attrFeatureDatas.size() <= dataIndex) {
        return;
    }
    if (m_attrFeatureDatas[dataIndex].idInBins.size() < static_cast<size_t>(m_binNum)) {
        return;
    }

    bool check = m_checkBoxes[binIndex]->isChecked();
    m_attrFeatureDatas[dataIndex].checkStatus[binIndex] = check;

    // 更新关键元素标记
    const auto &binVec = m_attrFeatureDatas[dataIndex].idInBins[binIndex];
    for (auto idx : binVec)
    {
        m_params.errorBoundSetting[dataIndex].isKeyElement[idx] = check;
    }
}

void igQtMeshCodecDialog::ClearCurrentHistogram() {
    if (m_chartView) {
        // 当前绑定的chart会被QChartView接管并删除
        m_chartView->setChart(new QChart());
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
    int dataIndex = GetCurrentDataIndex();

    if (!IsValidAttrIndex(dataIndex))
        return;

    AttrFeatureData& data = m_attrFeatureDatas[dataIndex];
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
    int dataIndex = GetCurrentDataIndex();

    if (dataIndex < 0 || dataIndex >= m_DataNum)
    {
        return;
    }

    // 隐藏刷新按钮
    HideRefreshButton();

    // 若未生成过则计算并缓存 x/y，之后按需重绘
    if (m_attrFeatureDatas[dataIndex].genStatus != FeatureHistoGenStatus::Yes) {
        QChart* chart = new QChart();
        // 生成并绘制；在 DrawFeatureHistogram 内部会通过 CalFeatureHistogram 计算 x/y
        DrawFeatureHistogram(chart);
        if (m_chartView) m_chartView->setChart(chart);
        m_attrFeatureDatas[dataIndex].genStatus = FeatureHistoGenStatus::Yes;
    } else {
        // 已经有缓存 x/y，直接重绘
        const auto& x = m_attrFeatureDatas[dataIndex].xAxis;
        const auto& y = m_attrFeatureDatas[dataIndex].yAxis;
        QChart* chart = new QChart();
        DrawFeatureHistogramFromData(chart, x, y);
        if (m_chartView) m_chartView->setChart(chart);
    }
    LoadAllCheckBoxes();
    // 同步复选状态到关键元素掩码
    ApplyCheckStatusToKeyElements(dataIndex);
    
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
    QDialog::resizeEvent(event);
    // 窗口大小改变时重新定位按钮
    PositionRefreshButton();
}

void igQtMeshCodecDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // 窗口首次显示时定位按钮
    PositionRefreshButton();
}

// 应用设置到全体数据或当前数据的辅助函数
template<typename Func>
void igQtMeshCodecDialog::ApplySettingToData(Func setter)
{
    int dataIndex = GetCurrentDataIndex();
    if (!IsValidAttrIndex(dataIndex)) {
        return;
    }
    
    // 如果当前选择的是"全体数据"（索引0），则应用到所有数据项
    if (dataIndex == 0) {
        for (int i = 0; i < m_DataNum; i++) {
            setter(m_params.errorBoundSetting[i]);
        }
    } else {
        // 否则只修改当前数据项
        setter(m_params.errorBoundSetting[dataIndex]);
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

        ApplySettingToData([&](iGame::FloatErrorControlParameters& setting) {
            setting.errorMode = iGame::ErrorMode::None;
        });
        // 更新Combo前缀
        int idx = GetCurrentDataIndex();
        if (idx == 0) RefreshAllComboItemMarks(); else RefreshComboItemMark(idx);
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

        ApplySettingToData([&](iGame::FloatErrorControlParameters& setting) {
            setting.errorMode = iGame::ErrorMode::Default;
        });
        // 更新Combo前缀
        int idx = GetCurrentDataIndex();
        if (idx == 0) RefreshAllComboItemMarks(); else RefreshComboItemMark(idx);
        RecomputeDialogSize();
    }
}

void igQtMeshCodecDialog::on_radio_areaModel_toggled(bool checked)
{
    if (checked) {
        // 分区：启用关键/非关键两个量化
        ApplyModeUI(false, false, true);

        // 先写入模式参数，再显示区域并恢复直方图
        ApplySettingToData([&](iGame::FloatErrorControlParameters& setting) {
            setting.errorMode = iGame::ErrorMode::KeyArea;
        });
        // 更新Combo前缀
        int idx = GetCurrentDataIndex();
        if (idx == 0) RefreshAllComboItemMarks(); else RefreshComboItemMark(idx);

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
    ApplySettingToData([index](iGame::FloatErrorControlParameters& setting) {
        setting.globalQuantizeLevel = index + 1;
    });
}

void igQtMeshCodecDialog::on_comboBox_criticalLevel_currentIndexChanged(int index)
{
    ApplySettingToData([index](iGame::FloatErrorControlParameters& setting) {
        setting.criticalQuantizeLevel = index;
    });
}

void igQtMeshCodecDialog::on_comboBox_normalLevel_currentIndexChanged(int index)
{
    ApplySettingToData([index](iGame::FloatErrorControlParameters& setting) {
        setting.normalQuantizeLevel = index;
    });
}

void igQtMeshCodecDialog::on_checkbox_showReport_stateChanged(int state)
{
    m_params.showReport = ui->checkbox_showReport->isChecked();
}

void igQtMeshCodecDialog::DrawFeatureHistogram(QChart* chart)
{
    std::vector<float> xAxis;
    std::vector<int> yAxis;

    CalFeatureHistogram(xAxis, yAxis);

    // 缓存 x/y 数据供切换复用
    int idx = GetCurrentDataIndex();
    if (IsValidAttrIndex(idx)) {
        m_attrFeatureDatas[idx].xAxis = xAxis;
        m_attrFeatureDatas[idx].yAxis = yAxis;
    }

    // 若为空数据，直接清空并返回，避免 front()/back() 越界
    if (xAxis.empty() || yAxis.empty()) {
        chart->removeAllSeries();
        foreach(QAbstractAxis * axis, chart->axes()) {
            chart->removeAxis(axis);
        }
        chart->legend()->setVisible(false);
        return;
    }

    chart->removeAllSeries();
    foreach(QAbstractAxis * axis, chart->axes()) {
        chart->removeAxis(axis);
    }
    chart->legend()->setVisible(false);

    QValueAxis* axisX = new QValueAxis();
    QValueAxis* axisY = new QValueAxis();

    // 轴范围健壮性：若前后相等，增加微小扩展
    float minX = xAxis.front();
    float maxX = xAxis.back();
    if (minX == maxX) { minX -= 1e-6f; maxX += 1e-6f; }
    axisX->setRange(minX, maxX);
    axisX->setTickCount(static_cast<int>(xAxis.size()));
    axisX->setLabelFormat("%.3e");
    axisX->setLabelsAngle(70);
    chart->addAxis(axisX, Qt::AlignBottom);

    int maxY = *std::max_element(yAxis.begin(), yAxis.end());
    double upperY = (maxY > 0) ? (maxY * 1.05) : 1.0; // 避免 Y 轴上下界相等
    axisY->setRange(0, upperY);
    axisY->setTickCount(5);
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);

    for (size_t i = 0; i < yAxis.size(); i++) {
        QLineSeries* lowerLine = new QLineSeries();
        QLineSeries* upperLine = new QLineSeries();
        QAreaSeries* barSeries = new QAreaSeries();

        float x1 = xAxis[i];
        float x2 = xAxis[i + 1];

        *lowerLine << QPointF(x1, 0) << QPointF(x2, 0);
        *upperLine << QPointF(x1, yAxis[i]) << QPointF(x2, yAxis[i]);

        barSeries->setLowerSeries(lowerLine);
        barSeries->setUpperSeries(upperLine);

        barSeries->setColor(QColor(0, 114, 189));
        barSeries->setBorderColor(QColor(0, 114, 189));

        chart->addSeries(barSeries);
        barSeries->attachAxis(axisX);
        barSeries->attachAxis(axisY);
    }

    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(Qt::white));
    chart->setPlotAreaBackgroundPen(Qt::NoPen);
}

void igQtMeshCodecDialog::DrawFeatureHistogramFromData(QChart* chart, const std::vector<float>& xAxis, const std::vector<int>& yAxis)
{
    if (!chart || xAxis.empty() || yAxis.empty()) return;

    chart->removeAllSeries();
    foreach(QAbstractAxis * axis, chart->axes()) {
        chart->removeAxis(axis);
    }
    chart->legend()->setVisible(false);

    QValueAxis* axisX = new QValueAxis();
    QValueAxis* axisY = new QValueAxis();

    float minX = xAxis.front();
    float maxX = xAxis.back();
    if (minX == maxX) { minX -= 1e-6f; maxX += 1e-6f; }
    axisX->setRange(minX, maxX);
    axisX->setTickCount(static_cast<int>(xAxis.size()));
    axisX->setLabelFormat("%.3e");
    axisX->setLabelsAngle(70);
    chart->addAxis(axisX, Qt::AlignBottom);

    int maxY = *std::max_element(yAxis.begin(), yAxis.end());
    double upperY = (maxY > 0) ? (maxY * 1.05) : 1.0;
    axisY->setRange(0, upperY);
    axisY->setTickCount(5);
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);

    for (size_t i = 0; i < yAxis.size(); i++) {
        QLineSeries* lowerLine = new QLineSeries();
        QLineSeries* upperLine = new QLineSeries();
        QAreaSeries* barSeries = new QAreaSeries();

        float x1 = xAxis[i];
        float x2 = xAxis[i + 1];

        *lowerLine << QPointF(x1, 0) << QPointF(x2, 0);
        *upperLine << QPointF(x1, yAxis[i]) << QPointF(x2, yAxis[i]);

        barSeries->setLowerSeries(lowerLine);
        barSeries->setUpperSeries(upperLine);

        barSeries->setColor(QColor(0, 114, 189));
        barSeries->setBorderColor(QColor(0, 114, 189));

        chart->addSeries(barSeries);
        barSeries->attachAxis(axisX);
        barSeries->attachAxis(axisY);
    }

    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(Qt::white));
    chart->setPlotAreaBackgroundPen(Qt::NoPen);
}

// 旧实现：均匀分箱，无需按 bin 宽度动态伸展


void igQtMeshCodecDialog::CalFeatureHistogram(std::vector<float>& xAxis, std::vector<int>& yAxis)
{
    int dataIndex = GetCurrentDataIndex();

    if (!IsValidAttrIndex(dataIndex))
        return;

    // 0号为“全部数据”，不参与特征直方图计算，防御性短路
    if (dataIndex == 0) {
        xAxis.clear();
        yAxis.clear();
        return;
    }

    // UI 索引：0=全部数据(不用于特征)，1=几何，2+=属性
    // MeshCodecFeature 期望：1=几何，其他属性用 (uiIndex-1)
    int featIndex = (dataIndex <= 1) ? dataIndex : (dataIndex - 1);
    iGame::MeshCodecFeature* featureExtractor = new iGame::MeshCodecFeature(this->m_dataObj, featIndex);
    std::vector<float> norms;
    
    // 使用梯度 Frobenius 范数并进行对数缩放，参考历史实现
    std::vector<std::vector<std::array<float, 3>>> result = featureExtractor->GetDataPointGradient();
    FrobeniusNorm(result, norms);
    for (auto& v : norms) {
        if (v > 0.0f) v = std::log10(v);
        else v = std::numeric_limits<float>::lowest();
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
        m_attrFeatureDatas[dataIndex].idInBins.assign(m_binNum, {});
        for (igIndex id = 0; id < static_cast<igIndex>(norms.size()); ++id) {
            m_attrFeatureDatas[dataIndex].idInBins[0].push_back(id);
        }
        // 根据当前复选状态应用到关键元素掩码
        ApplyCheckStatusToKeyElements(dataIndex);
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

        m_attrFeatureDatas[dataIndex].idInBins.assign(m_binNum, {});
        for (igIndex id = 0; id < norms.size(); id++) {
            float norm = norms[id];
            if (norm < minVal || norm > maxVal) continue;
            int binIndex = std::min(static_cast<int>((norm - minVal) / binWidth), m_binNum - 1);
            yAxis[binIndex]++;
            m_attrFeatureDatas[dataIndex].idInBins[binIndex].push_back(id);
        }
        // 根据当前复选状态应用到关键元素掩码
        ApplyCheckStatusToKeyElements(dataIndex);
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
    std::string saveFilePath =
        QFileDialog::getSaveFileName(nullptr, "Compress file as ", "", "Compress Mesh(*.igc)")
        .toStdString();
    if (saveFilePath.empty()) {
        igDebug("Could not save file with error file path\n");
        return;
    }

    auto writer = iGame::IGCWriter::New();
    writer->SetUIControlParams(m_params);

    bool result = writer->WriteToFile(m_dataObj, saveFilePath);
    
    if (!result) {
        QMessageBox::critical(this, "错误", "压缩失败！");
        return;
    }

    if (m_params.showReport)
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
    if (auto lay = this->layout()) {
        lay->setSizeConstraint(QLayout::SetMinAndMaxSize);
        lay->invalidate();
        lay->activate();
    }
    this->setMinimumHeight(0);
    QTimer::singleShot(0, this, [this]() {
        this->adjustSize();
        this->resize(this->width(), this->sizeHint().height());
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
    int dataIndex = GetCurrentDataIndex();
    if (dataIndex < 0 || dataIndex >= m_DataNum) { HideRefreshButton(); return; }

    // 仅在分区模式且当前数据未生成直方图时显示按钮
    if (ui->radio_areaModel->isChecked()) {
        if (m_attrFeatureDatas[dataIndex].genStatus == FeatureHistoGenStatus::No) {
            ShowRefreshButton();
        } else {
            HideRefreshButton();
        }
    } else {
        HideRefreshButton();
    }
}

void igQtMeshCodecDialog::SetRadiosFromErrorMode(iGame::ErrorMode mode)
{
    // 阻断信号，避免反复触发 UI 逻辑
    QSignalBlocker b1(ui->radio_losslessMode);
    QSignalBlocker b2(ui->radio_globalMode);
    QSignalBlocker b3(ui->radio_areaModel);
    ui->radio_losslessMode->setChecked(mode == iGame::ErrorMode::None);
    ui->radio_globalMode->setChecked(mode == iGame::ErrorMode::Default);
    ui->radio_areaModel->setChecked(mode == iGame::ErrorMode::KeyArea);
}

// 将当前属性的分箱勾选状态应用到关键元素掩码
void igQtMeshCodecDialog::ApplyCheckStatusToKeyElements(int dataIndex)
{
    if (!IsValidAttrIndex(dataIndex)) return;
    if (m_attrFeatureDatas.size() <= dataIndex) return;
    if (m_params.errorBoundSetting.size() <= dataIndex) return;

    const auto& bins = m_attrFeatureDatas[dataIndex].idInBins;
    const auto& checks = m_attrFeatureDatas[dataIndex].checkStatus;
    if (bins.size() != checks.size()) return; // 尚未完成分箱

    auto& keyMask = m_params.errorBoundSetting[dataIndex].isKeyElement;
    if (keyMask.empty()) return;

    std::fill(keyMask.begin(), keyMask.end(), false);
    for (int b = 0; b < (int)bins.size(); ++b) {
        if (!checks[b]) continue;
        for (auto idx : bins[b]) {
            if (idx >= 0 && idx < (int)keyMask.size()) keyMask[idx] = true;
        }
    }
}