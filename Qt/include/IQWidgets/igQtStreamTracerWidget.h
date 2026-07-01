#pragma once
#include <ui_igStreamTracer.h>
//#include <iGameManager.h>
#include <QHideEvent>
#include <StreamView/iGameStreamBase.h>
#include <iGamePointFinder.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iostream>
#include <QMessageBox>
#include<QDockWidget>
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
    void changeWidthOfStreamLine();
    void changelengthOfStep();
    void changemaxSteps();
    void changeStart();
    void changeEnd();
    void changeSplit();
    void changeterminalSpeed();
    void Pressed();
    void Released();
    void changeVecName();
    void refresh();
    void updateVectorNameList();
    void Simplifier();
    //void changeOffsetP1();
    //void changeOffsetP2();
signals:
    void AddStreamObject(iGame::DataObject::Pointer);
    void UpdateStreamObject(iGame::DataObject::Pointer);
    void SetUseBox(iGame::Model::Pointer);
    void SetSelectItemShow(bool show);


private:
    Ui::SteamLineTracer* ui;
    iGame::SmartPointer<iGame::StreamLineSelection> Selection;
    iGame::SmartPointer<iGame::Painter3D> Painter;
    bool isExisted = false;
    bool modelBound = false;
    int numOfSeeds;
    iGame::Vector3f startP;
    iGame::Vector3f endP;
    int splitX;
    int splitY;
    int splitZ;
    int control;
    float proportion;
    float lengthOfStreamLine;
    int widthOfStreamLine;
    float lengthOfStep;
    float maxSteps;
    float terminalSpeed;
    bool haveDraw = false;
    bool haveClicked = false ;
    std::string masterName;
    std::string vectorName;
    iGame::UnstructuredMesh::Pointer streamlineResult{};
    std::vector<iGame::PointFinder::Pointer> ptFinder;
    iGame::StreamBase* m_StreamBase{nullptr};
    iGame::UnstructuredMesh::Pointer m_ResultObject {nullptr};
    iGame::DataObject::Pointer m_DataObject;
    iGame::Vector3f offsetP1{0, 0, 0};
    iGame::Vector3f offsetP2{0, 0, 0};
    std::vector<iGame::Vector3f> seedPoints{
            {-0.440697, -0.291987, -0.1272},
            {-0.38931, 0.339697, 0.223517},
            {-0.603765, -0.340187, -0.1272},
            {-0.516121, 0.312594, 0.223517},
            {-0.320113, -0.348173, -0.1272},
            {-0.360381, 0.273479, 0.223517},
            {},
            {},
    };
    bool first = true;
    int m_StreamlineCounter = 0; // 用于给每次生成的流线对象编号
    int p1;
    int p2;
    
    iGame::UnstructuredMesh::Pointer m_OriginalStream;

    void ensureStreamBase();
    // 选取合适的源 Model：跳过流线结果对象、必须是含体单元的 VolumeMesh
    iGame::Model::Pointer pickSourceModel();
};
