#include "iGameSelection.h"
#include <IQWidgets/igQtStreamTracerWidget.h>
#include <iGameBoxStyle.h>
#include <iGameSceneManager.h>
#include "StreamView/iGameStreamlineSimplifier.h"

using namespace iGame;
igQtStreamTracerWidget::igQtStreamTracerWidget(QWidget* parent) : QWidget(parent), ui(new Ui::SteamLineTracer) {
    ui->setupUi(this);
    connect(ui->control_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeControl()));
    connect(ui->numOfSeedLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changenumOfSeeds()));
    connect(ui->lengthOfStreamLine, SIGNAL(textChanged(const QString&)), this, SLOT(changelengthOfStreamLine()));
    connect(ui->lineWidth, SIGNAL(textChanged(const QString&)), this, SLOT(changeWidthOfStreamLine()));
    connect(ui->lengthOfStep, SIGNAL(textChanged(const QString&)), this, SLOT(changelengthOfStep()));
    connect(ui->maxSteps, SIGNAL(textChanged(const QString&)), this, SLOT(changemaxSteps()));
    connect(ui->startX, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeStart);
    connect(ui->startY, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeStart);
    connect(ui->startZ, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeStart);
    ui->startX->setText("0");
    ui->startY->setText("0");
    ui->startZ->setText("0");
    connect(ui->endX, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeEnd);
    connect(ui->endY, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeEnd);
    connect(ui->endZ, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeEnd);

    ui->endX->setText("0");
    ui->endY->setText("0");
    ui->endZ->setText("0");

    connect(ui->terminalSpeed, SIGNAL(textChanged(const QString&)), this, SLOT(changeterminalSpeed()));


    connect(ui->comboBox, &QComboBox::currentTextChanged, this, [&]() { this->changeVecName(); });
    
    connect(ui->generate_streamline_btn, &QPushButton::clicked, this, &igQtStreamTracerWidget::generateStreamline);
    connect(ui->refreshBtn, &QPushButton::clicked, this, &igQtStreamTracerWidget::refresh);
    connect(ui->Cluster, &QPushButton::clicked, this, &igQtStreamTracerWidget::Simplifier);

    numOfSeeds = 200;
    ui->numOfSeedLineEdit->setText("200");
    control = 0;
    haveClicked = false;
    //	 proportion = 0.35;
    // ui->proportion_Slider->setValue(35);
    lengthOfStreamLine = 5;
    ui->lengthOfStreamLine->setText("5");
    widthOfStreamLine = 3;
    ui->lineWidth->setText("3");
    maxSteps = 1200;
    ui->maxSteps->setText("1200");
    lengthOfStep = 0.05;
    ui->lengthOfStep->setText("0.05");
    terminalSpeed = 0.005;
    ui->terminalSpeed->setText("0.005");
    haveDraw = false;
    haveClicked = true;
    ui->control_comboBox->setCurrentIndex(1);
    streamlineResult = UnstructuredMesh::New();

}

void igQtStreamTracerWidget::refresh() {
    modelBound = false;
    updateVectorNameList();
}
void igQtStreamTracerWidget::hideEvent(QHideEvent* event) {
    auto scene = SceneManager::Instance()->GetCurrentScene();
    scene->GetInteractor()->RequestBasicStyle();
}
void igQtStreamTracerWidget::showEvent(QShowEvent* event) {
    if (isExisted) {
        auto scene = SceneManager::Instance()->GetCurrentScene();
        Selection->Start = startP;
        Selection->End = endP;
        Selection->SetSelectionCallBackEvent(
                [&](IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope) {
                    if (itemType == IG_CHANGE) {
                        startP = Selection->Start;
                        endP = Selection->End;
                        auto temStart = startP;
                        auto temEnd = endP;
                        ui->startX->setText(QString::number(temStart[0]));
                        ui->startY->setText(QString::number(temStart[1]));
                        ui->startZ->setText(QString::number(temStart[2]));
                        ui->endX->setText(QString::number(temEnd[0]));
                        ui->endY->setText(QString::number(temEnd[1]));
                        ui->endZ->setText(QString::number(temEnd[2]));
                    }
                },
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);

        scene->GetInteractor()->RequestStreamLineStyle(Selection);
    }
    std::cout << first << std::endl;
    if (first) {
        std::cout << "do link" << std::endl;
        m_StreamBase = iGame::StreamBase::New();
        m_StreamBase->DrawObject::AddObserver(iGame::Command::DeleteEvent, [&]() -> void {
            haveDraw = false;
            first = true;
            std::cout << "change first" << first << std::endl;
            this->hide();
        });
        auto sceneManager = iGame::SceneManager::Instance();
        auto scene = sceneManager->GetCurrentScene();
        if (!scene) return;
        auto currentModel = scene->GetCurrentModel();
        if (!currentModel) return;
        auto obj = currentModel->GetDataObject();
        if (!obj) return;
        iGame::VolumeMesh::Pointer mesh;
        if (iGame::DynamicCast<UnstructuredMesh>(obj))
            mesh = iGame::DynamicCast<UnstructuredMesh>(obj)->TransferToVolumeMesh();
        else if (DynamicCast<VolumeMesh>(obj))
            mesh = DynamicCast<VolumeMesh>(obj);
        if (!mesh) return;
        first = false;
    }
    std::cout << "show" << std::endl;
    updateVectorNameList();
}
void igQtStreamTracerWidget::changeControl() {
    control = ui->control_comboBox->currentIndex();
    //std::cout << "current index=" << control <<std::endl;
}
void igQtStreamTracerWidget::changenumOfSeeds() {
    numOfSeeds = ui->numOfSeedLineEdit->text().toInt();
    //std::cout << "current seeds=" << numOfSeeds << std::endl;
}
void igQtStreamTracerWidget::changeStart() {
    startP = Vector3f(ui->startX->text().toFloat(), ui->startY->text().toFloat(), ui->startZ->text().toFloat());
    if (Selection) {
        Selection->Start = startP;
        auto scene = SceneManager::Instance()->GetCurrentScene();
        Painter = scene->GetCurrentModel()->GetPainter3D();
        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);
        scene->GetInteractor()->RequestStreamLineStyle(Selection);
        scene->Update();
    }
    //std::cout << "current seeds=" << numOfSeeds << std::endl;
}
void igQtStreamTracerWidget::changeEnd() {
    endP = Vector3f(ui->endX->text().toFloat(), ui->endY->text().toFloat(), ui->endZ->text().toFloat());
    if (Selection) {
        Selection->End = endP;
        auto scene = SceneManager::Instance()->GetCurrentScene();
        Painter = scene->GetCurrentModel()->GetPainter3D();
        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);
        scene->GetInteractor()->RequestStreamLineStyle(Selection);
        scene->Update();
    }
    //std::cout << "current seeds=" << numOfSeeds << std::endl;
}
void igQtStreamTracerWidget::changelengthOfStreamLine() {
    lengthOfStreamLine = ui->lengthOfStreamLine->text().toFloat();
}
void igQtStreamTracerWidget::changeWidthOfStreamLine() { widthOfStreamLine = ui->lineWidth->text().toFloat(); }
void igQtStreamTracerWidget::changelengthOfStep() { lengthOfStep = ui->lengthOfStep->text().toFloat(); }
void igQtStreamTracerWidget::changemaxSteps() { maxSteps = ui->maxSteps->text().toFloat(); }
void igQtStreamTracerWidget::changeterminalSpeed() { terminalSpeed = ui->terminalSpeed->text().toFloat(); }
void igQtStreamTracerWidget::Pressed() {
    haveClicked = false;
    std::cout << "aaa" << std::endl;
}
void igQtStreamTracerWidget::Released() {
    haveClicked = true;
    generateStreamline();
}
//void igQtStreamTracerWidget::changeOffsetP1() {
//    p1 = ui->lineEdit->text().toInt();
//    p2 = ui->lineEdit_2->text().toInt();
//    offsetP1 = Vector3f(ui->lineEdit->text().toFloat(), ui->lineEdit_2->text().toFloat(), ui->lineEdit_3->text().toFloat());
//}
//void igQtStreamTracerWidget::changeOffsetP2() {
//    offsetP2 = Vector3f(ui->lineEdit_4->text().toFloat(), ui->lineEdit_5->text().toFloat(),
//                        ui->lineEdit_6->text().toFloat());
//}
void igQtStreamTracerWidget::changeProportion() {
    //	proportion = ui->proportion_Slider->value();
    //	proportion /= 100;
    if (haveClicked) generateStreamline();
    //std::cout << "current value=" << proportion << std::endl;
}
void igQtStreamTracerWidget::reduceProportion() {
    //	proportion = ui->proportion_Slider->value();
    if (proportion) {
        proportion = proportion - 1;
        //	ui->proportion_Slider->setValue(proportion);
        //std::cout << "current value=" << proportion << std::endl;
    }
}
void igQtStreamTracerWidget::increaseProportion() {
    //proportion = ui->proportion_Slider->value();
    if (proportion < 99) {
        proportion = proportion + 1;
        //	ui->proportion_Slider->setValue(proportion);
    }
    //std::cout << "current value=" << proportion << std::endl;
}
void igQtStreamTracerWidget::updateVectorNameList() {
    ui->comboBox->clear();
    auto sceneManager = iGame::SceneManager::Instance();
    auto scene = sceneManager->GetCurrentScene();
    if (!scene) return;
    auto currentModel = scene->GetCurrentModel();
    if (!currentModel) return;
    auto obj = currentModel->GetDataObject();
    if (!obj) return;
    //startP = obj->GetBoundingBox().min;
    //endP = obj->GetBoundingBox().max;

    startP = Vector3f(-0.3, -4.4, 0.13);
    endP = Vector3f(-0.3, 4.4, 0.13);

    auto temStart = startP;
    auto temEnd = endP;
    ui->startX->setText(QString::number(temStart[0]));
    ui->startY->setText(QString::number(temStart[1]));
    ui->startZ->setText(QString::number(temStart[2]));
    ui->endX->setText(QString::number(temEnd[0]));
    ui->endY->setText(QString::number(temEnd[1]));
    ui->endZ->setText(QString::number(temEnd[2]));
    iGame::AttributeSet* _AttributeSet;
    if (obj->HasSubDataObject()) {
        auto it = obj->SubDataObjectIteratorBegin();
        // it++;
        _AttributeSet = it->second->GetAttributeSet();
    } else {
        _AttributeSet = obj->GetAttributeSet();
    }
    if (!_AttributeSet) return;
    auto allAttributes = _AttributeSet->GetAllAttributes();
    if (!allAttributes) return;

    for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
        auto attribute = allAttributes->GetElement(i);
        // if (attribute.type == IG_VECTOR&&attribute.attachmentType == IG_POINT) {
        if (attribute.type == IG_VECTOR) {
            if (attribute.pointer) { ui->comboBox->addItem(QString::fromStdString(attribute.pointer->GetName())); }
        }
    }

    iGame::StreamTracer* streamtracer = m_StreamBase->streamFilter;
    if (!modelBound) {
        std::cout << "[StreamTracer] First model binding\n";

        streamtracer->initStreamTracer(currentModel);
        masterName = currentModel->GetDataObject()->GetName();
        ui->source->setText(QString::fromStdString("Source: " + masterName));
        auto tem = currentModel->GetDataObject();
        m_DataObject = tem;

        modelBound = true;
    } else {
    }
}
void igQtStreamTracerWidget::changeVecName() {
    vectorName = ui->comboBox->currentText().toStdString();
    std::cout << "current value=" << vectorName << std::endl;
}
void igQtStreamTracerWidget::generateStreamline() {

    auto scene = SceneManager::Instance()->GetCurrentScene();

    iGame::StreamTracer* streamtracer = m_StreamBase->streamFilter;
    Model::Pointer model = scene->GetCurrentModel();
    if (!modelBound) {
        std::cout << "[StreamTracer] First model binding\n";
        streamtracer->initStreamTracer(model);
        masterName = model->GetDataObject()->GetName();
        ui->source->setText(QString::fromStdString("Source: " + masterName));
        auto tem = model->GetDataObject();
        m_DataObject = tem;
        modelBound = true;
    }
    std::vector<std::vector<int>> seedPids = {{1797284, 3468659},
                                              {536542, 2738820},
                                              {536542, 2658742},
                                              {5485895, 536542}};

    std::vector<std::vector<float>> streamlineColor;
    std::vector<std::vector<float>> streamline;
    iGame::AttributeSet* _AttributeSet;
    auto tem = m_DataObject;
    if (tem->HasSubDataObject()) {
        auto it = tem->SubDataObjectIteratorBegin();
        // it++;
        _AttributeSet = it->second->GetAttributeSet();
    } else {
        _AttributeSet = tem->GetAttributeSet();
    }
    if (!_AttributeSet) return;
    auto allAttributes = _AttributeSet->GetAllAttributes();
    if (!allAttributes) return;

    std::cout << vectorName << std::endl;
    std::vector<Vector3f> seeds;
    if (control == 0) {
        seeds = streamtracer->seedPCoordGenerate(numOfSeeds, startP, endP);
    } else if (control == 1) {
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) {
            Smodel= model;
        }
        Q_EMIT SetUseBox(Smodel);
        emit SetSelectItemShow(false);
        seeds = streamtracer->getModelSelectMax(vectorName, numOfSeeds);
        model->GetSelection()->ClearSelections();
        //seeds = streamtracer->getModelSelect();
    } else if (control == 2) {
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) { Smodel = model; }
        Q_EMIT SetUseBox(Smodel);
        emit SetSelectItemShow(false);
        seeds = streamtracer->getModelSelectMin(vectorName, numOfSeeds);
        model->GetSelection()->ClearSelections();
        //seeds = streamtracer->getModelSelect();
    } else if (control == 3) {
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) { Smodel = model; }
        Q_EMIT SetUseBox(Smodel);
        emit SetSelectItemShow(false);
        //seeds = streamtracer->getModelSelect();
        seeds = streamtracer->getModelSelectMax(vectorName, numOfSeeds);
        model->GetSelection()->ClearSelections();
        auto temSeeds = streamtracer->seedPCoordGenerate(numOfSeeds, startP, endP);
        //auto temSeeds = streamtracer->getModelSelectMax(vectorName, numOfSeeds);
        for (auto seed: temSeeds) { seeds.emplace_back(seed); }
    } else if (control == 4) {
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) { Smodel = model; }
        Q_EMIT SetUseBox(model);
        emit SetSelectItemShow(false);
        //seeds = streamtracer->getModelSelect();
        seeds = streamtracer->getModelSelectMin(vectorName, numOfSeeds);
        model->GetSelection()->ClearSelections();
        auto temSeeds = streamtracer->seedPCoordGenerate(numOfSeeds, startP, endP);
        //auto temSeeds = streamtracer->getModelSelectMax(vectorName, numOfSeeds);
        for (auto seed: temSeeds) { seeds.emplace_back(seed); }
    }
    streamtracer->SetInput(seeds, vectorName, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
    streamtracer->Execute();
    //m_StreamBase->SetUpdate(true);
    if (!m_ResultObject) {
        m_ResultObject = iGame::UnstructuredMesh::New();
        m_ResultObject->AddObserver(iGame::Command::DeleteEvent, [&]() -> void {
            m_StreamBase->streamFilter->meshId = -1;
            modelBound = false;
            haveDraw = false;
            isExisted = false;
            this->parentWidget()->hide();
        });
    }
    auto resObj = streamtracer->GetOutput();
    if (resObj) {
        m_ResultObject->SetPoints(resObj->GetPoints());
        m_ResultObject->SetCells(resObj->GetCells(), resObj->GetCellTypes());
        m_ResultObject->SetAttributeSet(resObj->GetAttributeSet());
        m_ResultObject->SetShellRenderingOption(false);
        // m_ResultObject->SetShellRenderingOption(false);
        m_ResultObject->ViewCloudPicture(scene, 0);
        m_ResultObject->SetLineWidth(widthOfStreamLine);
    } else {
        m_ResultObject->SetPoints(iGame::Points::New());
        m_ResultObject->SetCells(iGame::CellArray::New(), iGame::UnsignedIntArray::New());
        m_ResultObject->SetAttributeSet(iGame::AttributeSet::New());
        m_ResultObject->SetShellRenderingOption(false);
        m_ResultObject->SetAttributeIndex(-1);
        m_ResultObject->SetLineWidth(widthOfStreamLine);
    }
    //scene->ChangeModelVisibility(model, false);
    if (!haveDraw) {
        m_ResultObject->DataObject::SetName(masterName + "_StreamLine");

        Q_EMIT AddStreamObject(m_ResultObject);

        haveDraw = true;
    } else {
        m_ResultObject->ConvertToDrawableData();

       // m_ResultObject->ViewCloudPicture(scene, 0);

        Q_EMIT UpdateStreamObject(m_ResultObject);
    }
    if (!haveDraw) scene->GetCurrentModel()->SetViewPointsSwitch(true);
    // scene->SetCurrentModel(1);

    if (isExisted == false) {
        isExisted = true;
        Selection = StreamLineSelection::New();
        Painter = scene->GetCurrentModel()->GetPainter3D();
        Selection->Start = startP;
        Selection->End = endP;
        Selection->SetSelectionCallBackEvent(
                [&](IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope) {
                    if (itemType == IG_CHANGE) {
                        startP = Selection->Start;
                        endP = Selection->End;
                        auto temStart = startP;
                        auto temEnd = endP;
                        ui->startX->setText(QString::number(temStart[0]));
                        ui->startY->setText(QString::number(temStart[1]));
                        ui->startZ->setText(QString::number(temStart[2]));
                        ui->endX->setText(QString::number(temEnd[0]));
                        ui->endY->setText(QString::number(temEnd[1]));
                        ui->endZ->setText(QString::number(temEnd[2]));
                    }
                },
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);
        scene->GetInteractor()->RequestStreamLineStyle(Selection);
    }
    // 深拷贝一份原始流线,供 Simplifier 重复使用
    m_OriginalStream = iGame::UnstructuredMesh::New();
    m_OriginalStream->DeepCopy(m_ResultObject); 
}
void igQtStreamTracerWidget::Simplifier() {
    if (!haveDraw || !m_StreamBase || !m_StreamBase->streamFilter) {
        std::cout << "[Simplify] Please generate streamlines first\n";
        return;
    }

    auto original = m_StreamBase->streamFilter->GetOutput();
    if (!original) return;

    auto simp = iGame::StreamlineSimplifier::New();
    simp->SetInput(original);
    simp->SetCurvBins(40);
    simp->SetNumClusters(ui->clusterSpin->value());
    //simp->SetPerCluster(ui->perClusterSpin->value());
    simp->SetTotalTarget(ui->perClusterSpin->value());  
    if (!simp->Execute()) {
        std::cout << "[Simplify] Execute failed\n";
        return;
    }
    auto out = simp->GetOutput();
    if (!out) return;

    auto scene = SceneManager::Instance()->GetCurrentScene();
    m_ResultObject->SetPoints(out->GetPoints());
    m_ResultObject->SetCells(out->GetCells(), out->GetCellTypes());
    m_ResultObject->SetAttributeSet(out->GetAttributeSet());
    m_ResultObject->SetShellRenderingOption(false);
    m_ResultObject->ViewCloudPicture(scene, 0);
    m_ResultObject->SetLineWidth(widthOfStreamLine);
    m_ResultObject->ConvertToDrawableData();

    Q_EMIT UpdateStreamObject(m_ResultObject);
}