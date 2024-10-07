#include "IQWidgets/igQtCharts.h"
#include <QLineSeries>
#include <QVBoxLayout>
/**
 * @class   igQtCharts
 * @brief   This class provides a simple interface to draw a bar chart using Qt Charts.
 */

igQtCharts::igQtCharts(QWidget* parent)
    : QDialog(parent), chart(new QChart()), chartView(new QChartView(chart)) {

    this->setWindowTitle("Chart View");
    this->resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(chartView);

    this->setLayout(layout);
}

/**
 * @brief   Draws a bar chart based on the provided data.
 * @param   data A vector of integers representing the values for the bar chart.
 */
void igQtCharts::draw(iGame::ArrayObject::Pointer data) {
    // 初始化最小值和最大值
    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();

    // 遍历数据以找到最小值和最大值
    for (int i = 0; i < data->GetNumberOfValues(); i++) {
        float value = data->GetValue(i);
        if (value < minValue) { minValue = value; }
        if (value > maxValue) { maxValue = value; }
    }

    // 确定组的数量
    int numberOfBins = 10; // 你可以根据需要调整这个值
    if (minValue == maxValue) { numberOfBins = 1; }

    // 创建条形集合
    auto set = new QBarSet(QString::fromStdString(data->GetName()));

    // 计算每个组的宽度
    float binWidth;
    if (numberOfBins > 1) {
        binWidth = (maxValue - minValue) / numberOfBins;
    } else {
        binWidth = 1; // 当numberOfBins为1时，设置宽度为1
    }

    // 初始化每个组的计数
    QVector<int> binCounts(numberOfBins, 0);

    // 遍历数据并计算每个组的计数
    for (int i = 0; i < data->GetNumberOfValues(); i++) {
        float value = data->GetValue(i);
        int binIndex;
        if (numberOfBins > 1) {
            binIndex = static_cast<int>((value - minValue) / binWidth);
            if (binIndex < 0) binIndex = 0;
            if (binIndex >= numberOfBins) binIndex = numberOfBins - 1;
        } else {
            binIndex = 0; // 当numberOfBins为1时，所有值都属于同一个组
        }
        binCounts[binIndex]++;
    }

    // 将计数添加到条形集合中
    for (int count: binCounts) { *set << count; }

    // 创建条形系列并清除图表中的先前系列
    chart->removeAllSeries();
    auto series = new QBarSeries();
    series->append(set);
    chart->addSeries(series);

    // 创建X轴的类别
    QVector<QString> categories;
    for (int i = 0; i < numberOfBins; ++i) {
        float binCenter = minValue + (i + 0.5f) * binWidth;
        categories.push_back(QString::number(binCenter, 'f', 2));
    }

    // 创建X轴并将其附加到图表
    auto axisX = new QBarCategoryAxis();
    for (const QString& category: categories) { axisX->append(category); }
    axisX->setTitleText("Value");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // 创建Y轴，设置范围，并将其附加到图表
    auto axisY = new QValueAxis();
    int maxCount = *std::max_element(binCounts.begin(), binCounts.end());
    axisY->setRange(0, qRound(maxCount * 1.1)); // 确保Y轴的范围是整数
    axisY->setTitleText("Count");
    axisY->setTickCount(5);      // 设置Y轴的刻度数量
    axisY->setMinorTickCount(0); // 禁用次要刻度
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 添加网格线
    QLineSeries* lineSeries = new QLineSeries();
    for (int i = 0; i <= numberOfBins; ++i) { lineSeries->append(i, 0); }
    QPen* pen = new QPen(QRgb(0x80000000)); // 半透明的黑色
    lineSeries->setPen(*pen);
    chart->addSeries(lineSeries);
    lineSeries->attachAxis(axisX);
    lineSeries->attachAxis(axisY);

    // 自定义图表外观
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->setTitle("Data Distribution Histogram");
}


void igQtCharts::drawLineChart(iGame::ArrayObject::Pointer m_data) {
    // 创建一个折线系列
    auto series = new QLineSeries();
    series->setName(QString::fromStdString(m_data->GetName()));

    // 遍历数据并添加到系列中
    for (int i = 0; i < m_data->GetNumberOfValues(); ++i) {
        float value = m_data->GetValue(i);
        series->append(i, value);
    }

    // 清空图表中的系列
    chart->removeAllSeries();

    // 创建图表并添加系列
    chart->addSeries(series);

    // 创建X轴
    QValueAxis* axisX = new QValueAxis();
    axisX->setRange(0, m_data->GetNumberOfValues() - 1);
    axisX->setTitleText("Index");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // 创建Y轴
    QValueAxis* axisY = new QValueAxis();
    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();

    // 重新遍历数据以找到最小值和最大值，用于设置Y轴范围
    for (int i = 0; i < m_data->GetNumberOfValues(); ++i) {
        float value = m_data->GetValue(i);
        if (value < minValue) { minValue = value; }
        if (value > maxValue) { maxValue = value; }
    }

    axisY->setRange(minValue, maxValue);
    axisY->setTitleText("Value");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 设置图表的标题
    chart->setTitle("Data Line Chart");

    // 更新图表视图
    chartView->setChart(chart);
}
/**
 * @brief   Returns the QChartView associated with this chart.
 * @return  A pointer to the QChartView object.
 */
QChartView* igQtCharts::getChartView() const { return chartView; }
