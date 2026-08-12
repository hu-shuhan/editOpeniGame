#include "IQWidgets/igQtContourExtractWidget.h"
#include "ModelSurface/iGameModelGeometryFilter.h"
#include "iGameProgressObserver.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameSmartPointer.h"

#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace {
unsigned int ContourViewStyle(const iGame::UnstructuredMesh::Pointer& mesh) {
    if (!mesh) { return IG_SURFACE; }
    unsigned int style = 0;
    const auto cellNum = mesh->GetNumberOfCells();
    for (auto i = decltype(cellNum){0}; i < cellNum; ++i) {
        const int dim = iGame::Cell::GetCellDimension(mesh->GetCellType(i));
        if (dim == 1) {
            style |= IG_WIREFRAME;
        } else if (dim >= 2) {
            style |= IG_SURFACE;
        }
        if ((style & IG_WIREFRAME) && (style & IG_SURFACE)) { break; }
    }
    return style ? style : IG_SURFACE;
}

constexpr float kContourLineWidth = 2.5f;
const igm::vec3 kContourLineColor{0.55f, 0.55f, 0.55f};

void ApplyContourAppearance(const iGame::UnstructuredMesh::Pointer& mesh) {
    if (!mesh) { return; }
    mesh->SetShellRenderingOption(false);
    mesh->SetViewStyle(ContourViewStyle(mesh));
    mesh->SetLineWidth(kContourLineWidth);
    mesh->SetLineColor(kContourLineColor);
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
    for (int i = 0; i < m_PointData->GetNumberOfElements(); i++) {
        auto array = m_PointData->GetElement(i).pointer;
        ui->comboBox_ScalarIndex->addItem(QString::fromStdString(array->GetName()));
    }
}
void igQtContourExtractWidget::UpdateScalarName() {
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
    this->m_OriginDataObject = m_d;
    this->m_PointData = m_OriginDataObject->GetAttributeSet()->GetAllPointAttributes();
    InitScalarName();
    m_Generated = false;
    m_Extracter = iGame::ContourFilter::New();
    m_ResultMesh = iGame::UnstructuredMesh::New();
    m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_Contour");
    m_ResultMesh->SetAttributeSet(m_OriginDataObject->GetAttributeSet());
    m_ResultMesh->SetShellRenderingOption(false);
    // 场景/模型树移除轮廓结果时会 Invoke DeleteEvent；不应关闭工具面板或清空源网格，
    // 否则用户删除结果模型后无法在同一面板内再次执行提取。
    m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [&]() -> void { m_Generated = false; });
}

void igQtContourExtractWidget::ContourExtract() {
    UpdateIsoValue();
    if (m_ScalarArray == nullptr) { return; }
    if (!m_Extracter) { m_Extracter = iGame::ContourFilter::New(); }
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
    auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();
    m_ResultMesh->ClearSubDataObject();
    // recover attribute
    m_ResultMesh->ViewCloudPicture(scene, -1, -1);

    auto progressObserver = iGame::ProgressObserver::Instance();
    progressObserver->UpdateText("轮廓提取中");
    progressObserver->UpdateProgress(0.0);
    auto finishProgress = [progressObserver]() {
        progressObserver->UpdateText("");
        progressObserver->UpdateProgress(1.0);
    };

    auto restoreView = [&]() {
        finishProgress();
        m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);
    };
    auto selectedScalar = m_ScalarArray;

    if (m_OriginDataObject->HasSubDataObject()) {
        int blockCount = 0;
        for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
             it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
            if (it->second) { ++blockCount; }
        }
        const double blockSlice = blockCount > 0 ? 1.0 / blockCount : 1.0;
        int blockIndex = 0;

        for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
             it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
            auto childObject = it->second;
            if (childObject == nullptr) { continue; }
            m_Extracter->SetProgressRange(blockIndex * blockSlice, blockSlice);
            progressObserver->UpdateText("轮廓提取中 " + std::to_string(blockIndex + 1) + " / " +
                                         std::to_string(blockCount));
            ++blockIndex;
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
                if (!m_Extracter->Execute()) { continue; }
                auto subOut = m_Extracter->GetContourMesh();
                if (!subOut || subOut->GetNumberOfCells() == 0) { continue; }
                ApplyContourAppearance(subOut);
                m_ResultMesh->AddSubDataObject(subOut);
            }
        }
        m_ScalarArray = selectedScalar;
        if (!m_ResultMesh->HasSubDataObject()) {
            restoreView();
            QMessageBox::information(this, tr("Contour Extract"),
                                     tr("当前等值 %1 未与任何单元相交，没有生成轮廓。请调整等值数值后重试。")
                                             .arg(m_IsoValue));
            return;
        }
    } else {
        m_Extracter->SetProgressRange(0.0, 1.0);
        m_Extracter->SetInput(m_OriginDataObject);
        m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValue, m_ScalarDimension);
        if (!m_Extracter->Execute()) {
            restoreView();
            QMessageBox::warning(this, tr("Contour Extract"), tr("轮廓提取失败：当前数据类型不支持或输入无效。"));
            return;
        }
        auto out = m_Extracter->GetContourMesh();
        if (!out || out->GetNumberOfCells() == 0) {
            restoreView();
            QMessageBox::information(this, tr("Contour Extract"),
                                     tr("当前等值 %1 未与任何单元相交，没有生成轮廓。请调整等值数值后重试。")
                                             .arg(m_IsoValue));
            return;
        }
        std::cout << out->GetNumberOfPoints() << " " << out->GetNumberOfCells() << '\n';
        m_ResultMesh->SetPoints(out->GetPoints());
        m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
        m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
        ApplyContourAppearance(m_ResultMesh);
    }

    finishProgress();

    m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);
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
    } else {
        DrawContourModel(m_ResultMesh);
        m_Generated = true;
    }
}