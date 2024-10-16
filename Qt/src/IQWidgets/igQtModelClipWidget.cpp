#include "IQWidgets/igQtModelClipWidget.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameSceneManager.h"

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


void igQtModelClipWidget::SetPlane(float o[3], float n[3]) {
    m_Clipper->SetPlane(o, n);
    ui->lineEdit_origin_x->setText(QString::number(o[0]));
    ui->lineEdit_origin_y->setText(QString::number(o[1]));
    ui->lineEdit_origin_z->setText(QString::number(o[2]));
    ui->lineEdit_normal_x->setText(QString::number(n[0]));
    ui->lineEdit_normal_y->setText(QString::number(n[1]));
    ui->lineEdit_normal_z->setText(QString::number(n[2]));
    ClipModel();
}

void igQtModelClipWidget::UpdatePlane() {
    float o[3] = {0};
    float n[3] = {0};
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
    if (m_Generated) {
        ClipModel();
    }
}
void igQtModelClipWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d)
{
    this->m_OriginDataObject = m_d;
    m_ResultMesh = iGame::SurfaceMesh::New();
    m_ResultMesh->SetName("Clip");
    DrawClipModel(m_ResultMesh);
    m_Generated=true;
}

void igQtModelClipWidget::ClipModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
    auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();

    m_ResultMesh->ClearSubDataObject();
    // recover attribute
    m_ResultMesh->ViewCloudPicture(scene, -1, -1);

    auto Result_ClipPart = iGame::SurfaceMesh::New();
    m_Clipper->SetInput(m_OriginDataObject);
    m_Clipper->Execute();
    iGame::iGameModelGeometryFilter::Pointer surfaceextract =
            iGame::iGameModelGeometryFilter::New();
    surfaceextract->Execute(m_Clipper->GetOutput(), Result_ClipPart);

    m_ResultMesh->AddSubDataObject(Result_ClipPart);
    m_ResultMesh->SetAttributeSet(Result_ClipPart->GetAttributeSet());

    if (!m_Clipper->GetIsSlice()) {
        auto Result_ExtractPart = iGame::SurfaceMesh::New();
        double o[3];
        double n[3];
        m_Clipper->GetPlane(o, n);
        surfaceextract->SetClipPlane(o, n);
        surfaceextract->Execute(m_OriginDataObject, Result_ExtractPart);
        m_ResultMesh->AddSubDataObject(Result_ExtractPart);
    }

    m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex,
                                   oldAttributeDimension);

    if (m_Generated) {
        UpdateClipModel(m_ResultMesh);
    } else {
        DrawClipModel(m_ResultMesh);
        m_Generated = true;
    }
}