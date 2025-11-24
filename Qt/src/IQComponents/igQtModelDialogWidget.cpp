#include "Sources/iGameLineTypePointsSource.h"
#include <IQComponents/igQtModelDialogWidget.h>
#include <Plugin/qtpropertybrowser/qtpropertymanager.h>
#include <QQueue>
#include <QSplitter>
#include <iGameSceneManager.h>
#include <qaction.h>
#include <qdebug.h>
#include <qmenu.h>

namespace
{
// Build sub-dataobject hierarchy under a given parent tree item
static void BuildSubObjectTree(QTreeWidget* tree, QTreeWidgetItem* parentItem, iGame::DataObject::Pointer obj) {
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

        // add attribute children under this sub-object
        if (auto attrSet = sub->GetAttributeSet()) {
            auto all = attrSet->GetAllAttributes();
            for (int i = 0; i < all->GetNumberOfElements(); ++i) {
                auto& attr = all->GetElement(i);
                if (attr.isDeleted) continue;
                auto* aitem = new SubAttribTreeWidgetItem(i, tree, childItem);
                aitem->setText(0, QString::fromStdString(attr.pointer->GetName()));
                if (attr.attachmentType == IG_POINT) aitem->setIcon(0, QIcon(":/Ticon/Icons/select/point.png"));
                else if (attr.attachmentType == IG_CELL)
                    aitem->setIcon(0, QIcon(":/Ticon/Icons/select/hex.png"));
                aitem->setDimension(attr.pointer->GetDimension());
            }
        }

        // recurse into deeper hierarchy
        BuildSubObjectTree(tree, childItem, sub);
    }
}
} // namespace

igQtModelDialogWidget::igQtModelDialogWidget(QWidget* parent) : QDockWidget(parent), ui(new Ui::LayerDialog) {
    ui->setupUi(this);
    this->setMinimumWidth(parent->width() / 4);

    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    this->setWidget(splitter);
    splitter->addWidget(ui->modelTreeWidget);
    splitter->addWidget(ui->ModelInformationWidget);
    splitter->addWidget(ui->propertyWidget);
    splitter->setChildrenCollapsible(false);

    modelTreeWidget = ui->modelTreeWidget;
    propertyWidget = ui->propertyWidget;

    modelTreeWidget->setColumnCount(2);
    modelTreeWidget->header()->hide();
    modelTreeWidget->setColumnWidth(0, 140);
    modelTreeWidget->setColumnWidth(1, 200);
    modelTreeWidget->setIndentation(15);


    propertyWidget->setHeaderVisible(false);
    propertyManager = new QtVariantPropertyManager(propertyWidget);
    editFactory = new QtVariantEditorFactory(propertyWidget);
    propertyWidget->setFactoryForManager(propertyManager, editFactory);

    propertyWidget->removeProperty(objectGroup);
    objectGroup =
            propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), QStringLiteral("Object propertys"));
    propertyWidget->addProperty(objectGroup);


    prop_PointSize = propertyManager->addProperty(QVariant::Int, "Point Size");
    prop_PointSize->setEnabled(false);
    prop_PointSize->setValue(0);
    objectGroup->addSubProperty(prop_PointSize);
    propertyManager->setAttribute(prop_PointSize, "minimum", 1);
    propertyManager->setAttribute(prop_PointSize, "maximum", 99);
    propertyManager->setAttribute(prop_PointSize, "singleStep", 1);

    pror_LineWidth = propertyManager->addProperty(QVariant::Int, "Line Width");
    pror_LineWidth->setEnabled(false);
    pror_LineWidth->setValue(0);
    objectGroup->addSubProperty(pror_LineWidth);
    propertyManager->setAttribute(pror_LineWidth, "minimum", 1);
    propertyManager->setAttribute(pror_LineWidth, "maximum", 10);
    propertyManager->setAttribute(pror_LineWidth, "singleStep", 1);

    prop_Transparency = propertyManager->addProperty(QVariant::Double, "Transparency");
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
            &igQtModelDialogWidget::updateCurrentModelProperty);
    connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this,
            &igQtModelDialogWidget::updateCurrentModelInfo);
    //connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this, &igQtModelDialogWidget::updateCloudPicture);
    connect(modelTreeWidget, &igQtModelTreeWidget::ViewCloudPicture, this, &igQtModelDialogWidget::updateCloudPicture);
}

void igQtModelDialogWidget::UpdateCurrentModel(iGame::Model::Pointer model) {
    //    propertyTreeWidget->clear();
    //    currentModel = model.get();
    ////        model->GetDataObject()->
    ////        model->GetDataObject()->GetMetadata();
    //    QtVariantPropertyManager* varManager = new QtVariantPropertyManager(propertyTreeWidget);
    //    QtVariantEditorFactory* editFactory = new QtVariantEditorFactory(propertyTreeWidget);
    //    propertyTreeWidget->setFactoryForManager(varManager, editFactory);
    //    QtProperty* properties_groupItem = varManager->addProperty(QtVariantPropertyManager::groupTypeId(), QString("propertys"));
    //    auto metadata = model->GetDataObject()->GetMetadata();
    //    for(auto& [propName, propValue] : metadata->entries()){
    //        QtVariantProperty* item = nullptr;
    //        std::visit([&](auto && arg){
    //            using T = std::decay_t<decltype(arg)>;
    //            if constexpr (std::is_same_v<T, int>) {
    //
    //                item = varManager->addProperty(QVariant::Int, QString(propName.c_str()));
    //                item->setValue(std::get<int32_t>(propValue));
    //            } else if constexpr (std::is_same_v<T, uint32_t>){
    //                item = varManager->addProperty(QVariant::UInt, QString(propName.c_str()));
    //                item->setValue(std::get<uint32_t>(propValue));
    //            }
    //
    //        }, propValue);
    //
    //        if(item != nullptr){
    //            properties_groupItem->addSubProperty(item);
    //        }
    //    }
    //    connect(varManager, &QtVariantPropertyManager::valueChanged, this, [&](QtProperty * _t1, const QVariant & _t2){
    //        auto f = currentModel->GetModelFilter();
    //        auto filter = reinterpret_cast<LineTypePointsSource*>(f);
    //        filter->SetResolution(_t2.toInt());
    //        filter->Execute();
    //    });
    //    propertyTreeWidget->addProperty(properties_groupItem);
    //
    //    objectGroup = propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), QStringLiteral("Object propertys"));
    //    propertyTreeWidget->addProperty(objectGroup);
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
    auto attrSet = obj->GetAttributeSet()->GetAllAttributes();
    for (int i = 0; i < attrSet->GetNumberOfElements(); i++) {
        auto& attr = attrSet->GetElement(i);
        if (attr.isDeleted) continue;
        AttribTreeWidgetItem* child = new AttribTreeWidgetItem(i, modelTreeWidget, item);
        //if (obj->GetAttributeIndex() == i) {
        //    item->setCurrentChild(child);
        //    child->setSelected(true);
        //}
        child->setText(0, QString::fromStdString(attr.pointer->GetName()));
        if (attr.attachmentType == IG_POINT) 
            child->setIcon(0, QIcon(":/Ticon/Icons/select/point.png"));
        else if (attr.attachmentType == IG_CELL)
            child->setIcon(0, QIcon(":/Ticon/Icons/select/hex.png"));
        child->setDimension(attr.pointer->GetDimension());
        // std::cout << i << " " << attr.pointer->GetName() << std::endl;
    }
    item->viewAttribute(-1);
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

    // build attribute children
    auto attrSet = obj->GetAttributeSet()->GetAllAttributes();
    for (int i = 0; i < attrSet->GetNumberOfElements(); i++) {
        auto& attr = attrSet->GetElement(i);
        if (attr.isDeleted) continue;
        AttribTreeWidgetItem* child = new AttribTreeWidgetItem(i, modelTreeWidget, item);
        child->setText(0, QString::fromStdString(attr.pointer->GetName()));
        if (attr.attachmentType == IG_POINT) child->setIcon(0, QIcon(":/Ticon/Icons/select/point.png"));
        else if (attr.attachmentType == IG_CELL)
            child->setIcon(0, QIcon(":/Ticon/Icons/select/hex.png"));
        child->setDimension(attr.pointer->GetDimension());
    }

    // build sub-data objects hierarchy
    BuildSubObjectTree(modelTreeWidget, item, obj);

    modelTreeWidget->addTopLevelItem(item);
    modelTreeWidget->setCurrentItem(item);

    updateCurrentModelProperty(model);
    updateCurrentModelInfo();
    //QTreeWidgetItem* currentItem = modelTreeWidget->getCurrentModelItem();
    //std::cout << "add current model: " << currentItem << std::endl;
    return id;
}

int igQtModelDialogWidget::addModelToModelTree(iGame::Model::Pointer model) {
    ModelTreeWidgetItem* item = new ModelTreeWidgetItem(modelTreeWidget);
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

    auto id = scene->AddModel(model->GetDataObject());

    item->setName(QString::fromStdString(model->GetDataObject()->GetName()));
    item->setModel(model);

    // build sub-data objects hierarchy
    BuildSubObjectTree(modelTreeWidget, item, model->GetDataObject());

    modelTreeWidget->addTopLevelItem(item);
    modelTreeWidget->setCurrentItem(item);
    return id;
}
int igQtModelDialogWidget::updateCurrentModelInfo() {
    //    qDebug() << ui->modelTreeWidget->currentIndex();

    ui->ModelInformationWidget->updateInformationFrame();
    Q_EMIT CurrendModelChanged();


    return 1;
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
    scene->RemoveModel(id);
    scene->Update();

//    std::cout << "Delete current model: " << currentItem << std::endl;

    int index = modelTreeWidget->indexOfTopLevelItem(currentItem);
    if (index != -1) { delete modelTreeWidget->takeTopLevelItem(index); }

    //iGame::SceneManager::Instance()->GetCurrentScene()->RemoveCurrentModel();
    //iGame::SceneManager::Instance()->GetCurrentScene()->Update();
    currentItem = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->currentItem());
    if (currentItem) {
        scene->SetCurrentModel(currentItem->getModelId());
        //modelTreeWidget->setCurrentModelItem(dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->currentItem()));
    } else {
        //modelTreeWidget->setCurrentModelItem(nullptr);
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
