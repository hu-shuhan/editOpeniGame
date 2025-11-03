#pragma once
#include <ui_igStreamTracer.h>
//#include <iGameManager.h>
#include <StreamView/iGameStreamBase.h>
#include <iGamePointFinder.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iostream>
#include <QHideEvent>
class igQtStreamTracerWidget : public QWidget {

    Q_OBJECT

public:
    igQtStreamTracerWidget(QWidget* parent = nullptr);

protected:
    void hideEvent(QHideEvent* event);
    void showEvent(QShowEvent* event);

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
    void changeStart();
    void changeEnd();
    void changeterminalSpeed();
    void Pressed();
    void Released();
    void changeVecName();
    void updateVectorNameList();
    //void changeOffsetP1();
    //void changeOffsetP2();
signals:
    void AddStreamObject(DataObject::Pointer);
    void UpdateStreamObject(DataObject::Pointer);

private:
    Ui::SteamLineTracer* ui;
    SmartPointer<StreamLineSelection> Selection;
    SmartPointer<Painter3D> Painter;
    bool isExisted = false;
    int numOfSeeds;
    Vector3f startP;
    Vector3f endP;
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
    DataObject::Pointer m_DataObject;
    Vector3f offsetP1{0,0,0};
    Vector3f offsetP2{0,0,0};
    std::vector<Vector3f> seedPoints{
            {-0.440697, -0.291987, -0.1272},
            {-0.38931, 0.339697, 0.223517},
            {-0.603765,-0.340187,-0.1272},
            {-0.516121,0.312594,0.223517},
            {-0.320113,-0.348173,-0.1272},
            {-0.360381,0.273479,0.223517},
            {},
            {},
    };
    bool first = true;
    int p1;
    int p2;
};
