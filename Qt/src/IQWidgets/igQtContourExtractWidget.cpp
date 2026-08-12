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
// DrawObject 的默认 view style 只有 IG_SURFACE（见 DrawObject 构造函数）。
// 等值线的输出全是 IG_LINE，一个三角形都没有，只开 IG_SURFACE 就等于什么都不画——
// 表现为"提取成功、点数单元数都对，但画面上看不到东西"。
// 线索引是在 IG_WIREFRAME 分支里用 m_LineVAO 绘制的，所以这里按输出实际的单元维度决定样式。
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
        // 线面都有了就不必再扫下去（混合网格）
        if ((style & IG_WIREFRAME) && (style & IG_SURFACE)) { break; }
    }
    return style ? style : IG_SURFACE;
}

constexpr float kContourLineWidth = 2.5f;
const igm::vec3 kContourLineColor{0.55f, 0.55f, 0.55f}; // 中性灰，和模型自身的黑色线框区分开

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
    // 结果本身就是等值线 / 等值面，再抽一次壳没有意义（线单元也抽不出壳）
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

    // 进度条：ContourFilter 内部按单元推进 0→1，这里负责文案与收尾。
    // 进度条收到 100% 会自动复位（见 igQtProgressBarWidget::updateProgressBar），
    // 所以任何退出路径都要走 finishProgress，避免文案和进度停在中间。
    auto progressObserver = iGame::ProgressObserver::Instance();
    progressObserver->UpdateText("轮廓提取中");
    progressObserver->UpdateProgress(0.0);
    auto finishProgress = [progressObserver]() {
        progressObserver->UpdateText("");
        progressObserver->UpdateProgress(1.0);
    };

    // 中途退出时把云图状态还原回去，否则上面那次 ViewCloudPicture(-1,-1) 会让已有结果失去着色
    auto restoreView = [&]() {
        finishProgress();
        m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);
    };
    // 多块分支会逐个子块改写 m_ScalarArray，循环结束后要还原，
    // 否则最后一个子块若没有该属性，m_ScalarArray 会留成 null，下次点击直接静默返回
    auto selectedScalar = m_ScalarArray;

    if (m_OriginDataObject->HasSubDataObject()) {
        // 先数一遍子块，好把每个子块的 0→1 映射到全局进度的一个片段上，
        // 否则进度条会一个子块循环一次
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
        // m_Extracter 是复用的，若上一次走过多块分支，进度区间还停在某个子块的片段上
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
            // 保留上一次的结果不动，只提示，避免把已有模型清成空的
            restoreView();
            QMessageBox::information(this, tr("Contour Extract"),
                                     tr("当前等值 %1 未与任何单元相交，没有生成轮廓。请调整等值数值后重试。")
                                             .arg(m_IsoValue));
            return;
        }
        std::cout << out->GetNumberOfPoints() << " " << out->GetNumberOfCells() << '\n';
        // 必须连同 cellType 一起接收：面网格输入产出 IG_LINE（等值线），
        // 体网格输入产出 IG_TRIANGLE（等值面），混合网格两者兼有。
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