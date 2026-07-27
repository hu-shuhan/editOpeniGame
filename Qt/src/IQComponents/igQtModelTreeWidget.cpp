#include <IQComponents/igQtModelTreeWidget.h>
#include <IQCore/igQtAttributeDataSourceManager.h>
#include <QAction>
#include <QMenu>
#include <QHeaderView>
#include <QSet>

#include <functional>

#include "iGameSceneManager.h"

ModelTreeWidgetItem::ModelTreeWidgetItem(QTreeWidget* parent) : QTreeWidgetItem(parent), visibility(true) {
    QWidget* buttonWidget = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(buttonWidget);

    view_bbox = new HoverButton(buttonWidget);
    view_points = new HoverButton(buttonWidget);
    view_wireframe = new HoverButton(buttonWidget);
    view_fill = new HoverButton(buttonWidget);
    view_pickedItem = new HoverButton(buttonWidget);

    view_bbox->setIcon(QIcon(":/Ticon/Icons/select/bbox.png"));
    view_points->setIcon(QIcon(":/Ticon/Icons/select/points.png"));
    view_wireframe->setIcon(QIcon(":/Ticon/Icons/select/wireframe.png"));
    view_fill->setIcon(QIcon(":/Ticon/Icons/select/fill.png"));
    view_pickedItem->setIcon(QIcon(":/Ticon/Icons/select/selected.png"));

    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(view_bbox);
    layout->addWidget(view_points);
    layout->addWidget(view_wireframe);
    layout->addWidget(view_fill);
    layout->addWidget(view_pickedItem);
    layout->setContentsMargins(0, 2, 2, 2);

    parent->setItemWidget(this, 1, buttonWidget);

    view_wireframe->setChecked(false);
    show();

    view_bbox->setConcernFunctor(&ModelTreeWidgetItem::showBoundingBox, this);
    view_bbox->setCancelFunctor(&ModelTreeWidgetItem::hideBoundingBox, this);

    view_points->setConcernFunctor(&ModelTreeWidgetItem::showPoints, this);
    view_points->setCancelFunctor(&ModelTreeWidgetItem::hidePoints, this);

    view_wireframe->setConcernFunctor(&ModelTreeWidgetItem::showWireframe, this);
    view_wireframe->setCancelFunctor(&ModelTreeWidgetItem::hideWireframe, this);

    view_fill->setConcernFunctor(&ModelTreeWidgetItem::showFill, this);
    view_fill->setCancelFunctor(&ModelTreeWidgetItem::hideFill, this);

    view_pickedItem->setConcernFunctor(&ModelTreeWidgetItem::showPickedItem, this);
    view_pickedItem->setCancelFunctor(&ModelTreeWidgetItem::hidePickedItem, this);
    this->parent = parent;
}
iGame::Model* ModelTreeWidgetItem::getModel() { return this->model; }

void ModelTreeWidgetItem::setModel(iGame::Model* model) {
    this->model = model;
    view_fill->setChecked(true);
    view_pickedItem->setChecked(true);
    //    view_wireframe->setChecked(true);
    //    showWireframe();
    showFill();
    showPickedItem();
}

void ModelTreeWidgetItem::setName(const QString& name) {
    setText(0, name);
    setToolTip(0, name);
}

void ModelTreeWidgetItem::changeVisibility() {
    if (getVisibility()) {
        hide();
    } else {
        show();
    }
}
void ModelTreeWidgetItem::changeVisibility(bool vis) {
    if (!vis) {
        hide();
    } else {
        show();
    }
}
bool ModelTreeWidgetItem::viewAttribute(const int index, const int dim) {
    const bool changed = model->ViewCloudPicture(index, dim, false);
    if (changed) {
        Q_EMIT dynamic_cast<igQtModelTreeWidget*>(this->parent)->ViewCloudPicture();
    }
    return changed;
}

void ModelTreeWidgetItem::setCurrentChild(QTreeWidgetItem* child) { current_child = child; }
QTreeWidgetItem* ModelTreeWidgetItem::getCurrentChild() { return current_child; }

bool ModelTreeWidgetItem::getVisibility() const { return visibility; }

void ModelTreeWidgetItem::show() {
    visibility = true;
    this->setIcon(0, QIcon(":/Ticon/Icons/select/eye-open.png"));
    if (!model) { return; }

    model->Show();
    update();
}

void ModelTreeWidgetItem::hide() {
    visibility = false;
    this->setIcon(0, QIcon(":/Ticon/Icons/select/eye-close.png"));
    if (!model) { return; }

    model->Hide();
    update();
}

void ModelTreeWidgetItem::showBoundingBox() {
    model->SetBoundingBoxSwitch(true);
    update();
}
void ModelTreeWidgetItem::hideBoundingBox() {
    model->SetBoundingBoxSwitch(false);
    update();
}

void ModelTreeWidgetItem::showPoints() {
    model->SetViewPointsSwitch(true);
    update();
}
void ModelTreeWidgetItem::hidePoints() {
    model->SetViewPointsSwitch(false);
    update();
}

void ModelTreeWidgetItem::showWireframe() {
    model->SetViewWireframeSwitch(true);
    update();
}
void ModelTreeWidgetItem::hideWireframe() {
    model->SetViewWireframeSwitch(false);
    update();
}

void ModelTreeWidgetItem::showFill() {
    model->SetViewFillSwitch(true);
    update();
}
void ModelTreeWidgetItem::hideFill() {
    model->SetViewFillSwitch(false);
    update();
}

void ModelTreeWidgetItem::showPickedItem() {
    model->SetPickedItemSwitch(true);
    update();
}
void ModelTreeWidgetItem::hidePickedItem() {
    model->SetPickedItemSwitch(false);
    update();
}

void ModelTreeWidgetItem::update() {
    if (model) { model->Update(); }
}

AttribTreeWidgetItem::AttribTreeWidgetItem(int index, QTreeWidget* treeview, ModelTreeWidgetItem* parent)
    : index(index), QTreeWidgetItem(parent), parent(parent) {

    QWidget* widget = new QWidget(treeview);
    comboBox = new MComboBox(this, widget);
    comboBox->setStyleSheet("QComboBox { background-color: transparent; }"
                            "QComboBox QAbstractItemView { background-color: white; }");

    setDimension(1);

    treeview->setItemWidget(this, 1, widget);

    hide();
}

void AttribTreeWidgetItem::setDimension(int length) {
    comboBox->clear();
    m_Dimension = length;

    // For Dimension=1, show only the single dimension value, not magnitude
    if (length == 1) {
        comboBox->addItem(QString::fromStdString("x"));
        comboBox->setCurrentIndex(0);
        return;
    }

    // For Dimension>=2, show magnitude first, then individual dimensions
    comboBox->addItem("magnitude");
    if (length < 4) {
        if (length > 0) comboBox->addItem(QString::fromStdString("x"));
        if (length > 1) comboBox->addItem(QString::fromStdString("y"));
        if (length > 2) comboBox->addItem(QString::fromStdString("z"));
    } else {
        for (int i = 0; i < length; i++) { comboBox->addItem(QString::fromStdString("D" + std::to_string(i))); }
    }
    comboBox->setCurrentIndex(0);
}

void AttribTreeWidgetItem::setAttributeName(const QString& name) {
    m_AttributeName = name;
    refreshLabel();
}

void AttribTreeWidgetItem::setAttributeData(
    const iGame::AttributeDataTarget& target,
    const iGame::AttributeDataLoadState state,
    const int nativeIndex) {
    m_HasAttributeData = true;
    m_Target = target;
    setLoadState(state, nativeIndex);
}

void AttribTreeWidgetItem::setLoadState(
    const iGame::AttributeDataLoadState state,
    const int nativeIndex) {
    m_LoadState = state;
    if (nativeIndex >= 0) index = nativeIndex;
    refreshLabel();
}

int AttribTreeWidgetItem::takePendingDimension() {
    const int dimension = m_PendingDimension;
    m_PendingDimension = -2;
    return dimension;
}

void AttribTreeWidgetItem::refreshLabel() {
    QString suffix;
    if (m_LoadState == iGame::AttributeDataLoadState::Unloaded) suffix = QStringLiteral("  [未加载]");
    if (m_LoadState == iGame::AttributeDataLoadState::Loading) suffix = QStringLiteral("  [加载中]");
    if (m_LoadState == iGame::AttributeDataLoadState::Failed) suffix = QStringLiteral("  [加载失败]");
    setText(0, m_AttributeName + suffix);
    setToolTip(0, m_AttributeName + suffix);
}

igQtModelTreeWidget::igQtModelTreeWidget(QWidget* parent) : QTreeWidget(parent) {
    // Keep eliding ("xxxx...") but show full name via tooltip.
    setTextElideMode(Qt::ElideRight);

    if (header()) {
        header()->setStretchLastSection(false);
        header()->setSectionResizeMode(QHeaderView::Interactive);
    }
    connect(
        igQtAttributeDataSourceManager::Instance(),
        &igQtAttributeDataSourceManager::AttributeLoadStateChanged,
        this,
        [this](
            const int rootObjectId,
            const iGame::AttributeDataTarget& target,
            const iGame::AttributeDataLoadState state,
            const int nativeIndex,
            const QString&) {
            const auto iterator = m_AttributeItems.constFind(
                AttributeItemKey(rootObjectId, target));
            if (iterator == m_AttributeItems.constEnd()) {
                return;
            }
            const bool activate = IsActiveAttributeTarget(rootObjectId, target);
            for (const auto& binding : iterator.value()) {
                if (auto* attributeItem = dynamic_cast<AttribTreeWidgetItem*>(binding.attributeItem);
                    attributeItem != nullptr) {
                    attributeItem->setLoadState(state, nativeIndex);
                    if (state == iGame::AttributeDataLoadState::Loaded) {
                        const int dimension = attributeItem->takePendingDimension();
                        if (activate && dimension != -2) {
                            attributeItem->show();
                            binding.modelItem->setCurrentChild(attributeItem);
                            attributeItem->viewAttribute(dimension);
                        }
                    }
                    continue;
                }
                if (auto* attributeItem = dynamic_cast<SubAttribTreeWidgetItem*>(binding.attributeItem);
                    attributeItem != nullptr) {
                    attributeItem->setLoadState(state, nativeIndex);
                    if (state == iGame::AttributeDataLoadState::Loaded) {
                        const int dimension = attributeItem->takePendingDimension();
                        if (activate && dimension != -2) {
                            attributeItem->show();
                            if (auto* parentItem = dynamic_cast<SubObjectTreeWidgetItem*>(attributeItem->parent());
                                parentItem != nullptr) {
                                parentItem->setCurrentChild(attributeItem);
                            }
                            if (attributeItem->viewAttribute(dimension)) {
                                emit ViewCloudPicture();
                            }
                        }
                    }
                }
            }
        });
}

QString igQtModelTreeWidget::AttributeItemKey(
    const int rootObjectId,
    const iGame::AttributeDataTarget& target) {
    return QStringLiteral("%1:%2:%3:%4")
        .arg(rootObjectId)
        .arg(target.frameIndex)
        .arg(QString::fromUtf8(target.blockPath.c_str()))
        .arg(static_cast<qulonglong>(target.sourceIndex));
}

void igQtModelTreeWidget::SetActiveAttributeTarget(
    const int rootObjectId,
    const iGame::AttributeDataTarget& target) {
    m_ActiveAttributeTargets.insert(rootObjectId, target);
    igQtAttributeDataSourceManager::Instance()->SetActiveAttributeTarget(
        rootObjectId, target);
}

bool igQtModelTreeWidget::IsActiveAttributeTarget(
    const int rootObjectId,
    const iGame::AttributeDataTarget& target) const {
    const auto iterator = m_ActiveAttributeTargets.constFind(rootObjectId);
    return iterator != m_ActiveAttributeTargets.constEnd() && iterator.value() == target;
}

void igQtModelTreeWidget::RebuildAttributeIndex() {
    m_AttributeItems.clear();
    QSet<int> liveRootObjectIds;
    for (int topIndex = 0; topIndex < topLevelItemCount(); ++topIndex) {
        auto* modelItem = dynamic_cast<ModelTreeWidgetItem*>(topLevelItem(topIndex));
        if (modelItem == nullptr ||
            modelItem->getModel() == nullptr ||
            modelItem->getModel()->GetDataObject() == nullptr) {
            continue;
        }
        const int rootObjectId = modelItem->getModel()->GetDataObject()->GetDataObjectId();
        liveRootObjectIds.insert(rootObjectId);
        std::function<void(QTreeWidgetItem*)> indexItem;
        indexItem = [&](QTreeWidgetItem* item) {
            if (auto* attributeItem = dynamic_cast<AttribTreeWidgetItem*>(item);
                attributeItem != nullptr && attributeItem->hasAttributeData()) {
                m_AttributeItems[AttributeItemKey(rootObjectId, attributeItem->attributeTarget())]
                    .push_back({modelItem, attributeItem});
            } else if (auto* attributeItem = dynamic_cast<SubAttribTreeWidgetItem*>(item);
                       attributeItem != nullptr && attributeItem->hasAttributeData()) {
                m_AttributeItems[AttributeItemKey(rootObjectId, attributeItem->attributeTarget())]
                    .push_back({modelItem, attributeItem});
            }
            for (int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
                indexItem(item->child(childIndex));
            }
        };
        indexItem(modelItem);
    }
    for (auto iterator = m_ActiveAttributeTargets.begin();
         iterator != m_ActiveAttributeTargets.end();) {
        if (!liveRootObjectIds.contains(iterator.key())) {
            iterator = m_ActiveAttributeTargets.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

ModelTreeWidgetItem* igQtModelTreeWidget::getItem(const QPoint& p) const {
    return dynamic_cast<ModelTreeWidgetItem*>(itemAt(p));
}
QTreeWidgetItem* igQtModelTreeWidget::getChild(const QPoint& p) const {
    return dynamic_cast<QTreeWidgetItem*>(itemAt(p));
}

//void igQtModelTreeWidget::setCurrentModelItem(ModelTreeWidgetItem* item) {
//    currentModelItem = item;
//    //std::cout << "change\n";
//}

//ModelTreeWidgetItem* igQtModelTreeWidget::getCurrentModelItem() { return currentModelItem; }

//void igQtModelTreeWidget::setCurrentModel(ModelTreeWidgetItem* item) {
//    if (currentModel) {
//        auto* current = dynamic_cast<AttribTreeWidgetItem*>(
//                currentModel->getCurrentChild());
//        if (current) { current->hide(); }
//        currentModel->setCurrentChild(nullptr);
//    }
//    currentModel = item;
//}

void igQtModelTreeWidget::mousePressEvent(QMouseEvent* event) {
    bool call = true;
    ModelTreeWidgetItem* item = getItem(event->pos());
    QTreeWidgetItem* child = nullptr;

    if (item) {
        // Gets the position of the click and the position of the icon
        QRect iconItem = visualItemRect(item);
        QSize iconSize = item->icon(0).actualSize(QSize(20, 24));
        QRect iconRect(iconItem.left() + 4, iconItem.top() + (iconItem.height() - iconSize.height()) / 2,
                       iconSize.width(), iconSize.height());

        // Check if click is on the expand/collapse indicator (branch arrow)
        int indentation_level = 0;
        QTreeWidgetItem* parentItem = static_cast<QTreeWidgetItem*>(item)->parent();
        while (parentItem) {
            indentation_level++;
            parentItem = parentItem->parent();
        }
        int indicatorWidth = indentation() * (indentation_level + 1);
        QRect indicatorRect(0, iconItem.top(), indicatorWidth, iconItem.height());
        bool clickedOnIndicator = indicatorRect.contains(event->pos());

        if (event->button() == Qt::RightButton) {
            QMenu menu(this);

            // 菜单项 1：设置旋转中心
            QAction* setCenterAction = menu.addAction(QString::fromUtf8("设置旋转中心为当前模型"));
            connect(setCenterAction, &QAction::triggered, this, [item]() {
                auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                if (scene && item->getModel()) { scene->ResetCameraView(item->getModel()->GetDataObject()); }
            });

            // 菜单项 2：构建渲染加速结构
            QAction* buildAccelAction = menu.addAction(QString::fromUtf8("构建渲染加速结构"));
            connect(buildAccelAction, &QAction::triggered, this, [item]() {
                auto model = item->getModel();
                if (model && model->GetDataObject()) {
                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
                    if (drawObj) {
                        drawObj->SetAccelerationOption(true);
                        scene->Update();
                    }
                }
            });

            // 菜单项3：关闭加速结构
            QAction* disableAccelAction = menu.addAction(QString::fromUtf8("关闭渲染加速结构"));
            connect(disableAccelAction, &QAction::triggered, this, [item]() {
                auto model = item->getModel();
                if (model && model->GetDataObject()) {
                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
                    if (drawObj) {
                        drawObj->SetAccelerationOption(false);
                        scene->Update();
                    }
                }
            });

            // 菜单项4：开启/关闭Meshlet可视化
            QAction* meshletRenderingAction = menu.addAction(QString::fromUtf8("开启/关闭Meshlet可视化"));
            connect(meshletRenderingAction, &QAction::triggered, this, [item]() {
                auto model = item->getModel();
                if (model && model->GetDataObject()) {
                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
                    if (drawObj) {
                        bool lastOption = drawObj->GetRenderWithMeshlet();
                        drawObj->SetRenderWithMeshlet(!lastOption);
                        scene->Update();
                    }
                }
            });

            // 弹出菜单
            menu.exec(viewport()->mapToGlobal(event->pos()));
        }

        // Determine if the icon area has been clicked
        if (iconRect.contains(event->pos())) {
            item->changeVisibility();
            // sync all sub-block icons under this model to reflect current visibility
            for (int i = 0; i < item->childCount(); ++i) {
                if (auto* sub = dynamic_cast<SubObjectTreeWidgetItem*>(item->child(i))) {
                    sub->SyncIconWithVisibility(true);
                }
            }
            call = false;
        } else if (clickedOnIndicator) {
            // Clicked on expand/collapse indicator, only handle expand/collapse, don't change attribute display
            // Just let the base class handle the expand/collapse
        } else if (currentItem() != item) { // Check operation - only when clicking on the model itself
            if (item->getModel() != iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()) {
                iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(item->getModel());
                emit ChangeCurrentModel(item->getModel());
            }

            item->setSelected(true);
            if (item->getModel()->GetDataObject() != nullptr) {
                m_ActiveAttributeTargets.remove(
                    item->getModel()->GetDataObject()->GetDataObjectId());
                igQtAttributeDataSourceManager::Instance()->ClearActiveAttributeTarget(
                    item->getModel()->GetDataObject()->GetDataObjectId());
            }
            if (item->getModel()->ViewCloudPicture(-1, -1, false)) {
                Q_EMIT ViewCloudPicture();
            }
            auto* current = dynamic_cast<AttribTreeWidgetItem*>(item->getCurrentChild());
            if (current) { current->hide(); }
            item->setCurrentChild(nullptr);

            //if (currentModelItem != item) { this->setCurrentModelItem(item); }
        }


    } else if ((child = getChild(event->pos())) && child) {
        // Sub-data object item
        if (auto* sub = dynamic_cast<SubObjectTreeWidgetItem*>(child)) {
            // Right-click: set rotation center to sub-block bbox center
            if (event->button() == Qt::RightButton) {
                QMenu menu(this);
                QAction* setCenterAction = menu.addAction(QString::fromUtf8("设置旋转中心为当前子块"));
                connect(setCenterAction, &QAction::triggered, this, [sub]() {
                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    if (!scene) { return; }
                    auto draw = DynamicCast<iGame::DrawObject>(sub->getDataObject());
                    if (!draw) { return; }
                    scene->ResetCameraView(sub->getDataObject());
                    scene->Update();
                });
                menu.exec(viewport()->mapToGlobal(event->pos()));
            } else {
                // Left click: eye icon toggle or select parent model
                QRect iconItem = visualItemRect(sub);
                QSize iconSize = sub->icon(0).actualSize(QSize(20, 24));
                QRect iconRect(iconItem.left() + 4, iconItem.top() + (iconItem.height() - iconSize.height()) / 2,
                               iconSize.width(), iconSize.height());
                if (iconRect.contains(event->pos())) {
                    sub->changeVisibility();
                    call = false;
                } else {
                    if (auto* parent = dynamic_cast<ModelTreeWidgetItem*>(sub->parent())) {
                        if (currentItem() != parent) {
                            iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(parent->getModel());
                            emit ChangeCurrentModel(parent->getModel());
                        }
                    }

                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    if (!scene) { return; }
                    auto draw = DynamicCast<iGame::DrawObject>(sub->getDataObject());
                    if (!draw) { return; }
                    QTreeWidgetItem* rootItem = sub;
                    while (rootItem->parent() != nullptr) rootItem = rootItem->parent();
                    if (auto* modelItem = dynamic_cast<ModelTreeWidgetItem*>(rootItem);
                        modelItem != nullptr &&
                        modelItem->getModel() != nullptr &&
                        modelItem->getModel()->GetDataObject() != nullptr) {
                        m_ActiveAttributeTargets.remove(
                            modelItem->getModel()->GetDataObject()->GetDataObjectId());
                        igQtAttributeDataSourceManager::Instance()->ClearActiveAttributeTarget(
                            modelItem->getModel()->GetDataObject()->GetDataObjectId());
                    }
                    if (draw->ViewCloudPicture(scene, -1, -1, false)) {
                        Q_EMIT ViewCloudPicture();
                    }
                }
            }
        } else if (auto* sa = dynamic_cast<SubAttribTreeWidgetItem*>(child)) {
            // Handle sub-attribute selection display and apply
            auto* parent = dynamic_cast<SubObjectTreeWidgetItem*>(sa->parent());
            if (parent) {
                // Hide previous
                if (auto* current = dynamic_cast<SubAttribTreeWidgetItem*>(parent->getCurrentChild())) {
                    current->hide();
                }
                // Show current
                sa->show();
                parent->setCurrentChild(sa);
                int dim = sa->currentIndex();
                if (dim == -1) dim = 0;
                // For single-component fields: use component 0
                // For multi-component fields: index 0=Magnitude(-1), 1=x(0), 2=y(1), etc.
                int actualDim = (sa->getDimension() == 1) ? 0 : (dim - 1);
                int rootObjectId = -1;
                QTreeWidgetItem* rootItem = sa;
                while (rootItem->parent() != nullptr) rootItem = rootItem->parent();
                auto* modelItem = dynamic_cast<ModelTreeWidgetItem*>(rootItem);
                if (modelItem != nullptr &&
                    modelItem->getModel() != nullptr &&
                    modelItem->getModel()->GetDataObject() != nullptr) {
                    rootObjectId = modelItem->getModel()->GetDataObject()->GetDataObjectId();
                    SetActiveAttributeTarget(rootObjectId, sa->attributeTarget());
                }
                if (sa->hasAttributeData() &&
                    sa->loadState() != iGame::AttributeDataLoadState::Loaded) {
                    sa->setPendingDimension(actualDim);
                    if (modelItem != nullptr &&
                        modelItem->getModel() != nullptr &&
                        modelItem->getModel()->GetDataObject() != nullptr) {
                        igQtAttributeDataSourceManager::Instance()->RequestAttribute(
                            modelItem->getModel()->GetDataObject()->GetDataObjectId(),
                            sa->attributeTarget());
                    }
                    call = false;
                    return;
                }
                if (sa->viewAttribute(actualDim)) {
                    emit ViewCloudPicture();
                }
                call = false;
            }
        } else {
            // Top-level attribute item under model
            int index = child->data(0, Qt::UserRole).toInt();
            ModelTreeWidgetItem* parent = dynamic_cast<ModelTreeWidgetItem*>(child->parent());
            if (parent) {
                if (parent->getModel() != iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()) {
                    iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(parent->getModelId());

                    emit ChangeCurrentModel(parent->getModel());
                }
                AttribTreeWidgetItem* current{nullptr};
                if (parent->getCurrentChild()) {
                    current = dynamic_cast<AttribTreeWidgetItem*>(parent->getCurrentChild());
                }

                if (current) { current->hide(); }
                AttribTreeWidgetItem* c = dynamic_cast<AttribTreeWidgetItem*>(child);
                if (c) {
                    c->show();
                    parent->setCurrentChild(child);

                    int dim = c->currentIndex();
                    if (dim == -1) { dim = 0; }
                    // For single-component fields: use component 0
                    // For multi-component fields: index 0=Magnitude(-1), 1=x(0), 2=y(1), etc.
                    int actualDim = (c->getDimension() == 1) ? 0 : (dim - 1);
                    const int rootObjectId = parent->getModel()->GetDataObject()->GetDataObjectId();
                    SetActiveAttributeTarget(rootObjectId, c->attributeTarget());
                    if (c->hasAttributeData() &&
                        c->loadState() != iGame::AttributeDataLoadState::Loaded) {
                        c->setPendingDimension(actualDim);
                        igQtAttributeDataSourceManager::Instance()->RequestAttribute(
                            rootObjectId,
                            c->attributeTarget());
                        call = false;
                        return;
                    }
                    c->viewAttribute(actualDim);
                }
            }
        }
    }
    if (call) {
        // Call the base class's mousePressEvent to ensure that other events continue to be handled
        QTreeWidget::mousePressEvent(event);
    }
}
