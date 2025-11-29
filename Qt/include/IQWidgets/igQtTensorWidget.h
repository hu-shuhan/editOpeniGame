/**
 * @class   igQtTensorWidget
 * @brief   igQtTensorWidget's brief
 */

#pragma once
#include "TensorView/iGameTensorBase.h"
#include <IQCore/igQtExportModule.h>
#include <ui_TensorView.h>

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
	void UpdateInfoCombox();
	void InitTensorWidget();
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
	iGameTensorBase* m_Manager=nullptr;
	DataObject* m_DataObject=nullptr;
	bool m_Generated = false;
	//iGame::iGamePoints* Points;
	//iGame::iGameFloatArray* TensorAttributes;
	//iGame::iGameTensorRepresentation* tensorManager;
	//iGame::iGameModelColorManager* tensorColorManager;
	//iGame::iGameFloatArray* EllipsoidGlyphPoints;
	//iGame::iGameIntArray* EllipsoidGlyphPointOrders;
	//iGame::iGameFloatArray* EllipsoidGlyphColors;

};
