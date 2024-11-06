#pragma once

#include <iGameModel.h>
#include <iGameSceneManager.h>

#include <IQCore/igQtExportModule.h>
#include <IQComponents/igQtComponents.h>

#include <QMouseEvent>
#include <QDockWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <qboxlayout.h>
#include <QObject>
#include <QComboBox>

#include <iostream>

class IG_QT_MODULE_EXPORT ModelTreeWidgetItem : public QTreeWidgetItem {
public:
    ModelTreeWidgetItem(QTreeWidget* parent = nullptr);

    iGame::Model* getModel();

    void setModel(iGame::Model::Pointer model);

    void setName(const QString& name);

    void changeVisibility();

    void viewAttribute(int index, int dim = -1);

    void setCurrentChild(QTreeWidgetItem* child);
    QTreeWidgetItem* getCurrentChild();

protected:
    bool getVisibility() const;

    void show();
    void hide();

    void showBoundingBox();
    void hideBoundingBox();

    void showPoints();
    void hidePoints();

    void showWireframe();
    void hideWireframe();

    void showFill();
    void hideFill();

    void showPickedItem();
    void hidePickedItem();

    void update();

private:
    bool visibility;
    HoverButton* view_bbox;
    HoverButton* view_points;
    HoverButton* view_wireframe;
    HoverButton* view_fill;
    HoverButton* view_pickedItem;

    int modelId;
    iGame::Model::Pointer model;
    QTreeWidget* parent=nullptr;
    QTreeWidgetItem* current_child{nullptr};
};

class IG_QT_MODULE_EXPORT AttribTreeWidgetItem : public QTreeWidgetItem {
public:
    AttribTreeWidgetItem(int index, QTreeWidget* treeview = nullptr,
                         ModelTreeWidgetItem* parent = nullptr);

    void setDimension(int length);

    int currentIndex() const { return comboBox->currentIndex();}
    void show() { comboBox->show(); }
    void hide() { comboBox->hide(); }
    MComboBox* get() { return comboBox; }
    void viewAttribute(int dim) {
        parent->viewAttribute(index, dim);
        //std::cout << index << std::endl;
    }

private:
    int index;
    MComboBox* comboBox;
    ModelTreeWidgetItem* parent;
};

class IG_QT_MODULE_EXPORT igQtModelTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    igQtModelTreeWidget(QWidget* parent = nullptr);

    ModelTreeWidgetItem* getItem(const QPoint& p) const;
    QTreeWidgetItem* getChild(const QPoint& p) const;

    void setCurrentModelItem(ModelTreeWidgetItem* item);
    ModelTreeWidgetItem* getCurrentModelItem();

protected:
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void ChangeCurrentModel(iGame::Model* model);
    void ViewCloudPicture();

private:
    ModelTreeWidgetItem* currentModelItem{nullptr};
};