#include "IQWidgets/igQtModelClipWidget.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameSceneManager.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include "iGameThreadPool.h"
igQtModelClipWidget::igQtModelClipWidget(QWidget* parent) : QWidget(parent), ui(new Ui::ModelClipWidget) {
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::clicked, this, [&]() {
        this->UpdatePlane();
        this->ClipModel();
    });
    connect(ui->radioButton_Slice, &QRadioButton::toggled, this, [&](bool isChecked) {
        if (isChecked) { this->SetViewMode(IG_SLICE_MODE); }
    });
    connect(ui->radioButton_Clip, &QRadioButton::toggled, this, [&](bool isChecked) {
        if (isChecked) { this->SetViewMode(IG_CLIP_MODE); }
    });
    connect(ui->radioButton_Mesh, &QRadioButton::toggled, this, [&](bool isChecked) {
        if (isChecked) { this->SetViewMode(IG_MESH_MODE); }
    });
    ui->radioButton_Mesh->setChecked(true);

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

void igQtModelClipWidget::UpdatePlane() {
    this->m_Origin[0] = ui->lineEdit_origin_x->text().toFloat();
    this->m_Origin[1] = ui->lineEdit_origin_y->text().toFloat();
    this->m_Origin[2] = ui->lineEdit_origin_z->text().toFloat();
    this->m_Normal[0] = ui->lineEdit_normal_x->text().toFloat();
    this->m_Normal[1] = ui->lineEdit_normal_y->text().toFloat();
    this->m_Normal[2] = ui->lineEdit_normal_z->text().toFloat();
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
            auto m_Clipper = iGame::QuickModelClip::New();
            m_Clipper->SetPlane(m_Origin, m_Normal);
            if (m_OriginDataObject->HasSubDataObject()) {
                for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
                     it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
                    auto childObject = it->second;
                    if (childObject == nullptr) { continue; }
                    m_Clipper->SetInput(childObject);
                    m_Clipper->Execute();
                    auto out = m_Clipper->GetClipMesh();
                    if (out) { m_ResultMesh->AddSubDataObject(out); }
                }
            } else {
                m_Clipper->SetInput(m_OriginDataObject);
                m_Clipper->Execute();
                auto out = m_Clipper->GetClipMesh();
                if (out) {
                    m_ResultMesh->SetPoints(out->GetPoints());
                    m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
                    m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
                }
            }
            clock_t time_clip = clock();
            std::cout << "clip cost " << time_clip - time_1 << '\n';
            m_ResultMesh->SetViewStyle(m_ResultMesh->GetViewStyle());
            m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);
            m_ResultMesh->ConvertToDrawableData();
            auto time_view = clock();
            std::cout << "all time  " << time_view - time_1 << "\n";
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
            auto m_Contourer = iGame::ContourFilter::New();
            if (m_OriginDataObject->HasSubDataObject()) {
                for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
                     it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
                    auto childObject = it->second;
                    if (childObject == nullptr) { continue; }
                    m_Contourer->SetInput(childObject);
                    m_Contourer->SetPlane(m_Origin, m_Normal);
                    m_Contourer->Execute();
                    auto out = m_Contourer->GetContourMesh();
                    if (out) { m_ResultMesh->AddSubDataObject(out); }
                }
            } else {
                m_Contourer->SetInput(m_OriginDataObject);
                m_Contourer->SetPlane(m_Origin, m_Normal);
                m_Contourer->Execute();
                auto out = m_Contourer->GetContourMesh();
                if (out) {
                    m_ResultMesh->SetPoints(out->GetPoints());
                    m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
                    m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
                }
            }
            clock_t time_clip = clock();
            std::cout << "clip cost " << time_clip - time_1 << '\n';
            m_ResultMesh->SetViewStyle(m_ResultMesh->GetViewStyle());
            m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);
            m_ResultMesh->ConvertToDrawableData();
            auto time_view = clock();
            std::cout << "all time  " << time_view - time_1 << "\n";
            UpdateClipModel(m_ResultMesh);
        } break;
        case igQtModelClipWidget::IG_MESH_MODE: {
            clock_t time_1 = clock();
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
            auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();
            m_ResultMesh->ClearSubDataObject();
            // recover attribute
            m_ResultMesh->ViewCloudPicture(scene, -1, -1);
            auto m_Extracter = iGame::iGameModelGeometryFilter::New();
            m_Extracter->SetClipPlane(m_Origin, m_Normal);
            if (m_OriginDataObject->HasSubDataObject()) {
                for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
                     it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
                    auto childObject = it->second;
                    if (childObject == nullptr) { continue; }
                    m_Extracter->SetInput(childObject);
                    m_Extracter->Execute();
                    auto out = m_Extracter->GetExtractMesh();
                    if (out) { m_ResultMesh->AddSubDataObject(out); }
                }
            } else {
                m_Extracter->SetInput(m_OriginDataObject);
                m_Extracter->Execute();
                auto out = m_Extracter->GetExtractMesh();
                if (out) { m_ResultMesh->GenerateFromSurfaceMesh(out); }
            }
            clock_t time_clip = clock();
            std::cout << "clip cost " << time_clip - time_1 << '\n';
            m_ResultMesh->SetViewStyle(m_ResultMesh->GetViewStyle());
            m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);
            m_ResultMesh->ConvertToDrawableData();
            auto time_view = clock();
            std::cout << "all time  " << time_view - time_1 << "\n";
            UpdateClipModel(m_ResultMesh);
        } break;
        default:
            break;
    }
}