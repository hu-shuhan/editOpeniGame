#include "iGameSelection.h"
#include <IQWidgets/igQtStreamTracerWidget.h>
#include <iGameSceneManager.h>

using namespace iGame;
igQtStreamTracerWidget::igQtStreamTracerWidget(QWidget* parent) : QWidget(parent), ui(new Ui::SteamLineTracer) {
    ui->setupUi(this);
    connect(ui->control_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeControl()));
    connect(ui->numOfSeedLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changenumOfSeeds()));
    connect(ui->lengthOfStreamLine, SIGNAL(textChanged(const QString&)), this, SLOT(changelengthOfStreamLine()));
    connect(ui->lengthOfStep, SIGNAL(textChanged(const QString&)), this, SLOT(changelengthOfStep()));
    connect(ui->maxSteps, SIGNAL(textChanged(const QString&)), this, SLOT(changemaxSteps()));
    connect(ui->startX, SIGNAL(textChanged(const QString&)), this, SLOT(changeStart()));
    connect(ui->startY, SIGNAL(textChanged(const QString&)), this, SLOT(changeStart()));
    connect(ui->startZ, SIGNAL(textChanged(const QString&)), this, SLOT(changeStart()));
    ui->startX->setText("0");
    ui->startY->setText("0");
    ui->startZ->setText("0");
    connect(ui->endX, SIGNAL(textChanged(const QString&)), this, SLOT(changeEnd()));
    connect(ui->endY, SIGNAL(textChanged(const QString&)), this, SLOT(changeEnd()));
    connect(ui->endZ, SIGNAL(textChanged(const QString&)), this, SLOT(changeEnd()));
    ui->endX->setText("0");
    ui->endY->setText("0");
    ui->endZ->setText("0");

    connect(ui->terminalSpeed, SIGNAL(textChanged(const QString&)), this, SLOT(changeterminalSpeed()));


    connect(ui->comboBox, &QComboBox::currentTextChanged, this, [&]() { this->changeVecName(); });

    connect(ui->generate_streamline_btn, &QPushButton::clicked, this, &igQtStreamTracerWidget::generateStreamline);

    numOfSeeds = 200;
    ui->numOfSeedLineEdit->setText("200");
    control = 0;
    haveClicked = false;
    //	 proportion = 0.35;
    // ui->proportion_Slider->setValue(35);
    lengthOfStreamLine = 5;
    ui->lengthOfStreamLine->setText("5");
    maxSteps = 1000;
    ui->maxSteps->setText("1000");
    lengthOfStep = 0.3;
    ui->lengthOfStep->setText("0.03");
    terminalSpeed = 0.005;
    ui->terminalSpeed->setText("0.005");
    haveDraw = false;
    haveClicked = true;
    ui->control_comboBox->setCurrentIndex(0);
    streamlineResult = UnstructuredMesh::New();
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

        //if (rendererWidget->GetScene()->GetInteractor()) {
        //    rendererWidget->GetScene()->GetInteractor()->SetCallBack(&igQtModelClipWidget::FilterSignal, SliceWidget);
        //}

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
    //std::cout << "current seeds=" << numOfSeeds << std::endl;
}
void igQtStreamTracerWidget::changeEnd() {
    endP = Vector3f(ui->endX->text().toFloat(), ui->endY->text().toFloat(), ui->endZ->text().toFloat());
    //std::cout << "current seeds=" << numOfSeeds << std::endl;
}
void igQtStreamTracerWidget::changelengthOfStreamLine() {
    lengthOfStreamLine = ui->lengthOfStreamLine->text().toFloat();
}
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
    startP = obj->GetBoundingBox().min;
    endP = obj->GetBoundingBox().max;
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
}
void igQtStreamTracerWidget::changeVecName() {
    vectorName = ui->comboBox->currentText().toStdString();
    std::cout << "current value=" << vectorName << std::endl;
}
void igQtStreamTracerWidget::generateStreamline() {

    auto scene = SceneManager::Instance()->GetCurrentScene();

    iGame::StreamTracer* streamtracer = m_StreamBase->streamFilter;
    Model::Pointer model = scene->GetCurrentModel();
    VolumeMesh::Pointer mesh;
    std::cout << model->GetDataObject()->GetName() << std::endl;
    auto tem = model->GetDataObject();
    m_DataObject = tem;

    streamtracer->initStreamTracer(model);
    masterName = model->GetDataObject()->GetName();
    std::vector<std::vector<int>> seedPids = {{1797284, 3468659},
                                              {536542, 2738820},
                                              {536542, 2658742},
                                              {5485895, 536542}};

    std::vector<std::vector<float>> streamlineColor;
    std::vector<std::vector<float>> streamline;
    iGame::AttributeSet* _AttributeSet;
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
        seeds = streamtracer->getModelSelectMax(vectorName);
    } else {
        seeds = streamtracer->getModelSelect();
    }
    streamtracer->SetInput(seeds, vectorName, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
    streamtracer->Execute();
    if (!m_ResultObject) {
        m_ResultObject = iGame::UnstructuredMesh::New();
    }
    auto resObj = streamtracer->GetOutput();
    if (resObj) {
        m_ResultObject->SetPoints(resObj->GetPoints());
        m_ResultObject->SetCells(resObj->GetCells(), resObj->GetCellTypes());
        m_ResultObject->SetAttributeSet(resObj->GetAttributeSet());

    }

    scene->ChangeModelVisibility(model, false);


    if (!haveDraw) {
        m_ResultObject->DataObject::SetName(masterName + "_StreamLine");
        Q_EMIT AddStreamObject(m_ResultObject);
        haveDraw = true;
    } else {
        Q_EMIT UpdateStreamObject(m_ResultObject);
    }

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
}
