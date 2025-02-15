#include<IQWidgets/igQtStreamTracerWidget.h>
#include <iGameSceneManager.h>
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
	connect(ui->proportion_Slider, SIGNAL(valueChanged(int)), this, SLOT(changeProportion()));
	connect(ui->proportion_Slider, SIGNAL(sliderPressed()), this, SLOT(Pressed()));
	connect(ui->proportion_Slider, SIGNAL(sliderReleased()), this, SLOT(Released()));
	connect(ui->generate_streamline_btn, &QPushButton::clicked, this, &igQtStreamTracerWidget::generateStreamline);
	connect(ui->pushButton, &QPushButton::clicked, this, &igQtStreamTracerWidget::increaseProportion);
	connect(ui->pushButton_2, &QPushButton::clicked, this, &igQtStreamTracerWidget::reduceProportion);
	 numOfSeeds = 20;
	 ui->numOfSeedLineEdit->setText("20");
	 control = 0;
     haveClicked = false;
	 proportion = 0.35;
	 ui->proportion_Slider->setValue(35);
	 lengthOfStreamLine = 5;
	 ui->lengthOfStreamLine->setText("5");
	 maxSteps =500;
	 ui->maxSteps->setText("500");
	 lengthOfStep = 0.001;
	 ui->lengthOfStep->setText("0.001");
	 terminalSpeed = 0.001;
	 ui->terminalSpeed->setText("0.001");
	 haveDraw = false;
	 haveClicked = true;
	ui->control_comboBox->setCurrentIndex(0);
	streamlineResult = UnstructuredMesh::New();
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
void igQtStreamTracerWidget::changeProportion() {
	proportion = ui->proportion_Slider->value();
	proportion /= 100;
	if(haveClicked)
    generateStreamline();
	//std::cout << "current value=" << proportion << std::endl;
}
void igQtStreamTracerWidget::reduceProportion() {
	proportion = ui->proportion_Slider->value();
	if (proportion) {
		proportion = proportion - 1;
		ui->proportion_Slider->setValue(proportion);
		//std::cout << "current value=" << proportion << std::endl;
	}
}
void igQtStreamTracerWidget::increaseProportion() {
	proportion = ui->proportion_Slider->value();
	if (proportion<99) {
		proportion = proportion + 1;
		ui->proportion_Slider->setValue(proportion);
	}
	//std::cout << "current value=" << proportion << std::endl;
}
void igQtStreamTracerWidget::generateStreamline() {
	
	auto scene = SceneManager::Instance()->GetCurrentScene();
	iGameStreamTracer* streamtracer=m_StreamBase->streamFilter;
	Model::Pointer model = scene->GetCurrentModel();
	VolumeMesh::Pointer mesh;
    std::cout << model->GetDataObject()->GetName() << std::endl;
 	auto tem=model->GetDataObject();
    streamtracer->initStreamTracer(model);
    //streamtracer->seedLineGenerate(numOfSeeds);
    masterName = model->GetDataObject()->GetName();
  //  auto seeds = streamtracer->streamSeedGenerate(control, proportion, numOfSeeds);
  std::vector<int>seedid{1227606,1397916,1482809,1612232,1696906,1821171,1901653};
  auto seeds=streamtracer->streamseedGenerateWithPId(seedid);
  //  auto seeds = streamtracer->subdataSeedGenerate(numOfSeeds);
	std::vector<std::vector<float>> streamlineColor;
	std::vector<std::vector<float>> streamline;
    streamline = streamtracer->showStreamLineMix(seeds, "V", streamlineColor, lengthOfStreamLine, lengthOfStep,
                                                 terminalSpeed, maxSteps);
 //   if (streamtracer->GetMesh()->GetIsPolyhedronType()) {
	//	 streamline = streamtracer->showStreamLineCellData(seeds, "V", streamlineColor, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
	//}
	//else {
	//	 streamline = streamtracer->showStreamLineMix(seeds, "V", streamlineColor, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
	//}
	m_StreamBase->SetStreamLine(streamline,streamlineColor);

	if (!haveDraw) {
		m_StreamBase->DataObject::SetName(masterName+"_StreamLine");
		Q_EMIT AddStreamObject(m_StreamBase);
		haveDraw = true;
	}
	else {
		Q_EMIT UpdateStreamObject(m_StreamBase);
	}
   

}