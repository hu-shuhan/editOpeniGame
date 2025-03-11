#include<IQWidgets/igQtStreamTracerWidget.h>
#include <iGameSceneManager.h>
#include "iGameSelection.h"

using namespace iGame;
igQtStreamTracerWidget::igQtStreamTracerWidget(QWidget* parent) : QWidget(parent), ui(new Ui::SteamLineTracer)
{
    ui->setupUi(this);
	m_StreamBase = iGameStreamBase::New();
    connect(ui->control_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeControl()));
	connect(ui->numOfSeedLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changenumOfSeeds()));
	connect(ui->lengthOfStreamLine, SIGNAL(textChanged(const QString&)), this, SLOT(changelengthOfStreamLine()));
	connect(ui->lengthOfStep, SIGNAL(textChanged(const QString&)), this, SLOT(changelengthOfStep()));
    connect(ui->maxSteps, SIGNAL(textChanged(const QString&)), this, SLOT(changemaxSteps()));

    //connect(ui->lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeOffsetP1()));
    //connect(ui->lineEdit_2, SIGNAL(textChanged(const QString&)), this, SLOT(changeOffsetP1()));
    //connect(ui->lineEdit_3, SIGNAL(textChanged(const QString&)), this, SLOT(changeOffsetP1()));

    //connect(ui->lineEdit_4, SIGNAL(textChanged(const QString&)), this, SLOT(changeOffsetP2()));
    //connect(ui->lineEdit_5, SIGNAL(textChanged(const QString&)), this, SLOT(changeOffsetP2()));
    //connect(ui->lineEdit_6, SIGNAL(textChanged(const QString&)), this, SLOT(changeOffsetP2()));



    connect(ui->comboBox, &QComboBox::currentTextChanged, this, [&]() { this->changeVecName(); });
//	connect(ui->proportion_Slider, SIGNAL(valueChanged(int)), this, SLOT(changeProportion()));
//	connect(ui->proportion_Slider, SIGNAL(sliderPressed()), this, SLOT(Pressed()));
//	connect(ui->proportion_Slider, SIGNAL(sliderReleased()), this, SLOT(Released()));
	connect(ui->generate_streamline_btn, &QPushButton::clicked, this, &igQtStreamTracerWidget::generateStreamline);
	//connect(ui->pushButton, &QPushButton::clicked, this, &igQtStreamTracerWidget::increaseProportion);
//	connect(ui->pushButton_2, &QPushButton::clicked, this, &igQtStreamTracerWidget::reduceProportion);
	 numOfSeeds = 200;
	 ui->numOfSeedLineEdit->setText("200");
	 control = 0;
     haveClicked = false;
//	 proportion = 0.35;
	// ui->proportion_Slider->setValue(35);
	 lengthOfStreamLine = 5;
	 ui->lengthOfStreamLine->setText("5");
	 maxSteps =1000;
	 ui->maxSteps->setText("1000");
	 lengthOfStep = 0.3;
	 ui->lengthOfStep->setText("0.3");
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
        Selection->Start = seedPoints[control * 2];
        Selection->End = seedPoints[control * 2 + 1];
        Selection->SetFilterEvent(
                [&](iGame::Selection::Event event) {
                    if (event.type == iGame::Selection::Event::Change) {
                        seedPoints[control * 2] = Selection->Start;
                        seedPoints[control * 2 + 1] = Selection->End;
                    }
                },
                std::placeholders::_1);

        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);

        //if (rendererWidget->GetScene()->GetInteractor()) {
        //    rendererWidget->GetScene()->GetInteractor()->SetCallBack(&igQtModelClipWidget::FilterSignal, SliceWidget);
        //}

        scene->GetInteractor()->RequestStreamLineStyle(Selection);
    }
    if (first) {
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
        if (mesh->GetIsPolyhedronType()) { mesh->InitPolyhedronVertices();
        }
        first = false;
    }
}
void igQtStreamTracerWidget::changeControl() {
	control=ui->control_comboBox->currentIndex();

    generateStreamline();
	//std::cout << "current index=" << control <<std::endl;
}
void igQtStreamTracerWidget::changenumOfSeeds() {
	numOfSeeds = ui->numOfSeedLineEdit->text().toInt();
	//std::cout << "current seeds=" << numOfSeeds << std::endl;
}
void igQtStreamTracerWidget::changelengthOfStreamLine() {
	lengthOfStreamLine = ui->lengthOfStreamLine->text().toFloat();
}
void igQtStreamTracerWidget::changelengthOfStep() {
	lengthOfStep = ui->lengthOfStep->text().toFloat();
}
void igQtStreamTracerWidget::changemaxSteps() {
	maxSteps = ui->maxSteps->text().toFloat();
}
void igQtStreamTracerWidget::changeterminalSpeed() {
	terminalSpeed = ui->terminalSpeed->text().toFloat();
}
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
	if(haveClicked)
    generateStreamline();
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
	if (proportion<99) {
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

	iGameStreamTracer* streamtracer= m_StreamBase->streamFilter;
	Model::Pointer model = scene->GetCurrentModel();
	VolumeMesh::Pointer mesh;
    std::cout << model->GetDataObject()->GetName() << std::endl;
 	auto tem = model->GetDataObject();
    m_DataObject = tem;

    streamtracer->initStreamTracer(model);
    //streamtracer->seedLineGenerate(numOfSeeds);
    masterName = model->GetDataObject()->GetName();
    //auto seeds = streamtracer->streamSeedGenerate(control, proportion, numOfSeeds);
  //  auto seeds = streamtracer->streamBoundSeedGenerate(numOfSeeds);
    std::vector<std::vector<int>> seedPids = {{1797284, 3468659},
                                              {536542, 2738820},
                                              {536542, 2658742},
                                              {5485895, 536542}};
    seedPoints[5] = streamtracer->GetMesh()->GetBoundingBox().max;
    seedPoints[6] = streamtracer->GetMesh()->GetBoundingBox().min;
   // auto seeds = streamtracer->seedPidGenerate(numOfSeeds, seedPids[control][0], seedPids[control][1]);
    auto seeds = streamtracer->seedPCoordGenerate(numOfSeeds, seedPoints[control * 2], seedPoints[control*2+1]);
    //auto seeds = streamtracer->seedPidGenerate(numOfSeeds, p1, p2);
  //  auto seeds = streamtracer->subdataSeedGenerate(numOfSeeds);
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
    for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
        auto attribute = allAttributes->GetElement(i);
        // if (attribute.type == IG_VECTOR&&attribute.attachmentType == IG_POINT) {
        if (attribute.type == IG_VECTOR) {
            if (attribute.pointer) {
                vectorName = attribute.pointer->GetName();
                break;
            }
        }
    }
    std::cout << vectorName << std::endl;
    streamline = streamtracer->showStreamLineMix(seeds, vectorName, streamlineColor, lengthOfStreamLine, lengthOfStep,
                                                 terminalSpeed, maxSteps);
 //   if (streamtracer->GetMesh()->GetIsPolyhedronType()) {
	//	 streamline = streamtracer->showStreamLineCellData(seeds, "V", streamlineColor, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
	//}
	//else {
	//	 streamline = streamtracer->showStreamLineMix(seeds, "V", streamlineColor, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
	//}
	m_StreamBase->SetStreamLine(streamline,streamlineColor);
 //   auto MaxLen = streamtracer->GetMesh()->GetBoundingBox().diagVector().length();
 //   std::string msg = "当前精度为:" + std::to_string(streamtracer->AccuracyCul(streamline, MaxLen / 60, 5) * 100) + "%";
 //   ATL::CString ch(msg.c_str());
 //   MessageBox(NULL, ch, "提示", MB_OK);

	if (!haveDraw) {
		m_StreamBase->DataObject::SetName(masterName+"_StreamLine");
		Q_EMIT AddStreamObject(m_StreamBase);
		haveDraw = true;
	}
	else {
		Q_EMIT UpdateStreamObject(m_StreamBase);
	}
   
    if (isExisted == false) {
        isExisted = true;
        Selection = StreamLineSelection::New();
        Painter = scene->GetCurrentModel()->GetPainter3D();
        Selection->Start = seedPoints[control * 2];
        Selection->End = seedPoints[control * 2 + 1];
        Selection->SetFilterEvent(
                [&](iGame::Selection::Event event) {
                    if (event.type == iGame::Selection::Event::Change) {
                        seedPoints[control * 2] = Selection->Start;
                        seedPoints[control * 2 + 1] = Selection->End;
                    }
                },
                std::placeholders::_1);

        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);

        //if (rendererWidget->GetScene()->GetInteractor()) {
        //    rendererWidget->GetScene()->GetInteractor()->SetCallBack(&igQtModelClipWidget::FilterSignal, SliceWidget);
        //}

        scene->GetInteractor()->RequestStreamLineStyle(Selection);
    }
}