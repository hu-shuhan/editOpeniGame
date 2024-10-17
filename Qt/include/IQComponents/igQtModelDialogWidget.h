#pragma once

#include <iGameSceneManager.h>

#include <IQComponents/igQtModelTreeWidget.h>
#include <IQComponents/igQtPropertyTreeWidget.h>
#include <IQCore/igQtExportModule.h>

#include <ui_layerDialog.h>

#include <QString>
#include <QMouseEvent>
#include <QDockWidget>
#include <QTreeWidget>
#include <iostream>
#include <Plugin/qtpropertybrowser/qteditorfactory.h>
#include <Plugin/qtpropertybrowser/qttreepropertybrowser.h>
#include <Plugin/qtpropertybrowser/qtvariantproperty.h>

using namespace iGame;

class IG_QT_MODULE_EXPORT igQtModelDialogWidget : public QDockWidget {
	Q_OBJECT
public:
	igQtModelDialogWidget(QWidget* parent);
	~igQtModelDialogWidget() override = default;

public slots:
	int addModelToModelTree(Model::Pointer model);
	ModelTreeWidgetItem* getItemFromObject(DataObject::Pointer obj);
	void updateAllAttriubute(DataObject::Pointer obj);
    void updateItemName(DataObject::Pointer obj);
	int addDataObjectToModelTree(DataObject::Pointer obj, ItemSource source);
	int updateCurrentModelInfo();
    int updateCloudPicture();
    void deleteCurrentModel();


signals:
	void CurrendModelChanged();
    void CloudPictureChanged();
protected:
	void UpdateCurrentModel(Model::Pointer model);

private:
    Model* currentModel;

	igQtModelTreeWidget* modelTreeWidget;
    QtTreePropertyBrowser* propertyWidget;

	QtVariantPropertyManager* propertyManager;
	QtVariantEditorFactory* editFactory;
	QtProperty* objectGroup;
	Ui::LayerDialog* ui;
};

