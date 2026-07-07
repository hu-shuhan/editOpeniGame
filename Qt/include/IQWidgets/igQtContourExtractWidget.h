/**
 * @class   igQtContourExtractWidget
 * @brief   igQtContourExtractWidget's brief
 */

#pragma once
#include "Clip/iGameModelClip.h"
#include "Contour/iGameContourFilter.h"
#include "iGameUnstructuredMesh.h"

#include <ui_ContourExtract.h>
class igQtContourExtractWidget : public QWidget {

    Q_OBJECT

public:
    igQtContourExtractWidget(QWidget* parent = nullptr);


public slots:

    //Widget 输入
    void InitScalarName();

    void UpdateScalarName();

    void UpdateScalarDimension();

    void UpdateIsoValue();

    void ContourExtract();

    void SetOriginDataObject(iGame::DataObject::Pointer m_d);



signals:
    void DrawContourModel(iGame::DataObject::Pointer);
    void UpdateContourModel(iGame::DataObject::Pointer);

protected:
private:
    Ui::ContourExtract* ui;

    iGame::DataObject::Pointer m_OriginDataObject{ nullptr };
    iGame::UnstructuredMesh::Pointer m_ResultMesh{ nullptr };
    iGame::ContourFilter::Pointer m_Extracter{ nullptr };
    iGame::ElementArray<iGame::AttributeSet::Attribute>::Pointer m_PointData=nullptr;
    iGame::ArrayObject::Pointer m_ScalarArray=nullptr;
    int m_ScalarDimension=0;
    std::string m_ScalarName="";
    bool m_Generated = false;
    double m_IsoValue=0.0;
    unsigned long m_ResultObserverTag{0};
    
};
