/**
 * @class   igQtModelClipWidget
 * @brief   igQtModelClipWidget's brief
 */

#pragma once
#include "Clip/iGameModelClip.h"
#include "Clip/iGameQuickModelClip.h"
#include "Contour/iGameContourFilter.h"
#include "Core/Interactor/iGameSlicingStyle.h"
#include "iGameSurfaceMesh.h"

#include <ui_ModelClip.h>
class igQtModelClipWidget : public QWidget {

    Q_OBJECT

public:
    igQtModelClipWidget(QWidget* parent = nullptr);

    enum ViewMode {
        IG_CLIP_MODE,
        IG_CONTOUR_MODE,
        IG_MESH_MODE,
        IG_VIEW_MODE_NUM
    };
    void SetViewMode(ViewMode);
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
    void DrawClipModel(iGame::DrawObject::Pointer);
    void UpdateClipModel(iGame::DrawObject::Pointer);
    void ResetInteractor();
protected:
private:
    Ui::ModelClipWidget* ui;

    double m_Normal[3]={1,0,0};
    double m_Origin[3]={0,0,0};
	iGame::DataObject::Pointer m_OriginDataObject{nullptr};
    iGame::UnstructuredMesh::Pointer m_ResultMesh{nullptr};
    ViewMode m_ViewMode{IG_CLIP_MODE};
};
