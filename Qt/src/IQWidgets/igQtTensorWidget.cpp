#include<IQWidgets/igQtTensorWidget.h>
#include "iGameSceneManager.h"
#include <iomanip>
/**
 * @class   igQtTensorWidget
 * @brief   igQtTensorWidget's brief
 */
igQtTensorWidget::igQtTensorWidget(QWidget* parent) : QWidget(parent), ui(new Ui::TensorView) {
	ui->setupUi(this);
	UpdateComponentsShow(false);
	this->m_Manager = iGame::iGameTensorWidgetBase::New();

	ui->GlyphTypeComboBox->clear();
	ui->GlyphTypeComboBox->addItem("Ellipsoid");
	ui->GlyphTypeComboBox->addItem("Cuboid");

	connect(ui->TensorInfoComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(InitTensorAttributes()));
	connect(ui->btn_Apply, SIGNAL(clicked(bool)), this, SLOT(ShowTensorField()));
	connect(ui->GlyphTypeComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(UpdateGlyphType()));
	QRegExp rx = QRegExp("^?\\d+(\\.\\d+)?([eE][-+]?\\d+)?$");
	ui->lineEdit_TensorScale->setValidator(new QRegExpValidator(rx, this));
	connect(ui->horizontalSlider_ScaleSlider, &QSlider::valueChanged, this, [&]() {
		double scale = ui->horizontalSlider_ScaleSlider->value() * 1.0 / 1000.0;
		this->UpdateGlyphScale(scale);
		std::stringstream ss;
		ss << std::setprecision(4) << scale;
		ui->lineEdit_TensorScale->setText(QString::fromStdString(ss.str()));
		});
	connect(ui->lineEdit_TensorScale, &QLineEdit::editingFinished, this, [&]() {
		QString data = ui->lineEdit_TensorScale->text();
		std::stringstream ss(data.toStdString());
		double scale = .0;
		ss >> scale;
		this->UpdateGlyphScale(scale);
		ui->horizontalSlider_ScaleSlider->setValue(scale * 1000.0);
		});
	connect(ui->ScalarInfoComboBox, &QComboBox::currentTextChanged, this, [&]() {
		this->UpdateGlyphColors();
		this->m_Manager->UpdateGlyphDrawColor();
		});
	connect(ui->btn_GenerateVector, SIGNAL(clicked(bool)), this, SLOT(GenerateVectorField()));


}
igQtTensorWidget::~igQtTensorWidget()
{

}

void igQtTensorWidget::UpdateGlyphType()
{
	auto type = ui->GlyphTypeComboBox->currentIndex();
	switch (type)
	{
	case 0:
		this->m_Manager->SetGlyphType(iGameTensorRepresentation::DrawType::ELLIPSOID);
		break;
	case 1:
		this->m_Manager->SetGlyphType(iGameTensorRepresentation::DrawType::CUBOID);
		break;
	default:
		break;
	}
	this->m_Manager->ShowTensorField();
	if (m_Generated) {
		Q_EMIT UpdateTensorGlyphs(m_Manager);
	}
}
void igQtTensorWidget::UpdateComponentsShow(bool flag)
{
	if (flag) {
		ui->lineEdit_TensorScale->show();
		ui->horizontalSlider_ScaleSlider->show();
		ui->ScalarInfoComboBox->show();
		ui->label_ColorFrom->show();
		ui->label_ScaleData->show();
		ui->btn_Apply->hide();
	}
	else {
		ui->lineEdit_TensorScale->hide();
		ui->horizontalSlider_ScaleSlider->hide();
		ui->ScalarInfoComboBox->hide();
		ui->label_ColorFrom->hide();
		ui->label_ScaleData->hide();
		ui->btn_Apply->show();
	}
}
void igQtTensorWidget::UpdateScalarsNameList()
{
	ui->ScalarInfoComboBox->clear();
	ui->ScalarInfoComboBox->addItem("Solid");
	auto sceneManager = iGame::SceneManager::Instance();
	auto scene = sceneManager->GetCurrentScene();
	if (!scene)return;
	auto currentModel = scene->GetCurrentModel();
	if (!currentModel)return;
	auto obj = currentModel->GetDataObject();
	if (!obj)return;
	auto AttributeSet = obj->GetAttributeSet();
	if (!AttributeSet)return;
	auto allAttributes = AttributeSet->GetAllAttributes();
	if (!allAttributes)return;
	for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
		auto attribute = allAttributes->GetElement(i);
		if (attribute.type == IG_SCALAR && attribute.attachmentType == IG_POINT) {
			if (attribute.pointer) {
				ui->ScalarInfoComboBox->addItem(QString::fromStdString(attribute.pointer->GetName()));
			}
		}
	}
}
void igQtTensorWidget::UpdateTensorsNameList()
{
	ui->TensorInfoComboBox->clear();
	auto sceneManager = iGame::SceneManager::Instance();
	auto scene = sceneManager->GetCurrentScene();
	if (!scene)return;
	auto currentModel = scene->GetCurrentModel();
	if (!currentModel)return;
	auto obj = currentModel->GetDataObject();
	if (!obj)return;
	this->m_DataObject = obj;
	auto AttributeSet = obj->GetAttributeSet();
	if (!AttributeSet)return;
	auto allAttributes = AttributeSet->GetAllAttributes();
	if (!allAttributes)return;
	for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
		auto attribute = allAttributes->GetElement(i);
		if (attribute.type == IG_TENSOR && attribute.attachmentType == IG_POINT) {
			if (attribute.pointer) {
				ui->TensorInfoComboBox->addItem(QString::fromStdString(attribute.pointer->GetName()));
			}
		}
	}

}
void igQtTensorWidget::InitTensorAttributes()
{
	if (!this->m_DataObject)return;
	auto tmpObj = DynamicCast<PointSet>(m_DataObject);
	if (!tmpObj)return;
	this->m_Manager->SetPoints(tmpObj->GetPoints());
	auto tensorName = ui->TensorInfoComboBox->currentText();
	//findTensor
	auto attribute = tmpObj->GetAttributeSet()->GetAttribute(tensorName.toStdString(), IG_TENSOR);
	this->m_Manager->SetTensorAttributes(attribute.pointer);
	this->UpdateComponentsShow(false);
	m_Generated = false;
}
void igQtTensorWidget::ShowTensorField()
{
	this->m_Manager->ShowTensorField();
	this->UpdateComponentsShow(true);
	m_Generated = true;
	m_Manager->DataObject::SetName(m_DataObject->GetName() + "_TensorView");
	Q_EMIT DrawTensorGlyphs(m_Manager);
}
void igQtTensorWidget::UpdateGlyphScale(double s)
{
	if (!m_Generated)return;
	this->m_Manager->UpdateGlyphScale(s);
	Q_EMIT UpdateTensorGlyphs(m_Manager);

}
void igQtTensorWidget::UpdateGlyphColors()
{
	if (!this->m_DataObject || !m_Generated)return;
	auto scalarName = ui->ScalarInfoComboBox->currentText();
	if (ui->ScalarInfoComboBox->currentIndex()) {
		auto scalar = m_DataObject->GetAttributeSet()->GetAttribute(scalarName.toStdString(), IG_SCALAR);
		auto mapper = ScalarsToColors::New();
		if (!scalar.pointer) {
			this->m_Manager->SetPositionColors({ nullptr });
		}
		else {
			auto array = scalar.pointer;
			mapper->InitRange(array, -1);
			auto colors = mapper->MapScalars(array, -1);
			this->m_Manager->SetPositionColors(colors);
		}
	}
	else {
		this->m_Manager->SetPositionColors({ nullptr });
	}
	Q_EMIT UpdateTensorGlyphs(m_Manager);
}
void igQtTensorWidget::GenerateVectorField()
{
	auto data = this->m_Manager->GenerateVectorField();
	if (!m_DataObject->GetAttributeSet()) {
		m_DataObject->SetAttributeSet(AttributeSet::New());
	}
	data->SetName(ui->TensorInfoComboBox->currentText().toStdString() + "_EigenVector");
	m_DataObject->GetAttributeSet()->AddAttribute(IG_VECTOR, IG_POINT, data);
	Q_EMIT UpdateAttributes(m_DataObject);

}
