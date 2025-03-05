#pragma once
#include <ui_igStreamTracer.h>
//#include <iGameManager.h>
#include <StreamView/iGameStreamBase.h>
#include <iGamePointFinder.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iostream>
class igQtStreamTracerWidget : public QWidget {

    Q_OBJECT

public:
    igQtStreamTracerWidget(QWidget* parent = nullptr);
public slots:
    void generateStreamline();
    void changeControl();
    void changeProportion();
    void reduceProportion();
    void increaseProportion();
    void changenumOfSeeds();
    void changelengthOfStreamLine();
    void changelengthOfStep();
    void changemaxSteps();
    void changeterminalSpeed();
    void Pressed();
    void Released();
    void changeVecName();
    void updateVectorNameList();
    void changeOffsetP1();
    void changeOffsetP2();
signals:
    void AddStreamObject(DataObject::Pointer);
    void UpdateStreamObject(DataObject::Pointer);

private:
    Ui::SteamLineTracer* ui;
    int numOfSeeds;
    int control;
    float proportion;
    float lengthOfStreamLine;
    float lengthOfStep;
    float maxSteps;
    float terminalSpeed;
    bool haveDraw;
    bool haveClicked;
    std::string masterName;
    std::string vectorName;
    UnstructuredMesh::Pointer streamlineResult{};
    std::vector<PointFinder::Pointer> ptFinder;
    iGameStreamBase* m_StreamBase{nullptr};
    Vector3f offsetP1{0, 0, 0};
    Vector3f offsetP2{0, 0, 0};
    int p1;
    int p2;
};
