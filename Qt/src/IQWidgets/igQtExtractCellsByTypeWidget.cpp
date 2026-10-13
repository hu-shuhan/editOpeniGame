#include <IQWidgets/igQtExtractCellsByTypeWidget.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

igQtExtractCellsByTypeWidget::igQtExtractCellsByTypeWidget(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("ExtractCellsByTypeWidget"));
    setMinimumWidth(280);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // 标题 / 说明
    m_infoLabel = new QLabel(QStringLiteral("按单元类型提取"), this);
    m_infoLabel->setObjectName(QStringLiteral("ExtractCellsByTypeInfoLabel"));
    m_infoLabel->setWordWrap(true);
    root->addWidget(m_infoLabel);

    // 全选 / 全不选
    auto* toolbar = new QHBoxLayout();
    m_selectAllButton = new QPushButton(QStringLiteral("全选"), this);
    m_clearAllButton = new QPushButton(QStringLiteral("全不选"), this);
    toolbar->addWidget(m_selectAllButton);
    toolbar->addWidget(m_clearAllButton);
    root->addLayout(toolbar);

    // 提取按钮
    m_applyButton = new QPushButton(QStringLiteral("提取 (Extract)"), this);
    root->addWidget(m_applyButton);

    connect(m_selectAllButton, &QPushButton::clicked, this, [this]() {
        for (auto* cb : m_CheckBoxes) { cb->setChecked(true); }
    });
    connect(m_clearAllButton, &QPushButton::clicked, this, [this]() {
        for (auto* cb : m_CheckBoxes) { cb->setChecked(false); }
    });
    connect(m_applyButton, &QPushButton::clicked, this, &igQtExtractCellsByTypeWidget::onApplyClicked);
}

void igQtExtractCellsByTypeWidget::SetDataObject(iGame::DataObject::Pointer obj) {
    m_DataObject = obj;

    // 用核心 filter 扫描模型中实际存在的单元类型（复用框架逻辑，避免重复实现）
    m_Types.clear();
    if (obj) {
        auto scanner = iGame::ExtractCellsByTypeFilter::New();
        scanner->SetInput(obj);
        m_Types = scanner->GetAvailableCellTypes();
    }
    rebuildCheckBoxes();
}

std::vector<IGenum> igQtExtractCellsByTypeWidget::GetSelectedCellTypes() const {
    std::vector<IGenum> selected;
    const size_t n = (m_Types.size() < m_CheckBoxes.size()) ? m_Types.size() : m_CheckBoxes.size();
    for (size_t i = 0; i < n; i++) {
        if (m_CheckBoxes[i] && m_CheckBoxes[i]->isChecked()) { selected.push_back(m_Types[i]); }
    }
    return selected;
}

void igQtExtractCellsByTypeWidget::rebuildCheckBoxes() {
    auto* root = qobject_cast<QVBoxLayout*>(layout());

    // 清掉旧勾选框（先从布局移除，再延迟销毁）
    for (auto* cb : m_CheckBoxes) {
        if (cb) {
            if (root) { root->removeWidget(cb); }
            cb->deleteLater();
        }
    }
    m_CheckBoxes.clear();

    // 说明：当前模型有几种单元类型
    if (m_Types.empty()) {
        m_infoLabel->setText(QStringLiteral("按单元类型提取\n当前模型没有可提取的单元"));
        return;
    }
    m_infoLabel->setText(QStringLiteral("按单元类型提取（勾选要提取的类型）"));

    // 注意：勾选框必须插到"全选/全不选"和"提取"按钮之间，
    // 因此插入到布局第 2 个位置（标题=0，工具栏=1）
    int insertPos = 2;
    for (size_t i = 0; i < m_Types.size(); i++) {
        auto* cb = new QCheckBox(
            QString::fromStdString(iGame::ExtractCellsByTypeFilter::GetCellTypeDisplayName(m_Types[i])), this);
        cb->setChecked(true); // 默认全部勾选
        root->insertWidget(insertPos++, cb);
        m_CheckBoxes.push_back(cb);
    }
}

void igQtExtractCellsByTypeWidget::onApplyClicked() {
    if (onApply) { onApply(); }
    Q_EMIT applyRequested();
}
