#pragma once
#include<ui_igStreamTracer.h>
//#include <iGameManager.h>
#include <iostream>
#include <StreamView/iGameStreamBase.h>
#include <iGameUnstructuredMesh.h>
#include<iGameStructuredMesh.h>
#include<iGamePointFinder.h>
#include<windows.h>
#include<atlstr.h>
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
    iGameStreamBase* m_StreamBase{ nullptr };
};
