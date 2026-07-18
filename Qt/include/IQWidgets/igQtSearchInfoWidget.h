#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QVector>

class QDockWidget;

namespace Ui {
class igQtSearchInfo;
}

class igQtSearchInfoWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtSearchInfoWidget(QWidget* parent = nullptr);
    ~igQtSearchInfoWidget();

    static QDockWidget* createDockWidget(QWidget* parent);

    void setCurrentModelData(void* modelData);
    void refreshProperties();

private slots:
    void onQueryButtonClicked();
    void refreshData();
    void onDataTypeChanged();
    void onPrevPageClicked();
    void onNextPageClicked();

private:
    struct AttrColumn {
        QString key;       // e.g. "attr:Pressure" / "attr:Vel:0"
        QString header;    // table header text
        int attrIndex{-1}; // index in AttributeSet
        int component{-1}; // >=0 component index
        int dimension{1};
    };

    struct QuerySpec {
        QString property;
        QString op;
        double value{0.0};
        bool active{false}; // true = filter by condition; false = show all elements
    };

    static constexpr int kPageSize = 200;

    Ui::igQtSearchInfo* ui;
    void* m_currentModelData{nullptr};
    QVector<AttrColumn> m_attrColumns;
    QStringList m_headers;
    int m_currentDataType{0}; // 0: points, 1: cells/faces

    QuerySpec m_query;
    int m_totalMatches{0};
    int m_currentPage{0}; // 0-based
    QVector<int> m_pageIds;

    void initUI();
    void initConnections();
    void collectAttributeColumns(int attachmentType);
    double readAttributeComponent(int attrIndex, int elementIndex, int component) const;
    int getElementCount() const;
    bool readCellGeometry(int cellId, int& numPts, QString& ptsCsv, double& cx, double& cy, double& cz) const;
    bool readPropertyValue(int elementId, const QString& property, double& outValue) const;
    void fillTableRow(int row, int elementId);
    void executeQuery();
    void loadPage(int page);
    void populateResultsTable();
    void updatePaginationControls();
    void rebuildTableHeaders();
    void clearResults();
    int pageCount() const;
};
