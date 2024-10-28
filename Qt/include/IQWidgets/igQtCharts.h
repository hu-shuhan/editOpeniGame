/**
 * @class   igQtCharts
 * @brief   igQtCharts's brief
 */
#pragma once

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QDialog>
#include "iGameArrayObject.h"
QT_CHARTS_USE_NAMESPACE

class igQtCharts : public QDialog {
    Q_OBJECT

public:
    igQtCharts(QWidget* parent = nullptr);
    void drawBarChart(iGame::ArrayObject::Pointer m_data);
    void drawLineChart(iGame::ArrayObject::Pointer m_data);
    QChartView* getChartView() const;

private:
    QChart* chart;
    QChartView* chartView;
};


