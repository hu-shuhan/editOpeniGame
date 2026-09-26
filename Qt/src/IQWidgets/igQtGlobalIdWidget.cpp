#include "IQWidgets/igQtGlobalIdWidget.h"
#include "ui_igQtGlobalId.h"

#include <GlobalIds/iGameGenerateGlobalIdsFilter.h>
#include <iGameArrayObject.h>
#include <iGameAttributeSet.h>
#include <iGameDataObject.h>
#include <iGameModel.h>

#include <QComboBox>
#include <QDockWidget>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QTableWidgetItem>

#include <algorithm>
#include <cmath>
#include <limits>

using namespace iGame;

igQtGlobalIdWidget::igQtGlobalIdWidget(QWidget* parent) : QWidget(parent), ui(new Ui::igQtGlobalId) {
    ui->setupUi(this);
    initUI();
    initConnections();
}

igQtGlobalIdWidget::~igQtGlobalIdWidget() { delete ui; }

QDockWidget* igQtGlobalIdWidget::createDockWidget(QWidget* parent) {
    auto* dockWidget = new QDockWidget(QStringLiteral("全局ID"), parent);
    dockWidget->setObjectName(QStringLiteral("dockWidget_GlobalIds"));
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    dockWidget->setWidget(new igQtGlobalIdWidget(dockWidget));
    return dockWidget;
}

void igQtGlobalIdWidget::setCurrentModel(iGame::Model* model) {
    m_currentModel = model;
    m_currentModelData = model ? model->GetDataObject() : nullptr;
    resetOffsets();

    const bool hasPointIds = globalIdArray(0) != nullptr;
    const bool hasCellIds = globalIdArray(1) != nullptr;
    m_currentDataType = hasPointIds || !hasCellIds ? 0 : 1;
    {
        const QSignalBlocker blockPoints(ui->radioButton_PointIds);
        const QSignalBlocker blockCells(ui->radioButton_CellIds);
        ui->radioButton_PointIds->setChecked(m_currentDataType == 0);
        ui->radioButton_CellIds->setChecked(m_currentDataType == 1);
    }

    m_currentPage = 0;
    refreshResults();
}

void igQtGlobalIdWidget::resetOffsets() {
    ui->lineEdit_PointOffset->setText(QStringLiteral("0"));
    ui->lineEdit_CellOffset->setText(QStringLiteral("0"));
}

void igQtGlobalIdWidget::initUI() {
    const QRegularExpression unsignedInteger(QStringLiteral("[0-9]+"));
    ui->lineEdit_PointOffset->setValidator(new QRegularExpressionValidator(unsignedInteger, this));
    ui->lineEdit_CellOffset->setValidator(new QRegularExpressionValidator(unsignedInteger, this));

    auto* tableHeader = ui->tableWidget_Results->horizontalHeader();
    tableHeader->setStyleSheet(QStringLiteral(
            "QHeaderView { background-color: #333333; color: #FFFFFF; }"
            "QHeaderView::section {"
            "  background-color: #333333;"
            "  color: #FFFFFF;"
            "  border: 0px;"
            "  border-right: 1px solid #555555;"
            "  border-bottom: 1px solid #555555;"
            "  padding: 4px;"
            "}"));
    tableHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget_Results->verticalHeader()->setVisible(false);
    ui->tableWidget_Results->setSortingEnabled(false);

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
    // Keep the initial render light-weight. Larger pages remain available on demand.
    m_pageSizeComboBox->setCurrentIndex(0);
    m_pageInfoLabel = new QLabel(QStringLiteral("第 0 / 0 页，共 0 条"), m_paginationWidget);
    m_pageInfoLabel->setAlignment(Qt::AlignCenter);

    paginationLayout->addWidget(m_previousPageButton);
    paginationLayout->addWidget(m_nextPageButton);
    paginationLayout->addWidget(m_pageSizeComboBox);
    paginationLayout->addStretch();
    paginationLayout->addWidget(m_pageInfoLabel);
    ui->verticalLayout_Results->addWidget(m_paginationWidget);
}

void igQtGlobalIdWidget::initConnections() {
    connect(ui->pushButton_Confirm, &QPushButton::clicked, this, &igQtGlobalIdWidget::generateGlobalIds);
    connect(ui->pushButton_Cancel, &QPushButton::clicked, this, &igQtGlobalIdWidget::cancel);
    connect(ui->radioButton_PointIds, &QRadioButton::clicked, this, [this]() {
        m_currentDataType = 0;
        m_currentPage = 0;
        refreshResults();
    });
    connect(ui->radioButton_CellIds, &QRadioButton::clicked, this, [this]() {
        m_currentDataType = 1;
        m_currentPage = 0;
        refreshResults();
    });
    connect(m_previousPageButton, &QPushButton::clicked, this, [this]() {
        if (m_currentPage == 0) return;
        --m_currentPage;
        renderCurrentPage();
    });
    connect(m_nextPageButton, &QPushButton::clicked, this, [this]() {
        auto* array = globalIdArray(m_currentDataType);
        if (!array) return;
        const quint64 pageSize = static_cast<quint64>(m_pageSizeComboBox->currentData().toUInt());
        const quint64 totalCount = static_cast<quint64>(array->GetNumberOfElements());
        const quint64 pageCount = pageSize == 0 ? 0 : (totalCount + pageSize - 1) / pageSize;
        if (m_currentPage + 1 >= pageCount) return;
        ++m_currentPage;
        renderCurrentPage();
    });
    connect(m_pageSizeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        m_currentPage = 0;
        renderCurrentPage();
    });
}

void igQtGlobalIdWidget::generateGlobalIds() {
    if (!m_currentModel) {
        QMessageBox::warning(this, QStringLiteral("全局ID"), QStringLiteral("请先选择一个模型。"));
        return;
    }

    bool pointOffsetOk = false;
    bool cellOffsetOk = false;
    const auto pointOffset = ui->lineEdit_PointOffset->text().trimmed().toULongLong(&pointOffsetOk);
    const auto cellOffset = ui->lineEdit_CellOffset->text().trimmed().toULongLong(&cellOffsetOk);
    if (!pointOffsetOk || !cellOffsetOk) {
        QMessageBox::warning(this, QStringLiteral("全局ID"), QStringLiteral("点和单元 Offset 必须是非负整数。"));
        return;
    }

    auto dataObject = m_currentModel->GetDataObject();
    if (!dataObject) {
        QMessageBox::warning(this, QStringLiteral("全局ID"), QStringLiteral("当前模型没有可处理的数据。"));
        return;
    }

    const bool hasExistingIds = globalIdArray(0) != nullptr || globalIdArray(1) != nullptr;
    if (hasExistingIds) {
        const auto answer = QMessageBox::question(
                this, QStringLiteral("覆盖全局ID"),
                QStringLiteral("当前模型已存在全局 ID，是否使用新的 Offset 覆盖？"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
    }

    auto filter = GenerateGlobalIdsFilter::New();
    filter->SetInput(dataObject);
    filter->SetGeneratePointIds(true);
    filter->SetGenerateCellIds(true);
    filter->SetOffsets(static_cast<iguIndex64>(pointOffset), static_cast<iguIndex64>(cellOffset));
    filter->SetExistingIdPolicy(hasExistingIds ? GenerateGlobalIdsFilter::ExistingIdPolicy::Replace
                                               : GenerateGlobalIdsFilter::ExistingIdPolicy::Error);

    if (!filter->Execute()) {
        QMessageBox::warning(this, QStringLiteral("全局ID生成失败"), QString::fromStdString(filter->GetMessage()));
        return;
    }

    auto output = filter->GetOutput();
    if (!output) {
        QMessageBox::warning(this, QStringLiteral("全局ID生成失败"), QStringLiteral("过滤器没有生成输出模型。"));
        return;
    }

    m_currentModelData = output;
    emit resultReady(output);
    m_currentDataType = 0;
    m_currentPage = 0;
    {
        const QSignalBlocker blockPoints(ui->radioButton_PointIds);
        const QSignalBlocker blockCells(ui->radioButton_CellIds);
        ui->radioButton_PointIds->setChecked(true);
        ui->radioButton_CellIds->setChecked(false);
    }

    refreshResults();
}

void igQtGlobalIdWidget::cancel() {
    resetOffsets();
    emit cancelRequested();
}

iGame::ArrayObject* igQtGlobalIdWidget::globalIdArray(int dataType) const {
    if (!m_currentModelData) return nullptr;
    auto* attributeSet = m_currentModelData->GetAttributeSet();
    if (!attributeSet) return nullptr;
    auto attributes = attributeSet->GetAllAttributes();
    if (!attributes) return nullptr;

    const IGenum attachmentType = dataType == 0 ? IG_POINT : IG_CELL;
    const std::string attributeName = dataType == 0 ? "GlobalPointIds" : "GlobalCellIds";
    for (IGsize index = 0; index < attributes->GetNumberOfElements(); ++index) {
        auto& attribute = attributes->GetElement(index);
        if (attribute.isDeleted || !attribute.pointer || attribute.attachmentType != attachmentType) continue;
        if (attribute.pointer->GetName() != attributeName || attribute.pointer->GetDimension() != 1) continue;
        return attribute.pointer.GetPointer();
    }
    return nullptr;
}

QString igQtGlobalIdWidget::globalIdText(iGame::ArrayObject* array, quint64 index) const {
    if (!array || index >= static_cast<quint64>(array->GetNumberOfElements())) return QStringLiteral("—");
    const double value = array->GetElementValue(static_cast<IGsize>(index), 0);
    // quint64::max() rounds to 2^64 as double. Use a strict 2^64 upper bound so that
    // externally supplied arrays cannot trigger an out-of-range floating-to-integer conversion.
    const double quint64UpperBound = std::ldexp(1.0, std::numeric_limits<quint64>::digits);
    if (std::isfinite(value) && value >= 0.0 && std::floor(value) == value &&
        value < quint64UpperBound) {
        return QString::number(static_cast<quint64>(value));
    }
    return QString::number(value, 'g', 17);
}

void igQtGlobalIdWidget::refreshResults() {
    auto* array = globalIdArray(m_currentDataType);
    const QString arrayName = m_currentDataType == 0 ? QStringLiteral("GlobalPointIds")
                                                     : QStringLiteral("GlobalCellIds");
    if (!array) {
        ui->label_Summary->setText(QStringLiteral("%1：尚未生成").arg(arrayName));
    } else if (array->GetNumberOfElements() == 0) {
        ui->label_Summary->setText(QStringLiteral("%1 = []").arg(arrayName));
    } else {
        const quint64 lastIndex = static_cast<quint64>(array->GetNumberOfElements() - 1);
        ui->label_Summary->setText(QStringLiteral("%1 = [%2, %3]")
                                           .arg(arrayName, globalIdText(array, 0), globalIdText(array, lastIndex)));
    }
    renderCurrentPage();
}

void igQtGlobalIdWidget::renderCurrentPage() {
    auto* array = globalIdArray(m_currentDataType);
    const quint64 totalCount = array ? static_cast<quint64>(array->GetNumberOfElements()) : 0;
    const quint64 pageSize = static_cast<quint64>(m_pageSizeComboBox->currentData().toUInt());
    const quint64 pageCount = pageSize == 0 ? 0 : (totalCount + pageSize - 1) / pageSize;
    if (pageCount == 0) m_currentPage = 0;
    else if (m_currentPage >= pageCount)
        m_currentPage = pageCount - 1;

    const quint64 start = pageCount == 0 ? 0 : m_currentPage * pageSize;
    const quint64 end = std::min(start + pageSize, totalCount);
    const int displayedCount = static_cast<int>(end - start);

    auto* table = ui->tableWidget_Results;
    // Avoid a paint/layout pass for every inserted item while switching arrays.
    table->setUpdatesEnabled(false);
    table->clearContents();
    table->setRowCount(displayedCount);
    for (int row = 0; row < displayedCount; ++row) {
        const quint64 localId = start + static_cast<quint64>(row);
        auto* localItem = new QTableWidgetItem;
        localItem->setData(Qt::DisplayRole, QVariant::fromValue(localId));
        table->setItem(row, 0, localItem);

        auto* globalItem = new QTableWidgetItem;
        globalItem->setText(globalIdText(array, localId));
        table->setItem(row, 1, globalItem);
    }

    m_pageInfoLabel->setText(
            pageCount == 0
                    ? QStringLiteral("第 0 / 0 页，共 0 条")
                    : QStringLiteral("第 %1 / %2 页，共 %3 条")
                              .arg(QString::number(m_currentPage + 1), QString::number(pageCount),
                                   QString::number(totalCount)));
    m_previousPageButton->setEnabled(m_currentPage > 0);
    m_nextPageButton->setEnabled(pageCount > 0 && m_currentPage + 1 < pageCount);
    table->setUpdatesEnabled(true);
    table->viewport()->update();
}
