#include "IQComponents/Dialog/igQtMeshCodecDialog.h"

igQtMeshCodecDialog::igQtMeshCodecDialog(QWidget* parent, iGame::DataObject::Pointer obj) :
    QDialog(parent),
    ui(new Ui::MeshCodecDialog),
    m_dataObj(obj)
{
    ui->setupUi(this);

    ui->cbVisualizeError->setVisible(false);

    InitUIControlParams();
    InitIntro();
    InitAttrFeatureDatas();
    // SetupErrorInputValidators();
    InitFeatureTabs();
    InitAttributeList();

    // 连接标签页切换信号
    connect(ui->tabDataDist, &QTabWidget::currentChanged, this, &igQtMeshCodecDialog::on_tabDataDist_currentChanged);

    // 设置滑块控件属性
    ui->sliderDefaultLevel->setMinimum(1);
    ui->sliderDefaultLevel->setMaximum(10);
    ui->sliderDefaultLevel->setTickInterval(1);

    ui->sliderKeyLevel->setMinimum(1);
    ui->sliderKeyLevel->setMaximum(10);
    ui->sliderKeyLevel->setTickInterval(1);

    ui->sliderNonKeyLevel->setMinimum(1);
    ui->sliderNonKeyLevel->setMaximum(10);
    ui->sliderNonKeyLevel->setTickInterval(1);
}

bool igQtMeshCodecDialog::IsVaildAttrIndex(int dataIndex)
{
    return (dataIndex >= 0 && dataIndex < m_DataNum);
}

bool igQtMeshCodecDialog::IsVaildFeatureIndex(int featureIndex)
{
    return (featureIndex >= 0 && featureIndex < m_featureNum);
}

void igQtMeshCodecDialog::InitIntro()
{
    ui->lbIntro->setText("1. 通过浮点数数据的梯度/拉普拉斯直方图以选定关键区域 \n2. 在未选定关键区域并采用区域压缩等级模式时，将以非关键区域压缩等级处理数据 \n3. “将统一压缩等级应用到全体数据”将不会覆盖处于区域压缩等级模式下的数据, 亦不能将区域压缩等级应用于全体数据");
}

void igQtMeshCodecDialog::InitUIControlParams()
{
    for (int i = 0; i < m_dataObj->GetAttributeSet()->GetNumberOfAttributes() + 1; i++)
    {
        iGame::FloatErrorControlParameters p;
       
        if (i == 0)
        {
            p.dataName = m_GeomName;
            p.dimension = 3;
            p.elementCount = iGame::DynamicCast<iGame::PointSet>(m_dataObj)->GetNumberOfPoints();
            p.isKeyElement =
                std::vector<bool>(p.elementCount, false);
        }
        else
        {
            auto attr = m_dataObj->GetAttributeSet()->GetAttribute(i - 1);
            p.dataName = attr.pointer->GetName();
            p.dimension = attr.pointer->GetDimension();
            p.elementCount = attr.pointer->GetNumberOfElements();
            p.isKeyElement = std::vector<bool>(p.elementCount, false);
        }
        
        p.lossyMode = iGame::LossyMode::MantissaTruncation;
        p.errorMode = iGame::ErrorMode::None;
        p.defaultErrorBound = 0.01;
        p.keyAreaErrorBound = 0.01;
        p.nonKeyAreaErrorBound = 0.01;
        m_params.errorBoundSetting.push_back(p);
    }

    m_DataNum = m_params.errorBoundSetting.size();
    m_featureNum = m_featureNames.size();
    m_params.showReport = false;
}

void igQtMeshCodecDialog::InitAttributeList()
{
    // 清除属性列表
    ui->listAttributes->clear();

    for (const auto& info : m_params.errorBoundSetting)
    {
        ui->listAttributes->addItem(QString::fromStdString(info.dataName));
    }
    if (!m_params.errorBoundSetting.empty()) {
        ui->listAttributes->setCurrentRow(0);
    }
}

int igQtMeshCodecDialog::GetCurrentFeatureIndex() const
{
    return ui->tabDataDist->currentIndex();
}

int igQtMeshCodecDialog::GetCurrentDataIndex() const
{
    return ui->listAttributes->currentRow();
}

void igQtMeshCodecDialog::on_listAttributes_currentRowChanged(int dataIndex)
{
    if (!IsVaildAttrIndex(dataIndex))
        return;

    // 更新参数中的属性名称
    const auto& errorBoundSetting = m_params.errorBoundSetting[dataIndex];

    // 将百分比转为1-10的整数范围（百分比*10作为滑块值）
    ui->sliderDefaultLevel->setValue(static_cast<int>(errorBoundSetting.defaultErrorBound * 10));
    ui->sliderKeyLevel->setValue(static_cast<int>(errorBoundSetting.keyAreaErrorBound * 10));
    ui->sliderNonKeyLevel->setValue(static_cast<int>(errorBoundSetting.nonKeyAreaErrorBound * 10));

    // 更新数值显示标签（从滑块值获取）
    ui->lblPercent1->setText(QString::number(ui->sliderDefaultLevel->value()));
    ui->lblPercent2->setText(QString::number(ui->sliderKeyLevel->value()));
    ui->lblPercent3->setText(QString::number(ui->sliderNonKeyLevel->value()));

    ui->radioLossless->setChecked(errorBoundSetting.errorMode == iGame::ErrorMode::None);
    ui->radioDefaultErrorBound->setChecked(errorBoundSetting.errorMode == iGame::ErrorMode::Default);
    ui->radioKeyErrorBound->setChecked(errorBoundSetting.errorMode == iGame::ErrorMode::KeyArea);

    ui->radioMantissaTruncation->setChecked(errorBoundSetting.lossyMode == iGame::LossyMode::MantissaTruncation);
    ui->radioLogQuantization->setChecked(errorBoundSetting.lossyMode == iGame::LossyMode::Quantization);

    ui->lblAttributeTitle->setText(ui->lblAttributeTitle->property("textTemplate").toString().arg(
        QString::fromStdString(errorBoundSetting.dataName)));
    
    LoadAttrFeatureWidget();
}

void igQtMeshCodecDialog::LoadAttrFeatureWidget()
{
    // 获取当前选择的特征索引
    int featureIndex = GetCurrentFeatureIndex();
    int dataIndex = GetCurrentDataIndex();

    if (!IsVaildAttrIndex(dataIndex) || !IsVaildFeatureIndex(featureIndex))
        return;

    // 检查是否有直方图数据
    if (m_attrFeatureDatas[dataIndex][featureIndex].genStatus == FeatureHistoGenStatus::Yes) {
        LoadAllCheckBoxes();
    }
    else {
        // 没有数据，清空直方图并隐藏复选框
        ClearCurrentHistogram();
        HideAllCheckBoxes();
    }
}

/**
 * @brief 初始化特征选项卡
 */
void igQtMeshCodecDialog::InitFeatureTabs()
{
    // 清空现有标签页
    ui->tabDataDist->clear();

    // 为每个特征创建标签页
    for (int featureIndex = 0; featureIndex < m_featureNum; featureIndex++) {
        // 创建标签页内容
        QWidget* tabPage = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(tabPage);

        QChartView* chartView = new QChartView;
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setMinimumHeight(220);
        layout->addWidget(chartView);

        // 创建复选框容器和布局
        QWidget* checkBoxContainer = new QWidget();
        checkBoxContainer->setObjectName("checkBoxContainer");
        checkBoxContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QGridLayout* checkBoxLayout = new QGridLayout(checkBoxContainer);
        checkBoxLayout->setContentsMargins(75, 5, 40, 5);
        checkBoxLayout->setHorizontalSpacing(0);  // 水平间距设为0
        checkBoxLayout->setVerticalSpacing(0);    // 垂直间距设为0

        QVector<QCheckBox*> checkBoxes;
        for (int i = 0; i < m_binNum; i++) {
            checkBoxLayout->setColumnStretch(i, 1);
            QCheckBox* checkBox = new QCheckBox(checkBoxContainer);
            checkBox->setText("");
            checkBoxLayout->addWidget(checkBox, 0, i, Qt::AlignCenter);
            checkBoxes.append(checkBox);

            connect(checkBox, &QCheckBox::stateChanged, this, [this, i](int state) {
                onCheckBoxStateChanged(i, state);  // 传递索引和状态
                });
        }

        layout->addWidget(checkBoxContainer);

        // 初始时隐藏复选框容器
        checkBoxContainer->setVisible(false);
        
        // 添加标签页
        ui->tabDataDist->addTab(tabPage, m_featureNames[featureIndex]);

        // 保存每个特征的控件
        FeatureTab ft;
        ft.chartView = chartView;
        ft.checkBoxes = checkBoxes;
        ft.checkBoxContainer = checkBoxContainer;

        m_featureTabs.push_back(ft);
    }
}

void igQtMeshCodecDialog::InitAttrFeatureDatas()
{
    for (int i = 0; i < m_DataNum; i++)
    {
        QVector<AttrFeatureData> featureDatas;
        for (int j = 0; j < m_featureNum; j++)
        {
            AttrFeatureData data;
            data.histogram = new QChart;
            data.checkStatus = std::vector<bool>(m_binNum, false);
            data.genStatus = FeatureHistoGenStatus::No;

            featureDatas.push_back(data);
        }
        m_attrFeatureDatas.push_back(featureDatas);
    }
}

/**
 * @brief 标签页切换响应函数
 * @param index 新的标签页索引
 */
void igQtMeshCodecDialog::on_tabDataDist_currentChanged(int index)
{
    LoadAttrFeatureWidget();
}

void igQtMeshCodecDialog::onCheckBoxStateChanged(int binIndex, int state)
{
    int dataIndex = GetCurrentDataIndex();
    int featureIndex = GetCurrentFeatureIndex();
    
    int dim = m_params.errorBoundSetting[dataIndex].dimension;
    int elementNum = m_params.errorBoundSetting[dataIndex].elementCount;

    if (!IsVaildAttrIndex(dataIndex) || !IsVaildFeatureIndex(featureIndex))
    {
        return;
    }

    bool check = m_featureTabs[featureIndex].checkBoxes[binIndex]->isChecked();
    m_attrFeatureDatas[dataIndex][featureIndex].checkStatus[binIndex] = check;

    // AND
    bool isKey = false;
    for (int i = 0; i < m_featureNum; i++)
    {
        if (m_attrFeatureDatas[dataIndex][i].checkStatus[binIndex] == true)
        {
            isKey = true;
        }
    }

    for (auto idx : m_attrFeatureDatas[dataIndex][featureIndex].idInBins[binIndex])
    {
        m_params.errorBoundSetting[dataIndex].isKeyElement[idx] = isKey;
    }
}

void igQtMeshCodecDialog::ClearCurrentHistogram() {
    int featureIndex = GetCurrentFeatureIndex();
    if (IsVaildFeatureIndex(featureIndex)) {
        QChart* emptyChart = new QChart();
        m_featureTabs[featureIndex].chartView->setChart(emptyChart);
    }
}

void igQtMeshCodecDialog::HideAllCheckBoxes() {
    int featureIndex = GetCurrentFeatureIndex();
    if (featureIndex >= 0 && featureIndex < m_featureNum) {
        m_featureTabs[featureIndex].checkBoxContainer->setVisible(false);
    }
}

void igQtMeshCodecDialog::DisableAllCheckBoxes()
{
    int featureIndex = GetCurrentFeatureIndex();
    if (featureIndex >= 0 && featureIndex < m_featureNum) {
        m_featureTabs[featureIndex].checkBoxContainer->setEnabled(false);
    }
}

void igQtMeshCodecDialog::EnableAllCheckBoxes()
{
    int featureIndex = GetCurrentFeatureIndex();
    if (featureIndex >= 0 && featureIndex < m_featureNum) {
        m_featureTabs[featureIndex].checkBoxContainer->setEnabled(true);
    }
}

void igQtMeshCodecDialog::LoadAllCheckBoxes() {
    int featureIndex = GetCurrentFeatureIndex();
    int dataIndex = GetCurrentDataIndex();

    if (!IsVaildAttrIndex(dataIndex) || !IsVaildFeatureIndex(featureIndex))
        return;

    AttrFeatureData data = m_attrFeatureDatas[dataIndex][featureIndex];

    m_featureTabs[featureIndex].chartView->setChart(data.histogram);
    m_featureTabs[featureIndex].checkBoxContainer->setVisible(true);

    for (int i = 0; i < data.checkStatus.size(); i++)
    {
        m_featureTabs[featureIndex].checkBoxes[i]->blockSignals(true);
        m_featureTabs[featureIndex].checkBoxes[i]->setChecked(data.checkStatus[i]);
        m_featureTabs[featureIndex].checkBoxes[i]->blockSignals(false);
    }
}

// 原有输入验证函数，已保留但注释
/*
void igQtMeshCodecDialog::SetupErrorInputValidators()
{
    // 使用正则表达式验证器
    // 允许: 0-100，最多5位小数
    // 格式: 0-100 或 0-99.xxxxx
    QRegExpValidator* validator = new QRegExpValidator(
        QRegExp("(100(\\.0{0,5})?|[0-9]{1,2}(\\.[0-9]{0,5})?)"), this);

    // 应用验证器到三个输入框
    ui->txtDefaultError->setValidator(validator);
    ui->txtKeyError->setValidator(validator);
    ui->txtNonKeyError->setValidator(validator);
}
*/

void igQtMeshCodecDialog::on_btnRefreshDataDist_clicked()
{
    int dataIndex = GetCurrentDataIndex();
    int featureIndex = GetCurrentFeatureIndex();

    if ((dataIndex<0 && dataIndex>m_DataNum) || (featureIndex<0 && featureIndex>m_featureNum))
    {
        return;
    }

    switch (m_attrFeatureDatas[dataIndex][featureIndex].genStatus)
    {
    case FeatureHistoGenStatus::Cant:
    case FeatureHistoGenStatus::Yes:
    {
        return;
    }
    case FeatureHistoGenStatus::No:
    {
        /*
        if (FeatureName(featureIndex) == FeatureName::Vortex)
        {m_featureNames[featureIndex]
            int dim = m_dataObj->GetAttributeSet()->GetAttribute(attrIndex).pointer->GetDimension();
            if (dim != 2 && dim != 3)
            {
                QChart* chart = new QChart();
                // 添加一些默认设置让图表正确渲染
                chart->setTitle("");
                chart->legend()->hide();
                chart->createDefaultAxes();
                chart->axes(Qt::Horizontal).first()->setVisible(false);
                chart->axes(Qt::Vertical).first()->setVisible(false);

                // 先设置图表到视图
                m_featureTabs[featureIndex].chartView->setChart(chart);

                // 确保图表完成初始布局
                QApplication::processEvents();

                // 创建简单文本标签
                QGraphicsTextItem* label = new QGraphicsTextItem(chart);
                label->setHtml("<div style='color: red; font-weight: bold; font-size: 12pt;'>仅支持二维/三维数据</div>");
                label->setZValue(100); // 确保标签在前面

                // 使用图表的几何中心而不是plotArea
                qreal xCenter = chart->rect().width() / 2 - label->boundingRect().width() / 2;
                qreal yCenter = chart->rect().height() / 2 - label->boundingRect().height() / 2;
                label->setPos(xCenter, yCenter);

                // 强制更新
                m_featureTabs[featureIndex].chartView->update();
                m_attrFeatureDatas[attrIndex][featureIndex].genStatus = FeatureHistoGenStatus::Cant;
                break;
            }
        }
        */

        DrawFeatureHistogram(m_attrFeatureDatas[dataIndex][featureIndex].histogram);
        m_featureTabs[featureIndex].chartView->setChart(m_attrFeatureDatas[dataIndex][featureIndex].histogram);
        LoadAllCheckBoxes();
        m_attrFeatureDatas[dataIndex][featureIndex].genStatus = FeatureHistoGenStatus::Yes;
        
        break;
    }
    }
}

void igQtMeshCodecDialog::on_radioLossless_toggled(bool checked)
{
    if (checked) {
        // 无损模式下，禁用所有滑块控件
        ui->sliderDefaultLevel->setEnabled(false);
        ui->sliderKeyLevel->setEnabled(false);
        ui->sliderNonKeyLevel->setEnabled(false);
        ui->btnRefreshDataDist->setEnabled(false);

        // 无损模式下，禁用右侧量化方式选择
        ui->radioMantissaTruncation->setEnabled(false);
        ui->radioLogQuantization->setEnabled(false);

        // 无损模式下，启用全体数据设置按钮
        ui->btnSetGlobalCompressMode->setEnabled(true);

        DisableAllCheckBoxes();

        m_params.errorBoundSetting[GetCurrentDataIndex()].errorMode = iGame::ErrorMode::None;
    }
}

void igQtMeshCodecDialog::on_radioDefaultErrorBound_toggled(bool checked)
{
    if (checked) {
        // 默认误差模式下，仅启用默认滑块控件
        ui->sliderDefaultLevel->setEnabled(true);
        ui->sliderKeyLevel->setEnabled(false);
        ui->sliderNonKeyLevel->setEnabled(false);
        ui->btnRefreshDataDist->setEnabled(false);

        // 默认误差模式下，启用右侧量化方式选择
        ui->radioMantissaTruncation->setEnabled(true);
        ui->radioLogQuantization->setEnabled(true);

        // 统一压缩等级模式下，启用全体数据设置按钮
        ui->btnSetGlobalCompressMode->setEnabled(true);

        DisableAllCheckBoxes();

        m_params.errorBoundSetting[GetCurrentDataIndex()].errorMode = iGame::ErrorMode::Default;
    }
}

void igQtMeshCodecDialog::on_radioKeyErrorBound_toggled(bool checked)
{
    if (checked) {
        // 区域误差模式下，仅启用关键区域和非关键区域滑块控件
        ui->sliderDefaultLevel->setEnabled(false);
        ui->sliderKeyLevel->setEnabled(true);
        ui->sliderNonKeyLevel->setEnabled(true);
        ui->btnRefreshDataDist->setEnabled(true);

        // 区域压缩模式下，禁用全体数据设置按钮
        ui->btnSetGlobalCompressMode->setEnabled(false);

        // 区域误差模式下，启用右侧量化方式选择
        ui->radioMantissaTruncation->setEnabled(true);
        ui->radioLogQuantization->setEnabled(true);

        m_params.errorBoundSetting[GetCurrentDataIndex()].errorMode = iGame::ErrorMode::KeyArea;

        EnableAllCheckBoxes();
    }
}

void igQtMeshCodecDialog::on_radioMantissaTruncation_toggled(bool checked)
{
    m_params.errorBoundSetting[GetCurrentDataIndex()].lossyMode = iGame::LossyMode::MantissaTruncation;
}

void igQtMeshCodecDialog::on_radioLogQuantization_toggled(bool checked)
{
    m_params.errorBoundSetting[GetCurrentDataIndex()].lossyMode = iGame::LossyMode::Quantization;
}

// 原有输入框事件处理函数，已保留但注释
/*
void igQtMeshCodecDialog::on_txtDefaultError_textChanged(const QString& text)
{
    m_params.errorBoundSetting[GetCurrentDataIndex()].defaultErrorBound = text.toFloat() / 100;
}

void igQtMeshCodecDialog::on_txtKeyError_textChanged(const QString& text)
{
    m_params.errorBoundSetting[GetCurrentDataIndex()].keyAreaErrorBound = text.toFloat() / 100;
}

void igQtMeshCodecDialog::on_txtNonKeyError_textChanged(const QString& text)
{
    m_params.errorBoundSetting[GetCurrentDataIndex()].nonKeyAreaErrorBound = text.toFloat() / 100;
}
*/

// 新的滑块控件事件处理函数
void igQtMeshCodecDialog::on_sliderDefaultLevel_valueChanged(int value)
{
    int dataIndex = GetCurrentDataIndex();
    if (!IsVaildAttrIndex(dataIndex)) {
        return;
    }
    // 将滑块值（1-10）转为百分比（滑块值/10）
    m_params.errorBoundSetting[dataIndex].defaultErrorBound = static_cast<float>(value) / 10.0f;
    
    // 更新数值显示
    ui->lblPercent1->setText(QString::number(value));
}

void igQtMeshCodecDialog::on_sliderKeyLevel_valueChanged(int value)
{
    int dataIndex = GetCurrentDataIndex();
    if (!IsVaildAttrIndex(dataIndex)) {
        return;
    }
    // 将滑块值（1-10）转为百分比（滑块值/10）
    m_params.errorBoundSetting[dataIndex].keyAreaErrorBound = static_cast<float>(value) / 10.0f;
    
    // 更新数值显示
    ui->lblPercent2->setText(QString::number(value));
}

void igQtMeshCodecDialog::on_sliderNonKeyLevel_valueChanged(int value)
{
    int dataIndex = GetCurrentDataIndex();
    if (!IsVaildAttrIndex(dataIndex)) {
        return;
    }
    // 将滑块值（1-10）转为百分比（滑块值/10）
    m_params.errorBoundSetting[dataIndex].nonKeyAreaErrorBound = static_cast<float>(value) / 10.0f;
    
    // 更新数值显示
    ui->lblPercent3->setText(QString::number(value));
}

void igQtMeshCodecDialog::on_cbVisualizeError_stateChanged(int state)
{
    m_params.visualError = (state == 2);
}

void igQtMeshCodecDialog::on_cbShowReport_stateChanged(int state)
{
    m_params.showReport = ui->cbShowReport->isChecked();
}

void igQtMeshCodecDialog::DrawFeatureHistogram(QChart* chart)
{
    std::vector<float> xAxis;
    std::vector<int> yAxis;

    CalFeatureHistogram(xAxis, yAxis);

    chart->removeAllSeries();
    foreach(QAbstractAxis * axis, chart->axes()) {
        chart->removeAxis(axis);
    }
    chart->legend()->setVisible(false);

    QValueAxis* axisX = new QValueAxis();
    QValueAxis* axisY = new QValueAxis();

    // Setup X axis with scientific notation

    axisX->setRange(xAxis.front(), xAxis.back());
    axisX->setTickCount(xAxis.size());
    axisX->setLabelFormat("%.3e");
    axisX->setLabelsAngle(70);
    chart->addAxis(axisX, Qt::AlignBottom);

    // Setup Y axis with integer format
    int maxY = *std::max_element(yAxis.begin(), yAxis.end());
    axisY->setRange(0, maxY * 1.05); // 5% margin
    axisY->setTickCount(5);
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);

    // Create a series for each bar in the histogram
    for (size_t i = 0; i < yAxis.size(); i++) {
        QLineSeries* lowerLine = new QLineSeries();
        QLineSeries* upperLine = new QLineSeries();
        QAreaSeries* barSeries = new QAreaSeries();

        // Create the bar shape using points
        float x1 = xAxis[i];
        float x2 = xAxis[i + 1];

        // Lower line (at y=0)
        *lowerLine << QPointF(x1, 0) << QPointF(x2, 0);

        // Upper line (at y=count)
        *upperLine << QPointF(x1, yAxis[i]) << QPointF(x2, yAxis[i]);

        // Create area between lower and upper lines
        barSeries->setLowerSeries(lowerLine);
        barSeries->setUpperSeries(upperLine);

        // Style the bar
        barSeries->setColor(QColor(0, 114, 189)); // Blue color
        barSeries->setBorderColor(QColor(0, 114, 189));

        // Add to chart
        chart->addSeries(barSeries);

        // Attach axes
        barSeries->attachAxis(axisX);
        barSeries->attachAxis(axisY);
    }

    // Additional styling
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(Qt::white));
    chart->setPlotAreaBackgroundPen(Qt::NoPen);
}


void igQtMeshCodecDialog::CalFeatureHistogram(std::vector<float>& xAxis, std::vector<int>& yAxis) // bins_num == 10
{
    int featureIndex = GetCurrentFeatureIndex();
    int dataIndex = GetCurrentDataIndex();

    if (!IsVaildAttrIndex(dataIndex) || !IsVaildFeatureIndex(featureIndex))
        return;

    iGame::MeshCodecFeature* featureExtractor = new iGame::MeshCodecFeature(this->m_dataObj, dataIndex);
    std::vector<float> norms;
    switch (FeatureName(featureIndex))
    {
    //case FeatureName::Vortex:
    //{
    //    // 每个元素矢量为[v1, v2, v3]或[v1, v2], 结果均是三维矢量序列, 元素矢量三维时结果矢量维度均有值, 元素矢量二维时结果矢量只有z有值
    //    std::vector<std::vector<float>> result = featureExtractor->GetDataPointVortex();
    //    L2Norm(result, norms);
    //    break;
    //}
    case FeatureName::Laplacian:
    {
        // 每个元素矢量为[v1, v2, ...], 每个分量有一个算子结果
        std::vector<std::vector<float>> result = featureExtractor->GetDataPointLaplacian();
        L2Norm(result, norms);
        break;
    }
    case FeatureName::Gradient:
    {
        // 每个元素矢量为[v1, v2, ...], 每个矢量拥有一个雅可比矩阵
        std::vector<std::vector<std::array<float, 3>>> result = featureExtractor->GetDataPointGradient();
        FrobeniusNorm(result, norms);
        break;
    }
    }

    for (auto& val : norms) {
        // 避免对0或负数取对数
        if (val > 0) {
            val = std::log10(val);
        }
        else {
            val = std::numeric_limits<float>::lowest();  // 对于0或负值使用一个很小的值
        }
    }

    // Find min and max values
    float minVal = *std::min_element(norms.begin(), norms.end());
    float maxVal = *std::max_element(norms.begin(), norms.end());

    // Handle case where all values are the same
    if (minVal == maxVal) {
        xAxis = { minVal, maxVal };
        yAxis = { (int)norms.size() };
    }
    else
    {
        // Calculate bin width
        float binWidth = (maxVal - minVal) / m_binNum;

        // Initialize histogram containers
        xAxis.resize(m_binNum + 1);
        yAxis.resize(m_binNum, 0.0f);

        // Populate xAxis with bin centers
        xAxis[0] = minVal;
        for (int i = 1; i <= m_binNum; ++i) {
            xAxis[i] = minVal + i * binWidth;
        }
        xAxis[m_binNum] = maxVal; // 填平

        // Calculate histogram
        m_attrFeatureDatas[dataIndex][featureIndex].idInBins.resize(m_binNum);
        for (igIndex id = 0; id < norms.size(); id++)
        {
            float norm = norms[id];
            if (norm < minVal || norm > maxVal) continue;

            int binIndex = std::min(static_cast<int>((norm - minVal) / binWidth), m_binNum - 1);
            yAxis[binIndex]++;
            m_attrFeatureDatas[dataIndex][featureIndex].idInBins[binIndex].push_back(id);
        }
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

void igQtMeshCodecDialog::on_btnCancel_clicked()
{
    // 关闭对话框并返回拒绝结果
    reject();
}

void igQtMeshCodecDialog::on_btnSetGlobalCompressMode_clicked()
{
    int featureIndex = GetCurrentFeatureIndex();
    int dataIndex = GetCurrentDataIndex();

    if (!IsVaildAttrIndex(dataIndex) || !IsVaildFeatureIndex(featureIndex))
        return;

    // if (m_params.errorBoundSetting[dataIndex].errorMode == iGame::ErrorMode::KeyArea)
    // {
    //     QMessageBox::information(this, "提示", "不能将区域压缩等级设置应用于全体数据");
    //     return;
    // }

    auto copySetting = m_params.errorBoundSetting[dataIndex];

    for (int i = 0; i < m_DataNum; i++)
    {
        auto& curSetting = m_params.errorBoundSetting[i];
        if (curSetting.errorMode != iGame::ErrorMode::KeyArea)
        {
            curSetting.defaultErrorBound = copySetting.defaultErrorBound;
            curSetting.lossyMode = copySetting.lossyMode;
            curSetting.errorMode = copySetting.errorMode;
        }
    }
}

void igQtMeshCodecDialog::updateAttributeDisplay()
{
    int dataIndex = GetCurrentDataIndex();
    if (dataIndex < 0 || dataIndex >= m_DataNum)
        return;

    // 更新标题
    ui->lblAttributeTitle->setText(tr("属性误差界设置 - %1").arg(QString::fromStdString(m_params.errorBoundSetting[dataIndex].dataName)));
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