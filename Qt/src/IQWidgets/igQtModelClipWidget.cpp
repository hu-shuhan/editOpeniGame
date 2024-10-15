#include "IQWidgets/igQtModelClipWidget.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"

igQtModelClipWidget::igQtModelClipWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::Form) {


	ui->setupUi(this);

	m_Generated = false;
	m_Clipper = iGame::ModelClip::New();
	connect(ui->pushButton, &QPushButton::clicked, this,
		[&]() {
			this->UpdatePlane();
			this->ClipModel();
		});
	connect(ui->radioButton_Slice, &QRadioButton::toggled, this, [&](bool isChecked) {
		this->SetIsSlice(ui->radioButton_Slice->isChecked());
		});
	ui->radioButton_Slice->setChecked(true);
}


void igQtModelClipWidget::SetPlane(float o[3], float n[3])
{

	ui->lineEdit_origin_y->setText(QString::fromStdString(std::to_string(o[1])));
	ui->lineEdit_normal_x->setText(QString::fromStdString(std::to_string(n[0])));
	ui->lineEdit_normal_y->setText(QString::fromStdString(std::to_string(n[1])));
	ui->lineEdit_normal_z->setText(QString::fromStdString(std::to_string(n[2])));

	m_Clipper->SetPlane(o, n);
	ClipModel();
	ClipModel();
    ui->lineEdit_origin_y->setText(QString::number(o[1]));
    ui->lineEdit_origin_z->setText(QString::number(o[2]));
    ui->lineEdit_normal_x->setText(QString::number(n[0]));
    ui->lineEdit_normal_y->setText(QString::number(n[1]));
    ui->lineEdit_normal_z->setText(QString::number(n[2]));

}

void igQtModelClipWidget::UpdatePlane()
{
	float o[3] = { 0 };
	float n[3] = { 0 };
	o[0] = ui->lineEdit_origin_x->text().toFloat();
	o[1] = ui->lineEdit_origin_y->text().toFloat();
	o[2] = ui->lineEdit_origin_z->text().toFloat();
	n[0] = ui->lineEdit_normal_x->text().toFloat();
	n[1] = ui->lineEdit_normal_y->text().toFloat();
	n[2] = ui->lineEdit_normal_z->text().toFloat();
	m_Clipper->SetPlane(o, n);
}
void igQtModelClipWidget::SetIsSlice(bool s)
{
	m_Clipper->SetIsSlice(s);
    DrawClipModel(m_ResultMesh);
	m_Generated = false;

void igQtModelClipWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d)
{
	this->m_OriginDataObject = m_d;
	m_ResultMesh = iGame::SurfaceMesh::New();
	m_Drawed = false;
}

void igQtModelClipWidget::ClipModel() {
<<<<<<< HEAD
	auto tmp1= iGame::SurfaceMesh::New();
=======

	auto Result_ClipPart= iGame::SurfaceMesh::New();
>>>>>>> 540d4b2a3d9f5542113e1b2997bbd81eabea50ee
	m_ResultMesh->ClearSubDataObject();

	m_Clipper->SetInput(m_OriginDataObject);
	m_Clipper->Execute();

	iGame::iGameModelGeometryFilter::Pointer surfaceextract =
		iGame::iGameModelGeometryFilter::New();
	surfaceextract->Execute(m_Clipper->GetOutput(), Result_ClipPart);
	Result_ClipPart->ConvertToDrawableData();
	m_ResultMesh->AddSubDataObject(Result_ClipPart);
	//m_ResultMesh=Result_ClipPart;

		surfaceextract->Execute(m_OriginDataObject, tmp);
		m_ResultMesh->AddSubDataObject(tmp);
		tmp->ConvertToDrawableData();
        
	}
	tmp1->ConvertToDrawableData();
	m_ResultMesh->AddSubDataObject(tmp1);
	m_ResultMesh=tmp1;
    //m_ResultMesh->ConvertToDrawableData();
    UpdateClipModel(m_ResultMesh);
}
		surfaceextract->Execute(m_OriginDataObject, Result_ExtractPart);
		m_ResultMesh->AddSubDataObject(Result_ExtractPart);
		Result_ExtractPart->ConvertToDrawableData();
	}

	if (m_Generated) {
		//m_ResultMesh->ConvertToDrawableData();
		UpdateClipModel(m_ResultMesh);
	}
	else {
		//m_ResultMesh->ConvertToDrawableData();
		DrawClipModel(m_ResultMesh);
		//DrawClipModel(tmp);
		m_Generated = true;
	}

}	else {
		//m_ResultMesh->ConvertToDrawableData();
		DrawClipModel(m_ResultMesh);
		//DrawClipModel(tmp);
		m_Drawed = true;
	}

}
