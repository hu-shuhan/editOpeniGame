/**
 * @class   igQtModelClipWidget
 * @brief   igQtModelClipWidget's brief
 */

#pragma once
#include <QDockWidget>
#include "iGameSurfaceMesh.h"
#include "Clip/iGameModelClip.h"
#include <ui_Slice.h>
class igQtModelClipWidget : public QWidget {

	Q_OBJECT

public:
	igQtModelClipWidget(QWidget* parent = nullptr);


public slots:

	//交互传过来
	void SetPlane(float n[3], float o[3]);
	//Widget 输入
	void UpdatePlane();

	void ClipModel();

	void SetIsSlice(bool s);

	void SetOriginDataObject(iGame::DataObject::Pointer m_d);
signals:
	void DrawClipModel(iGame::SurfaceMesh::Pointer);
	void UpdateClipModel(iGame::SurfaceMesh::Pointer);
protected:

private:
	Ui::Form* ui;

	iGame::DataObject::Pointer m_OriginDataObject = { nullptr };
	iGame::SurfaceMesh::Pointer m_ResultMesh = { nullptr };
	iGame::ModelClip::Pointer m_Clipper = { nullptr };

};
