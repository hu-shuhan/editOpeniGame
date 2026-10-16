/**
 * @class   igQtMedianFilterWidget
 * @brief   igQtMedianFilterWidget's brief
 */

#pragma once
#include "Median/iGameMedianFilter.h"
#include "iGameStructuredMesh.h"

#include <ui_MedianFilter.h>

class igQtMedianFilterWidget : public QWidget {

    Q_OBJECT

public:
    igQtMedianFilterWidget(QWidget* parent = nullptr);

public slots:
    void InitScalarList();
    void UpdateScalarType();
    void MedianFilterExecute();
    void SetOriginDataObject(iGame::DataObject::Pointer m_d);

signals:
    void DrawMedianModel(iGame::DataObject::Pointer);
    void UpdateMedianModel(iGame::DataObject::Pointer);

private:
    Ui::MedianFilter* ui;

    iGame::DataObject::Pointer m_OriginDataObject{nullptr};
    iGame::StructuredMesh::Pointer m_ResultMesh{nullptr};
    iGame::MedianFilter::Pointer m_MedianFilter{nullptr};
    iGame::ArrayObject::Pointer m_ScalarArray = nullptr;
    int m_ScalarIndex = -1;
    bool m_Generated = false;
};
