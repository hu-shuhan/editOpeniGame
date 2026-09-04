#include "IQWidgets/igQtContourExtractWidget.h"
#include "ModelSurface/iGameModelGeometryFilter.h"
#include "iGameProgressObserver.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameSmartPointer.h"

#include <algorithm>

#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QAbstractItemView>
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
    // 允许一次填写多个等值数值：数字之间用逗号（中英文）/ 空格 / 分号分隔
    QRegularExpression rx(R"(\s*[-+0-9.eE]+(?:\s*[,，;；\s]\s*[-+0-9.eE]+)*\s*)");
    ui->lineEdit_IsoValue->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_IsoValue->setPlaceholderText(
            QStringLiteral("数值，可逗号分隔"));
    connect(ui->btnExecute, &QPushButton::clicked, this, &igQtContourExtractWidget::ContourExtract);
    connect(ui->comboBox_ScalarIndex, &QComboBox::currentTextChanged, this,
            &igQtContourExtractWidget::UpdateScalarName);
    connect(ui->comboBox_ScalarDimension, &QComboBox::currentTextChanged, this,
            &igQtContourExtractWidget::UpdateScalarDimension);

    ui->listWidget_IsoValues->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(ui->btnAddIsoValue, &QPushButton::clicked, this, &igQtContourExtractWidget::AddIsoValueFromInput);
    connect(ui->lineEdit_IsoValue, &QLineEdit::returnPressed, this,
            &igQtContourExtractWidget::AddIsoValueFromInput);
    connect(ui->btnRemoveIsoValue, &QPushButton::clicked, this,
            &igQtContourExtractWidget::RemoveSelectedIsoValues);
    connect(ui->btnClearIsoValues, &QPushButton::clicked, this, &igQtContourExtractWidget::ClearIsoValues);
    connect(ui->btnGenRange, &QPushButton::clicked, this, &igQtContourExtractWidget::GenerateIsoValuesByRange);
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

void igQtContourExtractWidget::UpdateIsoValue() {
    // 以列表为准；列表为空时把输入框里没来得及"添加"的内容也算上，
    // 这样直接填一个数就点执行的旧用法仍然有效。
    if (m_IsoValues.empty()) {
        const QStringList tokens =
                ui->lineEdit_IsoValue->text().split(QRegularExpression(R"([,，;；\s]+)"), Qt::SkipEmptyParts);
        std::vector<double> parsed;
        for (const QString& token: tokens) {
            bool ok = false;
            const double v = token.toDouble(&ok);
            if (ok) { parsed.push_back(v); }
        }
        if (!parsed.empty()) { AppendIsoValues(parsed); }
    }
    m_IsoValue = m_IsoValues.empty() ? 0.0 : m_IsoValues.front();
}

QString igQtContourExtractWidget::isoValueText() const {
    if (m_IsoValues.empty()) return QStringLiteral("(空)");
    QStringList parts;
    for (double v: m_IsoValues) { parts << QString::number(v); }
    if (m_IsoValues.size() == 1) return parts.front();
    return QStringLiteral("%1 个（%2）").arg(m_IsoValues.size()).arg(parts.join(QStringLiteral(", ")));
}


void igQtContourExtractWidget::AppendIsoValues(const std::vector<double>& values) {
    for (double v: values) { m_IsoValues.push_back(v); }
    std::sort(m_IsoValues.begin(), m_IsoValues.end());
    m_IsoValues.erase(std::unique(m_IsoValues.begin(), m_IsoValues.end()), m_IsoValues.end());
    SyncIsoValueList();
}

void igQtContourExtractWidget::SyncIsoValueList() {
    ui->listWidget_IsoValues->clear();
    for (double v: m_IsoValues) { ui->listWidget_IsoValues->addItem(QString::number(v)); }
}

bool igQtContourExtractWidget::CurrentScalarRange(double& lo, double& hi) const {
    if (!m_OriginDataObject || !m_OriginDataObject->GetAttributeSet()) return false;
    auto dataRange = m_OriginDataObject->GetAttributeSet()->GetAttribute(m_ScalarName).GetDataRange();
    if (!dataRange) return false;
    const int base = 2 * m_ScalarDimension + 2;
    if (dataRange->GetNumberOfElements() < base + 2) return false;
    lo = dataRange->GetValue(base);
    hi = dataRange->GetValue(base + 1);
    return hi > lo;
}

void igQtContourExtractWidget::AddIsoValueFromInput() {
    // 输入框仍支持一次粘贴多个（逗号 / 空格 / 分号分隔）
    const QStringList tokens =
            ui->lineEdit_IsoValue->text().split(QRegularExpression(R"([,，;；\s]+)"), Qt::SkipEmptyParts);
    std::vector<double> parsed;
    for (const QString& token: tokens) {
        bool ok = false;
        const double v = token.toDouble(&ok);
        if (ok) { parsed.push_back(v); }
    }
    if (parsed.empty()) {
        QMessageBox::information(this, tr("Contour Extract"),
                                 tr("请输入有效数值。可用逗号分隔一次输入多个，例如 1.5, 2.0, 2.5。"));
        return;
    }
    AppendIsoValues(parsed);
    ui->lineEdit_IsoValue->clear();
}

void igQtContourExtractWidget::RemoveSelectedIsoValues() {
    const auto selected = ui->listWidget_IsoValues->selectedItems();
    if (selected.isEmpty()) return;
    std::vector<double> keep;
    for (int row = 0; row < ui->listWidget_IsoValues->count(); ++row) {
        auto* item = ui->listWidget_IsoValues->item(row);
        if (!item->isSelected() && row < static_cast<int>(m_IsoValues.size())) {
            keep.push_back(m_IsoValues[static_cast<size_t>(row)]);
        }
    }
    m_IsoValues.swap(keep);
    SyncIsoValueList();
}

void igQtContourExtractWidget::ClearIsoValues() {
    m_IsoValues.clear();
    SyncIsoValueList();
}

void igQtContourExtractWidget::GenerateIsoValuesByRange() {
    double lo = 0.0, hi = 0.0;
    if (!CurrentScalarRange(lo, hi)) {
        QMessageBox::information(this, tr("Contour Extract"),
                                 tr("无法获取当前标量分量的数据范围，请先选择属性与分量。"));
        return;
    }
    const int n = ui->spinBox_RangeCount->value();
    std::vector<double> generated;
    if (n <= 1) {
        generated.push_back(0.5 * (lo + hi));
    } else {
        // 在 [lo, hi] 内均匀取 n 个数值，含两端
        for (int i = 0; i < n; ++i) {
            generated.push_back(lo + (hi - lo) * static_cast<double>(i) / static_cast<double>(n - 1));
        }
    }
    AppendIsoValues(generated);
}

void igQtContourExtractWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    if (!m_d) { return; }
    if (m_OriginDataObject.GetPointer() == m_d.GetPointer()) { return; }
    if (m_d->GetName().find("_Contour") != std::string::npos) { return; }
    auto attrSet = m_d->GetAttributeSet();
    if (!attrSet) { return; }

    this->m_OriginDataObject = m_d;
    this->m_PointData = attrSet->GetAllPointAttributes();
    this->m_ScalarArray = nullptr;
    this->m_ScalarName.clear();
    ui->label_RangeInfo->clear();
    ClearIsoValues();
    InitScalarName();
    m_Generated = false;
    m_Extracter = iGame::ContourFilter::New();
    m_ResultMesh = iGame::UnstructuredMesh::New();
    m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_Contour");
    m_ResultMesh->SetAttributeSet(attrSet);
    m_ResultMesh->SetShellRenderingOption(false);
    // 场景/模型树移除轮廓结果时会 Invoke DeleteEvent；不应关闭工具面板或清空源网格，
    // 否则用户删除结果模型后无法在同一面板内再次执行提取。
    m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [&]() -> void { m_Generated = false; });
}

void igQtContourExtractWidget::ContourExtract() {
    UpdateIsoValue();
    if (m_ScalarArray == nullptr) { return; }
    if (m_IsoValues.empty()) {
        QMessageBox::information(this, tr("Contour Extract"),
                                 tr("等值数值列表为空。请在输入框填入数值后点「添加」，或用「按范围生成」自动生成。"));
        return;
    }
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
                m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValues, m_ScalarDimension);
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
                                             .arg(isoValueText()));
            return;
        }
    } else {
        m_Extracter->SetProgressRange(0.0, 1.0);
        m_Extracter->SetInput(m_OriginDataObject);
        m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValues, m_ScalarDimension);
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
                                             .arg(isoValueText()));
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
    //m_Extracter->SetIsoScalarData(m_ScalarArray, m_IsoValues, m_ScalarDimension);
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