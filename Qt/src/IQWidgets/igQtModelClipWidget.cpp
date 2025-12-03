#include "IQWidgets/igQtModelClipWidget.h"
#include "ModelSurface/iGameModelGeometryFilter.h"
#include "iGameSceneManager.h"
#include "iGameThreadPool.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
igQtModelClipWidget::igQtModelClipWidget(QWidget* parent) : QWidget(parent), ui(new Ui::ModelClipWidget) {
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::clicked, this, [&]() {
        this->UpdatePlane();
        this->ClipModel();
    });
    connect(ui->radioButton_Slice, &QRadioButton::toggled, this, [&](bool isChecked) {
        if (isChecked) { this->SetViewMode(IG_SLICE_MODE); }
        ui->checkBox_Invert->hide();
    });
    connect(ui->radioButton_Clip, &QRadioButton::toggled, this, [&](bool isChecked) {
        if (isChecked) { this->SetViewMode(IG_CLIP_MODE); }
        ui->checkBox_Invert->show();
    });

    connect(ui->checkBox_Invert, &QCheckBox::toggled, this, [&](bool isChecked) { 
        this->m_Invert = isChecked;
        this->ClipModel();
        });
    connect(ui->checkBox_Crinkle, &QCheckBox::toggled, this, [&](bool isChecked) { 
        this->m_Crinkle = isChecked; 
        this->ClipModel();
        });

    connect(ui->checkBox_Preview, &QCheckBox::toggled, this, [&](bool isChecked) { GetSelection()->Preview = isChecked; });
    ui->radioButton_Slice->setChecked(true);
    ui->checkBox_Preview->setChecked(false);
    ui->checkBox_Invert->setChecked(true);
    ui->checkBox_Crinkle->setChecked(false);
    QRegularExpression rx("-?\\d*\\.?\\d+");
    ui->lineEdit_origin_x->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_origin_y->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_origin_z->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_normal_x->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_normal_y->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_normal_z->setValidator(new QRegularExpressionValidator(rx, this));
}


void igQtModelClipWidget::SetPlane(float o[3], float n[3]) {
    this->m_Origin[0] = o[0];
    this->m_Origin[1] = o[1];
    this->m_Origin[2] = o[2];
    this->m_Normal[0] = n[0];
    this->m_Normal[1] = n[1];
    this->m_Normal[2] = n[2];
    ui->lineEdit_origin_x->setText(QString::number(o[0]));
    ui->lineEdit_origin_y->setText(QString::number(o[1]));
    ui->lineEdit_origin_z->setText(QString::number(o[2]));
    ui->lineEdit_normal_x->setText(QString::number(n[0]));
    ui->lineEdit_normal_y->setText(QString::number(n[1]));
    ui->lineEdit_normal_z->setText(QString::number(n[2]));
    ClipModel();
}

void igQtModelClipWidget::SetPlane(iGame::Vector3d p, iGame::Vector3d normal) {
    float o[3], n[3];
    o[0] = p[0];
    o[1] = p[1];
    o[2] = p[2];
    n[0] = normal[0];
    n[1] = normal[1];
    n[2] = normal[2];
    SetPlane(o, n);
}

void igQtModelClipWidget::UpdatePlane() {
    this->m_Origin[0] = ui->lineEdit_origin_x->text().toFloat();
    this->m_Origin[1] = ui->lineEdit_origin_y->text().toFloat();
    this->m_Origin[2] = ui->lineEdit_origin_z->text().toFloat();
    this->m_Normal[0] = ui->lineEdit_normal_x->text().toFloat();
    this->m_Normal[1] = ui->lineEdit_normal_y->text().toFloat();
    this->m_Normal[2] = ui->lineEdit_normal_z->text().toFloat();

    m_Selection->PlanePoint[0] = m_Origin[0];
    m_Selection->PlanePoint[1] = m_Origin[1];
    m_Selection->PlanePoint[2] = m_Origin[2];
    m_Selection->PlaneNormal[0] = m_Normal[0];
    m_Selection->PlaneNormal[1] = m_Normal[1];
    m_Selection->PlaneNormal[2] = m_Normal[2];
    m_Selection->UpdatePlane();
}
void igQtModelClipWidget::SetViewMode(ViewMode vm) {
    this->m_ViewMode = vm;
    if (this->m_OriginDataObject) { ClipModel(); }
}
void igQtModelClipWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    this->m_OriginDataObject = m_d;
    m_ResultMesh = iGame::UnstructuredMesh::New();
    m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_Clip");
    m_ResultMesh->SetAttributeSet(m_d->GetAttributeSet());
    DrawClipModel(m_ResultMesh);
    m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [&]() -> void {
        this->m_OriginDataObject = nullptr;
        this->parentWidget()->hide();
        ResetInteractor();
    });
}

iGame::ClipSelection::Pointer igQtModelClipWidget::GetSelection() {
    if (m_Selection == nullptr) {
        m_Selection = iGame::ClipSelection::New();
        m_Selection->SetSelectionCallBackEvent(
                [&](IGenum itemType, const std::vector<igIndex>& ids, iGame::Selection::Operate ope) {
                    if (itemType != IG_CHANGE) return;
                    SetPlane(m_Selection->PlanePoint, m_Selection->PlaneNormal);
                },
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }
    return m_Selection;
}

void igQtModelClipWidget::ClipModel() {
    if (!this->m_OriginDataObject) return;
    switch (m_ViewMode) {
        case igQtModelClipWidget::IG_CLIP_MODE: {
            clock_t time_1 = clock();
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
            auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();
            m_ResultMesh->ClearSubDataObject();
            // recover attribute
            m_ResultMesh->ViewCloudPicture(scene, -1, -1);
            m_ResultMesh->SetColorMapper(m_OriginDataObject->GetColorMapper());
            std::cout << m_ResultMesh->GetColorMapper() << '\n';
            auto Clipper = iGame::ClipFilter::New();
            Clipper->SetPlane(m_Origin, m_Normal);
            if (m_OriginDataObject->HasSubDataObject()) {
                for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
                     it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
                    auto childObject = it->second;
                    if (childObject == nullptr) { continue; }
                    Clipper->SetInput(childObject);
                    Clipper->SetCrinkle(m_Crinkle);
                    Clipper->SetInvert(m_Invert);
                    Clipper->Execute();
                    auto out = Clipper->GetClipMesh();
                    if (out) { m_ResultMesh->AddSubDataObject(out); }
                }
            } else {
                Clipper->SetInput(m_OriginDataObject);
                Clipper->SetCrinkle(m_Crinkle);
                Clipper->SetInvert(m_Invert);
                Clipper->Execute();
                auto out = Clipper->GetClipMesh();
                if (out) {
                    m_ResultMesh->SetPoints(out->GetPoints());
                    m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
                    m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
                }
            }
            clock_t time_clip = clock();
            //std::cout << "clip cost " << time_clip - time_1 << '\n';
            m_ResultMesh->SetViewStyle(m_ResultMesh->GetViewStyle());
            m_ResultMesh->ConvertToDrawableData();
            m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);
            auto time_view = clock();
            //std::cout << "all time  " << time_view - time_1 << "\n";
            UpdateClipModel(m_ResultMesh);
        } break;
        case igQtModelClipWidget::IG_SLICE_MODE: {
            clock_t time_1 = clock();
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
            auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();
            m_ResultMesh->ClearSubDataObject();
            // recover attribute
            m_ResultMesh->ViewCloudPicture(scene, -1, -1);
            m_ResultMesh->SetColorMapper(m_OriginDataObject->GetColorMapper());
            auto Slicer = iGame::SliceFilter::New();
            if (m_OriginDataObject->HasSubDataObject()) {
                for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
                     it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
                    auto childObject = it->second;
                    if (childObject == nullptr) { continue; }
                    Slicer->SetInput(childObject);
                    Slicer->SetPlane(m_Origin, m_Normal);
                    Slicer->SetCrinkle(m_Crinkle);
                    Slicer->Execute();
                    auto out = Slicer->GetSliceMesh();
                    if (out) { m_ResultMesh->AddSubDataObject(out); }
                }
            } else {
                Slicer->SetInput(m_OriginDataObject);
                Slicer->SetPlane(m_Origin, m_Normal);
                Slicer->SetCrinkle(m_Crinkle);
                Slicer->Execute();
                auto out = Slicer->GetSliceMesh();
                if (out) {
                    m_ResultMesh->SetPoints(out->GetPoints());
                    m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
                    m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
                }
            }
            clock_t time_clip = clock();
            //std::cout << "clip cost " << time_clip - time_1 << '\n';
            m_ResultMesh->SetViewStyle(m_ResultMesh->GetViewStyle());
            m_ResultMesh->ConvertToDrawableData();
            m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);
            auto time_view = clock();
            //std::cout << "all time  " << time_view - time_1 << "\n";
            UpdateClipModel(m_ResultMesh);
        } break;
        default:
            break;
    }
}
