#include <iGameSceneManager.h>
#include "Sources/iGameLineTypePointsSource.h"
#include <Plugin/qtpropertybrowser/qtpropertymanager.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <qdebug.h>
#include <QQueue>
#include <qmenu.h>
#include <qaction.h>
#include <QSplitter>

igQtModelDialogWidget::igQtModelDialogWidget(QWidget* parent)
	: QDockWidget(parent),
	ui(new Ui::LayerDialog)
{
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
	modelTreeWidget->setColumnWidth(1, 150);
    modelTreeWidget->setIndentation(15);


	propertyWidget->setHeaderVisible(false);
	propertyManager = new QtVariantPropertyManager(propertyWidget);
	editFactory = new QtVariantEditorFactory(propertyWidget);
	propertyWidget->setFactoryForManager(propertyManager, editFactory);

	propertyWidget->removeProperty(objectGroup);
    objectGroup = propertyManager->addProperty(
            QtVariantPropertyManager::groupTypeId(),
            QStringLiteral("Object propertys"));
    propertyWidget->addProperty(objectGroup);

    
    prop_PointSize = propertyManager->addProperty(QVariant::Int, "Point Size");
    prop_PointSize->setEnabled(false);
    prop_PointSize->setValue(0);
    objectGroup->addSubProperty(prop_PointSize);
    propertyManager->setAttribute(prop_PointSize, "minimum", 1);
    propertyManager->setAttribute(prop_PointSize, "maximum", 99);
    propertyManager->setAttribute(prop_PointSize, "singleStep", 1);

    prop_Transparency =
            propertyManager->addProperty(QVariant::Double, "Transparency");
    prop_Transparency->setEnabled(false);
    prop_Transparency->setValue(0);
    objectGroup->addSubProperty(prop_Transparency);
    propertyManager->setAttribute(prop_Transparency, "minimum", 0.0);
    propertyManager->setAttribute(prop_Transparency, "maximum", 1.0);
    propertyManager->setAttribute(prop_Transparency, "singleStep", 0.1);

	connect(propertyManager, &QtVariantPropertyManager::valueChanged, this,
            &igQtModelDialogWidget::onPropertyChanged);

	ui->ModelInformationWidget->hide();
	//connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this, &igQtModelDialogWidget::UpdateCurrentModel);
	connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this, &igQtModelDialogWidget::updateCurrentModelProperty);
	connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this, &igQtModelDialogWidget::updateCurrentModelInfo);
	//connect(modelTreeWidget, &igQtModelTreeWidget::ChangeCurrentModel, this, &igQtModelDialogWidget::updateCloudPicture);
	connect(modelTreeWidget, &igQtModelTreeWidget::ViewCloudPicture, this, &igQtModelDialogWidget::updateCloudPicture);

}

void igQtModelDialogWidget::UpdateCurrentModel(Model::Pointer model) {
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

ModelTreeWidgetItem* igQtModelDialogWidget::getItemFromObject(DataObject::Pointer obj)
{
	// 遍历子项
	for (int i = 0; i < modelTreeWidget->topLevelItemCount(); ++i) {
		ModelTreeWidgetItem* item = dynamic_cast<ModelTreeWidgetItem*>(modelTreeWidget->topLevelItem(i));
		if (item->getModel()->GetDataObject() == obj) {
			return item;
		}
	}
	return nullptr;
}
void igQtModelDialogWidget::updateItemName(DataObject::Pointer obj) {
    auto item = getItemFromObject(obj);
    if (!item) return;
    item->setName(QString::fromStdString(obj->GetName()));
    return;

}
void igQtModelDialogWidget::updateAllAttriubute(DataObject::Pointer obj)
{
	auto item = getItemFromObject(obj);
	if (!item)return;
	while (item->childCount() > 0) {
		delete item->takeChild(0);
	}
	auto attrSet = obj->GetAttributeSet()->GetAllAttributes();
	for (int i = 0; i < attrSet->GetNumberOfElements(); i++) {
		auto& attr = attrSet->GetElement(i);
		if (attr.isDeleted) continue;
        AttribTreeWidgetItem* child =
                new AttribTreeWidgetItem(i, modelTreeWidget, item);
        child->setText(0, QString::fromStdString(attr.pointer->GetName()));
        child->setIcon(0, QIcon(":/Ticon/Icons/select/file.png"));
        child->setDimension(attr.pointer->GetDimension());
	}
}
int igQtModelDialogWidget::addDataObjectToModelTree(DataObject::Pointer obj, ItemSource source) {
	ModelTreeWidgetItem* item = new ModelTreeWidgetItem(modelTreeWidget);
    modelTreeWidget->setCurrentModelItem(item);
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
	auto model = scene->CreateModel(obj);
	int id = scene->AddModel(model);
    currentModel = model;

	item->setName(QString::fromStdString(obj->GetName()));
	item->setModel(model);

	//QTreeWidgetItem* attributes = new QTreeWidgetItem(item);
 //   attributes->setText(0, QString::fromStdString("属性"));
 //   attributes->setIcon(0, QIcon(":/Ticon/Icons/select/selected.png"));
 //   

 //   QTreeWidgetItem* liuchang = new QTreeWidgetItem(item);
 //   liuchang->setText(0, QString::fromStdString("流场"));
 //   liuchang->setIcon(0, QIcon(":/Ticon/Icons/select/selected.png"));

 //   item->addChild(attributes);
 //   item->addChild(liuchang);
    
	auto attrSet = obj->GetAttributeSet()->GetAllAttributes();
	for (int i = 0; i < attrSet->GetNumberOfElements(); i++) {
		auto& attr = attrSet->GetElement(i);
		if (attr.isDeleted) continue;
        AttribTreeWidgetItem* child =
                new AttribTreeWidgetItem(i, modelTreeWidget, item);
		child->setText(0, QString::fromStdString(attr.pointer->GetName()));
		child->setIcon(0, QIcon(":/Ticon/Icons/select/file.png"));
        child->setDimension(attr.pointer->GetDimension());
	}


	modelTreeWidget->addTopLevelItem(item);
	modelTreeWidget->setCurrentItem(item);
	updateCurrentModelInfo();
    updateCurrentModelProperty(model.get());

	return id;
}

int igQtModelDialogWidget::addModelToModelTree(Model::Pointer model) {
	ModelTreeWidgetItem* item = new ModelTreeWidgetItem(modelTreeWidget);
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
	int id = scene->AddModel(model);

	item->setName(QString::fromStdString(model->GetDataObject()->GetName()));
	item->setModel(model);

	modelTreeWidget->addTopLevelItem(item);
	modelTreeWidget->setCurrentItem(item);
	return id;
}
int igQtModelDialogWidget::updateCurrentModelInfo()
{
//    qDebug() << ui->modelTreeWidget->currentIndex();
	ui->ModelInformationWidget->updateInformationFrame();
	Q_EMIT CurrendModelChanged();
	return 1;
}
void igQtModelDialogWidget::updateCurrentModelProperty(iGame::Model* model) { 
	currentModel = model;
	auto obj = DynamicCast<DrawObject>(model->GetDataObject());
	if (obj) {
        prop_PointSize->setEnabled(true);
        prop_PointSize->setValue(obj->GetPointSize());
        prop_Transparency->setEnabled(true);
        prop_Transparency->setValue(obj->GetTransparency());
    } else {
        prop_PointSize->setEnabled(false);
        prop_PointSize->setValue(0);
        prop_Transparency->setEnabled(false);
        prop_Transparency->setValue(0);
	}
}
int igQtModelDialogWidget::updateCloudPicture() {
 
    Q_EMIT CloudPictureChanged();
    return 1;
}
void igQtModelDialogWidget::deleteCurrentModel() {

    // 获取当前选中的QTreeWidgetItem
    QTreeWidgetItem* currentItem = modelTreeWidget->setCurrentModelItem();
    if(currentItem == nullptr) return;
    int index = modelTreeWidget->indexOfTopLevelItem(currentItem);
    if (index != -1) {
        delete modelTreeWidget->takeTopLevelItem(index);
    }

    iGame::SceneManager::Instance()->GetCurrentScene()->RemoveCurrentModel();
    iGame::SceneManager::Instance()->GetCurrentScene()->Update();

	modelTreeWidget->setCurrentModelItem(nullptr);
}

void igQtModelDialogWidget::onPropertyChanged(QtProperty* property,
                                              const QVariant& value) {
    if (property == prop_PointSize) { 
		//std::cout << value.toInt() << std::endl;
		if (currentModel) {
            auto obj = DynamicCast<DrawObject>(currentModel->GetDataObject());
            if (obj && obj->GetPointSize() != value.toInt() && value.toInt() > 0) {
                obj->SetPointSize(value.toInt());
                Update();
			}
		}
    } else if (property == prop_Transparency) {
        //std::cout << value.toDouble() << std::endl;
        if (currentModel) {
            auto obj = DynamicCast<DrawObject>(currentModel->GetDataObject());
            if (obj && obj->GetTransparency() != value.toDouble() &&
                value.toDouble() >= 0 && value.toDouble() <= 1.0) {
                obj->SetTransparency(value.toDouble());
                Update();
            }
        }
	}
}
