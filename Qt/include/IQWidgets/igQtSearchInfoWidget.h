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

// 前向声明
class QDockWidget;

namespace Ui {
class igQtSearchInfo;
}

class igQtSearchInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit igQtSearchInfoWidget(QWidget *parent = nullptr);
    ~igQtSearchInfoWidget();

    // 初始化dockwidget
    static QDockWidget* createDockWidget(QWidget* parent);

    // 设置当前模型数据
    void setCurrentModelData(void* modelData);

    // 刷新属性列表
    void refreshProperties();

private slots:
    // 查询按钮点击槽函数
    void onQueryButtonClicked();

    // 刷新数据
    void refreshData();

private:
    Ui::igQtSearchInfo *ui;
    
    // 当前模型数据
    void* m_currentModelData;
    
    // 存储所有数据用于查询
    QList<QMap<QString, QVariant>> m_allData;
    
    // 属性信息
    QList<QString> m_attributeNames;
    QList<QString> m_attributeTypes;
    QList<int> m_attributeDimensions;
    QList<int> m_attributeAttachmentTypes;
    
    // 当前选择的数据类型 (0: 点数据, 1: 面数据)
    int m_currentDataType;
    
    // 初始化UI
    void initUI();
    
    // 初始化信号槽连接
    void initConnections();
    
    // 读取模型数据
    void readModelData();
    
    // 执行查询
    void executeQuery();
    
    // 填充结果表格
    void populateResultsTable(const QList<QMap<QString, QVariant>>& results);
};
