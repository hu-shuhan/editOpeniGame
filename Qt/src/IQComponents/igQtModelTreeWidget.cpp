#include <IQComponents/igQtModelTreeWidget.h>
#include <QAction>
#include <QMenu>
#include <QHeaderView>
#include <QEvent>
#include <QStyle>
#include <QStyleOptionViewItem>

#include "iGameSceneManager.h"

ModelTreeWidgetItem::ModelTreeWidgetItem(QTreeWidget* parent) : QTreeWidgetItem(parent), visibility(true) {
    QWidget* buttonWidget = new QWidget(parent);
    buttonWidget->setStyleSheet(QStringLiteral("background-color: transparent; border: none;"));
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
    layout->addStretch();
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
    // Reflect the style chosen by the reader/filter without changing the model.
    auto drawObject = DynamicCast<iGame::DrawObject>(model->GetDataObject());
    const auto viewStyle = drawObject ? drawObject->GetViewStyle() : 0;
    view_points->setChecked((viewStyle & IG_POINTS) != 0);
    view_wireframe->setChecked((viewStyle & IG_WIREFRAME) != 0);
    view_fill->setChecked((viewStyle & IG_SURFACE) != 0);
    view_pickedItem->setChecked(true);
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
void ModelTreeWidgetItem::viewAttribute(int index, int dim) {
    model->ViewCloudPicture(index, dim);
    Q_EMIT dynamic_cast<igQtModelTreeWidget*>(this->parent)->ViewCloudPicture();
}

void ModelTreeWidgetItem::setCurrentChild(QTreeWidgetItem* child) { current_child = child; }
QTreeWidgetItem* ModelTreeWidgetItem::getCurrentChild() { return current_child; }

bool ModelTreeWidgetItem::getVisibility() const { return visibility; }

void ModelTreeWidgetItem::show() {
    visibility = true;
    this->setIcon(0, igQtModelTreeIcons::EyeOpen());
    if (!model) { return; }

    model->Show();
    update();
}

void ModelTreeWidgetItem::hide() {
    visibility = false;
    this->setIcon(0, igQtModelTreeIcons::EyeClose());
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
    widget->setStyleSheet(QStringLiteral("background-color: transparent; border: none;"));
    comboBox = new MComboBox(this, widget);
    auto* comboLayout = new QHBoxLayout(widget);
    comboLayout->setContentsMargins(0, 0, 0, 0);
    comboLayout->addWidget(comboBox);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    comboBox->setStyleSheet("QComboBox { background-color: transparent; }");

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

igQtModelTreeWidget::igQtModelTreeWidget(QWidget* parent) : QTreeWidget(parent) {
    // Keep eliding ("xxxx...") but show full name via tooltip.
    setTextElideMode(Qt::ElideRight);

    if (header()) {
        header()->setStretchLastSection(false);
        header()->setSectionResizeMode(QHeaderView::Interactive);
    }
}

void igQtModelTreeWidget::setLeftColumnPercent(int percent) {
    m_leftPercent = qBound(30, percent, 60);
    m_lastLeft = -1;
    m_lastRight = -1;
    applyColumnProportions();
}

void igQtModelTreeWidget::resizeEvent(QResizeEvent* event) {
    QTreeWidget::resizeEvent(event);
    applyColumnProportions();
}

void igQtModelTreeWidget::showEvent(QShowEvent* event) {
    QTreeWidget::showEvent(event);
    applyColumnProportions();
}

void igQtModelTreeWidget::applyColumnProportions() {
    if (!header() || columnCount() < 2) return;
    const int sbW = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this);
    const int vw = qMax(220, width() - sbW);
    int leftW = qRound(vw * m_leftPercent / 100.0);
    const int minLeft = 108;
    const int maxLeft = qMax(minLeft, vw - 140);
    leftW = qBound(minLeft, leftW, maxLeft);
    const int rightW = vw - leftW;
    if (leftW == m_lastLeft && rightW == m_lastRight) return;
    m_lastLeft = leftW;
    m_lastRight = rightW;
    setColumnWidth(0, leftW);
    setColumnWidth(1, rightW);
}

ModelTreeWidgetItem* igQtModelTreeWidget::getItem(const QPoint& p) const {
    return dynamic_cast<ModelTreeWidgetItem*>(itemAt(p));
}
QTreeWidgetItem* igQtModelTreeWidget::getChild(const QPoint& p) const {
    return dynamic_cast<QTreeWidgetItem*>(itemAt(p));
}

QRect igQtModelTreeWidget::eyeHitRect(const QTreeWidgetItem* item) const {
    if (!item) return QRect();
    const QRect cell = visualItemRect(item);
    if (cell.isEmpty()) return QRect();

    QStyleOptionViewItem opt;
    opt.initFrom(this);
    opt.rect = cell;
    opt.features = QStyleOptionViewItem::HasDecoration | QStyleOptionViewItem::HasDisplay;
    opt.decorationPosition = QStyleOptionViewItem::Left;
    opt.decorationAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    opt.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    opt.text = item->text(0);
    opt.icon = item->icon(0);
    opt.decorationSize = iconSize();
    opt.font = font();
    opt.fontMetrics = QFontMetrics(opt.font);
    const QRect textRect = style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, this);
    if (textRect.isValid() && !textRect.isEmpty() && textRect.left() > cell.left() + 2) {
        return QRect(cell.left(), cell.top(), textRect.left() - cell.left(), cell.height());
    }

    const QSize sz = item->icon(0).actualSize(iconSize());
    return QRect(cell.left() + 4, cell.top() + (cell.height() - sz.height()) / 2, sz.width(), sz.height());
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
        const QRect iconItem = visualItemRect(item);
        const QRect eyeRect = eyeHitRect(item);

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

        if (clickedOnIndicator) {
        } else if (eyeRect.contains(event->pos())) {
            item->changeVisibility();
            // sync all sub-block icons under this model to reflect current visibility
            for (int i = 0; i < item->childCount(); ++i) {
                if (auto* sub = dynamic_cast<SubObjectTreeWidgetItem*>(item->child(i))) {
                    sub->SyncIconWithVisibility(true);
                }
            }
            call = false;
        } else if (currentItem() != item) { // Check operation - only when clicking on the model itself
            if (item->getModel() != iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()) {
                iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(item->getModel());
                emit ChangeCurrentModel(item->getModel());
            }

            item->setSelected(true);
            item->getModel()->ViewCloudPicture(-1);
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
                const QRect eyeRect = eyeHitRect(sub);
                int subLevel = 0;
                for (QTreeWidgetItem* p = sub->parent(); p; p = p->parent()) { ++subLevel; }
                const QRect subIndicator(0, eyeRect.top(), indentation() * (subLevel + 1), eyeRect.height());
                if (!subIndicator.contains(event->pos()) && eyeRect.contains(event->pos())) {
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
                // For single-component fields: use component 0
                // For multi-component fields: index 0=Magnitude(-1), 1=x(0), 2=y(1), etc.
                int actualDim = (sa->getDimension() == 1) ? 0 : (dim - 1);
                sa->viewAttribute(actualDim);
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
                    c->viewAttribute(actualDim);
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
