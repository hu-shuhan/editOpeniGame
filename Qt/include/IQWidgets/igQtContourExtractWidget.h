/**
 * @class   igQtContourExtractWidget
 * @brief   igQtContourExtractWidget's brief
 */

#pragma once
#include "Clip/iGameModelClip.h"
#include "Contour/iGameContourFilter.h"
#include "iGameSurfaceMesh.h"
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
    // 轮廓结果必须是 UnstructuredMesh：面网格输入时 ContourFilter 产出的是 IG_LINE（等值线，
    // 每个单元只有 2 个点），体网格输入时才是 IG_TRIANGLE（等值面）。SurfaceMesh 按单元大小
    // 反推面类型，没有"2 个点的面"这一档，SurfaceMesh::GetFace 会落到 Polygon 分支的
    // assert(ncells > 4) 上直接崩溃。UnstructuredMesh 带 cellType，两种输出都能正确表达。
    iGame::UnstructuredMesh::Pointer m_ResultMesh{ nullptr };
    iGame::ContourFilter::Pointer m_Extracter{ nullptr };
    iGame::ElementArray<iGame::AttributeSet::Attribute>::Pointer m_PointData=nullptr;
    iGame::ArrayObject::Pointer m_ScalarArray=nullptr;
    int m_ScalarDimension=-1;
    std::string m_ScalarName="";
    bool m_Generated = false;
    double m_IsoValue=0.0;
    
};
