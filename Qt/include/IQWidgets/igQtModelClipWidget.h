/**
 * @class   igQtModelClipWidget
 * @brief   igQtModelClipWidget's brief
 */

#pragma once
#include "Clip/iGameModelClip.h"
#include "Core/Interactor/iGameSlicingStyle.h"
#include "iGameSurfaceMesh.h"

#include <ui_Slice.h>
class igQtModelClipWidget : public QWidget {

    Q_OBJECT

public:
    igQtModelClipWidget(QWidget* parent = nullptr);


public slots:

    //交互传过来
    void SetPlane(float o[3], float normal[3]);
    //Widget 输入
    void UpdatePlane();

    void ClipModel();

    void SetIsSlice(bool s);

    void SetOriginDataObject(iGame::DataObject::Pointer m_d);

    void FilterSignal(iGame::InteractorStyle::Signal signal, void* callData) {
        switch (signal) {
            case iGame::InteractorStyle::Signal::Slicing: {
                iGame::SlicingStyle::SlicingPlane* plane =
                        reinterpret_cast<iGame::SlicingStyle::SlicingPlane*>(
                                callData);
                if (plane) { this->SetPlane(plane->point, plane->normal); }
                break;
            }
            default:
                break;
        }
    }

signals:
    void DrawClipModel(iGame::SurfaceMesh::Pointer);
    void UpdateClipModel(iGame::SurfaceMesh::Pointer);

protected:
private:
    Ui::Form* ui;

	iGame::DataObject::Pointer m_OriginDataObject{nullptr};
    iGame::SurfaceMesh::Pointer m_ResultMesh{nullptr};
    iGame::ModelClip::Pointer m_Clipper{nullptr};
	bool m_Generated=false;
    iGame::DrawObject::Pointer m_tmp{nullptr};
};
