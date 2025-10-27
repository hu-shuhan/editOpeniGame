//#include "igQtModelListView.h"
#include <iGameSceneManager.h>

#include <IQComponents/igQtModelListView.h>
#include <QModelIndexList>
#include <QQueue>
#include <QStandardItemModel>
#include <qaction.h>
#include <qdebug.h>
#include <qmenu.h>

igQtModelListView::igQtModelListView(QWidget* parent) : QTreeView(parent) {
    m_Manager = iGame::SceneManager::Instance();

    this->setHeaderHidden(true);
    // Create a QStandardItemModel to hold data
    model = new QStandardItemModel(this);
    // Populate the model with some data
    this->setModel(model);
    this->setStyleSheet("QTreeView::item:selected { background-color:  rgb(0, 115, 153); color: white; }");

    iconDelegate = new IconDelegate(this);
    setItemDelegate(iconDelegate);
    //setItemDelegate(new LineDelegate(this));

    connect(iconDelegate, &IconDelegate::iconClicked, [&](const QModelIndex& index) {
        QStandardItem* item = model->itemFromIndex(index);
        if (item) {
            ReverseItemVisibility(item);
            int id = GetObjectIdFromItem(item);

            //m_Manager->GetCurrentScene()->ChangeDataObjectVisibility(id, itemVisibleList[item]);
            //auto* obj = m_Manager->GetCurrentScene()->GetDataObject(id);
            //obj->SetVisibility(itemVisibleList[item]);

            //this->set
            Q_EMIT UpdateCurrentScene();

            for (int i = 0; i < item->rowCount(); i++) {
                auto child = item->child(i);
                if (itemVisibleList[child] != itemVisibleList[item]) { ReverseItemVisibility(child); }
            }

            auto par = item->parent();
            if (par != nullptr) {
                bool haveVisibleChild = false;
                for (int i = 0; i < par->rowCount(); i++) {
                    if (itemVisibleList[par->child(i)]) {
                        haveVisibleChild = true;
                        break;
                    }
                }
                if ((itemVisibleList[par] && !haveVisibleChild) || (!itemVisibleList[par] && haveVisibleChild))
                    ReverseItemVisibility(par);
            }
        }
    });

    connect(this, &QTreeView::clicked, this, [&](const QModelIndex& index) {
        QStandardItem* item = model->itemFromIndex(index);
        this->setCurrentIndex(index);
        if (item) {
            int newId = GetObjectIdFromItem(item);
            if (newId == this->currentObjectIdx) { return; }
            this->currentObjectIdx = newId;

            //m_Manager->GetCurrentScene()->UpdateCurrentDataObject(currentObjectIdx);
            Q_EMIT UpdateCurrentItemToOtherQtModule();
        }
    });

    // 连接右键点击事件
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeView::customContextMenuRequested, this, &igQtModelListView::UpdateCustomMenu);


    this->contextMenu = new QMenu(this);
    QAction* action_Open = new QAction(QIcon(":/Ticon/Icons/Open.png"), "Open", this->contextMenu);
    QAction* action_ShwoAll = new QAction(QIcon(":/Ticon/Icons/Eyeball.svg"), "Show All", this->contextMenu);
    QAction* action_HideAll = new QAction(QIcon(":/Ticon/Icons/EyeballClosed.svg"), "Hide All", this->contextMenu);
    QAction* action_Delete = new QAction(QIcon(":/Ticon/Icons/Delete.png"), "Delete", this->contextMenu);
    QAction* action_Rename = new QAction("Rename", this->contextMenu);
    QAction* action_Reload = new QAction("Reload", this->contextMenu);

    QAction* action_Extract = new QAction("Extract Group", this->contextMenu);
    QAction* action_AddPlane = new QAction("Add Plane", this->contextMenu);

    connect(action_Open, &QAction::triggered, this, [&]() { Q_EMIT OpenFile(); });
    connect(action_ShwoAll, &QAction::triggered, this, &igQtModelListView::ShowAllModel);
    connect(action_HideAll, &QAction::triggered, this, &igQtModelListView::HideAllModel);
    connect(action_Delete, &QAction::triggered, this, [&]() { Q_EMIT DeleteModel(); });
    connect(action_Rename, &QAction::triggered, this, [&]() { edit(currentIndex()); });

    contextMenu->addAction(action_Open);
    contextMenu->addAction(action_ShwoAll);
    contextMenu->addAction(action_HideAll);
    contextMenu->addSeparator();
    contextMenu->addAction(action_Delete);
    contextMenu->addAction(action_Rename);
    contextMenu->addAction(action_Reload);
    contextMenu->addSeparator();
    contextMenu->addAction(action_Extract);
    contextMenu->addAction(action_AddPlane);
}
void igQtModelListView::UpdateModelList() {
    //	QStandardItem* rootItem = model->invisibleRootItem();
    //	QStandardItem* newModel = new QStandardItem(QIcon(":/Ticon/Icons/Eyeball.svg"), modelName);
    //	rootItem->appendRow(newModel);
    //	this->setCurrentIndex(newModel->index());
    //	auto curObj = m_Manager->GetCurrentScene()->GetCurrentObject();
    //	itemObjectIds[newModel] = curObj->GetDataObjectId();
    //	currentObjectIdx = curObj->GetDataObjectId();

    //	if(curObj->HasSubDataObject()){
    //		int index = 0;
    //		for(auto it = curObj->SubBegin(); it != curObj->SubEnd(); it ++){
    //			auto& subObj = it->second;
    //			std::string name = subObj->GetName();
    //			AddChildToItem(newModel, QString::fromStdString(name), subObj->GetDataObjectId());
    //		}
    //	}
    //	auto& modelList  = m_Manager->GetCurrentScene()->GetModelList();
    //	for(auto& [mId, obj] : modelList)
    //	{
    //
    //	}
}


void igQtModelListView::ShowAllModel() {
    for (auto it = itemObjectIds.begin(); it != itemObjectIds.end(); ++it) {
        auto item = it.key();
        if (!itemVisibleList[item]) {
            item->setIcon(QIcon(":/Ticon/Icons/Eyeball.svg"));
            itemVisibleList[item] = true;
        }
    }
}
void igQtModelListView::HideAllModel() {
    for (auto it = itemObjectIds.begin(); it != itemObjectIds.end(); ++it) {
        auto item = it.key();
        if (itemVisibleList[item]) {
            item->setIcon(QIcon(":/Ticon/Icons/EyeballClosed.svg"));
            itemVisibleList[item] = false;
        }
    }
}
void igQtModelListView::UpdateCustomMenu(const QPoint& point) {
    QModelIndex index = indexAt(point);
    if (index.isValid()) {
        QStandardItem* item = model->itemFromIndex(index);
        if (item) { this->contextMenu->exec(mapToGlobal(point)); }
    } else {
        this->contextMenu->exec(mapToGlobal(point));
    }
}

int igQtModelListView::GetObjectIdFromItem(QStandardItem* item) {
    return itemObjectIds.count(item) ? itemObjectIds[item] : -1;
}

QStandardItem* igQtModelListView::GetObjectItemFromObjectId(int currentObjectId) {
    for (auto it = itemObjectIds.begin(); it != itemObjectIds.end(); ++it) {
        if (it.value() == currentObjectId) return it.key();
    }
    return nullptr;
}

void igQtModelListView::DeleteCurrentModelItem(QStandardItem* deleteItem) {
    QQueue<QStandardItem*> Q;
    for (int i = 0; i < model->rowCount(); i++) {
        if (model->item(i, 0) == deleteItem) {
            model->removeRow(i);
            return;
        }
        Q.enqueue(model->item(i, 0));
    }
    while (!Q.empty()) {
        auto item = Q.first();
        for (int i = 0; i < item->rowCount(); i++) {
            if (item->child(i) == deleteItem) {
                item->removeRow(i);
                return;
            }
            Q.push_back(item->child(i));
        }
        Q.dequeue();
    }
}

void igQtModelListView::AddModel(QString modelName) {
    QStandardItem* rootItem = model->invisibleRootItem();
    QStandardItem* newModel = new QStandardItem(QIcon(":/Ticon/Icons/Eyeball.svg"), modelName);
    rootItem->appendRow(newModel);
    this->setCurrentIndex(newModel->index());
    //auto curObj = m_Manager->GetCurrentScene()->GetCurrentObject();
    itemVisibleList[newModel] = true;
    
    //itemObjectIds[newModel] = curObj->GetDataObjectId();
    //currentObjectIdx = curObj->GetDataObjectId();

    //if(curObj->HasSubDataObject()){
    //	for(auto it = curObj->SubBegin(); it != curObj->SubEnd(); it ++){
    //		auto& subObj = it->second;
    //		std::string name = subObj->GetName();
    //		AddChildToItem(newModel, QString::fromStdString(name), subObj->GetDataObjectId());
    //	}
    //}
}

void igQtModelListView::DeleteCurrentFile() {
    QModelIndex a = this->currentIndex();
    auto item = model->itemFromIndex(this->currentIndex());
    //printf("%s\n", item->text().toStdString());
    if (item && itemObjectIds.count(item)) {

        int childNum = item->rowCount();
        for (int i = 0; i < childNum; i++) {
            auto child = item->child(i, 0);
            itemVisibleList.remove(child);
            itemObjectIds.remove(child);
            //			itemModelActors.remove(child);
        }

        DeleteCurrentModelItem(item);
        itemVisibleList.remove(item);
        itemObjectIds.remove(item);
    }
    //m_Manager->GetCurrentScene()->RemoveCurrentDataObject();
    item = model->itemFromIndex(this->currentIndex());
    if (item) { this->currentObjectIdx = GetObjectIdFromItem(item); }
    //printf("%s\n", model->itemFromIndex(this->currentIndex())->text().toStdString());
}

void igQtModelListView::ChangeSelected2NextItem() {
    if (itemVisibleList.empty()) return;
    currentObjectIdx = (currentObjectIdx + 1) % itemVisibleList.size();
}

void igQtModelListView::ChangeSelected2LastItem() {
    if (itemVisibleList.empty()) return;
    currentObjectIdx = (currentObjectIdx + itemVisibleList.size() - 1) % itemVisibleList.size();
}

void igQtModelListView::AddChildToItem(QStandardItem* parentItem, const QString& modelName, int objId) {
    int rowIndex = parentItem->rowCount();
    QStandardItem* newModel = new QStandardItem(QIcon(":/Ticon/Icons/Eyeball.svg"), modelName);
    parentItem->setChild(rowIndex, 0, newModel);
    itemVisibleList[newModel] = true;
    itemObjectIds[newModel] = objId;
}

void igQtModelListView::InsertObject(int idx, const QString& modelName) {
    QStandardItem* rootItem = model->invisibleRootItem();
    QStandardItem* item = nullptr;
    while (item == nullptr) item = GetObjectItemFromObjectId(idx--);

    QStandardItem* newModel = new QStandardItem(QIcon(":/Ticon/Icons/Eyeball.svg"), modelName);

    //    item->appendRow(newModel);
    auto par = item->parent();
    if (par == nullptr) rootItem->insertRow(item->row() + 1, newModel);
    else
        par->setChild(item->row() + 1, newModel);
    //    par->setChild(item->row() + 1, 0, newModel);
    //            item->insertRow(idx, newModel);
    this->setCurrentIndex(newModel->index());
    this->currentObjectIdx = nextObjectIdx++;
    itemVisibleList[newModel] = true;
    itemObjectIds[newModel] = this->currentObjectIdx;
}

void igQtModelListView::ReverseItemVisibility(QStandardItem* item) {
    if (itemVisibleList[item]) {
        item->setIcon(QIcon(":/Ticon/Icons/EyeballClosed.svg"));
    } else {
        item->setIcon(QIcon(":/Ticon/Icons/Eyeball.svg"));
    }
    itemVisibleList[item] = !itemVisibleList[item];
}

igQtModelListView::~igQtModelListView() = default;