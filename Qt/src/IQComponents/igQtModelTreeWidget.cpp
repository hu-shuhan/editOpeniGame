#include <IQComponents/igQtModelTreeWidget.h>
#include <QAction>
#include <QMenu>

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

void ModelTreeWidgetItem::setName(const QString& name) { setText(0, name); }

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
void ModelTreeWidgetItem::viewAttribute(int index, int dim) {
    model->ViewCloudPicture(index, dim);
    Q_EMIT dynamic_cast<igQtModelTreeWidget*>(this->parent)->ViewCloudPicture();
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

    comboBox->addItem("magnitude");
    if (length < 2) return;
    if (length < 4) {
        if (length > 0) comboBox->addItem(QString::fromStdString("x"));
        if (length > 1) comboBox->addItem(QString::fromStdString("y"));
        if (length > 2) comboBox->addItem(QString::fromStdString("z"));
    } else {
        for (int i = 0; i < length; i++) { comboBox->addItem(QString::fromStdString("D" + std::to_string(i))); }
    }
    comboBox->setCurrentIndex(0);
}

igQtModelTreeWidget::igQtModelTreeWidget(QWidget* parent) : QTreeWidget(parent) {}

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

        if (event->button() == Qt::RightButton) {
            QMenu menu(this);
            QAction* setCenterAction = menu.addAction(QString::fromUtf8("设置旋转中心为当前模型"));
            connect(setCenterAction, &QAction::triggered, this, [item]() {
                auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                if (scene && item->getModel()) { scene->ResetCameraView(item->getModel()->GetDataObject()); }
            });
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
        } else if (currentItem() != item) { // Check operation
            iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(item->getModel());
            setItemSelected(item, true);
            item->getModel()->ViewCloudPicture(-1);
            emit ChangeCurrentModel(item->getModel());
            Q_EMIT ViewCloudPicture();
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
                    draw->ViewCloudPicture(scene, -1);
                    Q_EMIT ViewCloudPicture();
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
                sa->viewAttribute(dim - 1);
                call = false;
            }
        } else {
            // Top-level attribute item under model
            int index = child->data(0, Qt::UserRole).toInt();
            ModelTreeWidgetItem* parent = dynamic_cast<ModelTreeWidgetItem*>(child->parent());
            if (parent) {
                auto currentModelItem = currentItem();
                if (currentModelItem != parent) {
                    //this->setCurrentModelItem(parent);
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
                    c->viewAttribute(dim - 1);
                    Q_EMIT ViewCloudPicture();
                }
            }
        }
    }
    if (call) {
        // Call the base class's mousePressEvent to ensure that other events continue to be handled
        QTreeWidget::mousePressEvent(event);
    }
}
