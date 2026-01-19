#pragma once

#include <iGameModel.h>
#include <iGameSceneManager.h>

#include <IQComponents/igQtComponents.h>
#include <IQCore/igQtExportModule.h>

#include <QComboBox>
#include <QDockWidget>
#include <QMouseEvent>
#include <QObject>
#include <QPushButton>
#include <QTreeWidget>
#include <qboxlayout.h>

#include <iostream>

class igQtModelTreeWidget; // forward declaration for dynamic_cast in SubAttribTreeWidgetItem

class IG_QT_MODULE_EXPORT ModelTreeWidgetItem : public QTreeWidgetItem {
public:
    ModelTreeWidgetItem(QTreeWidget* parent = nullptr);

    iGame::Model* getModel();

    void setModel(iGame::Model* model);

    void setName(const QString& name);

    void changeVisibility();

    void changeVisibility(bool);

    void viewAttribute(int index, int dim = -1);

    void setCurrentChild(QTreeWidgetItem* child);
    QTreeWidgetItem* getCurrentChild();

    int getModelId() const { return modelId; }
    void setModelId(int id) { modelId = id; }

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
    iGame::Model* model{nullptr};
    QTreeWidget* parent{nullptr};
    QTreeWidgetItem* current_child{nullptr};
};

class IG_QT_MODULE_EXPORT AttribTreeWidgetItem : public QTreeWidgetItem {
public:
    AttribTreeWidgetItem(int index, QTreeWidget* treeview = nullptr, ModelTreeWidgetItem* parent = nullptr);

    void setDimension(int length);
    int getDimension() const { return m_Dimension; }

    int currentIndex() const { return comboBox->currentIndex(); }
    void show() { comboBox->show(); }
    void hide() { comboBox->hide(); }
    MComboBox* get() { return comboBox; }
    void viewAttribute(int dim) { parent->viewAttribute(index, dim); }

private:
    int index;
    int m_Dimension{1};
    MComboBox* comboBox;
    ModelTreeWidgetItem* parent;
};

// Sub-data object tree item to represent DataObject hierarchy
class IG_QT_MODULE_EXPORT SubObjectTreeWidgetItem : public QTreeWidgetItem {
public:
    SubObjectTreeWidgetItem(QTreeWidgetItem* parent = nullptr) : QTreeWidgetItem(parent) {}

    void setDataObject(iGame::DataObject::Pointer obj) { m_DataObject = obj; }
    iGame::DataObject::Pointer getDataObject() const { return m_DataObject; }

    void setName(const QString& name) { setText(0, name); }

    bool getVisibility() const {
        auto draw = DynamicCast<iGame::DrawObject>(m_DataObject);
        return draw ? draw->GetVisibility() : true;
    }

    void changeVisibility() {
        if (getVisibility()) hide();
        else
            show();
    }

    void changeVisibility(bool vis) {
        if (!vis) hide();
        else
            show();
    }

    void show() {
        auto draw = DynamicCast<iGame::DrawObject>(m_DataObject);
        if (draw) { draw->SetVisibility(true); }
        this->setIcon(0, QIcon(":/Ticon/Icons/select/eye-open.png"));
        // sync descendants' icons to current (data already recursively set by DrawObject::SetVisibility)
        SyncIconWithVisibility(true);
        updateScene();
    }

    void hide() {
        auto draw = DynamicCast<iGame::DrawObject>(m_DataObject);
        if (draw) { draw->SetVisibility(false); }
        this->setIcon(0, QIcon(":/Ticon/Icons/select/eye-close.png"));
        // sync descendants' icons to current (data already recursively set by DrawObject::SetVisibility)
        SyncIconWithVisibility(true);
        updateScene();
    }

    void setCurrentChild(QTreeWidgetItem* c) { m_CurrentChild = c; }
    QTreeWidgetItem* getCurrentChild() const { return m_CurrentChild; }

    // Update only icons to reflect current visibility status (no data changes)
    void SyncIconWithVisibility(bool deep = true) {
        auto draw = DynamicCast<iGame::DrawObject>(m_DataObject);
        if (draw) {
            this->setIcon(0, QIcon(draw->GetVisibility() ? ":/Ticon/Icons/select/eye-open.png"
                                                         : ":/Ticon/Icons/select/eye-close.png"));
        }
        if (deep) {
            for (int i = 0; i < childCount(); ++i) {
                if (auto* sub = dynamic_cast<SubObjectTreeWidgetItem*>(child(i))) { sub->SyncIconWithVisibility(true); }
            }
        }
    }

private:
    void updateScene() {
        // Find parent model item and trigger scene update
        QTreeWidgetItem* p = parent();
        while (p) {
            if (auto* mi = dynamic_cast<ModelTreeWidgetItem*>(p)) {
                if (mi->getModel()) mi->getModel()->Update();
                break;
            }
            p = p->parent();
        }
    }

    iGame::DataObject::Pointer m_DataObject{nullptr};
    QTreeWidgetItem* m_CurrentChild{nullptr};
};

// Attribute item for sub-data objects
class IG_QT_MODULE_EXPORT SubAttribTreeWidgetItem : public QTreeWidgetItem {
public:
    SubAttribTreeWidgetItem(int index, QTreeWidget* treeview = nullptr, SubObjectTreeWidgetItem* parent = nullptr)
        : QTreeWidgetItem(parent), m_Index(index), m_Tree(treeview), m_Parent(parent) {
        QWidget* widget = new QWidget(treeview);
        m_Combo = new MComboBox(this, widget);
        m_Combo->setStyleSheet("QComboBox { background-color: transparent; }"
                               "QComboBox QAbstractItemView { background-color: white; }");
        setDimension(1);
        treeview->setItemWidget(this, 1, widget);
        hide();
    }


    void setDimension(int length) {
        m_Combo->clear();
        m_Dimension = length;
        
        // For Dimension=1, show only the single dimension value, not magnitude
        if (length == 1) {
            m_Combo->addItem(QString::fromStdString("x"));
            m_Combo->setCurrentIndex(0);
            return;
        }
        
        // For Dimension>=2, show magnitude first, then individual dimensions
        m_Combo->addItem("magnitude");
        if (length < 4) {
            if (length > 0) m_Combo->addItem(QString::fromStdString("x"));
            if (length > 1) m_Combo->addItem(QString::fromStdString("y"));
            if (length > 2) m_Combo->addItem(QString::fromStdString("z"));
        } else {
            for (int i = 0; i < length; i++) { m_Combo->addItem(QString::fromStdString("D" + std::to_string(i))); }
        }
        m_Combo->setCurrentIndex(0);
    }

    int getDimension() const { return m_Dimension; }

    int currentIndex() const { return m_Combo->currentIndex(); }
    void show() { m_Combo->show(); }
    void hide() { m_Combo->hide(); }

    void viewAttribute(int dim) {
        if (!m_Parent) return;
        auto obj = m_Parent->getDataObject();
        auto draw = DynamicCast<iGame::DrawObject>(obj);
        if (draw) {
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            draw->ViewCloudPicture(scene, m_Index, dim);
            if (scene) scene->Update();
        }
    }

private:
    int m_Index;
    int m_Dimension{1};
    MComboBox* m_Combo{nullptr};
    QTreeWidget* m_Tree{nullptr};
    SubObjectTreeWidgetItem* m_Parent{nullptr};
};

class IG_QT_MODULE_EXPORT igQtModelTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    igQtModelTreeWidget(QWidget* parent = nullptr);

    ModelTreeWidgetItem* getItem(const QPoint& p) const;
    QTreeWidgetItem* getChild(const QPoint& p) const;

    //void setCurrentModelItem(ModelTreeWidgetItem* item);
    //ModelTreeWidgetItem* getCurrentModelItem();

protected:
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void ChangeCurrentModel(iGame::Model* model);
    void ViewCloudPicture();

private:
    //ModelTreeWidgetItem* currentModelItem{nullptr};
};
