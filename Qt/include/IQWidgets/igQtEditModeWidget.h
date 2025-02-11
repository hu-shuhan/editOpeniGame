/**
 * @class   igQtEditModeWidget
 * @brief   igQtEditModeWidget's brief
 */

#pragma once
#include <ui_MeshEditMode.h>


class igQtEditModeWidget : public QWidget {

	Q_OBJECT

public:
	igQtEditModeWidget(QWidget* parent = nullptr);

	void drawModelWithCutPlane();

public slots:
	void changeCutFlag();


signals:
	void UpdateCurrentModelCutPlane(QVector<QVector4D>&);
	void ChangeCutFlag(bool);
	void ChangeEditModeToModelView();
	void ChangeEditModeToPickItem();
private:
	Ui::MeshEditMode* ui;
	bool cutFlag = false;
	float ratioX[2] = { 0.0,1.0 };
	float ratioY[2] = { 0.0,1.0 };
	float ratioZ[2] = { 0.0,1.0 };
	QVector<QVector4D>cutPlane;

};
