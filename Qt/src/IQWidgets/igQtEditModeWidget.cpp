#include "IQWidgets/igQtEditModeWidget.h"
//#include<iGameManager.h>
#include<qvector4d.h>
/**
 * @class   igQtEditModeWidget
 * @brief   igQtEditModeWidget's brief
 */
igQtEditModeWidget::igQtEditModeWidget(QWidget* parent) : QWidget(parent), ui(new Ui::MeshEditMode) {
	ui->setupUi(this);
	this->cutPlane.resize(6);
	connect(ui->ckbCutPlane, SIGNAL(clicked(bool)), this, SLOT(changeCutFlag()));
	QColor tmp(160, 160, 160);
	ui->cutPlaneX->setDrawOutColor(tmp);
	ui->cutPlaneY->setDrawOutColor(tmp);
	ui->cutPlaneZ->setDrawOutColor(tmp);
	connect(ui->cutPlaneX, &igQtDataRangeSlider::DataRangeChanged, this, [&](float _min, float _max) {
		ratioX[0] = _min;
		ratioX[1] = _max;
		if (this->cutFlag) {
			drawModelWithCutPlane();
		}
		});
	connect(ui->cutPlaneY, &igQtDataRangeSlider::DataRangeChanged, this, [&](float _min, float _max) {
		ratioY[0] = _min;
		ratioY[1] = _max;
		if (this->cutFlag) {
			drawModelWithCutPlane();
		}
		});
	connect(ui->cutPlaneZ, &igQtDataRangeSlider::DataRangeChanged, this, [&](float _min, float _max) {
		ratioZ[0] = _min;
		ratioZ[1] = _max;
		if (this->cutFlag) {
			drawModelWithCutPlane();
		}
		});
	connect(ui->rtnModelView, &QRadioButton::clicked, this, [&]() {
		Q_EMIT ChangeEditModeToModelView();
		});
	connect(ui->rtnPickItem, &QRadioButton::clicked, this, [&]() {
		Q_EMIT ChangeEditModeToPickItem();
		});
}


void igQtEditModeWidget::changeCutFlag()
{
	this->cutFlag = !cutFlag;
	Q_EMIT ChangeCutFlag(this->cutFlag);
	if (this->cutFlag) {
		drawModelWithCutPlane();
	}
}

void igQtEditModeWidget::drawModelWithCutPlane()
{
	//auto manager = iGame::iGameManager::Instance();
	//auto model = manager->GetCurrentModel();
	//if (model == nullptr)return;
	//float minX = manager->bound[0];
	//float maxX = manager->bound[1];
	//float minY = manager->bound[2];
	//float maxY = manager->bound[3];
	//float minZ = manager->bound[4];
	//float maxZ = manager->bound[5];
	//float draw_min_x = ratioX[0] * (maxX - minX) + minX;
	//float draw_min_y = ratioY[0] * (maxY - minY) + minY;
	//float draw_min_z = ratioZ[0] * (maxZ - minZ) + minZ;
	//float draw_max_x = ratioX[1] * (maxX - minX) + minX;
	//float draw_max_y = ratioY[1] * (maxY - minY) + minY;
	//float draw_max_z = ratioZ[1] * (maxZ - minZ) + minZ;
	//cutPlane[0][0] = 1.0;
	//cutPlane[0][3] = -draw_min_x;
	//cutPlane[1][1] = 1.0;
	//cutPlane[1][3] = -draw_min_y;
	//cutPlane[2][2] = 1.0;
	//cutPlane[2][3] = -draw_min_z;

	//cutPlane[3][0] = -1.0;
	//cutPlane[3][3] = draw_max_x;
	//cutPlane[4][1] = -1.0;
	//cutPlane[4][3] = draw_max_y;
	//cutPlane[5][2] = -1.0;
	//cutPlane[5][3] = draw_max_z;


	//Q_EMIT UpdateCurrentModelCutPlane(cutPlane);
}
