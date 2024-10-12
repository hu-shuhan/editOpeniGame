#include "IQWidgets/igQtModelClipWidget.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"

igQtModelClipWidget::igQtModelClipWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::Form) {

	ui->setupUi(this);
	m_Clipper = iGame::ModelClip::New();
    connect(ui->pushButton, &QPushButton::clicked, this,
            &igQtModelClipWidget::ClipModel);
}


void igQtModelClipWidget::SetPlane(float o[3], float n[3])
{
	m_Clipper->SetPlane(o, n);
    ui->lineEdit_origin_x->setText(QString::number(o[0]));
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

	o[0] = .0;
	o[1] = .0;
	o[2] = .0;
	n[0] = .0;
	n[1] = .0;
	n[2] = .0;

	m_Clipper->SetPlane(o, n);
}
void igQtModelClipWidget::SetIsSlice(bool s)
{
	m_Clipper->SetIsSlice(s);
}

void igQtModelClipWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d)
{
    this->m_OriginDataObject = m_d;
}

void igQtModelClipWidget::ClipModel() {
    m_Clipper->SetInput(m_OriginDataObject);
    m_Clipper->Execute();
        iGame::iGameModelGeometryFilter::Pointer surfaceextract =
                iGame::iGameModelGeometryFilter::New();
    surfaceextract->Execute(m_Clipper->GetOutput(), m_ResultMesh);
        //m_ResultMesh = m_Clipper->GetOutput();
    auto tmp = iGame::SurfaceMesh::New();
    double o[3];
    double n[3];
    m_Clipper->GetPlane(o, n);
    surfaceextract->SetClipPlane(o, n);
    surfaceextract->Execute(m_OriginDataObject, tmp);
    m_ResultMesh->AddSubDataObject(tmp);
    DrawClipModel(m_ResultMesh);
    }