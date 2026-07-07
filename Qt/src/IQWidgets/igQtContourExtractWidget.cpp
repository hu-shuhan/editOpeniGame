#include "IQWidgets/igQtContourExtractWidget.h"
#include "iGameCellType.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameSmartPointer.h"

#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace {

int FindAttributeIndex(iGame::AttributeSet::Pointer attrSet, const std::string& name) {
    if (!attrSet) { return -1; }
    auto attrs = attrSet->GetAllAttributes();
    for (int i = 0; i < attrs->GetNumberOfElements(); ++i) {
        if (attrs->GetElement(i).pointer->GetName() == name) { return i; }
    }
    return -1;
}

void ApplyContourDisplayStyle(iGame::UnstructuredMesh::Pointer mesh) {
    if (!mesh) { return; }
    bool hasLine = false;
    bool hasSurface = false;
    for (IGsize i = 0; i < mesh->GetNumberOfCells(); ++i) {
        switch (mesh->GetCellType(i)) {
            case iGame::IG_LINE:
            case iGame::IG_POLY_LINE:
                hasLine = true;
                break;
            case iGame::IG_TRIANGLE:
            case iGame::IG_QUAD:
            case iGame::IG_POLYGON:
                hasSurface = true;
                break;
            default:
                break;
        }
    }
    IGenum style = 0;
    if (hasLine) { style |= IG_WIREFRAME; }
    if (hasSurface) { style |= IG_SURFACE; }
    if (style == 0) { style = IG_WIREFRAME | IG_SURFACE; }
    mesh->SetViewStyle(style);
}

void FinalizeContourResult(iGame::UnstructuredMesh::Pointer mesh, iGame::DataObject::Pointer origin,
                           iGame::Scene::Pointer scene, const std::string& scalarName, int scalarDimension) {
    if (!mesh || !origin || !scene) { return; }
    mesh->SetColorMapper(origin->GetColorMapper());
    ApplyContourDisplayStyle(mesh);
    const int attrIndex = FindAttributeIndex(mesh->GetAttributeSet(), scalarName);
    mesh->ForceReConvertToDrawableData();
    mesh->ConvertToDrawableData();
    mesh->ViewCloudPicture(scene, attrIndex, scalarDimension);
}

} // namespace

igQtContourExtractWidget::igQtContourExtractWidget(QWidget* parent) : QWidget(parent), ui(new Ui::ContourExtract) {


    ui->setupUi(this);
    ui->label_RangeInfo->clear();
    m_Generated = false;
    m_Extracter = nullptr;
    m_PointData = nullptr;
    QRegularExpression rx("-?\\d*\\.?\\d+");
    ui->lineEdit_IsoValue->setValidator(new QRegularExpressionValidator(rx, this));
    connect(ui->btnExecute, &QPushButton::clicked, this, &igQtContourExtractWidget::ContourExtract);
    connect(ui->comboBox_ScalarIndex, &QComboBox::currentTextChanged, this,
            &igQtContourExtractWidget::UpdateScalarName);
    connect(ui->comboBox_ScalarDimension, &QComboBox::currentTextChanged, this,
            &igQtContourExtractWidget::UpdateScalarDimension);
}


void igQtContourExtractWidget::InitScalarName() {
    ui->comboBox_ScalarIndex->clear();
    if (!m_PointData) { return; }
    for (int i = 0; i < m_PointData->GetNumberOfElements(); i++) {
        auto array = m_PointData->GetElement(i).pointer;
        ui->comboBox_ScalarIndex->addItem(QString::fromStdString(array->GetName()));
    }
}
void igQtContourExtractWidget::UpdateScalarName() {
    ui->comboBox_ScalarDimension->clear();
    this->m_ScalarName = ui->comboBox_ScalarIndex->currentText().toStdString();
    this->m_ScalarArray = nullptr;
    if (!m_PointData) { return; }
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
        } else {
            for (int i = 0; i < size; i++) {
                ui->comboBox_ScalarDimension->addItem(QString::fromStdString("D" + std::to_string(i)));
            }
        }
    }
}
void igQtContourExtractWidget::UpdateScalarDimension() {
    this->m_ScalarDimension = ui->comboBox_ScalarDimension->currentIndex();
    ui->label_RangeInfo->clear();
    if (!m_OriginDataObject || m_ScalarName.empty()) { return; }
    auto dataRange = this->m_OriginDataObject->GetAttributeSet()->GetAttribute(m_ScalarName).GetDataRange();
    if (dataRange) {
        double range[2] = {dataRange->GetValue(2 * m_ScalarDimension + 2),
                           dataRange->GetValue(2 * m_ScalarDimension + 3)};
        std::string info = "Range:(" + std::to_string(range[0]) + ", " + std::to_string(range[1]) + ")\n";
        ui->label_RangeInfo->setText(QString::fromStdString(info));
    } else {
        ui->label_RangeInfo->setText("Data error!");
    }
}

void igQtContourExtractWidget::UpdateIsoValue() { this->m_IsoValue = ui->lineEdit_IsoValue->text().toDouble(); }


void igQtContourExtractWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    if (m_ResultMesh && m_ResultObserverTag) {
        m_ResultMesh->RemoveObserver(m_ResultObserverTag);
        m_ResultObserverTag = 0;
    }

    this->m_OriginDataObject = m_d;
    if (!m_OriginDataObject) { return; }
    this->m_PointData = m_OriginDataObject->GetAttributeSet()->GetAllPointAttributes();
    InitScalarName();
    UpdateScalarName();
    if (ui->comboBox_ScalarDimension->count() > 0) {
        ui->comboBox_ScalarDimension->setCurrentIndex(0);
    }
    UpdateScalarDimension();
    m_Generated = false;
    m_Extracter = iGame::ContourFilter::New();
    m_ResultMesh = iGame::UnstructuredMesh::New();
    m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_Contour");
    m_ResultMesh->SetAttributeSet(m_OriginDataObject->GetAttributeSet());
    // 场景/模型树移除轮廓结果时会 Invoke DeleteEvent；不应关闭工具面板或清空源网格，
    // 否则用户删除结果模型后无法在同一面板内再次执行提取。
    m_ResultObserverTag = m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [this]() -> void {
        m_Generated = false;
        m_ResultObserverTag = 0;
    });
}

void igQtContourExtractWidget::ContourExtract() {
    UpdateIsoValue();
    UpdateScalarName();
    UpdateScalarDimension();
    if (!m_OriginDataObject || !m_ResultMesh || m_ScalarArray == nullptr) { return; }
    if (!m_Extracter) { m_Extracter = iGame::ContourFilter::New(); }
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (!scene) { return; }
    m_ResultMesh->ClearSubDataObject();

    if (m_OriginDataObject->HasSubDataObject()) {
        for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
             it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
            auto childObject = it->second;
            if (childObject == nullptr) { continue; }
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
            if (!m_ScalarArray) { continue; }
            m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValue, m_ScalarDimension);
            if (!m_Extracter->Execute()) { continue; }
            auto out = m_Extracter->GetContourMesh();
            if (out) {
                out->SetColorMapper(m_OriginDataObject->GetColorMapper());
                ApplyContourDisplayStyle(out);
                m_ResultMesh->AddSubDataObject(out);
            }
        }
    } else {
        m_Extracter->SetInput(m_OriginDataObject);
        m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValue, m_ScalarDimension);
        if (!m_Extracter->Execute()) { return; }
        auto out = m_Extracter->GetContourMesh();
        if (!out || out->GetNumberOfCells() == 0) { return; }
        m_ResultMesh->SetPoints(out->GetPoints());
        m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
        m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
    }

    if (m_ResultMesh->GetNumberOfCells() == 0 && !m_ResultMesh->HasSubDataObject()) { return; }

    FinalizeContourResult(m_ResultMesh, m_OriginDataObject, scene, m_ScalarName, m_ScalarDimension);
    scene->Update();

    if (m_Generated) {
        UpdateContourModel(m_ResultMesh);
    } else {
        DrawContourModel(m_ResultMesh);
        m_Generated = true;
    }
}
