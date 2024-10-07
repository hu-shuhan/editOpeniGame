#include<IQWidgets/igQtVectorWidget.h>
igQtVectorWidget::igQtVectorWidget(QWidget* parent) : QWidget(parent), ui(new Ui::igVector) {
    ui->setupUi(this);
    m_VectorBase = iGame::iGameVectorBase::New();
    headRadiusP = 1;
    headLengthP = 1;
    tailLengthP = 1;
    tailRadiusP = 1;
    headRadius = 1;
    headLength = 1;
    tailLength = 2;
    tailRadius = 0.5;
	connect(ui->headRSlider, SIGNAL(valueChanged(int)),   this,  SLOT(changeHRProportion()));
	connect(ui->headLSlider_2, SIGNAL(valueChanged(int)), this,  SLOT(changeHLProportion()));
    connect(ui->tailRSlider_3, SIGNAL(valueChanged(int)), this,  SLOT(changeTRProportion()));
    connect(ui->tailLSlider_4, SIGNAL(valueChanged(int)), this,  SLOT(changeTLProportion()));

	connect(ui->headRlineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeHR()));
    connect(ui->headLlineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeHL()));
    connect(ui->tailRlineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeTR()));
    connect(ui->tailLlineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeTL()));

    connect(ui->comboBox, &QComboBox::currentTextChanged, this,[&]() {this->changeVecName();});

}
void igQtVectorWidget::changeHRProportion() {
	headRadiusP = ui->headRSlider->value();
	headRadiusP /= 100;
    std::cout << "current value=" << headRadiusP << std::endl;
}
void igQtVectorWidget::changeHLProportion() {

	headLengthP = ui->headLSlider_2->value();
	headLengthP /= 100;
    std::cout << "current value=" << headLengthP << std::endl;
}
void igQtVectorWidget::changeTRProportion() {

	tailRadiusP= ui->tailRSlider_3->value();
	tailRadiusP/= 100;
    std::cout << "current value=" << tailRadiusP << std::endl;
}
void igQtVectorWidget::changeTLProportion() {

	tailLengthP= ui->tailLSlider_4 ->value();
	tailLengthP/= 100;
    std::cout << "current value=" << tailLengthP << std::endl;
}
void igQtVectorWidget::changeHR() {

    headRadius = ui->headRlineEdit->text().toFloat();
    std::cout << "current value=" << headRadius << std::endl;
}
void igQtVectorWidget::changeHL() {

    headLength = ui->headLlineEdit->text().toFloat();
    std::cout << "current value=" << headLength << std::endl;
}
void igQtVectorWidget::changeTR() {

    tailRadius = ui->tailRlineEdit->text().toFloat();
    std::cout << "current value=" << tailRadius << std::endl;
}
void igQtVectorWidget::changeTL() {
    tailLength = ui->tailLlineEdit->text().toFloat();
    std::cout << "current value=" << tailLength << std::endl;
}
void igQtVectorWidget::updateVectorNameList() {
    ui->comboBox->clear();
    auto sceneManager = iGame::SceneManager::Instance();
    auto scene = sceneManager->GetCurrentScene();
    if (!scene) return;
    auto currentModel = scene->GetCurrentModel();
    if (!currentModel) return;
    auto obj = currentModel->GetDataObject();
    if (!obj) return;
    auto AttributeSet = obj->GetAttributeSet();
    if (!AttributeSet) return;
    auto allAttributes = AttributeSet->GetAllAttributes();
    if (!allAttributes) return;
    for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
        auto attribute = allAttributes->GetElement(i);
        if (attribute.type == IG_VECTOR&&attribute.attachmentType == IG_POINT) {
            if (attribute.pointer) {
                ui->comboBox->addItem(
                        QString::fromStdString(attribute.pointer->GetName()));
            }
        }
    }
}
 void igQtVectorWidget::changeVecName() {
    vecName = ui->comboBox->currentText().toStdString();
     std::cout << "current value=" << vecName << std::endl;
 }