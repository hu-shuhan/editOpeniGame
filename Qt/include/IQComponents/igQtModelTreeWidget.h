#pragma once

#include <iGameModel.h>
#include <iGameSceneManager.h>

#include <IQCore/igQtExportModule.h>

#include <QMouseEvent>
#include <QDockWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <qboxlayout.h>
#include <QObject>

#include <iostream>

class HoverButton : public QPushButton {
    Q_OBJECT
public:
    HoverButton(const QString& text, QWidget* parent = nullptr)
        : QPushButton(text, parent), m_checked(false) {
        init();
    }
    HoverButton(QWidget* parent = nullptr)
        : QPushButton(parent), m_checked(false) {
        init();
    }

    template<typename Functor, typename ...Args>
    void setConcernFunctor(Functor&& functor, Args&&... args) {
        concernFunctor = std::bind(functor, args...);
    }
    template<typename Functor, typename ...Args>
    void setCancelFunctor(Functor&& functor, Args&&... args) {
        cancelFunctor = std::bind(functor, args...);
    }

    bool isChecked() const {
        return m_checked;
    }

    void setChecked(bool flag) {
        if (m_checked != flag) {
            m_checked = flag;
            if (m_checked) {
                setStyleSheet(checkedStyle);
            }
            else {
                setStyleSheet(honorStyle);
            }
        }
    }

protected:
    void init() {
        defaultStyle = "width:24px;height:24px;border-style:solid;border-width:1px;border-color:rgba(0,0,0,0);border-radius:2px;";
        honorStyle = "width:24px;height:24px;border-style:solid;border-width:1px;border-color:rgba(0,0,0,0);border-radius:2px;background-color: #cce8ff;";
        checkedStyle = "width:24px;height:24px;border-style:solid;border-width:1px;border-color:#99d1ff;border-radius:2px;background-color: #cce8ff;";
        setAttribute(Qt::WA_Hover, true);
        setFlat(true);
        setStyleSheet(defaultStyle);
    }

    void mousePressEvent(QMouseEvent* event) override {
        QPushButton::mousePressEvent(event); 

        if (this->isChecked()) {
            setChecked(false);
            if (concernFunctor) {
                cancelFunctor();
            }
            
        }
        else {
            setChecked(true);
            if (cancelFunctor) {
                concernFunctor();
            }
            
        }
    }

    void enterEvent(QEvent* event) override {
        QPushButton::enterEvent(event); 
        if (!m_checked) {
            setStyleSheet(honorStyle);
        }
    }

    void leaveEvent(QEvent* event) override {
        QPushButton::leaveEvent(event); 
        if (!m_checked) {
            setStyleSheet(defaultStyle);
        }
    }

private:
    bool m_checked;
    QString defaultStyle;
    QString honorStyle;
    QString checkedStyle;

    std::function<void()> concernFunctor;
    std::function<void()> cancelFunctor;
};


class IG_QT_MODULE_EXPORT ModelTreeWidgetItem : public QTreeWidgetItem {
public:
    ModelTreeWidgetItem(QTreeWidget* parent = nullptr) 
        : QTreeWidgetItem(parent), visibility(true)
    {
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

        view_wireframe->setConcernFunctor(&ModelTreeWidgetItem::showWireframe, this);
        view_wireframe->setCancelFunctor(&ModelTreeWidgetItem::hideWireframe, this);

        view_fill->setConcernFunctor(&ModelTreeWidgetItem::showFill, this);
        view_fill->setCancelFunctor(&ModelTreeWidgetItem::hideFill, this);

        view_pickedItem->setConcernFunctor(&ModelTreeWidgetItem::showPickedItem, this);
        view_pickedItem->setCancelFunctor(&ModelTreeWidgetItem::hidePickedItem, this);
    }
    iGame::Model* getModel(){
        return this->model.get();
    }

    void setModel(iGame::Model::Pointer model) {
        this->model = model;
        view_wireframe->setChecked(true);
        view_fill->setChecked(true);
        view_pickedItem->setChecked(true);
        showWireframe();
        showFill();
        showPickedItem();
    }

    void setName(const QString& name) {
        setText(0, name);
    }

    void changeVisibility() {
        if (getVisibility()) {
            hide();
        }
        else {
            show();
        }
    }

protected:
    bool getVisibility() const {
        return visibility;
    }

    void show() {
        visibility = true;
        this->setIcon(0, QIcon(":/Ticon/Icons/select/eye-open.png"));
        if (!model) {
            return;
        }

        model->Show();
        update();
    }

    void hide() {
        visibility = false;
        this->setIcon(0, QIcon(":/Ticon/Icons/select/eye-close.png"));
        if (!model) {
            return;
        }

        model->Hide();
        update();
    }

    void showBoundingBox() {
        model->SetBoundingBoxSwitch(true);
        update();
    }
    void hideBoundingBox() {
        model->SetBoundingBoxSwitch(false);
        update();
    }

    void showPoints() {
        model->SetViewPointsSwitch(true);
        update();
    }
    void hidePoints() {
        model->SetViewPointsSwitch(false);
        update();
    }

    void showWireframe() {
        model->SetViewWireframeSwitch(true);
        update();
    }
    void hideWireframe() {
        model->SetViewWireframeSwitch(false);
        update();
    }

    void showFill() {
        model->SetViewFillSwitch(true);
        update();
    }
    void hideFill() {
        model->SetViewFillSwitch(false);
        update();
    }

    void showPickedItem() {
        model->SetPickedItemSwitch(true);
        update();
    }
    void hidePickedItem() {
        model->SetPickedItemSwitch(false);
        update();
    }

    void update() {
        if (model) {
            model->Update();
        }
    }

private:
    bool visibility;
    HoverButton* view_bbox;
    HoverButton* view_points;
    HoverButton* view_wireframe;
    HoverButton* view_fill;
    HoverButton* view_pickedItem;

    int modelId;
    iGame::Model::Pointer model;
};

class IG_QT_MODULE_EXPORT igQtModelTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    igQtModelTreeWidget(QWidget* parent = nullptr) : QTreeWidget(parent) {

    }

    ModelTreeWidgetItem* getItem(const QPoint& p) const {
        return dynamic_cast<ModelTreeWidgetItem*>(itemAt(p));
    }
    QTreeWidgetItem* getChild(const QPoint& p) const {
        return dynamic_cast<QTreeWidgetItem*>(itemAt(p));
    }
protected:
    void mousePressEvent(QMouseEvent* event) override {
        bool call = true;
        ModelTreeWidgetItem* item = getItem(event->pos());
        QTreeWidgetItem* child = nullptr;
        if (item) {
            // Gets the position of the click and the position of the icon
            QRect iconItem = visualItemRect(item);
            QSize iconSize = item->icon(0).actualSize(QSize(20, 24));
            QRect iconRect(iconItem.left() + 4, iconItem.top() + (iconItem.height() - iconSize.height()) / 2, iconSize.width(), iconSize.height());

            // Determine if the icon area has been clicked
            if (iconRect.contains(event->pos())) {
                item->changeVisibility();
                call = false;
            }
            else if(currentItem() != item){ // Check operation       
                iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(item->getModel());
                setItemSelected(item, true);
                item->getModel()->ViewCloudPicture(-1);
                emit ChangeCurrentModel(item->getModel());
            }
        }
        else if((child = getChild(event->pos())) && child) {
            int index = child->data(0, Qt::UserRole).toInt();
            ModelTreeWidgetItem* parent = dynamic_cast<ModelTreeWidgetItem*>(child->parent());
            if (parent) {
                parent->getModel()->ViewCloudPicture(index);
                Q_EMIT ViewCloudPicture();
            }
        }
        if (call)
        {
            // Call the base class's mousePressEvent to ensure that other events continue to be handled
            QTreeWidget::mousePressEvent(event);
        }

    }

signals:
    void ChangeCurrentModel(iGame::Model* model);
    void ViewCloudPicture();
private:

};