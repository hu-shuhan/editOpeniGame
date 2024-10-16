#include <IQComponents/igQtModelTreeWidget.h>

ModelTreeWidgetItem::ModelTreeWidgetItem(QTreeWidget* parent)
    : QTreeWidgetItem(parent), visibility(true) {
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

    show();

    view_bbox->setConcernFunctor(&ModelTreeWidgetItem::showBoundingBox, this);
    view_bbox->setCancelFunctor(&ModelTreeWidgetItem::hideBoundingBox, this);

    view_points->setConcernFunctor(&ModelTreeWidgetItem::showPoints, this);
    view_points->setCancelFunctor(&ModelTreeWidgetItem::hidePoints, this);

    view_wireframe->setConcernFunctor(&ModelTreeWidgetItem::showWireframe,
                                      this);
    view_wireframe->setCancelFunctor(&ModelTreeWidgetItem::hideWireframe, this);

    view_fill->setConcernFunctor(&ModelTreeWidgetItem::showFill, this);
    view_fill->setCancelFunctor(&ModelTreeWidgetItem::hideFill, this);

    view_pickedItem->setConcernFunctor(&ModelTreeWidgetItem::showPickedItem,
                                       this);
    view_pickedItem->setCancelFunctor(&ModelTreeWidgetItem::hidePickedItem,
                                      this);
}
iGame::Model* ModelTreeWidgetItem::getModel() { return this->model.get(); }

void ModelTreeWidgetItem::setModel(iGame::Model::Pointer model) {
    this->model = model;
    view_wireframe->setChecked(true);
    view_fill->setChecked(true);
    view_pickedItem->setChecked(true);
    showWireframe();
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

void ModelTreeWidgetItem::viewAttribute(int index, int dim) {
    model->ViewCloudPicture(index, dim);
}

void ModelTreeWidgetItem::setCurrentChild(QTreeWidgetItem* child) {
    current_child = child;
}
QTreeWidgetItem* ModelTreeWidgetItem::getCurrentChild() {
    return current_child;
}

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

AttribTreeWidgetItem::AttribTreeWidgetItem(int index,
                                         QTreeWidget* treeview,
                     ModelTreeWidgetItem* parent)
    : index(index), QTreeWidgetItem(parent), parent(parent) {

    QWidget* widget = new QWidget(treeview);
    comboBox = new MComboBox(this, widget);
    comboBox->setStyleSheet(
            "QComboBox { background-color: transparent; }"
            "QComboBox QAbstractItemView { background-color: white; }");

    setDimension(1);

    treeview->setItemWidget(this, 1, widget);

    hide();
}

void AttribTreeWidgetItem::setDimension(int length) {
    comboBox->clear();
    comboBox->addItem("magnitude");
    for (int i = 0; i < length && i < 4; i++) {
        comboBox->addItem(QString::fromUtf8(NAME[i]));
    }
}

igQtModelTreeWidget::igQtModelTreeWidget(QWidget* parent)
    : QTreeWidget(parent) {}

ModelTreeWidgetItem* igQtModelTreeWidget::getItem(const QPoint& p) const {
    return dynamic_cast<ModelTreeWidgetItem*>(itemAt(p));
}
QTreeWidgetItem* igQtModelTreeWidget::getChild(const QPoint& p) const {
    return dynamic_cast<QTreeWidgetItem*>(itemAt(p));
}

void igQtModelTreeWidget::mousePressEvent(QMouseEvent* event) {
    bool call = true;
    ModelTreeWidgetItem* item = getItem(event->pos());
    QTreeWidgetItem* child = nullptr;
    if (item) {
        // Gets the position of the click and the position of the icon
        QRect iconItem = visualItemRect(item);
        QSize iconSize = item->icon(0).actualSize(QSize(20, 24));
        QRect iconRect(iconItem.left() + 4,
                       iconItem.top() +
                               (iconItem.height() - iconSize.height()) / 2,
                       iconSize.width(), iconSize.height());

        // Determine if the icon area has been clicked
        if (iconRect.contains(event->pos())) {
            item->changeVisibility();
            call = false;
        } else if (currentItem() != item) { // Check operation
            iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(
                    item->getModel());
            setItemSelected(item, true);
            item->getModel()->ViewCloudPicture(-1);
            emit ChangeCurrentModel(item->getModel());
        }
        auto* current =
                dynamic_cast<AttribTreeWidgetItem*>(item->getCurrentChild());
        if (current) { current->hide(); }
        item->setCurrentChild(nullptr);
    } else if ((child = getChild(event->pos())) && child) {
        int index = child->data(0, Qt::UserRole).toInt();
        ModelTreeWidgetItem* parent =
                dynamic_cast<ModelTreeWidgetItem*>(child->parent());
        if (parent) {

            auto* current = dynamic_cast<AttribTreeWidgetItem*>(
                    parent->getCurrentChild());
            if (current) { current->hide(); }
            AttribTreeWidgetItem* c =
                    dynamic_cast<AttribTreeWidgetItem*>(child);
            c->show();
            parent->setCurrentChild(child);

            int dim = c->currentIndex();
            parent->viewAttribute(index, dim - 1);
            Q_EMIT ViewCloudPicture();
        }
    }
    if (call) {
        // Call the base class's mousePressEvent to ensure that other events continue to be handled
        QTreeWidget::mousePressEvent(event);
    }
}