#include "IQWidgets/igQtContourExtractWidget.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameSceneManager.h"
#include "iGameScene.h"
#include "iGameSmartPointer.h"

#include <QRegularExpression>
#include <QRegularExpressionValidator>
igQtContourExtractWidget::igQtContourExtractWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::ContourExtract) {


	ui->setupUi(this);
	ui->label_RangeInfo->clear();
	m_Generated = false;
	m_Extracter = nullptr;
	m_PointData = nullptr;
	QRegularExpression  rx("-?\\d*\\.?\\d+");
	ui->lineEdit_IsoValue->setValidator(new QRegularExpressionValidator(rx, this));
	connect(ui->btnExecute, &QPushButton::clicked, this, &igQtContourExtractWidget::ContourExtract);
	connect(ui->comboBox_ScalarIndex, &QComboBox::currentTextChanged, this, &igQtContourExtractWidget::UpdateScalarName);
	connect(ui->comboBox_ScalarDimension, &QComboBox::currentTextChanged, this, &igQtContourExtractWidget::UpdateScalarDimension);
}


void igQtContourExtractWidget::InitScalarName()
{
	ui->comboBox_ScalarIndex->clear();
	for (int i = 0; i < m_PointData->GetNumberOfElements(); i++) {
		auto array = m_PointData->GetElement(i).pointer;
		ui->comboBox_ScalarIndex->addItem(QString::fromStdString(array->GetName()));
	}
}
void igQtContourExtractWidget::UpdateScalarName()
{
	ui->comboBox_ScalarDimension->clear();
	this->m_ScalarName = ui->comboBox_ScalarIndex->currentText().toStdString();
	this->m_ScalarArray = nullptr;
	for (int i = 0; i < m_PointData->GetNumberOfElements(); i++) {
		auto array = m_PointData->GetElement(i).pointer;
		if (array->GetName() == m_ScalarName) {
			m_ScalarArray = array;
			break;
		}
	}
	if (m_ScalarArray) {
		int size = m_ScalarArray->GetDimension();
		if (size < 4) {
			if (size > 0) ui->comboBox_ScalarDimension->addItem(QString::fromStdString("x"));
			if (size > 1) ui->comboBox_ScalarDimension->addItem(QString::fromStdString("y"));
			if (size > 2) ui->comboBox_ScalarDimension->addItem(QString::fromStdString("z"));
		}
		else {
			for (int i = 0; i < size; i++) {
				ui->comboBox_ScalarDimension->addItem(QString::fromStdString("D" + std::to_string(i)));
			}
		}
	}
}
void igQtContourExtractWidget::UpdateScalarDimension()
{
	this->m_ScalarDimension = ui->comboBox_ScalarDimension->currentIndex();
	ui->label_RangeInfo->clear();
	auto dataRange = this->m_OriginDataObject->GetAttributeSet()->GetAttribute(m_ScalarName).GetDataRange();
	if (dataRange) {
		double range[2] = { dataRange->GetValue(2 * m_ScalarDimension + 2),dataRange->GetValue(2 * m_ScalarDimension + 3) };
		std::string info = "Range:(" + std::to_string(range[0]) + ", " + std::to_string(range[1]) + ")\n";
		ui->label_RangeInfo->setText(QString::fromStdString(info));
	}
	else {
		ui->label_RangeInfo->setText("Data error!");
	}
}

void igQtContourExtractWidget::UpdateIsoValue()
{
	this->m_IsoValue = ui->lineEdit_IsoValue->text().toDouble();
}



void igQtContourExtractWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d)
{
	this->m_OriginDataObject = m_d;
	this->m_PointData = m_OriginDataObject->GetAttributeSet()->GetAllPointAttributes();
	InitScalarName();
	m_Generated = false;
	m_Extracter = iGame::ModelClip::New();
	m_ResultMesh = iGame::SurfaceMesh::New();
	m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_Contour");
	m_ResultMesh->SetAttributeSet(m_OriginDataObject->GetAttributeSet());
	m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [&]() -> void {
		m_Generated = false;
		this->m_OriginDataObject=nullptr;
		this->m_PointData=nullptr;
		this->m_Extracter=nullptr;
		ui->comboBox_ScalarIndex->clear();
		this->parentWidget()->hide();
		});
}

void igQtContourExtractWidget::ContourExtract()
{
	UpdateIsoValue();
	if (m_ScalarArray == nullptr) {
		return;
	}
	if (!m_Extracter) {
		m_Extracter = iGame::ModelClip::New();
	}
	auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
	auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
	auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();
	m_ResultMesh->ClearSubDataObject();
	// recover attribute
	m_ResultMesh->ViewCloudPicture(scene, -1, -1);

	if (m_OriginDataObject->HasSubDataObject()) {
		for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin(); it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
			auto childObject = it->second;
			if (childObject == nullptr) {
				continue;
			}
			m_Extracter->SetInput(childObject);
			this->m_PointData = childObject->GetAttributeSet()->GetAllPointAttributes();
			this->m_ScalarArray = nullptr;
			for (int i = 0; i < m_PointData->GetNumberOfElements(); i++) {
				auto array = m_PointData->GetElement(i).pointer;
				if (array->GetName() == m_ScalarName) {
					m_ScalarArray = array;
					break;
				}
			}
			if (m_ScalarArray) {
				m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValue, m_ScalarDimension);
				m_Extracter->Execute();
				m_ResultMesh->AddSubDataObject(m_Extracter->GetOutput());
			}
		}
	}
	else {
		m_Extracter->SetInput(m_OriginDataObject);
		m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValue, m_ScalarDimension);
		m_Extracter->Execute();
		m_ResultMesh->AddSubDataObject(m_Extracter->GetOutput());
	}


	m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex,
		oldAttributeDimension);
	//m_Extracter->SetInput(m_OriginDataObject);
	//m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValue, m_ScalarDimension);
	//m_Extracter->Execute();
	//auto output = DynamicCast<iGame::UnstructuredMesh>(m_Extracter->GetOutput());

	//m_ResultMesh->SetPoints(output->GetPoints());
	//m_ResultMesh->SetFaces(output->GetCells());
	//m_ResultMesh->SetAttributeSet(output->GetAttributeSet());
	//m_ResultMesh->BuildEdges();

	if (m_Generated) {
		UpdateContourModel(m_ResultMesh);
	}
	else {
		DrawContourModel(m_ResultMesh);
		m_Generated = true;
	}
}