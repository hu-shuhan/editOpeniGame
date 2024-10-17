#pragma once
#include<ui_igVector.h>
//#include <iGameManager.h>
#include<iostream>
#include <VectorView/iGameVectorBase.h>
#include <iGameUnstructuredMesh.h>
#include "iGameSceneManager.h"
class igQtVectorWidget : public QWidget {

    Q_OBJECT

public:
    igQtVectorWidget(QWidget* parent = nullptr);
public slots:
    void changeHRProportion();
    void changeHLProportion();
    void changeTRProportion();
    void changeTLProportion();
    void changeHR();
    void changeHL();
    void changeTR();
    void changeTL();
    void updateVectorNameList();
    void changeVecName();
    void drawV();
signals:
    void DrawDireVector(iGame::DataObject::Pointer);
    void UpdateDireVector(iGame::DataObject::Pointer);

private:
    Ui::igVector* ui;
    float headRadiusP;
    float headLengthP;
    float tailRadiusP;
    float tailLengthP;
    float headRadius;
    float headLength;
    float tailRadius;
    float tailLength;
    std::string vecName;
    std::string masterName;
    bool isInit = false;
    bool isDraw = false;
    bool haveChange = false;
    iGame::iGameVectorBase::Pointer m_VectorBase{nullptr};
    iGame::Model::Pointer m_Model{nullptr};
    
};