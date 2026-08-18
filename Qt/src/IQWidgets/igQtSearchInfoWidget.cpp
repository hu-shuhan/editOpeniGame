#include "IQWidgets/igQtSearchInfoWidget.h"
#include "ui_igQtSearchInfo.h"

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidgetItem>

#include <iGameDataObject.h>
#include <iGameModel.h>
#include <iGameSelection.h>

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
    m_currentSelection = nullptr;
    if (m_currentModelData != modelData) m_selectedPartIds.clear();
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

void igQtSearchInfoWidget::setCurrentModel(iGame::Model* model) {
    auto* modelData = model ? model->GetDataObject().GetPointer() : nullptr;
    if (m_currentModelData != modelData) m_selectedPartIds.clear();
    m_currentModelData = modelData;
    auto selectionPointer = model ? model->GetSelection() : Selection::Pointer{};
    m_currentSelection = selectionPointer.GetPointer();

    if (m_currentSelection) {
        auto* selection = m_currentSelection;
        const QPointer<igQtSearchInfoWidget> self(this);
        selection->_SetSelectionCallBackEvent_(
                "igQtSearchInfoWidget",
                [self, selection](IGenum itemType, const std::vector<igIndex>&, Selection::Operate) {
                    if (!self || self->m_currentSelection != selection) return;
                    if (itemType == IG_POINT) self->updateFromSelection(0);
                    else if (itemType == IG_CELL)
                        self->updateFromSelection(1);
                    else if (itemType == IG_CHANGE)
                        self->refreshData();
                });
        selection->_SetClearSelectionCallBackEvent_("igQtSearchInfoWidget", [self, selection]() {
            if (!self || self->m_currentSelection != selection) return;
            self->refreshData();
        });
    }

    const bool hasSelectedCells =
            m_currentSelection && !m_currentSelection->GetSelectedItems(IG_CELL).empty();
    const bool hasSelectedPoints =
            m_currentSelection && !m_currentSelection->GetSelectedItems(IG_POINT).empty();
    if (hasSelectedCells || (!hasSelectedPoints && m_currentModelData && m_currentModelData->HasBlockMapping())) {
        const QSignalBlocker blockPoints(ui->radioButton_Points);
        const QSignalBlocker blockCells(ui->radioButton_Cells);
        ui->radioButton_Cells->setChecked(true);
        m_currentDataType = 1;
    } else if (hasSelectedPoints) {
        const QSignalBlocker blockPoints(ui->radioButton_Points);
        const QSignalBlocker blockCells(ui->radioButton_Cells);
        ui->radioButton_Points->setChecked(true);
        m_currentDataType = 0;
    } else {
        m_currentDataType = ui->radioButton_Cells->isChecked() ? 1 : 0;
    }

    refreshProperties();
    refreshData();
}

void igQtSearchInfoWidget::setSelectedPartIds(const QVector<int>& partIds) {
    m_selectedPartIds.clear();
    for (int partId: partIds) {
        if (partId >= 0) m_selectedPartIds.insert(partId);
    }

    m_currentDataType = 1;
    {
        const QSignalBlocker blockPoints(ui->radioButton_Points);
        const QSignalBlocker blockCells(ui->radioButton_Cells);
        ui->radioButton_Points->setChecked(false);
        ui->radioButton_Cells->setChecked(true);
    }

    ui->lineEdit_Value->clear();
    refreshProperties();
    refreshData();
}

void igQtSearchInfoWidget::updateFromSelection(int dataType) {
    m_selectedPartIds.clear();
    m_currentDataType = dataType;
    {
        const QSignalBlocker blockPoints(ui->radioButton_Points);
        const QSignalBlocker blockCells(ui->radioButton_Cells);
        ui->radioButton_Points->setChecked(dataType == 0);
        ui->radioButton_Cells->setChecked(dataType == 1);
    }

    ui->lineEdit_Value->clear();
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
        return propertyIndex;
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
    int firstAttributePropertyIndex = -1;
    int preferredAttributePropertyIndex = -1;
    for (int attributeIndex = 0; attributeIndex < attributes->GetNumberOfElements(); ++attributeIndex) {
        auto& attribute = attributes->GetElement(attributeIndex);
        if (attribute.isDeleted || !attribute.pointer || attribute.attachmentType != expectedAttachment) continue;

        const int dimension = attribute.pointer->GetDimension();
        if (dimension <= 0 || attribute.pointer->GetNumberOfElements() == 0) continue;

        QString baseName = QString::fromStdString(attribute.pointer->GetName());
        if (baseName.isEmpty()) baseName = QStringLiteral("属性 %1").arg(attributeIndex);

        int attributePropertyIndex = -1;
        if (dimension > 1) {
            PropertyDescriptor magnitude;
            magnitude.kind = PropertyDescriptor::Kind::Attribute;
            magnitude.displayName = QStringLiteral("%1（模）").arg(baseName);
            magnitude.attributeIndex = attributeIndex;
            magnitude.component = -1;
            attributePropertyIndex = addProperty(std::move(magnitude));
        }

        for (int component = 0; component < dimension; ++component) {
            PropertyDescriptor descriptor;
            descriptor.kind = PropertyDescriptor::Kind::Attribute;
            descriptor.displayName = dimension == 1 ? baseName : QStringLiteral("%1[%2]").arg(baseName).arg(component);
            descriptor.attributeIndex = attributeIndex;
            descriptor.component = component;
            const int propertyIndex = addProperty(std::move(descriptor));
            if (attributePropertyIndex < 0) attributePropertyIndex = propertyIndex;
        }

        if (firstAttributePropertyIndex < 0) firstAttributePropertyIndex = attributePropertyIndex;
        const bool isBlockProperty =
                attribute.type == IG_BLOCK_MAPPING || baseName.compare(QStringLiteral("block_id"), Qt::CaseInsensitive) == 0 ||
                baseName.compare(QStringLiteral("part_id"), Qt::CaseInsensitive) == 0;
        if (isBlockProperty) preferredAttributePropertyIndex = attributePropertyIndex;
    }

    const int initialPropertyIndex =
            preferredAttributePropertyIndex >= 0 ? preferredAttributePropertyIndex : firstAttributePropertyIndex;
    if (initialPropertyIndex >= 0) ui->comboBox_Property->setCurrentIndex(initialPropertyIndex);
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
    auto* pageJumpLabel = new QLabel(QStringLiteral("跳转到"), m_paginationWidget);
    m_pageJumpSpinBox = new QSpinBox(m_paginationWidget);
    m_pageJumpSpinBox->setObjectName(QStringLiteral("spinBox_PageJump"));
    m_pageJumpSpinBox->setRange(0, 0);
    // QSpinBox only applies specialValueText when it is non-empty. A single
    // space keeps the initial/empty-data state visually blank instead of
    // displaying either "0" or "无页".
    m_pageJumpSpinBox->setSpecialValueText(QStringLiteral(" "));
    m_pageJumpSpinBox->setAlignment(Qt::AlignCenter);
    m_pageJumpSpinBox->setFixedWidth(78);
    m_pageJumpSpinBox->setEnabled(false);
    auto* pageUnitLabel = new QLabel(QStringLiteral("页"), m_paginationWidget);
    m_pageJumpButton = new QPushButton(QStringLiteral("跳转"), m_paginationWidget);
    m_pageJumpButton->setObjectName(QStringLiteral("pushButton_PageJump"));
    m_pageJumpButton->setEnabled(false);

    paginationLayout->addWidget(m_previousPageButton);
    paginationLayout->addWidget(m_nextPageButton);
    paginationLayout->addWidget(m_pageSizeComboBox);
    paginationLayout->addStretch();
    paginationLayout->addWidget(m_pageInfoLabel);
    paginationLayout->addWidget(pageJumpLabel);
    paginationLayout->addWidget(m_pageJumpSpinBox);
    paginationLayout->addWidget(pageUnitLabel);
    paginationLayout->addWidget(m_pageJumpButton);
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
    connect(m_pageJumpButton, &QPushButton::clicked, this, [this]() {
        if (!m_pageJumpSpinBox || !m_pageJumpSpinBox->isEnabled()) return;
        m_currentPage = m_pageJumpSpinBox->value() - 1;
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

bool igQtSearchInfoWidget::propertyValue(int itemId, const PropertyDescriptor& descriptor, double& value) const {
    if (!m_currentModelData || itemId < 0) return false;
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

bool igQtSearchInfoWidget::hasSelectionForCurrentDataType() const {
    if (!m_currentSelection) return false;
    return !m_currentSelection->GetSelectedItems(m_currentDataType == 0 ? IG_POINT : IG_CELL).empty();
}

bool igQtSearchInfoWidget::currentPropertyValue(int itemId, double& value) const {
    const int propertyIndex = ui->comboBox_Property->currentIndex();
    if (propertyIndex < 0 || propertyIndex >= m_properties.size()) return false;
    return propertyValue(itemId, m_properties[propertyIndex], value);
}

void igQtSearchInfoWidget::rebuildFilteredItems(const QString& operatorStr, bool hasFilter, double filterValue) {
    m_filteredItemIds.clear();
    const int itemCount = currentItemCount();
    QVector<int> candidateIds;
    if (m_currentDataType == 1 && !m_selectedPartIds.isEmpty() && m_currentModelData->HasBlockMapping()) {
        auto* blockMapping = m_currentModelData->GetBlockMapping();
        const int mappedCellCount = blockMapping
                                            ? std::min(itemCount, static_cast<int>(blockMapping->GetNumberOfValues()))
                                            : 0;
        candidateIds.reserve(mappedCellCount);
        for (int cellId = 0; cellId < mappedCellCount; ++cellId) {
            const int partId = static_cast<int>(blockMapping->GetValue(cellId));
            if (m_selectedPartIds.contains(partId)) candidateIds.push_back(cellId);
        }
    } else if (hasSelectionForCurrentDataType()) {
        const auto& selectedItems =
                m_currentSelection->GetSelectedItems(m_currentDataType == 0 ? IG_POINT : IG_CELL);
        candidateIds.reserve(static_cast<int>(selectedItems.size()));
        for (igIndex itemId: selectedItems) {
            if (itemId >= 0 && itemId < itemCount) candidateIds.push_back(static_cast<int>(itemId));
        }
    } else {
        candidateIds.reserve(itemCount);
        for (int itemId = 0; itemId < itemCount; ++itemId) candidateIds.push_back(itemId);
    }

    m_filteredItemIds.reserve(candidateIds.size());
    for (int itemId: candidateIds) {
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

    QVector<const PropertyDescriptor*> displayedProperties;
    displayedProperties.reserve(m_properties.size());
    for (const auto& property: m_properties) {
        if (property.kind == PropertyDescriptor::Kind::Attribute) displayedProperties.push_back(&property);
    }

    auto* table = ui->tableWidget_Results;
    table->setSortingEnabled(false);
    table->clearContents();

    const bool isPointData = m_currentDataType == 0;
    if (isPointData) {
        table->setColumnCount(4 + displayedProperties.size());
        QStringList headers{QStringLiteral("点 ID"), "X", "Y", "Z"};
        for (const auto* property: displayedProperties) headers.push_back(property->displayName);
        table->setHorizontalHeaderLabels(headers);
    } else {
        table->setColumnCount(1 + displayedProperties.size());
        QStringList headers{QStringLiteral("单元 ID")};
        for (const auto* property: displayedProperties) headers.push_back(property->displayName);
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

        const int firstPropertyColumn = isPointData ? 4 : 1;
        for (int propertyOffset = 0; propertyOffset < displayedProperties.size(); ++propertyOffset) {
            double value = 0.0;
            auto* valueItem = new QTableWidgetItem;
            if (propertyValue(itemId, *displayedProperties[propertyOffset], value))
                valueItem->setData(Qt::DisplayRole, value);
            else
                valueItem->setText(QStringLiteral("—"));
            table->setItem(row, firstPropertyColumn + propertyOffset, valueItem);
        }
    }

    const bool hasPartFilter = m_currentDataType == 1 && !m_selectedPartIds.isEmpty() &&
                               m_currentModelData->HasBlockMapping();
    const QString resultScope = hasPartFilter
                                        ? QStringLiteral("选中零件")
                                        : (hasSelectionForCurrentDataType() ? QStringLiteral("选中数据")
                                                                           : QStringLiteral("全部数据"));
    ui->groupBox_Results->setTitle(
            QStringLiteral("%1：当前页 %2 条，共 %3 条").arg(resultScope).arg(displayedCount).arg(totalCount));
    m_pageInfoLabel->setText(
            pageCount == 0
                    ? QStringLiteral("第 0 / 0 页，共 0 条")
                    : QStringLiteral("第 %1 / %2 页，共 %3 条").arg(m_currentPage + 1).arg(pageCount).arg(totalCount));
    m_previousPageButton->setEnabled(m_currentPage > 0);
    m_nextPageButton->setEnabled(pageCount > 0 && m_currentPage + 1 < pageCount);
    {
        const QSignalBlocker blockPageJump(m_pageJumpSpinBox);
        if (pageCount == 0) {
            m_pageJumpSpinBox->setRange(0, 0);
            m_pageJumpSpinBox->setValue(0);
            m_pageJumpSpinBox->setEnabled(false);
            m_pageJumpButton->setEnabled(false);
        } else {
            m_pageJumpSpinBox->setRange(1, pageCount);
            m_pageJumpSpinBox->setValue(m_currentPage + 1);
            m_pageJumpSpinBox->setEnabled(true);
            m_pageJumpButton->setEnabled(true);
        }
    }
    table->resizeColumnsToContents();
    table->setSortingEnabled(true);
}
