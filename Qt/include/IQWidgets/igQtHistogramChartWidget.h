#pragma once

#include <IQCore/igQtExportModule.h>
#include <QColor>
#include <QWidget>
#include <QtCharts/QChartView>
#include <iGameHistogramData.h>
#include <vector>

QT_CHARTS_USE_NAMESPACE
class QCheckBox;
class QComboBox;
class QTableWidget;

/** Shared chart with independent series and X-axis settings for each input/result. */
class IG_QT_MODULE_EXPORT igQtHistogramChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit igQtHistogramChartWidget(QWidget* parent = nullptr);
    // Reapplying replaces previousResult and preserves its display settings.
    void SetHistogram(iGame::DataObject::Pointer input, iGame::HistogramData::Pointer result, int attributeIndex,
                      int component, iGame::HistogramData* previousResult = nullptr);
    void SelectSource(iGame::DataObject* object);
    // The main window hosts these controls on the left; this widget contains only the plot.
    QWidget* settingsWidget() const { return m_Controls; }

private:
    struct Column {
        iGame::ArrayObject::Pointer array;
        int component = 0; // dimension denotes magnitude
        IGenum attachment = IG_POINT;
        QString name, legend;
        QColor color;
        bool visible = false;
    };
    struct Representation {
        iGame::DataObject::Pointer object;
        QString name;
        std::vector<Column> columns;
        IGenum attachment = IG_POINT;
        bool histogram = false;
        bool useIndex = true;
        QString xColumn;
    };
    void AddColumns(Representation& representation, iGame::ArrayObject::Pointer array, IGenum attachment);
    void RefreshControls();
    void Redraw();

    std::vector<Representation> m_Representations;
    QComboBox* m_Source = nullptr;
    QComboBox* m_Attachment = nullptr;
    QCheckBox* m_UseIndex = nullptr;
    QComboBox* m_XArray = nullptr;
    QTableWidget* m_Series = nullptr;
    QWidget* m_Controls = nullptr;
    QChartView* m_ChartView = nullptr;
    int m_ResultCount = 0;
    bool m_Refreshing = false;
};
