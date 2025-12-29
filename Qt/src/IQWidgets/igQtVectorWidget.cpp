#include <IQWidgets/igQtVectorWidget.h>
#include <QString>
igQtVectorWidget::igQtVectorWidget(QWidget* parent) : QWidget(parent), ui(new Ui::igVector) {
    ui->setupUi(this);
    m_VectorBase = iGame::iGameVectorBase::New();
    m_VectorBase->AddObserver(iGame::Command::DeleteEvent, [&]() -> void {
        isDraw = false;
        updateVectorNameList();
        m_VectorBase->SetInit(false);
    });

    headRadiusP = 1;
    ui->headRSlider->setValue(100);
    headLengthP = 1;
    ui->headLSlider_2->setValue(100);
    tailLengthP = 1;
    ui->tailRSlider_3->setValue(100);
    tailRadiusP = 1;
    ui->tailLSlider_4->setValue(100);
    headRadius = 0.005;

    ui->headRlineEdit->setText("0.005");
    headLength = 0.015;
    ui->headLlineEdit->setText("0.015");
    tailLength = 0.025;
    ui->tailLlineEdit->setText("0.025");
    tailRadius = 0.002;
    ui->tailRlineEdit->setText("0.002");
    ui->lineEdit->setText("1200");
    ui->comboBox_2->setCurrentIndex(2);

    connect(ui->headRSlider, SIGNAL(valueChanged(int)), this, SLOT(changeHRProportion()));
    connect(ui->headLSlider_2, SIGNAL(valueChanged(int)), this, SLOT(changeHLProportion()));
    connect(ui->tailRSlider_3, SIGNAL(valueChanged(int)), this, SLOT(changeTRProportion()));
    connect(ui->tailLSlider_4, SIGNAL(valueChanged(int)), this, SLOT(changeTLProportion()));

    connect(ui->headRlineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeHR()));
    connect(ui->headLlineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeHL()));
    connect(ui->tailRlineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeTR()));
    connect(ui->tailLlineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeTL()));

    connect(ui->lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changeDrawModeP()));
    connect(ui->lineEdit_2, SIGNAL(textChanged(const QString&)), this, SLOT(changeDrawModeP()));


    connect(ui->comboBox, &QComboBox::currentTextChanged, this, [&]() { this->changeVecName(); });
    connect(ui->comboBox_2, &QComboBox::currentTextChanged, this, [&]() { this->switchMode(); });
    connect(ui->DrawButton, SIGNAL(clicked(bool)), this, SLOT(drawV()));


    auto* LineValidator = new QRegExpValidator(QRegExp("^[0-9]*\\.?[0-9]*$"), this);
    ui->headRlineEdit->setValidator(LineValidator);
    ui->headLlineEdit->setValidator(LineValidator);
    ui->tailLlineEdit->setValidator(LineValidator);
    ui->tailRlineEdit->setValidator(LineValidator);
}
void igQtVectorWidget::switchMode() {
    switch (ui->comboBox_2->currentIndex()) {
        case 1: {
            m_VectorBase->SetDrawMode(iGame::iGameVectorBase::CellInRange);
            std::cout << "C 2 CELLINRANGE" << std::endl;
            auto tem = m_VectorBase->GetCellRange();
            ui->lineEdit->setText(QString::number(tem.first));
            ui->lineEdit_2->setText(QString::number(tem.second));
            break;
        }
        case 2: {
            std::cout << "C 2 Every" << std::endl;
            m_VectorBase->SetDrawMode(iGame::iGameVectorBase::EveryNth);
            ui->lineEdit->setText(QString::number(m_VectorBase->GetNth()));
            ui->lineEdit_2->setText("");
            break;
        }
        case 0: {
            std::cout << "C 2 ALL" << std::endl;
            m_VectorBase->SetDrawMode(iGame::iGameVectorBase::AllCell);
            ui->lineEdit->setText("");
            ui->lineEdit_2->setText("");
            break;
        }
    }
}

void igQtVectorWidget::drawV() {

    m_VectorBase->SetArrow(headRadius * headRadiusP, headLength * headLengthP, tailRadius * tailRadiusP,
                           tailLength * tailLengthP);
    bool check = m_VectorBase->DrawVector(vecName);
    if (!check) { return; }
    if (!isDraw) {
        m_VectorBase->DataObject::SetName(masterName + "_Vector");
        isDraw = true;
        haveChange = false;
        Q_EMIT DrawDireVector(m_VectorBase);
    } else {
        if (haveChange) {
            m_VectorBase->DataObject::SetName(masterName + "_Vector");
            haveChange = false;
        }
        Q_EMIT UpdateDireVector(m_VectorBase);
    }
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

    tailRadiusP = ui->tailRSlider_3->value();
    tailRadiusP /= 100;
    std::cout << "current value=" << tailRadiusP << std::endl;
}
void igQtVectorWidget::changeTLProportion() {

    tailLengthP = ui->tailLSlider_4->value();
    tailLengthP /= 100;
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
void igQtVectorWidget::changeDrawModeP() {
    switch (m_VectorBase->GetDrawMode()) {
        case iGame::iGameVectorBase::EveryNth: {
            m_VectorBase->SetNth(ui->lineEdit->text().toInt());
            break;
        }
        case iGame::iGameVectorBase::CellInRange: {
            m_VectorBase->SetCellRange(ui->lineEdit->text().toInt(), ui->lineEdit_2->text().toInt());
            break;
        }
        default:
            break;
    }
}
void igQtVectorWidget::updateVectorNameList() {
    ui->comboBox->clear();
    auto sceneManager = iGame::SceneManager::Instance();
    auto scene = sceneManager->GetCurrentScene();
    if (!scene) return;
    auto currentModel = scene->GetCurrentModel();
    if (!currentModel) return;
    auto obj = currentModel->GetDataObject();
    m_VectorBase->SetColorMapper(obj->GetColorMapper());
    if (masterName != obj->GetName()) {
        masterName = obj->GetName();
        std::cout << "masterName=" << masterName << std::endl;
        m_VectorBase->SetColorMapper(obj->GetColorMapper());
        m_VectorBase->SetInit(false);
        haveChange = true;
    }
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
void igQtVectorWidget::changeVecName() {
    vecName = ui->comboBox->currentText().toStdString();
    std::cout << "current value=" << vecName << std::endl;
}