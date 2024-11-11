#include "IQWidgets/igQtModelClipWidget.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameSceneManager.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
igQtModelClipWidget::igQtModelClipWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::ModelClipWidget) {


    ui->setupUi(this);

    m_Generated = false;
    m_Clipper = iGame::ModelClip::New();
    connect(ui->pushButton, &QPushButton::clicked, this, [&]() {
        this->UpdatePlane();
        this->ClipModel();
    });
    connect(ui->radioButton_Slice, &QRadioButton::toggled, this,
            [&](bool isChecked) {
                this->SetIsSlice(ui->radioButton_Slice->isChecked());
            });
    ui->radioButton_Slice->setChecked(true);

    QRegularExpression  rx("-?\\d*\\.?\\d+");
    ui->lineEdit_origin_x->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_origin_y->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_origin_z->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_normal_x->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_normal_y->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_normal_z->setValidator(new QRegularExpressionValidator(rx, this));

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
void igQtModelClipWidget::SetIsSlice(bool s) {
    m_Clipper->SetIsSlice(s);
    if (m_Generated) {
        ClipModel();
    }
}
void igQtModelClipWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    this->m_OriginDataObject = m_d;
    m_ResultMesh = iGame::SurfaceMesh::New();
    m_ResultMesh->SetName(m_OriginDataObject->GetName()+"_Clip");
    m_ResultMesh->SetAttributeSet(m_d->GetAttributeSet());
    DrawClipModel(m_ResultMesh);
    m_Generated=true;
    m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [&]() -> void {
        this->m_OriginDataObject=nullptr;
        this->parentWidget()->hide();
        this->m_Generated = false;
        ResetInteractor();
        });
}

void igQtModelClipWidget::ClipModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
    auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();
    m_ResultMesh->ClearSubDataObject();
    // recover attribute
    m_ResultMesh->ViewCloudPicture(scene, -1, -1);


    if (m_OriginDataObject->HasSubDataObject()) {
        for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin(); it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
            auto childObject=it->second;
            if (childObject == nullptr) {
                continue;
            }
            auto Result_ClipPart = iGame::SurfaceMesh::New();
            m_Clipper->SetInput(childObject);
            m_Clipper->Execute();
            if (m_Clipper->GetIsSlice() == false) {
               auto out= m_Clipper->GetOutput();
               for (auto it = out->SubDataObjectIteratorBegin(); it != out->SubDataObjectIteratorEnd(); it++) {
                   m_ResultMesh->AddSubDataObject(it->second);
               }
            }
            else {
                m_ResultMesh->AddSubDataObject(m_Clipper->GetOutput());
            }
        }
    }
    else {
        auto Result_ClipPart = iGame::SurfaceMesh::New();
        m_Clipper->SetInput(m_OriginDataObject);
        m_Clipper->Execute();
        if (m_Clipper->GetIsSlice() == false) {
            auto out = m_Clipper->GetOutput();
            for (auto it = out->SubDataObjectIteratorBegin(); it != out->SubDataObjectIteratorEnd(); it++) {
                m_ResultMesh->AddSubDataObject(it->second);
            }
        }
        else {
            m_ResultMesh->AddSubDataObject(m_Clipper->GetOutput());
        }
 /*       iGame::iGameModelGeometryFilter::Pointer surfaceextract =
            iGame::iGameModelGeometryFilter::New();
        surfaceextract->Execute(m_Clipper->GetOutput(), Result_ClipPart);
        if (Result_ClipPart) {
            Result_ClipPart->SetViewStyle(m_ResultMesh->GetViewStyle());
            Result_ClipPart->ConvertToDrawableData();
            m_ResultMesh->AddSubDataObject(Result_ClipPart);
        }

        if (!m_Clipper->GetIsSlice()) {
            auto Result_ExtractPart = iGame::SurfaceMesh::New();
            double o[3];
            double n[3];
            m_Clipper->GetPlane(o, n);
            surfaceextract->SetClipPlane(o, n);
            surfaceextract->Execute(m_OriginDataObject, Result_ExtractPart);
            if (Result_ExtractPart) {
                Result_ExtractPart->SetViewStyle(m_ResultMesh->GetViewStyle());
                Result_ExtractPart->ConvertToDrawableData();
                m_ResultMesh->AddSubDataObject(Result_ExtractPart);
            }
        }*/
    }

    m_ResultMesh->SetViewStyle(m_ResultMesh->GetViewStyle());
    m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex,
                                   oldAttributeDimension);

    UpdateClipModel(m_ResultMesh);

}