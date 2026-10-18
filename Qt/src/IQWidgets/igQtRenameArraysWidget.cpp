//
// Created by Ayanami on 2026/9/30.
//
// 重命名数组面板：左列显示输入模型的点/单元数组原名，右列可编辑新名；
// 点「执行」由 RenameArrays filter 生成一个全新的网格（输入不被修改）并加入模型树。
//

#include "IQWidgets/igQtRenameArraysWidget.h"

#include "IQComponents/igQtModelDialogWidget.h"
#include "RenameArrays/iGameRenameArrays.h"
#include "iGameModel.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameType.h"

#include <QApplication>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QShowEvent>
#include <QStyledItemDelegate>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace {

// Inline editors share a list row's limited height. Global input padding and
// translucent selection colors can otherwise clip text and wash out selection.
class ArrayNameDelegate final : public QStyledItemDelegate {
public:
    explicit ArrayNameDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override {
        auto* editor = qobject_cast<QLineEdit*>(QStyledItemDelegate::createEditor(parent, option, index));
        if (!editor) { return nullptr; }
        const bool light = option.palette.color(QPalette::Base).lightness() > 128;
        editor->setObjectName(QStringLiteral("renameArrayEditor"));
        editor->setFont(option.font);
        editor->setFrame(false);
        editor->setMinimumSize(0, 0);
        editor->setStyleSheet(QStringLiteral(
                "QLineEdit#renameArrayEditor { background-color: %1; color: %2;"
                " border: 1px solid #2563EB; border-radius: 3px;"
                " padding: 0px 5px; margin: 0px; min-height: 0px;"
                " selection-background-color: #2563EB; selection-color: #FFFFFF; }")
                .arg(light ? QStringLiteral("#FFFFFF") : QStringLiteral("#202938"),
                     light ? QStringLiteral("#1F2A3A") : QStringLiteral("#F1F5F9")));
        return editor;
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                              const QModelIndex&) const override {
        editor->setGeometry(option.rect.adjusted(2, 2, -2, -2));
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(std::max(size.height(), option.fontMetrics.height() + 16));
        return size;
    }
};

/** 把一组原名填进左右两个列表：左列只读，右列可编辑且默认等于原名 */
void FillListPair(QListWidget* original, QListWidget* renamed, const std::vector<std::string>& names) {
    if (original == nullptr || renamed == nullptr) { return; }
    original->clear();
    renamed->clear();

    for (const std::string& name : names) {
        const QString text = QString::fromStdString(name);
        original->addItem(text);

        auto* item = new QListWidgetItem(text, renamed);
        // QListWidgetItem 默认不含 ItemIsEditable，需要显式打开才能编辑新名
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
}

} // namespace

igQtRenameArrays::igQtRenameArrays(igQtModelDialogWidget* modelTreeWidget, QWidget* parent)
    : QWidget(parent), ui(new Ui::RenameArraysWidget) {
    ui->setupUi(this);
    for (auto* list : {ui->leftlistwidget, ui->rightlistwidget,
                       ui->leftlistwidget_2, ui->rightlistwidget_2}) {
        list->setItemDelegate(new ArrayNameDelegate(list));
    }

    // 从选择的模型获得最近的模型
    m_modelTreeWidget = (modelTreeWidget != nullptr) ? modelTreeWidget : ResolveModelTreeWidget();

    // 执行相应操作
    connect(ui->pushButton, &QPushButton::clicked, this, &igQtRenameArrays::Execute);

    // 输入框行为：同步左右两个列表框的滚动
    ui->leftlistwidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(ui->rightlistwidget->verticalScrollBar(), &QScrollBar::valueChanged,
            ui->leftlistwidget->verticalScrollBar(), &QScrollBar::setValue);

    ui->leftlistwidget_2->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(ui->rightlistwidget_2->verticalScrollBar(), &QScrollBar::valueChanged,
            ui->leftlistwidget_2->verticalScrollBar(), &QScrollBar::setValue);

    BindCurrentModel();
}

igQtRenameArrays::~igQtRenameArrays() {
    delete ui;
}

void igQtRenameArrays::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // 每次显示都按当前模型刷新两个列表
    BindCurrentModel();
}

igQtModelDialogWidget* igQtRenameArrays::ResolveModelTreeWidget() {
    if (m_modelTreeWidget != nullptr) { return m_modelTreeWidget; }

    // 1) 优先在自己的顶层窗口里查找
    QWidget* top = this->window();
    if (top != nullptr) {
        if (auto* widget = top->findChild<igQtModelDialogWidget*>()) { return widget; }
    }
    // 2) 回退：遍历应用程序的所有顶层窗口（浮动 Dock 等情况）
    const QWidgetList topLevels = QApplication::topLevelWidgets();
    for (QWidget* level : topLevels) {
        if (level == nullptr) { continue; }
        if (auto* widget = level->findChild<igQtModelDialogWidget*>()) { return widget; }
    }
    return nullptr;
}

/* ------------------------------------------------------------------ */
/* 输入与列表                                                          */
/* ------------------------------------------------------------------ */
void igQtRenameArrays::BindCurrentModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto model = (scene != nullptr) ? scene->GetCurrentModel() : nullptr;

    m_currentModel = (model != nullptr) ? model->GetDataObject() : nullptr;
    m_AttributeSet = (m_currentModel != nullptr) ? m_currentModel->GetAttributeSet() : nullptr;

    if (ui->label_ModelName != nullptr) {
        ui->label_ModelName->setText(m_currentModel != nullptr
                                             ? QString::fromStdString(m_currentModel->GetName())
                                             : QStringLiteral("未选择（请在模型树中选中一个模型）"));
    }

    SetPointArrays();
    SetCellArrays();
}

void igQtRenameArrays::SetPointArrays() {
    std::vector<std::string> names;
    if (m_currentModel != nullptr) {
        // 用 filter 的查询接口取名字，保证列表顺序与 Execute 时按下标解析的顺序一致
        iGame::RenameArrays::Pointer filter = iGame::RenameArrays::New();
        filter->SetInput(m_currentModel);
        const int count = filter->GetNumberOfArrays(IG_POINT);
        for (int i = 0; i < count; ++i) { names.push_back(filter->GetArrayOriginalName(IG_POINT, i)); }
    }
    FillListPair(ui->leftlistwidget, ui->rightlistwidget, names);
}

void igQtRenameArrays::SetCellArrays() {
    std::vector<std::string> names;
    if (m_currentModel != nullptr) {
        iGame::RenameArrays::Pointer filter = iGame::RenameArrays::New();
        filter->SetInput(m_currentModel);
        const int count = filter->GetNumberOfArrays(IG_CELL);
        for (int i = 0; i < count; ++i) { names.push_back(filter->GetArrayOriginalName(IG_CELL, i)); }
    }
    FillListPair(ui->leftlistwidget_2, ui->rightlistwidget_2, names);
}

/* ------------------------------------------------------------------ */
/* 执行                                                                */
/* ------------------------------------------------------------------ */
void igQtRenameArrays::Execute() {
    ui->label_Message->clear();

    if (m_currentModel == nullptr) { BindCurrentModel(); }
    if (m_currentModel == nullptr) {
        ui->label_Message->setText(QStringLiteral("请先在模型树中选中一个模型作为输入。"));
        return;
    }

    iGame::RenameArrays::Pointer filter = iGame::RenameArrays::New();
    filter->SetInput(m_currentModel);

    // Validate final names, including unchanged names, within each association.
    // The status label can be below the visible dock area; show failures in a
    // dialog as well so a rejected rename cannot look like an unresponsive Apply.
    const auto validateNames = [this](QListWidget* original, QListWidget* renamed, const QString& group) {
        std::set<QString> names;
        for (int i = 0; i < std::min(original->count(), renamed->count()); ++i) {
            QString name = renamed->item(i)->text().trimmed();
            if (name.isEmpty()) { name = original->item(i)->text(); }
            if (!names.insert(name).second) {
                const QString message = QStringLiteral("%1中存在重复的数组名“%2”。\n同一组中的数组名称必须唯一，请修改后再执行。")
                                                .arg(group, name);
                ui->label_Message->setText(message);
                renamed->setCurrentRow(i);
                renamed->scrollToItem(renamed->item(i));
                QMessageBox::warning(this, QStringLiteral("重命名失败"), message);
                renamed->setFocus();
                return false;
            }
        }
        return true;
    };
    if (!validateNames(ui->leftlistwidget, ui->rightlistwidget, QStringLiteral("点数组（Point）")) ||
        !validateNames(ui->leftlistwidget_2, ui->rightlistwidget_2, QStringLiteral("单元数组（Cell）"))) {
        return;
    }

    // 左右列表一一对应；右侧与左侧不同才配置改名（相同/为空都跳过）
    const auto collect = [&filter](IGenum attachmentType, QListWidget* original, QListWidget* renamed) {
        const int count = std::min(original->count(), renamed->count());
        for (int i = 0; i < count; ++i) {
            const std::string originalName = original->item(i)->text().toStdString();
            const std::string newName = renamed->item(i)->text().trimmed().toStdString();
            if (newName.empty() || newName == originalName) { continue; }
            filter->SetArrayName(attachmentType, originalName, newName);
        }
    };
    collect(IG_POINT, ui->leftlistwidget, ui->rightlistwidget);
    collect(IG_CELL, ui->leftlistwidget_2, ui->rightlistwidget_2);

    if (!filter->Execute()) {
        const QString message = QString::fromStdString(filter->GetMessage());
        ui->label_Message->setText(message);
        QMessageBox::warning(this, QStringLiteral("重命名失败"), message);
        return;
    }

    auto output = filter->GetOutput(0);
    if (output == nullptr) {
        ui->label_Message->setText(QStringLiteral("没有生成结果。"));
        return;
    }

    // 结果命名：<输入名>_rename，与场景中已有模型重名时加序号
    std::string resultName = m_currentModel->GetName() + "_rename";
    {
        std::set<std::string> existing;
        if (auto scene = iGame::SceneManager::Instance()->GetCurrentScene()) {
            for (auto it = scene->GetModelList()->Begin(); it != scene->GetModelList()->End(); ++it) {
            const auto& entry = *it;
                if (entry.second == nullptr) { continue; }
                auto object = entry.second->GetDataObject();
                if (object != nullptr) { existing.insert(object->GetName()); }
            }
        }
        if (existing.find(resultName) != existing.end()) {
            for (int i = 1; i < 100000; ++i) {
                const std::string candidate = resultName + "_" + std::to_string(i);
                if (existing.find(candidate) == existing.end()) {
                    resultName = candidate;
                    break;
                }
            }
        }
    }
    output->SetName(resultName);

    if (m_modelTreeWidget == nullptr) { m_modelTreeWidget = ResolveModelTreeWidget(); }
    if (m_modelTreeWidget != nullptr) {
        m_modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
        // 数组名变了，刷新模型树里的属性子项
        m_modelTreeWidget->updateAllAttriubute(output);
    }

    if (auto scene = iGame::SceneManager::Instance()->GetCurrentScene()) { scene->Update(); }

    //此处是显示执行后信息
    // ui->label_Message->setText(QStringLiteral("已生成新网格：") + QString::fromStdString(resultName) +
    //                            QStringLiteral("\n") + QString::fromStdString(filter->GetMessage()));

    // 新结果已成为当前模型，刷新列表：左列显示改名后的数组，右列回到原名
    BindCurrentModel();
}
