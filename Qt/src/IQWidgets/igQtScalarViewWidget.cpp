#include "iGameSceneManager.h"
#include <IQWidgets/igQtScalarViewWidget.h>
//#include <iGameManager.h>
//#include <iGameModelColorManager.h>
/**
 * @class   igQtScalarViewWidget
 * @brief   igQtScalarViewWidget's brief
 */
igQtScalarViewWidget::igQtScalarViewWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::ScalarView) {
    ui->setupUi(this);
    //modelColorManager = iGame::iGameModelColorManager::Instance();
    SetCustomScaleRangeWidget = new QWidget;
    SetCustomScaleRangeUi = new Ui::SetCustomScaleRange;
    SetCustomScaleRangeUi->setupUi(SetCustomScaleRangeWidget);
    SetCustomScaleRangeWidget->hide();
    ui->widget_DataRangeSlider->hide();
    connect(ui->scalarInfoComboBox, SIGNAL(currentIndexChanged(QString)), this,
            SLOT(showScalarItem()));
    connect(ui->scalarItemComboBox, SIGNAL(currentIndexChanged(QString)), this,
            SLOT(showScalarView()));

    connect(ui->btn_EditColorMap, &QPushButton::clicked, this,
            &igQtScalarViewWidget::editColorBar);
    connect(ui->btn_RescaleRange, &QPushButton::clicked, this,
            &igQtScalarViewWidget::rescaleRange);
    connect(ui->btn_SetRange, &QPushButton::clicked, this,
            &igQtScalarViewWidget::showCustomScaleRangeWidget);
    connect(ui->btn_IsShowColorLegend, &QPushButton::clicked, this,
            &igQtScalarViewWidget::isShowColorLegend);
    connect(ui->widget_DataRangeSlider, &igQtDataRangeSlider::DataRangeChanged,
            this, [&](float _min, float _max) {
                m_ColorMapper->SetRange(_min, _max);
                updateDrawStyle();
            });
    connect(SetCustomScaleRangeUi->btnRescale, &QPushButton::clicked, this,
            &igQtScalarViewWidget::setCustomScaleRange);
    connect(SetCustomScaleRangeUi->btnCancle, &QPushButton::clicked, this,
            [&]() { this->SetCustomScaleRangeWidget->hide(); });
}
void igQtScalarViewWidget::getScalarsName() {
    //ui->scalarInfoComboBox->clear();
    //ui->scalarInfoComboBox->addItem("Solider");
    //auto manager = iGame::iGameManager::Instance();
    //if (manager->GetModelList().size() == 0)return;
    //auto model = manager->GetCurrentModel();
    //auto dataset = model->DataSet;
    //scalarInfo.clear();
    //if (dataset->GetPointData() == nullptr)return;
    //auto attributes = dataset->GetPointData()->GetAllScalars();
    //for (int i = 0; i < attributes.size(); i++) {
    //	auto data = dataset->GetPointData()->GetScalars(i);
    //	std::string name = data->GetName();
    //	int size = data->GetNumberOfComponents();
    //	scalarInfo[name] = size;
    //}
    //for (auto it : scalarInfo) {
    //	ui->scalarInfoComboBox->addItem(QString::fromStdString(it.first));
    //}
}
void igQtScalarViewWidget::showScalarItem() {
    //   this->currentSelectedScalarIdx = ui->scalarInfoComboBox->currentIndex();
    //   modelColorManager->SetCurrentSelectedScalarIdx(currentSelectedScalarIdx - 1);
    //auto scalarName = ui->scalarInfoComboBox->currentText().toStdString();
    //int size = 0;
    //for (auto& [_name, _size] : scalarInfo) {
    //	if (_name == scalarName) {
    //		size = _size;
    //		break;
    //	}
    //}
    ////std::cout << scalarName << std::endl;
    ////std::cout << size << std::endl;
    //ui->scalarItemComboBox->clear();
    //if (size > 1) ui->scalarItemComboBox->addItem(QString::fromStdString("magnitude"));
    //if (size > 0) ui->scalarItemComboBox->addItem(QString::fromStdString("x"));
    //if (size > 1) ui->scalarItemComboBox->addItem(QString::fromStdString("y"));
    //if (size > 2) ui->scalarItemComboBox->addItem(QString::fromStdString("z"));
    //if (size > 3) ui->scalarItemComboBox->addItem(QString::fromStdString("u"));
}
void igQtScalarViewWidget::loadScalarData() {
    //   this->currentSelectedScalarIdx = ui->scalarInfoComboBox->currentIndex();
    //this->scalarName = ui->scalarInfoComboBox->currentText().toStdString();
    //   this->drawItem = ui->scalarItemComboBox->currentIndex();
    //   modelColorManager->SetCurrentSelectedScalarIdx(currentSelectedScalarIdx - 1);
    //   modelColorManager->SetCurrentScalarComponentIdx(drawItem - 1);
    //   this->scalarData = nullptr;
    //if (drawItem == -1 || scalarName == "Solider")return;
    //auto manager = iGame::iGameManager::Instance();
    //if (manager->GetModelList().size() == 0)return;
    //auto PointData = manager->GetCurrentModel()->DataSet->GetPointData();
    //if (PointData != nullptr){
    //	this->scalarData = PointData->GetScalars(scalarName);
    //}
    //return;


    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    m_ColorMapper = nullptr;
    iGame::DataObject::Pointer obj;
    if (scene) {
        auto model = scene->GetCurrentModel();
        if (model) { obj = model->GetDataObject(); }
    }
    if (!obj) return;
    this->currentSelectedScalarIdx = obj->GetAttributeIndex();
    if (currentSelectedScalarIdx < 0) return;

    this->m_ColorMapper = obj->GetColorMapper();
    this->scalarName = obj->GetAttributeSet()
                               ->GetAttribute(currentSelectedScalarIdx)
                               .pointer->GetName();
    this->drawItem = obj->GetAttributeDimension();
}
void igQtScalarViewWidget::initScalarRange() {

    if (currentSelectedScalarIdx < 0) {
        ui->widget_DataRangeSlider->hide();
        return;
    }
    this->scalarMin = m_ColorMapper->GetRange()[0];
    this->scalarMax = m_ColorMapper->GetRange()[1];
    ui->widget_DataRangeSlider->updateMinAndMax(scalarMin, scalarMax);
    ui->widget_DataRangeSlider->show();
}
void igQtScalarViewWidget::initScalarInfo()
{
    ui->labelScalarInfo->clear();
    if (currentSelectedScalarIdx < 0) return;
    std::string str = scalarName +
                      "\nMin Value : " + std::to_string(scalarMin) +
                      "\nMax Value : " + std::to_string(scalarMax);
    ui->labelScalarInfo->setText(QString::fromStdString(str));
}

void igQtScalarViewWidget::showScalarView() {
    loadScalarData();
    initScalarRange();
    //updateDrawStyle();
    initScalarInfo();
}
void igQtScalarViewWidget::updateDrawStyle() {
    m_ColorMapper->Modified();
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene) {
        auto model = scene->GetCurrentModel();
        if (model && model->GetDataObject()) {
            model->ViewCloudPicture(
                    currentSelectedScalarIdx,
                    drawItem);
        }
    }
}
void igQtScalarViewWidget::editColorBar() { Q_EMIT ChangeShowColorManager(); }
void igQtScalarViewWidget::rescaleRange() {
    if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper; }
    m_ColorMapper->SetRange(scalarMin, scalarMax);
    ui->widget_DataRangeSlider->updateMinAndMax(scalarMin, scalarMax);
    updateDrawStyle();
}

void igQtScalarViewWidget::setCustomScaleRange() {
    if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper; }
    QString strMin = SetCustomScaleRangeUi->lineEdit_min->text();
    QString strMax = SetCustomScaleRangeUi->lineEdit_max->text();
    float min = 0.0, max = 0.0;
    std::stringstream ssmin(strMin.toStdString()), ssmax(strMax.toStdString());
    ssmin >> min;
    ssmax >> max;
    m_ColorMapper->SetRange(min, max);
    ui->widget_DataRangeSlider->updateMinAndMax(min, max);
    updateDrawStyle();
    this->SetCustomScaleRangeWidget->hide();
}

void igQtScalarViewWidget::showCustomScaleRangeWidget() {
    if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper; }
    QRegExp rx("^-?\\d+(\\.\\d+)?([eE][-+]?\\d+)?$");
    SetCustomScaleRangeUi->lineEdit_min->setValidator(
            new QRegExpValidator(rx, this));
    SetCustomScaleRangeUi->lineEdit_max->setValidator(
            new QRegExpValidator(rx, this));
    auto min = m_ColorMapper->GetRange()[0];
    auto max = m_ColorMapper->GetRange()[1];
    SetCustomScaleRangeUi->lineEdit_min->setText(
            QString::fromStdString(std::to_string(min)));
    SetCustomScaleRangeUi->lineEdit_max->setText(
            QString::fromStdString(std::to_string(max)));
    this->SetCustomScaleRangeWidget->show();
}
void igQtScalarViewWidget::isShowColorLegend() { Q_EMIT changeColorBarShow(); }

int igQtScalarViewWidget::getCurrentSelectedScalarIdx() {
    return currentSelectedScalarIdx;
}
