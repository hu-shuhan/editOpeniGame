/**
 * @class   igQtTensorWidget
 * @brief   igQtTensorWidget's brief
 */

#pragma once
#include <ui_TensorView.h>
#include "TensorViewFilter/iGameTensorBase.h"
#include <IQCore/igQtExportModule.h>

using namespace iGame;
class IG_QT_MODULE_EXPORT igQtTensorWidget : public QWidget {

	Q_OBJECT

public:
	igQtTensorWidget(QWidget* parent = nullptr);
	~igQtTensorWidget() override;
public slots:

	void InitTensorAttributes();
	void ShowTensorField();
	void UpdateGlyphType();
	void UpdateGlyphScale(double s);
	void UpdateTensorsNameList();
	void UpdateScalarsNameList();
	void UpdateGlyphColors();
	void UpdateComponentsShow(bool);
	void GenerateVectorField();

signals:
	//void DrawEllipsoidGlyph(iGame::iGameFloatArray*, iGame::iGameIntArray*);
	void DrawTensorGlyphs(DataObject::Pointer);
	void UpdateTensorGlyphs(DataObject::Pointer);
	void UpdateAttributes(DataObject::Pointer);
private:
	Ui::TensorView* ui;
	iGameTensorBase* m_Manager;
	DataObject* m_DataObject;
	bool m_Generated = false;
	//iGame::iGamePoints* Points;
	//iGame::iGameFloatArray* TensorAttributes;
	//iGame::iGameTensorRepresentation* tensorManager;
	//iGame::iGameModelColorManager* tensorColorManager;
	//iGame::iGameFloatArray* EllipsoidGlyphPoints;
	//iGame::iGameIntArray* EllipsoidGlyphPointOrders;
	//iGame::iGameFloatArray* EllipsoidGlyphColors;

};
