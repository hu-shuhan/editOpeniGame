#include <IQWidgets/igQtExtractTimeStepsWidget.h>

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QBrush>
#include <QColor>
#include <QItemSelectionModel>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "iGameStreamingData.h"

#include <algorithm>

namespace {

constexpr int kColumnIndex = 0;
constexpr int kColumnTimeValue = 1;

} // namespace

igQtExtractTimeStepsWidget::igQtExtractTimeStepsWidget(QWidget* parent)
    : QWidget(parent) {
    BuildUi();
    Reset();
}

void igQtExtractTimeStepsWidget::BuildUi() {
    setObjectName(QStringLiteral("ExtractTimeStepsWidget"));
    setMinimumWidth(280);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // 当前模型的时间步概况
    m_infoLabel = new QLabel(this);
    m_infoLabel->setObjectName(QStringLiteral("ExtractTimeStepsInfoLabel"));
    m_infoLabel->setWordWrap(true);
    root->addWidget(m_infoLabel);

    // 选择模式
    auto* modeRow = new QHBoxLayout();
    modeRow->addWidget(new QLabel(QStringLiteral("选择模式"), this));
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(QStringLiteral("按索引选择"));
    m_modeCombo->addItem(QStringLiteral("按区间+步长"));
    modeRow->addWidget(m_modeCombo, 1);
    root->addLayout(modeRow);

    // 索引模式：索引列表（用「+」添加行、手动填索引）
    m_indexPanel = new QWidget(this);
    auto* indexLayout = new QVBoxLayout(m_indexPanel);
    indexLayout->setContentsMargins(0, 0, 0, 0);
    indexLayout->setSpacing(6);

    m_indexTable = new QTableWidget(m_indexPanel);
    m_indexTable->setObjectName(QStringLiteral("ExtractTimeStepsIndexTable"));
    m_indexTable->setColumnCount(2);
    m_indexTable->setHorizontalHeaderLabels(QStringList{QStringLiteral("索引"), QStringLiteral("时间值")});
    m_indexTable->verticalHeader()->setVisible(false);
    m_indexTable->setAlternatingRowColors(true);
    m_indexTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_indexTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_indexTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked |
                                  QAbstractItemView::EditKeyPressed);
    m_indexTable->setMinimumHeight(140);
    m_indexTable->horizontalHeader()->setMinimumSectionSize(64);
    m_indexTable->horizontalHeader()->setSectionResizeMode(kColumnIndex, QHeaderView::Interactive);
    m_indexTable->setColumnWidth(kColumnIndex, 72);
    m_indexTable->horizontalHeader()->setSectionResizeMode(kColumnTimeValue, QHeaderView::Stretch);
    // 深色主题：表格显式配深底浅字，避免表头/交替行白底白字
    m_indexTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #2b2b2b;
            alternate-background-color: #3a3a3a;
            color: #e8e8e8;
            gridline-color: #4a4a4a;
            selection-background-color: #4a6fa5;
            selection-color: #ffffff;
        }
        QHeaderView::section {
            background-color: #3a3a3a;
            color: #e8e8e8;
            border: none;
            padding: 4px;
        }
        QTableCornerButton::section { background-color: #3a3a3a; }
    )");
    indexLayout->addWidget(m_indexTable, 1);

    auto* indexButtonRow = new QHBoxLayout();
    m_addIndexButton = new QPushButton(QStringLiteral("+"), m_indexPanel);
    m_addIndexButton->setToolTip(QStringLiteral("添加一个索引"));
    m_addIndexButton->setFixedWidth(32);
    m_removeIndexButton = new QPushButton(QStringLiteral("−"), m_indexPanel);
    m_removeIndexButton->setToolTip(QStringLiteral("删除选中行"));
    m_removeIndexButton->setFixedWidth(32);
    m_clearIndicesButton = new QPushButton(QStringLiteral("清空"), m_indexPanel);
    indexButtonRow->addWidget(m_addIndexButton);
    indexButtonRow->addWidget(m_removeIndexButton);
    indexButtonRow->addWidget(m_clearIndicesButton);
    indexButtonRow->addStretch(1);
    indexLayout->addLayout(indexButtonRow);

    root->addWidget(m_indexPanel, 1);

    // 区间模式：起始帧 / 结束帧 / 步长（结束帧填 -1 表示到最后一帧）
    m_rangePanel = new QWidget(this);
    auto* rangeGrid = new QGridLayout(m_rangePanel);
    rangeGrid->setContentsMargins(0, 0, 0, 0);
    rangeGrid->setHorizontalSpacing(6);
    rangeGrid->setVerticalSpacing(4);
    m_beginSpin = new QSpinBox(m_rangePanel);
    m_endSpin = new QSpinBox(m_rangePanel);
    m_intervalSpin = new QSpinBox(m_rangePanel);
    m_intervalSpin->setMinimum(1);
    rangeGrid->addWidget(new QLabel(QStringLiteral("起始帧"), m_rangePanel), 0, 0);
    rangeGrid->addWidget(m_beginSpin, 0, 1);
    rangeGrid->addWidget(new QLabel(QStringLiteral("结束帧"), m_rangePanel), 1, 0);
    rangeGrid->addWidget(m_endSpin, 1, 1);
    rangeGrid->addWidget(new QLabel(QStringLiteral("步长"), m_rangePanel), 2, 0);
    rangeGrid->addWidget(m_intervalSpin, 2, 1);
    root->addWidget(m_rangePanel);

    // 执行 / 重置
    auto* buttonRow = new QHBoxLayout();
    m_applyButton = new QPushButton(QStringLiteral("执行 (Apply)"), this);
    m_resetButton = new QPushButton(QStringLiteral("重置"), this);
    buttonRow->addWidget(m_applyButton);
    buttonRow->addWidget(m_resetButton);
    root->addLayout(buttonRow);

    // 结果 / 错误提示
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("ExtractTimeStepsStatusLabel"));
    m_statusLabel->setWordWrap(true);
    root->addWidget(m_statusLabel);

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        UpdateModeUi();
    });
    connect(m_addIndexButton, &QPushButton::clicked, this, [this]() { AppendIndexRow(); });
    connect(m_removeIndexButton, &QPushButton::clicked, this, [this]() {
        auto selectedRows = m_indexTable->selectionModel()->selectedRows();
        if (selectedRows.isEmpty()) {
            if (m_indexTable->rowCount() > 0) {
                m_indexTable->removeRow(m_indexTable->rowCount() - 1);
            }
        } else {
            std::vector<int> rows;
            rows.reserve(static_cast<std::size_t>(selectedRows.size()));
            for (const auto& index : selectedRows) { rows.push_back(index.row()); }
            std::sort(rows.rbegin(), rows.rend());
            for (int row : rows) { m_indexTable->removeRow(row); }
        }
        SetStatus(QString());
    });
    connect(m_clearIndicesButton, &QPushButton::clicked, this, [this]() {
        m_indexTable->setRowCount(0);
        SetStatus(QString());
    });
    connect(m_indexTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (item == nullptr) { return; }
        if (item->column() == kColumnIndex) { RefreshTimeValueCell(item->row()); }
        SetStatus(QString());
    });
    connect(m_beginSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { UpdateStatusPreview(); });
    connect(m_endSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { UpdateStatusPreview(); });
    connect(m_intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { UpdateStatusPreview(); });
    connect(m_applyButton, &QPushButton::clicked, this, &igQtExtractTimeStepsWidget::Apply);
    connect(m_resetButton, &QPushButton::clicked, this, &igQtExtractTimeStepsWidget::Reset);

    UpdateModeUi();
}

void igQtExtractTimeStepsWidget::SetOriginDataObject(iGame::DataObject::Pointer object) {
    m_InputObject = object;
    RebuildTimeStepTable();
}

void igQtExtractTimeStepsWidget::RebuildTimeStepTable() {
    if (m_indexTable == nullptr) { return; }

    m_indexTable->setRowCount(0);

    auto frames = m_InputObject ? m_InputObject->PeekTimeFrames() : nullptr;
    if (!frames || frames->GetTimeNum() == 0) {
        m_infoLabel->setText(QStringLiteral("当前模型没有时间序列"));
        m_applyButton->setEnabled(false);
        SetStatus(QString());
        return;
    }

    const int timeStepCount = static_cast<int>(frames->GetTimeNum());
    m_infoLabel->setText(QStringLiteral("共 %1 个时间步（索引 0 ~ %2）").arg(timeStepCount).arg(timeStepCount - 1));

    m_beginSpin->setRange(0, timeStepCount - 1);
    m_endSpin->setRange(-1, timeStepCount - 1);
    m_intervalSpin->setRange(1, timeStepCount > 0 ? timeStepCount : 1);
    m_beginSpin->setValue(0);
    m_endSpin->setValue(timeStepCount - 1);
    m_intervalSpin->setValue(1);

    m_applyButton->setEnabled(true);
    UpdateStatusPreview();
}

void igQtExtractTimeStepsWidget::AppendIndexRow(int index) {
    const int row = m_indexTable->rowCount();
    m_indexTable->insertRow(row);

    auto* indexItem = new QTableWidgetItem(index >= 0 ? QString::number(index) : QString());
    m_indexTable->setItem(row, kColumnIndex, indexItem);

    auto* valueItem = new QTableWidgetItem();
    valueItem->setFlags(Qt::ItemIsEnabled); // 时间值只读，由索引自动带出
    m_indexTable->setItem(row, kColumnTimeValue, valueItem);

    RefreshTimeValueCell(row);
    m_indexTable->setCurrentCell(row, kColumnIndex);
    m_indexTable->editItem(indexItem); // 直接进入编辑，省一次点击
}

void igQtExtractTimeStepsWidget::RefreshTimeValueCell(int row) {
    auto* indexItem = m_indexTable->item(row, kColumnIndex);
    auto* valueItem = m_indexTable->item(row, kColumnTimeValue);
    if (indexItem == nullptr || valueItem == nullptr) { return; }

    auto frames = m_InputObject ? m_InputObject->PeekTimeFrames() : nullptr;
    if (!frames) {
        valueItem->setText(QString());
        return;
    }

    bool ok = false;
    const int index = indexItem->text().trimmed().toInt(&ok);
    const int timeStepCount = static_cast<int>(frames->GetTimeNum());
    if (!ok || index < 0 || index >= timeStepCount) {
        valueItem->setText(QString());
        return;
    }
    const float timeValue = frames->GetTargetTimeFrame(static_cast<unsigned int>(index)).GetTimeValue();
    valueItem->setText(QString::number(static_cast<double>(timeValue), 'g', 8));
}

bool igQtExtractTimeStepsWidget::CollectIndices(std::vector<int>& indices) {
    indices.clear();
    bool valid = true;

    for (int row = 0; row < m_indexTable->rowCount(); ++row) {
        auto* item = m_indexTable->item(row, kColumnIndex);
        const QString text = item != nullptr ? item->text().trimmed() : QString();
        if (text.isEmpty()) { continue; } // 空行忽略

        bool ok = false;
        const int index = text.toInt(&ok);
        if (!ok) {
            if (item != nullptr) { item->setBackground(QColor(120, 40, 40)); }
            valid = false;
            continue;
        }
        if (item != nullptr) { item->setBackground(QBrush()); }
        indices.push_back(index);
    }
    return valid;
}

void igQtExtractTimeStepsWidget::UpdateModeUi() {
    const bool rangeMode = (m_modeCombo != nullptr) && (m_modeCombo->currentIndex() == 1);
    if (m_indexPanel != nullptr) { m_indexPanel->setVisible(!rangeMode); }
    if (m_rangePanel != nullptr) { m_rangePanel->setVisible(rangeMode); }
    UpdateStatusPreview();
}

void igQtExtractTimeStepsWidget::UpdateStatusPreview() {
    auto frames = m_InputObject ? m_InputObject->PeekTimeFrames() : nullptr;
    if (!frames || frames->GetTimeNum() == 0) {
        SetStatus(QString());
        return;
    }
    if (m_modeCombo == nullptr || m_modeCombo->currentIndex() != 1) {
        SetStatus(QString()); // 索引模式不做实时预览
        return;
    }

    const int timeStepCount = static_cast<int>(frames->GetTimeNum());
    const int stride = (m_intervalSpin != nullptr && m_intervalSpin->value() > 0) ? m_intervalSpin->value() : 1;
    const int begin = (m_beginSpin != nullptr) ? std::max(m_beginSpin->value(), 0) : 0;
    const int rawEnd = (m_endSpin != nullptr) ? m_endSpin->value() : (timeStepCount - 1);
    const int end = (rawEnd < 0) ? (timeStepCount - 1) : std::min(rawEnd, timeStepCount - 1);
    const int kept = (end < begin) ? 0 : ((end - begin) / stride + 1);

    if (kept <= 0) {
        SetStatus(QStringLiteral("区间为空：将保留全部帧"));
    } else {
        SetStatus(QStringLiteral("将保留 %1 帧（%2 → %3，步长 %4）").arg(kept).arg(begin).arg(end).arg(stride));
    }
}

void igQtExtractTimeStepsWidget::SetStatus(const QString& text, bool isError) {
    if (m_statusLabel == nullptr) { return; }
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(isError ? QStringLiteral("color: rgb(255, 120, 120);")
                                         : QStringLiteral("color: rgb(200, 200, 200);"));
}

void igQtExtractTimeStepsWidget::Reset() {
    if (m_indexTable != nullptr) { m_indexTable->setRowCount(0); }
    if (m_modeCombo != nullptr) { m_modeCombo->setCurrentIndex(0); }
    if (m_beginSpin != nullptr) { m_beginSpin->setValue(m_beginSpin->minimum()); }
    if (m_endSpin != nullptr) { m_endSpin->setValue(m_endSpin->maximum()); }
    if (m_intervalSpin != nullptr) { m_intervalSpin->setValue(1); }
    UpdateModeUi();
    SetStatus(QString());
}

void igQtExtractTimeStepsWidget::Apply() {
    if (!m_InputObject) {
        SetStatus(QStringLiteral("当前没有模型"), true);
        return;
    }

    auto frames = m_InputObject->PeekTimeFrames();
    if (!frames || frames->GetTimeNum() == 0) {
        SetStatus(QStringLiteral("当前模型没有时间序列"), true);
        return;
    }

    // 每次执行都新建 filter，避免上一次参数/结果残留
    m_Filter = iGame::ExtractTimeStepsFilter::New();
    if (m_modeCombo != nullptr && m_modeCombo->currentIndex() == 1) {
        m_Filter->SetSelectionMode(iGame::ExtractTimeStepsFilter::SELECT_TIME_RANGE);
        m_Filter->SetTimeStepRange(m_beginSpin->value(), m_endSpin->value());
        m_Filter->SetTimeStepInterval(m_intervalSpin->value());
    } else {
        std::vector<int> indices;
        if (!CollectIndices(indices)) {
            SetStatus(QStringLiteral("索引必须是整数"), true);
            return;
        }
        m_Filter->SetSelectionMode(iGame::ExtractTimeStepsFilter::SELECT_TIME_STEPS);
        m_Filter->SetTimeStepIndices(indices);
    }

    m_Filter->SetInput(m_InputObject);
    if (!m_Filter->Execute()) {
        SetStatus(QStringLiteral("执行失败"), true);
        return;
    }

    auto output = m_Filter->GetOutput();
    if (!output) {
        SetStatus(QStringLiteral("输出对象为空"), true);
        return;
    }

    // 回显实际保留的帧
    QStringList keptText;
    for (int index : m_Filter->GetKeptTimeStepIndices()) {
        keptText << QString::number(index);
    }
    SetStatus(QStringLiteral("已保留 %1 帧（索引 %2）")
                      .arg(m_Filter->GetNumberOfKeptTimeSteps())
                      .arg(keptText.join(QStringLiteral(", "))));

    emit ExtractTimeStepsApplied(output);
}
