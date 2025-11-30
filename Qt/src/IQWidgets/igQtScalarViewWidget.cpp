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
	ui->label_Scalar->hide();
	ui->scalarInfoComboBox->hide();
	ui->scalarItemComboBox->hide();
	ui->label_Data->hide();
	ui->DataInfoCombox->hide();
	ui->DataItemCombox->hide();
	SetCustomScaleRangeWidget = new QWidget;
	SetCustomScaleRangeUi = new Ui::SetCustomScaleRange;
	SetCustomScaleRangeUi->setupUi(SetCustomScaleRangeWidget);
	SetCustomScaleRangeWidget->hide();
	ui->widget_DataRangeSlider->hide();

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
    connect(ui->radioButton_Liner, &QRadioButton::toggled, this, [&](bool checked){
		if(this->m_ColorMapper==nullptr)return;
        if(checked) this->m_ColorMapper->SetMapTypeToRGBLiner();
        else this->m_ColorMapper->SetMapTypeToRGBSTEP();
        showScalarView();
    });
}
void igQtScalarViewWidget::loadScalarData() {
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
	this->scalarDimension = obj->GetAttributeDimension();

	auto dataRange = obj->GetAttributeSet()->GetAttribute(scalarName).GetDataRange();
	if (dataRange) {
		this->scalarMin = dataRange->GetValue(2 * scalarDimension + 2);
		this->scalarMax = dataRange->GetValue(2 * scalarDimension + 3);
	}

}
void igQtScalarViewWidget::initScalarRange() {

	if (currentSelectedScalarIdx < 0) {
		ui->widget_DataRangeSlider->hide();
		return;
	}
	// 更新 ColorMapper 的范围到新的数据范围
	if (m_ColorMapper) {
		m_ColorMapper->SetRange(scalarMin, scalarMax);
	}
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
	initScalarInfo();
	// 确保 ColorMapper 更新后刷新视图
	if (m_ColorMapper) {
		updateDrawStyle();
	}
}
void igQtScalarViewWidget::updateDrawStyle() {
	if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper;
	}
	m_ColorMapper->Modified();
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
	if (scene) {
		scene->Update();
	}
}
void igQtScalarViewWidget::editColorBar() { 
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    iGame::DataObject::Pointer obj=nullptr;
    if (scene) {
        auto model = scene->GetCurrentModel();
        if (model) { obj = model->GetDataObject(); }
    }
    if (!obj) return;
	Q_EMIT ChangeShowColorManager(); 
}
void igQtScalarViewWidget::rescaleRange() {
	if (!m_ColorMapper) { m_ColorMapper = m_TmpColorMapper; }
	if (currentSelectedScalarIdx < 0) return;
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    iGame::DataObject::Pointer obj=nullptr;
    if (scene) {
        auto model = scene->GetCurrentModel();
        if (model) { obj = model->GetDataObject(); }
    }
    if (!obj) return;
    obj->ReCollectSubDataObjectDataRange();
    auto attribute = obj->GetAttributeSet()->GetAttribute(currentSelectedScalarIdx);
    scalarMin = attribute.dataRange->GetElement(scalarDimension + 1)[0];
    scalarMax = attribute.dataRange->GetElement(scalarDimension + 1)[1];

	m_ColorMapper->SetRange(scalarMin, scalarMax);
	ui->widget_DataRangeSlider->updateMinAndMax(scalarMin, scalarMax);
    initScalarInfo();
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
