#include <IQWidgets/igQtSearchInfoWidget.h>
#include "ui_igQtSearchInfo.h"
#include <QDockWidget>

#include <iGameDataObject.h>
#include <iGameAttributeSet.h>
#include <iGameType.h>
#include <vector>

using namespace Ui;

igQtSearchInfoWidget::igQtSearchInfoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::igQtSearchInfo),
    m_currentModelData(nullptr),
    m_currentDataType(0)
{
    ui->setupUi(this);
    initUI();
    initConnections();
}

igQtSearchInfoWidget::~igQtSearchInfoWidget()
{
    delete ui;
}

QDockWidget* igQtSearchInfoWidget::createDockWidget(QWidget* parent)
{
    QDockWidget* dockWidget = new QDockWidget("查找数据", parent);
    dockWidget->setObjectName("dockWidget_SearchInfo");
    dockWidget->setAllowedAreas(Qt::RightDockWidgetArea);
    
    igQtSearchInfoWidget* widget = new igQtSearchInfoWidget(dockWidget);
    dockWidget->setWidget(widget);
    
    return dockWidget;
}

void igQtSearchInfoWidget::setCurrentModelData(void* modelData)
{
    m_currentModelData = modelData;
    refreshProperties();
    // 初始化时显示所有数据，不进行过滤
    refreshData();
}

void igQtSearchInfoWidget::refreshProperties()
{
    ui->comboBox_Property->clear();
    m_attributeNames.clear();
    m_attributeTypes.clear();
    m_attributeDimensions.clear();
    m_attributeAttachmentTypes.clear();
    
    if (!m_currentModelData) {
        return;
    }
    
    // 更新当前数据类型选择
    m_currentDataType = ui->radioButton_Points->isChecked() ? 0 : 1;
    
    auto dataObject = static_cast<iGame::DataObject*>(m_currentModelData);
    
    if (m_currentDataType == 0) { // 点数据
        ui->comboBox_Property->addItem("Position X", "pos.x");
        m_attributeNames.append("Position X");
        m_attributeTypes.append("pos.x");
        m_attributeDimensions.append(1);
        m_attributeAttachmentTypes.append(0);
        
        ui->comboBox_Property->addItem("Position Y", "pos.y");
        m_attributeNames.append("Position Y");
        m_attributeTypes.append("pos.y");
        m_attributeDimensions.append(1);
        m_attributeAttachmentTypes.append(0);
        
        ui->comboBox_Property->addItem("Position Z", "pos.z");
        m_attributeNames.append("Position Z");
        m_attributeTypes.append("pos.z");
        m_attributeDimensions.append(1);
        m_attributeAttachmentTypes.append(0);
    } else { // 面数据
        ui->comboBox_Property->addItem("Center X", "center.x");
        m_attributeNames.append("Center X");
        m_attributeTypes.append("center.x");
        m_attributeDimensions.append(1);
        m_attributeAttachmentTypes.append(1);
        
        ui->comboBox_Property->addItem("Center Y", "center.y");
        m_attributeNames.append("Center Y");
        m_attributeTypes.append("center.y");
        m_attributeDimensions.append(1);
        m_attributeAttachmentTypes.append(1);
        
        ui->comboBox_Property->addItem("Center Z", "center.z");
        m_attributeNames.append("Center Z");
        m_attributeTypes.append("center.z");
        m_attributeDimensions.append(1);
        m_attributeAttachmentTypes.append(1);
    }
    
    auto attributeSet = dataObject->GetAttributeSet();
    if (attributeSet) {
        auto allAttributes = attributeSet->GetAllAttributes();
        if (allAttributes) {
            int numAttributes = allAttributes->GetNumberOfElements();
            for (int i = 0; i < numAttributes; i++) {
                auto& attr = allAttributes->GetElement(i);
                if (attr.pointer && !attr.isDeleted) {
                    // 只添加当前数据类型对应的属性
                    if ((m_currentDataType == 0 && attr.attachmentType == IG_POINT) ||
                        (m_currentDataType == 1 && attr.attachmentType == IG_CELL)) {
                        QString name = QString::fromStdString(attr.pointer->GetName());
                        
                        // 跳过positionX/positionY/positionZ这些重复字段
                        if (name == "positionX" || name == "positionY" || name == "positionZ" ||
                            name == "PositionX" || name == "PositionY" || name == "PositionZ") {
                            continue;
                        }
                        
                        QString typeStr;
                        int dimension = attr.pointer->GetDimension();
                        
                        switch (attr.type) {
                            case IG_SCALAR:
                                typeStr = "Scalar";
                                break;
                            case IG_VECTOR:
                                typeStr = "Vector";
                                break;
                            case IG_NORMAL:
                                typeStr = "Normal";
                                break;
                            case IG_TCOORD:
                                typeStr = "TextureCoord";
                                break;
                            case IG_TENSOR:
                                typeStr = "Tensor";
                                break;
                            default:
                                typeStr = "Unknown";
                                break;
                        }
                        
                        QString attachmentStr = (attr.attachmentType == IG_POINT) ? " (Point)" : " (Cell)";
                        
                        if (dimension == 3) {
                            // 向量类型拆分为x、y、z三个选项
                            ui->comboBox_Property->addItem(name + "_x [" + typeStr + attachmentStr + "]", name + "_x");
                            m_attributeNames.append(name + "_x");
                            m_attributeTypes.append("attr_" + QString::number(i) + "_x");
                            m_attributeDimensions.append(1);
                            m_attributeAttachmentTypes.append(attr.attachmentType);
                            
                            ui->comboBox_Property->addItem(name + "_y [" + typeStr + attachmentStr + "]", name + "_y");
                            m_attributeNames.append(name + "_y");
                            m_attributeTypes.append("attr_" + QString::number(i) + "_y");
                            m_attributeDimensions.append(1);
                            m_attributeAttachmentTypes.append(attr.attachmentType);
                            
                            ui->comboBox_Property->addItem(name + "_z [" + typeStr + attachmentStr + "]", name + "_z");
                            m_attributeNames.append(name + "_z");
                            m_attributeTypes.append("attr_" + QString::number(i) + "_z");
                            m_attributeDimensions.append(1);
                            m_attributeAttachmentTypes.append(attr.attachmentType);
                        } else {
                            // 一维或多维数据作为一个选项
                            QString displayName = name + " [" + typeStr + attachmentStr + "]";
                            ui->comboBox_Property->addItem(displayName, name);
                            m_attributeNames.append(name);
                            m_attributeTypes.append("attr_" + QString::number(i));
                            m_attributeDimensions.append(dimension);
                            m_attributeAttachmentTypes.append(attr.attachmentType);
                        }
                    }
                }
            }
        }
    }
}

void igQtSearchInfoWidget::onQueryButtonClicked()
{
    executeQuery();
}

void igQtSearchInfoWidget::refreshData()
{
    if (!m_currentModelData) {
        ui->tableWidget_Results->clearContents();
        ui->tableWidget_Results->setRowCount(0);
        return;
    }
    
    // 先刷新属性列表，再读取数据
    refreshProperties();
    readModelData();
}

void igQtSearchInfoWidget::initUI()
{
    // 初始化表格
    ui->tableWidget_Results->setColumnCount(4);
    ui->tableWidget_Results->setHorizontalHeaderLabels({"ID", "X", "Y", "Z"});
    ui->tableWidget_Results->setSortingEnabled(true);
    
    // 设置表格样式
    ui->tableWidget_Results->setStyleSheet(
        "QTableWidget { background-color: #1E1E1E; color: #C8C8C8; border: 1px solid #3C3C3C; } "
        "QTableWidget::item { padding: 4px; } "
        "QTableWidget::item:selected { background-color: #2A6099; color: #FFFFFF; } "
        "QHeaderView { background-color: #252526; color: #C8C8C8; border: 1px solid #3C3C3C; } "
        "QHeaderView::section { background-color: #252526; color: #C8C8C8; border: 1px solid #3C3C3C; padding: 4px; } "
        "QHeaderView::section:horizontal { border-bottom: 1px solid #3C3C3C; } "
        "QHeaderView::section:vertical { border-right: 1px solid #3C3C3C; } "
        "QTableCornerButton::section { background-color: #252526; border: 1px solid #3C3C3C; padding: 0px; } "
    );
    
    // 设置表格交替行颜色
    ui->tableWidget_Results->setAlternatingRowColors(true);
    ui->tableWidget_Results->setStyleSheet(ui->tableWidget_Results->styleSheet() + 
        "QTableWidget::item:even { background-color: #1E1E1E; } "
        "QTableWidget::item:odd { background-color: #252526; } "
    );
}

void igQtSearchInfoWidget::initConnections()
{
    connect(ui->pushButton_Query, &QPushButton::clicked, this, &igQtSearchInfoWidget::onQueryButtonClicked);
    connect(ui->radioButton_Points, &QRadioButton::toggled, this, &igQtSearchInfoWidget::refreshData);
    connect(ui->radioButton_Cells, &QRadioButton::toggled, this, &igQtSearchInfoWidget::refreshData);
}

void igQtSearchInfoWidget::readModelData()
{
    if (!m_currentModelData) {
        return;
    }
    
    m_allData.clear();
    
    // 更新当前数据类型选择
    m_currentDataType = ui->radioButton_Points->isChecked() ? 0 : 1;
    
    auto dataObject = static_cast<iGame::DataObject*>(m_currentModelData);
    auto points = dataObject->GetPoints();
    auto cells = dataObject->GetCellArray();
    auto attributeSet = dataObject->GetAttributeSet();
    
    if (m_currentDataType == 0) { // 点数据
        if (points) {
            int numPoints = points->GetNumberOfPoints();
            for (int i = 0; i < numPoints; i++) {
                auto point = points->GetPoint(i);
                QMap<QString, QVariant> item;
                item["id"] = i;
                item["x"] = point[0];
                item["y"] = point[1];
                item["z"] = point[2];
                
                if (attributeSet) {
                    auto allAttributes = attributeSet->GetAllAttributes();
                    if (allAttributes) {
                        int numAttributes = allAttributes->GetNumberOfElements();
                        for (int j = 0; j < numAttributes; j++) {
                            auto& attr = allAttributes->GetElement(j);
                            if (attr.pointer && !attr.isDeleted && attr.attachmentType == IG_POINT) {
                                QString attrName = QString::fromStdString(attr.pointer->GetName());
                                
                                // 跳过positionX/positionY/positionZ这些重复字段
                                if (attrName == "positionX" || attrName == "positionY" || attrName == "positionZ" ||
                                    attrName == "PositionX" || attrName == "PositionY" || attrName == "PositionZ") {
                                    continue;
                                }
                                
                                int dimension = attr.pointer->GetDimension();
                                
                                if (dimension == 1) {
                                    double value = attr.pointer->GetElementValue(i, 0);
                                    item[attrName] = value;
                                } else if (dimension == 3) {
                                    // 向量类型拆分为x、y、z三个字段
                                    std::vector<double> values(3);
                                    attr.pointer->GetElement(i, values);
                                    item[attrName + "_x"] = values[0];
                                    item[attrName + "_y"] = values[1];
                                    item[attrName + "_z"] = values[2];
                                } else {
                                    // 其他多维数据用逗号分隔
                                    std::vector<double> values(dimension);
                                    attr.pointer->GetElement(i, values);
                                    QString valueStr;
                                    for (int k = 0; k < dimension; k++) {
                                        if (k > 0) valueStr += ", ";
                                        valueStr += QString::number(values[k]);
                                    }
                                    item[attrName] = valueStr;
                                }
                            }
                        }
                    }
                }
                
                m_allData.append(item);
            }
        }
    } else { // 面数据
        if (cells) {
            int numCells = cells->GetNumberOfCells();
            for (int i = 0; i < numCells; i++) {
                QMap<QString, QVariant> item;
                item["id"] = i;
                
                // 获取面的点索引
                const igIndex* pointIds = nullptr;
                int numPointsPerCell = cells->GetCellIds(i, pointIds);
                if (pointIds && numPointsPerCell > 0) {
                    QString pointIdsStr;
                    for (int j = 0; j < numPointsPerCell; j++) {
                        if (j > 0) pointIdsStr += ", ";
                        pointIdsStr += QString::number(pointIds[j]);
                    }
                    item["Point IDs"] = pointIdsStr;
                }
                
                // 计算面的中心坐标
                if (points) {
                    const igIndex* pointIds = nullptr;
                    int numPointsPerCell = cells->GetCellIds(i, pointIds);
                    if (pointIds && numPointsPerCell > 0) {
                        double center[3] = {0, 0, 0};
                        for (int j = 0; j < numPointsPerCell; j++) {
                            auto point = points->GetPoint(pointIds[j]);
                            center[0] += point[0];
                            center[1] += point[1];
                            center[2] += point[2];
                        }
                        center[0] /= numPointsPerCell;
                        center[1] /= numPointsPerCell;
                        center[2] /= numPointsPerCell;
                        item["center.x"] = center[0];
                        item["center.y"] = center[1];
                        item["center.z"] = center[2];
                    }
                }
                
                if (attributeSet) {
                    auto allAttributes = attributeSet->GetAllAttributes();
                    if (allAttributes) {
                        int numAttributes = allAttributes->GetNumberOfElements();
                        for (int j = 0; j < numAttributes; j++) {
                            auto& attr = allAttributes->GetElement(j);
                            if (attr.pointer && !attr.isDeleted && attr.attachmentType == IG_CELL) {
                                QString attrName = QString::fromStdString(attr.pointer->GetName());
                                
                                // 跳过positionX/positionY/positionZ这些重复字段
                                if (attrName == "positionX" || attrName == "positionY" || attrName == "positionZ" ||
                                    attrName == "PositionX" || attrName == "PositionY" || attrName == "PositionZ") {
                                    continue;
                                }
                                
                                int dimension = attr.pointer->GetDimension();
                                
                                if (dimension == 1) {
                                    double value = attr.pointer->GetElementValue(i, 0);
                                    item[attrName] = value;
                                } else if (dimension == 3) {
                                    // 向量类型拆分为x、y、z三个字段
                                    std::vector<double> values(3);
                                    attr.pointer->GetElement(i, values);
                                    item[attrName + "_x"] = values[0];
                                    item[attrName + "_y"] = values[1];
                                    item[attrName + "_z"] = values[2];
                                } else {
                                    // 其他多维数据用逗号分隔
                                    std::vector<double> values(dimension);
                                    attr.pointer->GetElement(i, values);
                                    QString valueStr;
                                    for (int k = 0; k < dimension; k++) {
                                        if (k > 0) valueStr += ", ";
                                        valueStr += QString::number(values[k]);
                                    }
                                    item[attrName] = valueStr;
                                }
                            }
                        }
                    }
                }
                
                m_allData.append(item);
            }
        }
    }
    
    populateResultsTable(m_allData);
}

void igQtSearchInfoWidget::executeQuery()
{
    if (m_allData.isEmpty()) {
        return;
    }
    
    QString property = ui->comboBox_Property->currentData().toString();
    QString operatorStr = ui->comboBox_Operator->currentText();
    QString valueStr = ui->lineEdit_Value->text();
    
    if (property.isEmpty() || valueStr.isEmpty()) {
        populateResultsTable(m_allData);
        return;
    }
    
    bool ok;
    double value = valueStr.toDouble(&ok);
    if (!ok) {
        populateResultsTable(m_allData);
        return;
    }
    
    QList<QMap<QString, QVariant>> results;
    
    for (const QMap<QString, QVariant>& item : m_allData) {
        bool match = false;
        double propValue = 0;
        
        if (m_currentDataType == 0) { // 点数据
            if (property == "pos.x") {
                propValue = item["x"].toDouble();
            } else if (property == "pos.y") {
                propValue = item["y"].toDouble();
            } else if (property == "pos.z") {
                propValue = item["z"].toDouble();
            } else if (item.contains(property)) {
                propValue = item[property].toDouble();
            }
        } else { // 面数据
            if (property == "center.x") {
                propValue = item["center.x"].toDouble();
            } else if (property == "center.y") {
                propValue = item["center.y"].toDouble();
            } else if (property == "center.z") {
                propValue = item["center.z"].toDouble();
            } else if (item.contains(property)) {
                propValue = item[property].toDouble();
            }
        }
        
        if (operatorStr == "=") match = (propValue == value);
        else if (operatorStr == ">") match = (propValue > value);
        else if (operatorStr == "<") match = (propValue < value);
        
        if (match) {
            results.append(item);
        }
    }
    
    populateResultsTable(results);
}

void igQtSearchInfoWidget::populateResultsTable(const QList<QMap<QString, QVariant>>& results)
{
    ui->tableWidget_Results->clearContents();
    
    if (results.isEmpty()) {
        ui->tableWidget_Results->setRowCount(0);
        return;
    }
    
    QStringList columnNames;
    columnNames << "ID" << "X" << "Y" << "Z";
    
    for (const QString& attrName : m_attributeNames) {
        if (!columnNames.contains(attrName)) {
            columnNames << attrName;
        }
    }
    
    ui->tableWidget_Results->setColumnCount(columnNames.size());
    ui->tableWidget_Results->setHorizontalHeaderLabels(columnNames);
    
    ui->tableWidget_Results->setRowCount(results.size());
    
    for (int i = 0; i < results.size(); i++) {
        const QMap<QString, QVariant>& item = results[i];
        
        ui->tableWidget_Results->setItem(i, 0, new QTableWidgetItem(QString::number(item["id"].toInt())));
        ui->tableWidget_Results->setItem(i, 1, new QTableWidgetItem(QString::number(item["x"].toDouble())));
        ui->tableWidget_Results->setItem(i, 2, new QTableWidgetItem(QString::number(item["y"].toDouble())));
        ui->tableWidget_Results->setItem(i, 3, new QTableWidgetItem(QString::number(item["z"].toDouble())));
        
        for (int j = 4; j < columnNames.size(); j++) {
            QString attrName = columnNames[j];
            if (item.contains(attrName)) {
                ui->tableWidget_Results->setItem(i, j, new QTableWidgetItem(item[attrName].toString()));
            } else {
                ui->tableWidget_Results->setItem(i, j, new QTableWidgetItem(""));
            }
        }
    }
    
    ui->tableWidget_Results->resizeColumnsToContents();
}