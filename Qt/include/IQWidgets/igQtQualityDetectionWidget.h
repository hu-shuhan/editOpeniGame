/**
 * @class   igQtQualityDetectionWidget
 * @brief   igQtQualityDetectionWidget's brief
 */

#pragma once
#include <ui_QualityDetection.h>
//namespace iGame {
//	class iGameFloatArray;
//	class iGameTriMesh;
//	class iGameQuadMesh;
//	class iGameTetMesh;
//	class iGameHexMesh;
//	class iGameManager;
//}

class igQtQualityDetectionWidget : public QWidget {

	Q_OBJECT

public:
	igQtQualityDetectionWidget(QWidget* parent = nullptr);
	~igQtQualityDetectionWidget();
	// 网格质量检测类型
	enum MeshMetric {
		OFF,
		TRI_Area, TRI_MaxAngle, TRI_MinAngle, TRI_Jacobian, TRI_AspectRatio, TRI_EdgeRatio,
		QUAD_Area, QUAD_MaxAngle, QUAD_MinAngle, QUAD_Jacobian, QUAD_AspectRatio, QUAD_EdgeRatio, QUAD_Warpage, QUAD_Taper, QUAD_Skew,
		TET_Volume, TET_AspectRatio, TET_Jacobian, TET_CollapseRatio, TET_Skew, TET_InternalAngle, TET_VolAspectRatio, TET_EquiangleSkew,
		HEX_Volume, HEX_Taper, HEX_Jacobian, HEX_EdgeRatio, HEX_MaxEdgeRatio, HEX_Skew, HEX_Strech, HEX_Diagonal, HEX_RelativeSizeSquared
	};
	MeshMetric meshMetric = OFF;




	void loadTriMeshMetricData();
	void loadtTetMeshMetricData();
	void loadQuadMeshMetricData();
	void loadHexMeshMetricData();
	std::string loadMetricInfo(std::string);
	void initColorData();
	void updateMetricDataToDraw();
	bool changeMetricValue(float& value);
	//void LoadMeshInfoWithCellColor(iGame::iGameTriMesh* mesh, iGame::iGameFloatArray* ColorData);
	//void LoadMeshInfoWithCellColor(iGame::iGameQuadMesh* mesh, iGame::iGameFloatArray* ColorData);
	//void LoadMeshInfoWithCellColor(iGame::iGameTetMesh* mesh, iGame::iGameFloatArray* ColorData);
	//void LoadMeshInfoWithCellColor(iGame::iGameHexMesh* mesh, iGame::iGameFloatArray* ColorData);
	void clearData();
public slots:
	void showTriMeshQualityView();
	void showQuadMeshQualityView();
	void showTetMeshQualityView();
	void showHexMeshQualityView();
signals:
	void updateCurrentModelColor();

private:
	Ui::QualityDetection* ui;
	//iGame::iGameManager* manager;
	//iGame::iGameFloatArray* metricData{ nullptr };
	//iGame::iGameFloatArray* ColorData{ nullptr };
	//iGame::iGameFloatArray* tmpPoints{ nullptr };
	//iGame::iGameFloatArray* tmpColors{ nullptr };
	bool reScale = false;
};
