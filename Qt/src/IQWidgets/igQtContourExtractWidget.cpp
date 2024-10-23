#include "IQWidgets/igQtContourExtractWidget.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameSceneManager.h"
#include "iGameScene.h"
#include "iGameSmartPointer.h"
igQtContourExtractWidget::igQtContourExtractWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::ContourExtract) {


	ui->setupUi(this);

	m_Generated = false;
	m_Extracter = nullptr;
	m_PointData = nullptr;
	connect(ui->btnExecute, &QPushButton::clicked, this, &igQtContourExtractWidget::ContourExtract);
	connect(ui->comboBox_ScalarIndex, &QComboBox::currentTextChanged, this, &igQtContourExtractWidget::UpdateScalarName);
	connect(ui->comboBox_ScalarDimension, &QComboBox::currentTextChanged, this, &igQtContourExtractWidget::UpdateScalarDimension);
}


void igQtContourExtractWidget::InitScalarName()
{
	ui->comboBox_ScalarIndex->clear();
	for (int i = 0; i < m_PointData->GetNumberOfElements(); i++) {
		auto attr = m_PointData->GetElement(i).pointer;
		ui->comboBox_ScalarIndex->addItem(QString::fromStdString(attr->GetName()));
	}
}
void igQtContourExtractWidget::UpdateScalarName()
{
	ui->comboBox_ScalarDimension->clear();
	this->m_ScalarName = ui->comboBox_ScalarIndex->currentText().toStdString();
	this->m_ScalarArray = nullptr;
	for (int i = 0; i < m_PointData->GetNumberOfElements(); i++) {
		auto attr = m_PointData->GetElement(i).pointer;
		if (attr->GetName() == m_ScalarName) {
			m_ScalarArray = attr;
			break;
		}
	}
	if (m_ScalarArray) {
		int size = m_ScalarArray->GetDimension();
		if (size < 5) {
			if (size > 1) ui->comboBox_ScalarDimension->addItem(QString::fromStdString("magnitude"));
			if (size > 0) ui->comboBox_ScalarDimension->addItem(QString::fromStdString("x"));
			if (size > 1) ui->comboBox_ScalarDimension->addItem(QString::fromStdString("y"));
			if (size > 2) ui->comboBox_ScalarDimension->addItem(QString::fromStdString("z"));
			if (size > 3) ui->comboBox_ScalarDimension->addItem(QString::fromStdString("u"));
		}
		else {
			ui->comboBox_ScalarDimension->addItem(QString::fromStdString("magnitude"));
			for (int i = 0; i < size; i++) {
				ui->comboBox_ScalarDimension->addItem(QString::fromStdString("D" + std::to_string(i)));
			}
		}
	}
}
void igQtContourExtractWidget::UpdateScalarDimension()
{
	this->m_ScalarDimension = ui->comboBox_ScalarDimension->currentIndex() - 1;
	ui->label_RangeInfo->clear();
	auto mapper=iGame::ScalarsToColors::New();
	mapper->InitRange(m_ScalarArray,this->m_ScalarDimension);
	float* range=mapper->GetRange();
	std::string info= "Range:(" +std::to_string( range[0]) + ", " + std::to_string(range[1]) + ")\n";
	ui->label_RangeInfo->setText(QString::fromStdString(info));
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
	m_ResultMesh=iGame::SurfaceMesh::New();
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
	m_Extracter->SetInput(m_OriginDataObject);
	m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValue, m_ScalarDimension);
	m_Extracter->Execute();
	auto output= DynamicCast<iGame::UnstructuredMesh>(m_Extracter->GetOutput());

	m_ResultMesh->SetPoints(output->GetPoints());
	m_ResultMesh->SetFaces(output->GetCells());
	m_ResultMesh->SetAttributeSet(output->GetAttributeSet());
	m_ResultMesh->BuildEdges();

	if (m_Generated) {
		m_ResultMesh->ConvertToDrawableData();
	 UpdateContourModel(m_ResultMesh);
	}
	else {
		DrawContourModel(m_ResultMesh);
		m_Generated = true;
	}
}