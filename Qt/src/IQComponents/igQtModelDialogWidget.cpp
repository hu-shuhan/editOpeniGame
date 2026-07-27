#include "Sources/iGameLineTypePointsSourceFilter.h"
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQCore/igQtAttributeDataSourceManager.h>
#include <Plugin/qtpropertybrowser/qtpropertymanager.h>
#include <QApplication>
#include <QMainWindow>
#include <QQueue>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QColor>
#include <QHeaderView>
#include <QPalette>
#include <iGameSceneManager.h>
#include <qaction.h>
#include <qdebug.h>
#include <qmenu.h>

#include <map>
#include <unordered_map>
#include <vector>

namespace
{
// A small custom title bar for frameless floating QDockWidget.
// - Provides drag-to-move behavior
// - Provides a close button
class DockTitleBar final : public QWidget {
public:
    explicit DockTitleBar(QDockWidget* dock, const QString& title, QWidget* parent = nullptr)
        : QWidget(parent), m_dock(dock) {
        setObjectName("DockTitleBar");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(32);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 0, 6, 0);
        layout->setSpacing(8);

        m_titleLabel = new QLabel(title, this);
        m_titleLabel->setObjectName("DockTitleLabel");
        m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(m_titleLabel);

        auto* closeBtn = new QPushButton(this);
        closeBtn->setObjectName("DockTitleCloseButton");
        closeBtn->setFixedSize(24, 24);
        closeBtn->setFlat(true);
        closeBtn->setFocusPolicy(Qt::NoFocus);
        closeBtn->setIcon(QIcon(":/Ticon/Icons/dock_close_white.svg"));
        closeBtn->setIconSize(QSize(16, 16));
        layout->addWidget(closeBtn);

        if (m_dock) {
            connect(closeBtn, &QPushButton::clicked, m_dock, &QDockWidget::close);
        }
    }

    void setTitle(const QString& t) {
        if (m_titleLabel) m_titleLabel->setText(t);
    }

protected:
    void mousePressEvent(QMouseEvent* e) override {
        if (!m_dock) return QWidget::mousePressEvent(e);
        if (e->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragOffset = e->globalPos() - m_dock->frameGeometry().topLeft();
            e->accept();
            return;
        }
        QWidget::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        if (!m_dock) return QWidget::mouseMoveEvent(e);
        if (m_dragging && (e->buttons() & Qt::LeftButton)) {
            m_dock->move(e->globalPos() - m_dragOffset);
            e->accept();
            return;
        }
        QWidget::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            m_dragging = false;
            e->accept();
            return;
        }
        QWidget::mousePressEvent(e);
    }

private:
    QDockWidget* m_dock = nullptr;
    QLabel* m_titleLabel = nullptr;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

using AttributeDescriptorMap = std::map<int, std::vector<iGame::AttributeDataDescriptor>>;

static AttributeDescriptorMap BuildAttributeDescriptorMap(
    const iGame::AttributeDataSourcePointer& source) {
    AttributeDescriptorMap descriptorsByObject;
    if (source == nullptr) return descriptorsByObject;
    std::unordered_map<std::string, iGame::DataObject::Pointer> objectsByPath;
    for (const auto& descriptor : source->Attributes()) {
        auto [targetIterator, inserted] = objectsByPath.try_emplace(
            descriptor.target.blockPath,
            iGame::DataObject::Pointer{});
        if (inserted) {
            targetIterator->second = source->TargetObject(descriptor.target);
        }
        if (targetIterator->second != nullptr) {
            descriptorsByObject[targetIterator->second->GetDataObjectId()].push_back(descriptor);
        }
    }
    return descriptorsByObject;
}

static void AddTopLevelAttributes(
    QTreeWidget* tree,
    ModelTreeWidgetItem* parentItem,
    const std::vector<iGame::AttributeDataDescriptor>& descriptors) {
    for (const auto& descriptor : descriptors) {
        auto* attributeItem = new AttribTreeWidgetItem(descriptor.nativeIndex, tree, parentItem);
        attributeItem->setAttributeName(QString::fromUtf8(descriptor.name.c_str()));
        attributeItem->setAttributeData(descriptor.target, descriptor.state, descriptor.nativeIndex);
        if (descriptor.attachment == IG_POINT) {
            attributeItem->setIcon(0, QIcon(":/Ticon/Icons/select/point.png"));
        } else if (descriptor.attachment == IG_CELL) {
            attributeItem->setIcon(0, QIcon(":/Ticon/Icons/select/hex.png"));
        }
        attributeItem->setDimension(descriptor.componentCount);
    }
}

static void AddSubObjectAttributes(
    QTreeWidget* tree,
    SubObjectTreeWidgetItem* parentItem,
    const std::vector<iGame::AttributeDataDescriptor>& descriptors) {
    for (const auto& descriptor : descriptors) {
        auto* attributeItem = new SubAttribTreeWidgetItem(descriptor.nativeIndex, tree, parentItem);
        attributeItem->setAttributeName(QString::fromUtf8(descriptor.name.c_str()));
        attributeItem->setAttributeData(descriptor.target, descriptor.state, descriptor.nativeIndex);
        if (descriptor.attachment == IG_POINT) {
            attributeItem->setIcon(0, QIcon(":/Ticon/Icons/select/point.png"));
        } else if (descriptor.attachment == IG_CELL) {
            attributeItem->setIcon(0, QIcon(":/Ticon/Icons/select/hex.png"));
        }
        attributeItem->setDimension(descriptor.componentCount);
    }
}

// Build sub-dataobject hierarchy under a given parent tree item
static void BuildSubObjectTree(
    QTreeWidget* tree,
    QTreeWidgetItem* parentItem,
    iGame::DataObject::Pointer obj,
    const AttributeDescriptorMap& descriptorsByObject) {
    if (!obj || !obj->HasSubDataObject()) return;

    for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); ++it) {
        auto sub = it->second;
        auto* childItem = new SubObjectTreeWidgetItem(parentItem);
        childItem->setDataObject(sub);
        // default name fallback if empty
        std::string subName = sub->GetName();
        if (subName.empty()) { subName = std::string("Block_") + std::to_string(sub->GetDataObjectId()); }
        childItem->setName(QString::fromStdString(subName));
        // set initial eye icon by current visibility
        if (auto draw = DynamicCast<iGame::DrawObject>(sub)) { childItem->changeVisibility(draw->GetVisibility()); }

        if (const auto descriptorIterator = descriptorsByObject.find(sub->GetDataObjectId());
            descriptorIterator != descriptorsByObject.end()) {
            AddSubObjectAttributes(tree, childItem, descriptorIterator->second);
        }

        // recurse into deeper hierarchy
        BuildSubObjectTree(tree, childItem, sub, descriptorsByObject);
        childItem->setExpanded(true);
    }
}
} // namespace

igQtModelDialogWidget::igQtModelDialogWidget(QWidget* parent) : QObject(parent), ui(new Ui::LayerDialog) {
    // 用臨時 QDockWidget 載入 UI，以取得 modelTreeWidget 與 tabWidget
    QDockWidget dummy;
    ui->setupUi(&dummy);

    tabWidget = ui->tabWidget;
    modelTreeWidget = ui->modelTreeWidget;
    propertyWidget = ui->propertyWidget;

    int totalWidth = parent ? parent->width() / 6 : 200;

    // 上半部分：圖層/模型樹 Dock（可單獨拖出懸浮）
    m_treeDock = new QDockWidget(QStringLiteral("模型树"), parent);
    m_treeDock->setObjectName("LayerTreeDock");
    m_treeDock->setWidget(modelTreeWidget);
    m_treeDock->setMinimumWidth(totalWidth);
    // LayerDialog 允许悬浮 + 可拖动（可关闭、可移动、可浮动）
    m_treeDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable);
    m_treeDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea);

    // 避免透明背景造成 Dock 穿透
    m_treeDock->setAttribute(Qt::WA_TranslucentBackground, false);
    // 自定义标题栏（用于无边框 floating 时提供可拖拽移动）
    auto* treeTitle = new DockTitleBar(m_treeDock, m_treeDock->windowTitle(), m_treeDock);
    m_treeDock->setTitleBarWidget(treeTitle);

    //  Properties Dock（也可懸浮）
    m_propertiesDock = new QDockWidget(QStringLiteral("属性"), parent);
    m_propertiesDock->setObjectName("LayerPropertiesDock");
    m_propertiesDock->setWidget(tabWidget);
    m_propertiesDock->setMinimumWidth(totalWidth);
    // Properties 不允许悬浮/拖动（只保留可关闭）
    m_propertiesDock->setFeatures(QDockWidget::DockWidgetClosable);
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea);

    m_propertiesDock->setAttribute(Qt::WA_TranslucentBackground, false);
    // Properties 使用 Qt 默认 dock 标题栏（与普通 dock 一致）
    m_propertiesDock->setTitleBarWidget(nullptr);

    // floating 时强制无系统边框（但仍可通过自定义 title bar 拖拽移动）
    connect(m_treeDock, &QDockWidget::topLevelChanged, m_treeDock, [this](bool floating) {
        if (!m_treeDock) return;
        if (floating) {
            m_treeDock->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
            // 關閉透明背景：使用樣式表來控制外觀與邊框
            m_treeDock->setAttribute(Qt::WA_TranslucentBackground, false);
            m_treeDock->show();
        } else {
            // 回到 docked：让 Qt 恢复正常 DockWidget 行为
            m_treeDock->setWindowFlags(Qt::Widget);
            m_treeDock->show();
        }
    });
    connect(m_propertiesDock, &QDockWidget::topLevelChanged, m_propertiesDock, [this](bool floating) {
        if (!m_propertiesDock) return;
        if (floating) {
            m_propertiesDock->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
            m_propertiesDock->setAttribute(Qt::WA_TranslucentBackground, false);
            m_propertiesDock->show();
        } else {
            m_propertiesDock->setWindowFlags(Qt::Widget);
            m_propertiesDock->show();
        }
    });

    tabWidget->addTab(ui->ModelInformationWidget, QStringLiteral("模型信息"));
    tabWidget->addTab(ui->propertyWidget, QStringLiteral("模型属性"));

    // 根据总宽度调整列宽
    int col1Width = totalWidth * 0.4;
    int col2Width = totalWidth * 0.6;


    modelTreeWidget->setColumnCount(2);
    modelTreeWidget->header()->hide();
    modelTreeWidget->setColumnWidth(0, 140);
    modelTreeWidget->setColumnWidth(1, 200);
    // 减小缩进，让模型和 attribute 文本更靠近左侧
    modelTreeWidget->setIndentation(10);
    modelTreeWidget->setAlternatingRowColors(true);
    modelTreeWidget->setUniformRowHeights(true);
    // 修复：首行图标显示被裁剪（行高小于图标高度时会只显示上半截）
    modelTreeWidget->setIconSize(QSize(20, 24));
    modelTreeWidget->setStyleSheet(modelTreeWidget->styleSheet() +
                                   QStringLiteral("QTreeView::item{height:28px;}"));


    propertyWidget->setHeaderVisible(false);
    propertyManager = new QtVariantPropertyManager(propertyWidget);
    editFactory = new QtVariantEditorFactory(propertyWidget);
    propertyWidget->setFactoryForManager(propertyManager, editFactory);

    propertyWidget->removeProperty(objectGroup);
    objectGroup =
            propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), QStringLiteral("对象属性"));
    propertyWidget->addProperty(objectGroup);

    // QtTreePropertyBrowser 内部用 QItemDelegate 绘制选中行，会使用 QPalette::Highlight（Windows 上常为蓝色）。
    // 与主窗口 QSS 中银色选中行一致，改为银色 + 深色文字。
    for (QTreeWidget* tw : propertyWidget->findChildren<QTreeWidget*>()) {
        QPalette pal = tw->palette();
        pal.setColor(QPalette::Active, QPalette::Highlight, QColor(0xC0, 0xC0, 0xC0));
        pal.setColor(QPalette::Inactive, QPalette::Highlight, QColor(0xA8, 0xA8, 0xAC));
        pal.setColor(QPalette::Active, QPalette::HighlightedText, QColor(0x25, 0x25, 0x26));
        pal.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(0x25, 0x25, 0x26));
        tw->setPalette(pal);
    }

    prop_PointSize = propertyManager->addProperty(QVariant::Int, QStringLiteral("点大小"));
    prop_PointSize->setEnabled(false);
    prop_PointSize->setValue(0);
    objectGroup->addSubProperty(prop_PointSize);
    propertyManager->setAttribute(prop_PointSize, "minimum", 1);
    propertyManager->setAttribute(prop_PointSize, "maximum", 99);
    propertyManager->setAttribute(prop_PointSize, "singleStep", 1);

    pror_LineWidth = propertyManager->addProperty(QVariant::Int, QStringLiteral("线宽"));
    pror_LineWidth->setEnabled(false);
    pror_LineWidth->setValue(0);
    objectGroup->addSubProperty(pror_LineWidth);
    propertyManager->setAttribute(pror_LineWidth, "minimum", 1);
    propertyManager->setAttribute(pror_LineWidth, "maximum", 10);
    propertyManager->setAttribute(pror_LineWidth, "singleStep", 1);

    prop_Transparency = propertyManager->addProperty(QVariant::Double, QStringLiteral("透明度"));
    prop_Transparency->setEnabled(false);
    prop_Transparency->setValue(0);
    objectGroup->addSubProperty(prop_Transparency);
    propertyManager->setAttribute(prop_Transparency, "minimum", 0.0);
    propertyManager->setAttribute(prop_Transparency, "maximum", 1.0);
    propertyManager->setAttribute(prop_Transparency, "singleStep", 0.1);




    connect(propertyManager, &QtVariantPropertyManager::valueChanged, this, &igQtModelDialogWidget::onPropertyChanged);

    ui->ModelInformationWidget->hide();
    //connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this, &igQtModelDialogWidget::UpdateCurrentModel);
    connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this,
            static_cast<void (igQtModelDialogWidget::*)(iGame::Model*)>(
                    &igQtModelDialogWidget::updateCurrentModelProperty));

    connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this,
            &igQtModelDialogWidget::updateCurrentModelInfo);
    //connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this, &igQtModelDialogWidget::updateCloudPicture);
    connect(modelTreeWidget, &igQtModelTreeWidget::ViewCloudPicture, this, &igQtModelDialogWidget::updateCloudPicture);
    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int) {
        completeDeferredModelInformationUpdate();
    });
    connect(m_propertiesDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (visible) completeDeferredModelInformationUpdate();
    });
}

ModelTreeWidgetItem* igQtModelDialogWidget::getItemFromObject(iGame::DataObject::Pointer obj) {
    // 遍历子项
    for (int i = 0; i < modelTreeWidget->topLevelItemCount(); ++i) {
        ModelTreeWidgetItem* item = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->topLevelItem(i));
        if (item->getModel()->GetDataObject() == obj) { return item; }
    }
    return nullptr;
}
void igQtModelDialogWidget::updateItemName(iGame::DataObject::Pointer obj) {
    auto item = getItemFromObject(obj);
    if (!item) return;
    item->setName(QString::fromStdString(obj->GetName()));
    return;
}
void igQtModelDialogWidget::updateAllAttriubute(iGame::DataObject::Pointer obj) {
    auto item = getItemFromObject(obj);
    if (!item) return;
    item->setCurrentChild(nullptr);

    while (item->childCount() > 0) { delete item->takeChild(0); }
    auto source = igQtAttributeDataSourceManager::Instance()->EnsureSource(obj);
    const auto descriptorsByObject = BuildAttributeDescriptorMap(source);
    if (const auto iterator = descriptorsByObject.find(obj->GetDataObjectId());
        iterator != descriptorsByObject.end()) {
        AddTopLevelAttributes(modelTreeWidget, item, iterator->second);
    }
    BuildSubObjectTree(modelTreeWidget, item, obj, descriptorsByObject);
    item->setExpanded(true);
    modelTreeWidget->RebuildAttributeIndex();
    item->viewAttribute(-1);
    iGame::DynamicCast<iGame::DrawObject>(obj)->ForceReConvertToDrawableData();
}

int igQtModelDialogWidget::addDataObjectToModelTree(iGame::DataObject::Pointer obj, ItemSource source) {
    ModelTreeWidgetItem* item = new ModelTreeWidgetItem(modelTreeWidget);
    //modelTreeWidget->setCurrentModelItem(item);
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    unsigned int id = scene->AddModel(obj);
    iGame::Model* model = scene->GetModelById(id).get();

    //currentModel = model;
    scene->SetCurrentModel(model);

    item->setModelId(id);
    item->setName(QString::fromStdString(obj->GetName()));
    item->setModel(model);

    auto sourceController = igQtAttributeDataSourceManager::Instance();
    auto attributeSource = sourceController->EnsureSource(obj);
    const auto descriptorsByObject = BuildAttributeDescriptorMap(attributeSource);
    if (const auto iterator = descriptorsByObject.find(obj->GetDataObjectId());
        iterator != descriptorsByObject.end()) {
        AddTopLevelAttributes(modelTreeWidget, item, iterator->second);
    }

    // build sub-data objects hierarchy
    BuildSubObjectTree(modelTreeWidget, item, obj, descriptorsByObject);

    modelTreeWidget->addTopLevelItem(item);
    item->setExpanded(true);
    modelTreeWidget->RebuildAttributeIndex();
    modelTreeWidget->setCurrentItem(item);

    updateCurrentModelProperty(model);
    if (source == ItemSource::File) {
        m_PendingModelInformationObjectId = obj->GetDataObjectId();
        Q_EMIT CurrendModelChanged();
    } else {
        updateCurrentModelInfo();
    }
    //QTreeWidgetItem* currentItem = modelTreeWidget->getCurrentModelItem();
    //std::cout << "add current model: " << currentItem << std::endl;
    return id;
}

int igQtModelDialogWidget::addModelToModelTree(iGame::Model::Pointer model) {
    ModelTreeWidgetItem* item = new ModelTreeWidgetItem(modelTreeWidget);
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

    auto id = scene->AddModel(model->GetDataObject());

    item->setModelId(id);
    item->setName(QString::fromStdString(model->GetDataObject()->GetName()));
    item->setModel(model);

    auto attributeSource = igQtAttributeDataSourceManager::Instance()->EnsureSource(model->GetDataObject());
    const auto descriptorsByObject = BuildAttributeDescriptorMap(attributeSource);
    if (const auto iterator = descriptorsByObject.find(model->GetDataObject()->GetDataObjectId());
        iterator != descriptorsByObject.end()) {
        AddTopLevelAttributes(modelTreeWidget, item, iterator->second);
    }
    // build sub-data objects hierarchy
    BuildSubObjectTree(modelTreeWidget, item, model->GetDataObject(), descriptorsByObject);

    modelTreeWidget->addTopLevelItem(item);
    item->setExpanded(true);
    modelTreeWidget->RebuildAttributeIndex();
    modelTreeWidget->setCurrentItem(item);
    return id;
}

void igQtModelDialogWidget::completeDeferredModelInformationUpdate() {
    if (m_PendingModelInformationObjectId < 0) return;
    if (tabWidget == nullptr ||
        tabWidget->currentWidget() != ui->ModelInformationWidget ||
        !ui->ModelInformationWidget->isVisible()) {
        return;
    }

    const auto pendingObjectId = m_PendingModelInformationObjectId;
    m_PendingModelInformationObjectId = -1;

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene == nullptr) return;

    auto model = scene->GetCurrentModel();
    if (model == nullptr) return;

    auto object = model->GetDataObject();
    if (object == nullptr || object->GetDataObjectId() != pendingObjectId) return;

    ui->ModelInformationWidget->updateInformationFrame();
}

int igQtModelDialogWidget::updateCurrentModelInfo() {
    //    qDebug() << ui->modelTreeWidget->currentIndex();

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto model = scene != nullptr ? scene->GetCurrentModel() : nullptr;
    auto object = model != nullptr
        ? model->GetDataObject()
        : iGame::DataObject::Pointer{};
    m_PendingModelInformationObjectId = object != nullptr
        ? object->GetDataObjectId()
        : -1;
    completeDeferredModelInformationUpdate();
    Q_EMIT CurrendModelChanged();


    return 1;
}
void igQtModelDialogWidget::updateCurrentModelProperty() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto model=scene->GetCurrentModel();
    if (!model) {
        return;
    }
    //currentModel = model;
    auto obj = DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (obj) {
        prop_PointSize->setEnabled(true);
        prop_PointSize->setValue(obj->GetPointSize());
        pror_LineWidth->setEnabled(true);
        pror_LineWidth->setValue(obj->GetLineWidth());
        prop_Transparency->setEnabled(true);
        prop_Transparency->setValue(obj->GetTransparency());
    } else {
        prop_PointSize->setEnabled(false);
        prop_PointSize->setValue(0);
        pror_LineWidth->setEnabled(false);
        pror_LineWidth->setValue(0);
        prop_Transparency->setEnabled(false);
        prop_Transparency->setValue(0);
    }
}
void igQtModelDialogWidget::updateCurrentModelProperty(iGame::Model* model) {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    scene->SetCurrentModel(model);

    //currentModel = model;
    auto obj = DynamicCast<iGame::DrawObject>(model->GetDataObject());
    if (obj) {
        prop_PointSize->setEnabled(true);
        prop_PointSize->setValue(obj->GetPointSize());
        pror_LineWidth->setEnabled(true);
        pror_LineWidth->setValue(obj->GetLineWidth());
        prop_Transparency->setEnabled(true);
        prop_Transparency->setValue(obj->GetTransparency());
    } else {
        prop_PointSize->setEnabled(false);
        prop_PointSize->setValue(0);
        pror_LineWidth->setEnabled(false);
        pror_LineWidth->setValue(0);
        prop_Transparency->setEnabled(false);
        prop_Transparency->setValue(0);
    }
}
int igQtModelDialogWidget::updateCloudPicture() {

    Q_EMIT CloudPictureChanged();
    return 1;
}
void igQtModelDialogWidget::deleteCurrentModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    // 获取当前选中的QTreeWidgetItem
    ModelTreeWidgetItem* currentItem = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->currentItem());
    if (currentItem == nullptr) return;

    int id = currentItem->getModelId();
    
    // 在删除之前获取模型名称，避免在 RemoveModel 后持有引用
    std::string modelName;
    int rootObjectId = -1;
    {
        auto model = scene->GetModelById(id);
        if (model && model->GetDataObject()) {
            modelName = model->GetDataObject()->GetName();
            rootObjectId = model->GetDataObject()->GetDataObjectId();
        }
        // model 智能指针在这里离开作用域并释放
    }

    scene->RemoveModel(id);
    if (rootObjectId >= 0) {
        igQtAttributeDataSourceManager::Instance()->ReleaseSource(rootObjectId);
    }
    scene->Update();

    if (!modelName.empty()) {
        // Need to emit signal to ScalarViewWidget to clear states
        Q_EMIT ModelDeleted(modelName);
    }

    int index = modelTreeWidget->indexOfTopLevelItem(currentItem);
    if (index != -1) { delete modelTreeWidget->takeTopLevelItem(index); }
    modelTreeWidget->RebuildAttributeIndex();

    currentItem = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->currentItem());
    if (currentItem) {
        scene->SetCurrentModel(currentItem->getModelId());
    }
}

void igQtModelDialogWidget::onPropertyChanged(QtProperty* property, const QVariant& value) {
    auto currentModel = GetCurrentModel();
    if (property == prop_PointSize) {
        //std::cout << value.toInt() << std::endl;
        if (currentModel) {
            auto obj = DynamicCast<iGame::DrawObject>(currentModel->GetDataObject());
            if (obj && obj->GetPointSize() != value.toInt() && value.toInt() > 0) {
                obj->SetPointSize(value.toInt());
                Update();
            }
        }
    } else if (property == pror_LineWidth) {
        //std::cout << value.toDouble() << std::endl;
        if (currentModel) {
            auto obj = DynamicCast<iGame::DrawObject>(currentModel->GetDataObject());
            if (obj && obj->GetLineWidth() != value.toInt() && value.toInt() > 0) {
                obj->SetLineWidth(value.toInt());
                Update();
            }
        }
    } else if (property == prop_Transparency) {
        //std::cout << value.toDouble() << std::endl;
        if (currentModel) {
            auto obj = DynamicCast<iGame::DrawObject>(currentModel->GetDataObject());
            if (obj && obj->GetTransparency() != value.toDouble() && value.toDouble() >= 0 && value.toDouble() <= 1.0) {
                obj->SetTransparency(value.toFloat());
                Update();
            }
        }
    }
}

iGame::Model* igQtModelDialogWidget::GetCurrentModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    return scene->GetCurrentModel();
}


void igQtModelDialogWidget::positionTreeDockToRendererCorner(QWidget* rendererWidget) {
    if (!rendererWidget || !m_treeDock) return;

    // 如果不允许悬浮，就不要强制 setFloating(true)，否则会变成系统浮动窗
    if (!(m_treeDock->features() & QDockWidget::DockWidgetFloatable)) {
        return;
    }

    // 确保dock widget是悬浮状态，然后设置无边框
    m_treeDock->setFloating(true);
    // 使用无边框 floating（可通过自定义 title bar 拖拽移动）
    m_treeDock->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    // 關閉透明背景，讓樣式表的背景與邊框生效
    m_treeDock->setAttribute(Qt::WA_TranslucentBackground, false);

    // 先设置窗口大小
    int dockWidth = 380;  // 悬浮窗口宽度
    int dockHeight = 280; // 悬浮窗口高度
    m_treeDock->resize(dockWidth, dockHeight);
    m_treeDock->show(); // window flags 变更后需要 show()
    
    // 等待窗口完全显示后再计算位置
    QApplication::processEvents();
    
    // 获取dock窗口的实际大小（窗口显示后可能略有调整）
    QSize actualDockSize = m_treeDock->size();

    // 获取OpenGL渲染窗口在屏幕上的几何信息
    // rendererWidget本身就是centralWidget，直接使用它作为渲染窗口
    QWidget* actualRendererWidget = rendererWidget;
    
    // 获取渲染窗口的几何信息
    // 直接获取窗口的四个角点的全局坐标
    QPoint rendererTopLeft = actualRendererWidget->mapToGlobal(QPoint(0, 0));
    QPoint rendererBottomRight = actualRendererWidget->mapToGlobal(QPoint(actualRendererWidget->width(), actualRendererWidget->height()));

    // 计算悬浮窗口的位置：layerdialog的右下角对应渲染窗口的右下角，留出边距
    int margin = 15; // 边距
    
    // layerdialog的左上角位置（全局坐标）= 渲染窗口右下角 - layerdialog实际大小 - 边距
    QPoint dockTopLeft(
            rendererBottomRight.x() - actualDockSize.width() - margin,
            rendererBottomRight.y() - actualDockSize.height() - margin
    );

    // 获取渲染窗口所在的屏幕，确保窗口完全在屏幕内
    QScreen* screen = nullptr;
    if (QWidget* mainWindow = rendererWidget->window()) {
        screen = mainWindow->screen();
    }
    if (!screen) {
        // 如果无法获取，使用计算位置所在的屏幕
        screen = QApplication::screenAt(dockTopLeft);
    }
    if (!screen) {
        // 如果还是无法获取，使用主屏幕
        screen = QApplication::primaryScreen();
    }
    
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        
        // 计算layerdialog的右下角位置
        QPoint dockBottomRight = dockTopLeft + QPoint(actualDockSize.width(), actualDockSize.height());
        
        // 检查并调整位置，确保窗口完全在屏幕内
        // 优先保持layerdialog的右下角对应渲染窗口的右下角
        
        // 如果右下角超出屏幕右边界
        if (dockBottomRight.x() > screenGeometry.right()) {
            // 调整到屏幕右边界内，但确保仍然在渲染窗口右侧
            int newX = screenGeometry.right() - actualDockSize.width() - margin;
            // 计算调整后layerdialog的右下角X坐标
            int newDockRight = newX + actualDockSize.width();
            
            // 只有当调整后的layerdialog右下角仍然在渲染窗口右下角的右侧时才调整
            // 如果调整后会移到渲染窗口左侧，则保持原位置（即使部分超出屏幕）
            if (newDockRight >= rendererBottomRight.x() - margin) {
                // 调整后的位置仍然在渲染窗口右侧，使用新位置
                dockTopLeft.setX(newX);
            }
            // 否则保持原位置，即使部分超出屏幕也比移到左侧好
        }
        
        // 如果右下角超出屏幕下边界
        if (dockBottomRight.y() > screenGeometry.bottom()) {
            dockTopLeft.setY(screenGeometry.bottom() - actualDockSize.height() - margin);
        }
        
        // 确保左上角也在屏幕内（防止窗口完全超出屏幕）
        // 但如果渲染窗口在右侧，layerdialog绝对不应该被移到屏幕左侧
        if (dockTopLeft.x() < screenGeometry.left()) {
            // 检查渲染窗口的位置：如果渲染窗口在屏幕右侧，绝对不调整到左侧
            int rendererRight = rendererBottomRight.x();
            int rendererLeft = rendererTopLeft.x();
            
            // 如果渲染窗口的右边界在屏幕中心右侧，说明渲染窗口在右侧
            // 此时layerdialog不应该被移到屏幕左侧，保持原位置
            // 或者，如果layerdialog的右下角在渲染窗口右侧，也不应该移到左侧
            int dockRight = dockTopLeft.x() + actualDockSize.width();
            bool rendererOnRight = (rendererRight > screenGeometry.center().x() || rendererLeft > screenGeometry.center().x());
            bool dockOnRightOfRenderer = (dockRight >= rendererBottomRight.x() - margin);
            
            if (rendererOnRight || dockOnRightOfRenderer) {
                // 渲染窗口在右侧，或者layerdialog在渲染窗口右侧
                // 绝对不调整到屏幕左侧，保持原位置
                // 完全不进行调整，直接跳过
            } else {
                // 渲染窗口在屏幕左侧或中间，且layerdialog不在渲染窗口右侧
                // 可以调整到屏幕左边界（但这种情况不应该发生）
                dockTopLeft.setX(screenGeometry.left() + margin);
            }
        }
        if (dockTopLeft.y() < screenGeometry.top()) {
            dockTopLeft.setY(screenGeometry.top() + margin);
        }
    }

    // 最终检查：如果渲染窗口在屏幕右侧，确保layerdialog不会出现在屏幕左侧
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int rendererRight = rendererBottomRight.x();
        
        // 如果渲染窗口在屏幕右侧（右边界在屏幕中心右侧）
        if (rendererRight > screenGeometry.center().x()) {
            // 确保layerdialog不会出现在屏幕左侧
            if (dockTopLeft.x() < screenGeometry.left() + 100) {
                // layerdialog出现在屏幕左侧，这是错误的
                // 强制放在渲染窗口右下角
                dockTopLeft.setX(rendererBottomRight.x() - actualDockSize.width() - margin);
                dockTopLeft.setY(rendererBottomRight.y() - actualDockSize.height() - margin);
            }
        }
    }
    

    // 设置悬浮窗口的位置（使用全局坐标）
    m_treeDock->move(dockTopLeft);

    // 确保窗口可见并激活
    m_treeDock->show();
    m_treeDock->raise();
    m_treeDock->activateWindow();
}
