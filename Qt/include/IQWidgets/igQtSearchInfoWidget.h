#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QVariant>
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

private:
    struct AttrColumn {
        QString key;       // map key in m_allData, e.g. "attr:Pressure" / "attr:Vel:0"
        QString header;    // table header text
        int attrIndex{-1}; // index in AttributeSet
        int component{-1}; // -1 = scalar/all as magnitude for multi-comp query helper; >=0 component
        int dimension{1};
    };

    Ui::igQtSearchInfo* ui;
    void* m_currentModelData{nullptr};
    QList<QMap<QString, QVariant>> m_allData;
    QVector<AttrColumn> m_attrColumns;
    QStringList m_headers;
    int m_currentDataType{0}; // 0: points, 1: cells/faces

    void initUI();
    void initConnections();
    void readModelData();
    void readPointData();
    void readCellData();
    void collectAttributeColumns(int attachmentType);
    void appendAttributeValues(QMap<QString, QVariant>& item, int elementIndex, int attachmentType);
    double readAttributeComponent(int attrIndex, int elementIndex, int component) const;
    void executeQuery();
    void populateResultsTable(const QList<QMap<QString, QVariant>>& results);
    void rebuildTableHeaders();
};
