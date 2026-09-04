/**
 * @class   igQtContourExtractWidget
 * @brief   igQtContourExtractWidget's brief
 */

#pragma once
#include "Clip/iGameModelClip.h"
#include "Contour/iGameContourFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <vector>

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

    // 等值数值列表操作
    void AddIsoValueFromInput();
    void RemoveSelectedIsoValues();
    void ClearIsoValues();
    void GenerateIsoValuesByRange();

private:
    // 等值数值的展示文案：单个直接显示，多个显示个数与列表
    QString isoValueText() const;
    // 把 m_IsoValues 同步到列表控件
    void SyncIsoValueList();
    // 往 m_IsoValues 里追加若干数值并去重排序
    void AppendIsoValues(const std::vector<double>& values);
    // 取当前选中标量分量的数据范围，失败返回 false
    bool CurrentScalarRange(double& lo, double& hi) const;

public slots:

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
    int m_ScalarDimension=-1;
    std::string m_ScalarName="";
    bool m_Generated = false;
    double m_IsoValue=0.0;
    // 支持一次填写多个等值数值（逗号 / 空格 / 分号分隔），结果合并到同一个输出网格
    std::vector<double> m_IsoValues;
    
};
