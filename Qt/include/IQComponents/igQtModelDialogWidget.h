#pragma once

#include <iGameSceneManager.h>

#include <IQComponents/igQtModelTreeWidget.h>
#include <IQComponents/igQtPropertyTreeWidget.h>
#include <IQCore/igQtExportModule.h>

#include <ui_layerDialog.h>

#include <Plugin/qtpropertybrowser/qteditorfactory.h>
#include <Plugin/qtpropertybrowser/qttreepropertybrowser.h>
#include <Plugin/qtpropertybrowser/qtvariantproperty.h>
#include <QDockWidget>
#include <QMouseEvent>
#include <QString>
#include <QTreeWidget>
#include <iostream>


class IG_QT_MODULE_EXPORT igQtModelDialogWidget : public QDockWidget {
    Q_OBJECT
public:
    igQtModelDialogWidget(QWidget* parent);
    ~igQtModelDialogWidget() override = default;

public slots:
    int addModelToModelTree(iGame::Model::Pointer model);
    ModelTreeWidgetItem* getItemFromObject(iGame::DataObject::Pointer obj);
    void updateAllAttriubute(iGame::DataObject::Pointer obj);
    void updateItemName(iGame::DataObject::Pointer obj);
    int addDataObjectToModelTree(iGame::DataObject::Pointer obj, ItemSource source);
    int updateCurrentModelInfo();
    void updateCurrentModelProperty(iGame::Model* model);
    int updateCloudPicture();
    void deleteCurrentModel();
    void onPropertyChanged(QtProperty* property, const QVariant& value);
    iGame::Model* GetCurrentModel();

signals:
    void CurrendModelChanged();
    void CloudPictureChanged();
    void Update();

private:
    //iGame::Model* currentModel;

    igQtModelTreeWidget* modelTreeWidget;
    QtTreePropertyBrowser* propertyWidget;

    QtVariantPropertyManager* propertyManager;
    QtVariantEditorFactory* editFactory;

    QtProperty* objectGroup;
    QtVariantProperty* prop_PointSize;
    QtVariantProperty* pror_LineWidth;
    QtVariantProperty* prop_Transparency;

    Ui::LayerDialog* ui;
    static bool m_AutoAccelerate;
};
