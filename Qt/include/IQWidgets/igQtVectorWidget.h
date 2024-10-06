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
    bool isInit = false;
    iGame::iGameVectorBase* m_VectorBase{nullptr};
    
};