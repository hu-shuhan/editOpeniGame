#include "IQWidgets/igQtSearchInfoWidget.h"
#include "ui_igQtSearchInfo.h"

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidgetItem>

#include <iGameDataObject.h>

#include <algorithm>
#include <utility>

using namespace iGame;

igQtSearchInfoWidget::igQtSearchInfoWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtSearchInfo), m_currentModelData(nullptr), m_currentDataType(0) {
    ui->setupUi(this);
    initUI();
    initConnections();
}

igQtSearchInfoWidget::~igQtSearchInfoWidget() { delete ui; }

QDockWidget* igQtSearchInfoWidget::createDockWidget(QWidget* parent) {
    auto* dockWidget = new QDockWidget(QStringLiteral("查找数据"), parent);
    dockWidget->setObjectName("dockWidget_SearchInfo");
    dockWidget->setAllowedAreas(Qt::RightDockWidgetArea);

    dockWidget->setWidget(new igQtSearchInfoWidget(dockWidget));
    return dockWidget;
}

void igQtSearchInfoWidget::setCurrentModelData(iGame::DataObject* modelData) {
    m_currentModelData = modelData;

    // BlockMapping is an IG_CELL attribute, so mapped models open on cell data.
    if (m_currentModelData && m_currentModelData->HasBlockMapping()) {
        const QSignalBlocker blockPoints(ui->radioButton_Points);
        const QSignalBlocker blockCells(ui->radioButton_Cells);
        ui->radioButton_Cells->setChecked(true);
        m_currentDataType = 1;
    } else {
        m_currentDataType = ui->radioButton_Cells->isChecked() ? 1 : 0;
    }

    refreshProperties();
    refreshData();
}

void igQtSearchInfoWidget::refreshProperties() {
    const QSignalBlocker blockCombo(ui->comboBox_Property);
    ui->comboBox_Property->clear();
    m_properties.clear();
    if (!m_currentModelData) return;

    auto addProperty = [this](PropertyDescriptor descriptor) {
        const int propertyIndex = m_properties.size();
        m_properties.push_back(std::move(descriptor));
        ui->comboBox_Property->addItem(m_properties.back().displayName, propertyIndex);
    };

    if (m_currentDataType == 0) {
        const QString coordinateNames[] = {QStringLiteral("位置 X"), QStringLiteral("位置 Y"),
                                           QStringLiteral("位置 Z")};
        for (int component = 0; component < 3; ++component) {
            PropertyDescriptor descriptor;
            descriptor.kind = PropertyDescriptor::Kind::Coordinate;
            descriptor.displayName = coordinateNames[component];
            descriptor.component = component;
            addProperty(std::move(descriptor));
        }
    }

    auto* attributeSet = m_currentModelData->GetAttributeSet();
    if (!attributeSet) return;

    auto attributes = attributeSet->GetAllAttributes();
    const IGenum expectedAttachment = m_currentDataType == 0 ? IG_POINT : IG_CELL;
    for (int attributeIndex = 0; attributeIndex < attributes->GetNumberOfElements(); ++attributeIndex) {
        auto& attribute = attributes->GetElement(attributeIndex);
        if (attribute.isDeleted || !attribute.pointer || attribute.attachmentType != expectedAttachment) continue;

        const int dimension = attribute.pointer->GetDimension();
        if (dimension <= 0 || attribute.pointer->GetNumberOfElements() == 0) continue;

        QString baseName = QString::fromStdString(attribute.pointer->GetName());
        if (baseName.isEmpty()) baseName = QStringLiteral("属性 %1").arg(attributeIndex);

        if (dimension > 1) {
            PropertyDescriptor magnitude;
            magnitude.kind = PropertyDescriptor::Kind::Attribute;
            magnitude.displayName = QStringLiteral("%1（模）").arg(baseName);
            magnitude.attributeIndex = attributeIndex;
            magnitude.component = -1;
            addProperty(std::move(magnitude));
        }

        for (int component = 0; component < dimension; ++component) {
            PropertyDescriptor descriptor;
            descriptor.kind = PropertyDescriptor::Kind::Attribute;
            descriptor.displayName = dimension == 1 ? baseName : QStringLiteral("%1[%2]").arg(baseName).arg(component);
            descriptor.attributeIndex = attributeIndex;
            descriptor.component = component;
            addProperty(std::move(descriptor));
        }
    }
}

void igQtSearchInfoWidget::onQueryButtonClicked() { executeQuery(); }

void igQtSearchInfoWidget::refreshData() {
    if (!m_currentModelData) {
        m_filteredItemIds.clear();
        m_paginationWidget->hide();
        ui->tableWidget_Results->clearContents();
        ui->tableWidget_Results->setRowCount(0);
        return;
    }

    readModelData();
}

void igQtSearchInfoWidget::initUI() {
    ui->tableWidget_Results->setSortingEnabled(true);

    m_paginationWidget = new QWidget(this);
    auto* paginationLayout = new QHBoxLayout(m_paginationWidget);
    paginationLayout->setContentsMargins(0, 4, 0, 0);

    m_previousPageButton = new QPushButton(QStringLiteral("上一页"), m_paginationWidget);
    m_nextPageButton = new QPushButton(QStringLiteral("下一页"), m_paginationWidget);
    m_pageSizeComboBox = new QComboBox(m_paginationWidget);
    m_pageSizeComboBox->addItem(QStringLiteral("100 条/页"), 100);
    m_pageSizeComboBox->addItem(QStringLiteral("500 条/页"), 500);
    m_pageSizeComboBox->addItem(QStringLiteral("1000 条/页"), 1000);
    m_pageSizeComboBox->addItem(QStringLiteral("5000 条/页"), 5000);
    m_pageSizeComboBox->setCurrentIndex(2);
    m_pageInfoLabel = new QLabel(QStringLiteral("第 0 / 0 页，共 0 条"), m_paginationWidget);
    m_pageInfoLabel->setAlignment(Qt::AlignCenter);

    paginationLayout->addWidget(m_previousPageButton);
    paginationLayout->addWidget(m_nextPageButton);
    paginationLayout->addWidget(m_pageSizeComboBox);
    paginationLayout->addStretch();
    paginationLayout->addWidget(m_pageInfoLabel);
    ui->verticalLayout_Results->addWidget(m_paginationWidget);
    m_paginationWidget->hide();
}

void igQtSearchInfoWidget::initConnections() {
    connect(ui->pushButton_Query, &QPushButton::clicked, this, &igQtSearchInfoWidget::onQueryButtonClicked);
    connect(ui->radioButton_Points, &QRadioButton::toggled, this, [this](bool checked) {
        if (!checked) return;
        m_currentDataType = 0;
        refreshProperties();
        refreshData();
    });
    connect(ui->radioButton_Cells, &QRadioButton::toggled, this, [this](bool checked) {
        if (!checked) return;
        m_currentDataType = 1;
        refreshProperties();
        refreshData();
    });
    connect(ui->comboBox_Property, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { refreshData(); });
    connect(m_previousPageButton, &QPushButton::clicked, this, [this]() {
        if (m_currentPage <= 0) return;
        --m_currentPage;
        renderCurrentPage();
    });
    connect(m_nextPageButton, &QPushButton::clicked, this, [this]() {
        const int pageSize = m_pageSizeComboBox->currentData().toInt();
        const int pageCount = pageSize > 0 ? (m_filteredItemIds.size() + pageSize - 1) / pageSize : 0;
        if (m_currentPage + 1 >= pageCount) return;
        ++m_currentPage;
        renderCurrentPage();
    });
    connect(m_pageSizeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        m_currentPage = 0;
        renderCurrentPage();
    });
}

void igQtSearchInfoWidget::readModelData() {
    if (!m_currentModelData) return;
    m_paginationWidget->show();
    rebuildFilteredItems();
}

void igQtSearchInfoWidget::executeQuery() {
    const QString valueText = ui->lineEdit_Value->text().trimmed();
    if (valueText.isEmpty() || ui->comboBox_Property->currentIndex() < 0) {
        rebuildFilteredItems();
        return;
    }

    bool ok = false;
    const double filterValue = valueText.toDouble(&ok);
    if (!ok) {
        rebuildFilteredItems();
        return;
    }

    rebuildFilteredItems(ui->comboBox_Operator->currentText(), true, filterValue);
}

int igQtSearchInfoWidget::currentItemCount() const {
    if (!m_currentModelData) return 0;
    if (m_currentDataType == 0) {
        auto points = m_currentModelData->GetPoints();
        return points ? static_cast<int>(points->GetNumberOfPoints()) : 0;
    }

    auto cells = m_currentModelData->GetCellArray();
    if (cells) return static_cast<int>(cells->GetNumberOfCells());

    // Some structured/container objects do not expose CellArray directly. In
    // that case the selected cell attribute still provides a safe row count.
    const int propertyIndex = ui->comboBox_Property->currentIndex();
    if (propertyIndex < 0 || propertyIndex >= m_properties.size()) return 0;
    const auto& descriptor = m_properties[propertyIndex];
    if (descriptor.kind != PropertyDescriptor::Kind::Attribute) return 0;
    auto* attributeSet = m_currentModelData->GetAttributeSet();
    if (!attributeSet || descriptor.attributeIndex < 0 ||
        descriptor.attributeIndex >= attributeSet->GetNumberOfAttributes())
        return 0;
    auto& attribute = attributeSet->GetAttribute(descriptor.attributeIndex);
    return attribute.pointer ? static_cast<int>(attribute.pointer->GetNumberOfElements()) : 0;
}

bool igQtSearchInfoWidget::currentPropertyValue(int itemId, double& value) const {
    if (!m_currentModelData || itemId < 0) return false;
    const int propertyIndex = ui->comboBox_Property->currentIndex();
    if (propertyIndex < 0 || propertyIndex >= m_properties.size()) return false;
    const auto& descriptor = m_properties[propertyIndex];

    if (descriptor.kind == PropertyDescriptor::Kind::Coordinate) {
        auto points = m_currentModelData->GetPoints();
        if (!points || itemId >= static_cast<int>(points->GetNumberOfPoints())) return false;
        value = points->GetPoint(itemId)[descriptor.component];
        return true;
    }

    auto* attributeSet = m_currentModelData->GetAttributeSet();
    if (!attributeSet || descriptor.attributeIndex < 0 ||
        descriptor.attributeIndex >= attributeSet->GetNumberOfAttributes())
        return false;
    auto& attribute = attributeSet->GetAttribute(descriptor.attributeIndex);
    if (!attribute.pointer || itemId >= static_cast<int>(attribute.pointer->GetNumberOfElements())) return false;
    value = attribute.pointer->GetElementValue(itemId, descriptor.component);
    return true;
}

void igQtSearchInfoWidget::rebuildFilteredItems(const QString& operatorStr, bool hasFilter, double filterValue) {
    m_filteredItemIds.clear();
    const int itemCount = currentItemCount();
    m_filteredItemIds.reserve(itemCount);

    for (int itemId = 0; itemId < itemCount; ++itemId) {
        bool match = !hasFilter;
        if (hasFilter) {
            double value = 0.0;
            if (!currentPropertyValue(itemId, value)) continue;
            if (operatorStr == "=") match = (value == filterValue);
            else if (operatorStr == ">")
                match = (value > filterValue);
            else if (operatorStr == "<")
                match = (value < filterValue);
        }
        if (match) m_filteredItemIds.push_back(itemId);
    }

    m_currentPage = 0;
    renderCurrentPage();
}

void igQtSearchInfoWidget::renderCurrentPage() {
    if (!m_currentModelData || !m_pageSizeComboBox) return;

    const int pageSize = m_pageSizeComboBox->currentData().toInt();
    const int totalCount = m_filteredItemIds.size();
    const int pageCount = pageSize > 0 ? (totalCount + pageSize - 1) / pageSize : 0;
    m_currentPage = pageCount == 0 ? 0 : std::clamp(m_currentPage, 0, pageCount - 1);

    const int start = pageCount == 0 ? 0 : m_currentPage * pageSize;
    const int end = std::min(start + pageSize, totalCount);
    const int displayedCount = end - start;

    const int propertyIndex = ui->comboBox_Property->currentIndex();
    const PropertyDescriptor* property =
            propertyIndex >= 0 && propertyIndex < m_properties.size() ? &m_properties[propertyIndex] : nullptr;

    auto* table = ui->tableWidget_Results;
    table->setSortingEnabled(false);
    table->clearContents();

    const bool isPointData = m_currentDataType == 0;
    const bool showAttributeColumn = property && property->kind == PropertyDescriptor::Kind::Attribute;
    if (isPointData) {
        table->setColumnCount(showAttributeColumn ? 5 : 4);
        QStringList headers{QStringLiteral("点 ID"), "X", "Y", "Z"};
        if (showAttributeColumn) headers.push_back(property->displayName);
        table->setHorizontalHeaderLabels(headers);
    } else {
        table->setColumnCount(property ? 2 : 1);
        QStringList headers{QStringLiteral("单元 ID")};
        if (property) headers.push_back(property->displayName);
        table->setHorizontalHeaderLabels(headers);
    }
    table->setRowCount(displayedCount);

    auto points = m_currentModelData->GetPoints();
    for (int row = 0; row < displayedCount; ++row) {
        const int itemId = m_filteredItemIds[start + row];
        auto* idItem = new QTableWidgetItem;
        idItem->setData(Qt::DisplayRole, itemId);
        table->setItem(row, 0, idItem);

        if (isPointData && points) {
            const auto point = points->GetPoint(itemId);
            for (int component = 0; component < 3; ++component) {
                auto* coordinateItem = new QTableWidgetItem;
                coordinateItem->setData(Qt::DisplayRole, point[component]);
                table->setItem(row, component + 1, coordinateItem);
            }
        }

        if (property && (!isPointData || showAttributeColumn)) {
            double value = 0.0;
            auto* valueItem = new QTableWidgetItem;
            if (currentPropertyValue(itemId, value)) valueItem->setData(Qt::DisplayRole, value);
            else
                valueItem->setText(QStringLiteral("—"));
            table->setItem(row, isPointData ? 4 : 1, valueItem);
        }
    }

    ui->groupBox_Results->setTitle(
            QStringLiteral("查询结果：当前页 %1 条，共 %2 条").arg(displayedCount).arg(totalCount));
    m_pageInfoLabel->setText(
            pageCount == 0
                    ? QStringLiteral("第 0 / 0 页，共 0 条")
                    : QStringLiteral("第 %1 / %2 页，共 %3 条").arg(m_currentPage + 1).arg(pageCount).arg(totalCount));
    m_previousPageButton->setEnabled(m_currentPage > 0);
    m_nextPageButton->setEnabled(pageCount > 0 && m_currentPage + 1 < pageCount);
    table->resizeColumnsToContents();
    table->setSortingEnabled(true);
}
