#pragma once

#include <QComboBox>
#include <QPushButton>
#include <QString>
#include <QVector>
#include <QWidget>

// 前向声明
class QDockWidget;
class QLabel;

namespace iGame
{
class DataObject;
class Model;
class Selection;
}

namespace Ui
{
class igQtSearchInfo;
}

class igQtSearchInfoWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtSearchInfoWidget(QWidget* parent = nullptr);
    ~igQtSearchInfoWidget();

    // 初始化dockwidget
    static QDockWidget* createDockWidget(QWidget* parent);

    // 设置当前模型数据
    void setCurrentModelData(iGame::DataObject* modelData);

    // 设置当前模型，并监听其选择状态
    void setCurrentModel(iGame::Model* model);

    // 刷新属性列表
    void refreshProperties();

private slots:
    // 查询按钮点击槽函数
    void onQueryButtonClicked();

    // 刷新数据
    void refreshData();

private:
    struct PropertyDescriptor {
        enum class Kind { Coordinate, Attribute };

        Kind kind{Kind::Attribute};
        QString displayName;
        int attributeIndex{-1};
        int component{0};
    };

    Ui::igQtSearchInfo* ui;

    // 当前模型数据
    iGame::DataObject* m_currentModelData;
    iGame::Selection* m_currentSelection{nullptr};

    QVector<PropertyDescriptor> m_properties;
    QVector<int> m_filteredItemIds;

    QWidget* m_paginationWidget{nullptr};
    QPushButton* m_previousPageButton{nullptr};
    QPushButton* m_nextPageButton{nullptr};
    QComboBox* m_pageSizeComboBox{nullptr};
    QLabel* m_pageInfoLabel{nullptr};
    int m_currentPage{0};

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

    int currentItemCount() const;
    bool hasSelectionForCurrentDataType() const;
    void updateFromSelection(int dataType);
    bool propertyValue(int itemId, const PropertyDescriptor& descriptor, double& value) const;
    bool currentPropertyValue(int itemId, double& value) const;
    void rebuildFilteredItems(const QString& operatorStr = QString(), bool hasFilter = false, double filterValue = 0.0);
    void renderCurrentPage();
};
