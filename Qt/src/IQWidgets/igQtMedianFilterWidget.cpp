#include <QPointer>
#include "IQWidgets/igQtMedianFilterWidget.h"
#include "iGameProgressObserver.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"

#include <QMessageBox>
#include <QSignalBlocker>

#include <algorithm>
#include <string>

namespace {
QString ArrayTypeName(iGame::ArrayObject* array) {
    switch (array->GetArrayType()) {
        case IG_CharArray:
            return QStringLiteral("char");
        case IG_UnsignedCharArray:
            return QStringLiteral("unsigned char");
        case IG_ShortArray:
            return QStringLiteral("short");
        case IG_UnsignedShortArray:
            return QStringLiteral("unsigned short");
        case IG_IntArray:
            return QStringLiteral("int");
        case IG_UnsignedIntArray:
            return QStringLiteral("unsigned int");
        case IG_LongLongArray:
            return QStringLiteral("long long");
        case IG_UnsignedLongLongArray:
            return QStringLiteral("unsigned long long");
        case IG_FloatArray:
            return QStringLiteral("float");
        case IG_DoubleArray:
            return QStringLiteral("double");
        default:
            return {};
    }
}

bool IsMedianScalar(const iGame::AttributeSet::Attribute& attr, IGsize pointCount) {
    return !attr.isDeleted && attr.pointer && attr.type == IG_SCALAR &&
           attr.attachmentType == IG_POINT && attr.pointer->GetDimension() == 1 &&
           attr.pointer->GetNumberOfValues() >= pointCount &&
           !ArrayTypeName(attr.pointer.get()).isEmpty();
}
} // namespace

igQtMedianFilterWidget::igQtMedianFilterWidget(QWidget* parent) : QWidget(parent), ui(new Ui::MedianFilter) {
    ui->setupUi(this);
    m_Generated = false;
    m_MedianFilter = nullptr;

    connect(ui->btnExecute, &QPushButton::clicked, this, &igQtMedianFilterWidget::MedianFilterExecute);
    connect(ui->comboBox_ScalarIndex, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtMedianFilterWidget::UpdateScalarType);
    ui->label_ScalarType->setWordWrap(true);
    InitScalarList();
}

void igQtMedianFilterWidget::InitScalarList() {
    {
        const QSignalBlocker blocker(ui->comboBox_ScalarIndex);
        ui->comboBox_ScalarIndex->clear();
        auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(m_OriginDataObject);
        auto attrs = mesh && mesh->GetAttributeSet()
                         ? mesh->GetAttributeSet()->GetAllAttributes() : nullptr;
        if (attrs && mesh->GetNumberOfPoints() > 0) {
            for (int i = 0; i < attrs->GetNumberOfElements(); ++i) {
                auto& attr = attrs->GetElement(i);
                if (!IsMedianScalar(attr, mesh->GetNumberOfPoints())) { continue; }
                // 保存原始属性索引：筛选后的位置及数组名都不能唯一标识属性。
                ui->comboBox_ScalarIndex->addItem(
                    QString::fromStdString(attr.pointer->GetName()), i);
            }
        }
    }
    ui->comboBox_ScalarIndex->setEnabled(ui->comboBox_ScalarIndex->count() > 0);
    UpdateScalarType();
}

void igQtMedianFilterWidget::UpdateScalarType() {
    m_ScalarArray = nullptr;
    m_ScalarIndex = -1;
    ui->btnExecute->setEnabled(false);

    if (!m_OriginDataObject || !m_OriginDataObject->GetAttributeSet()) {
        ui->label_ScalarType->setText(QStringLiteral("请先选择包含单分量点标量的结构化网格。"));
        return;
    }
    auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(m_OriginDataObject);
    if (!mesh) {
        ui->label_ScalarType->setText(QStringLiteral("中值滤波仅支持结构化网格。"));
        return;
    }
    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    bool indexOk = false;
    const int index = ui->comboBox_ScalarIndex->currentData().toInt(&indexOk);
    if (!indexOk || !attrs || index < 0 || index >= attrs->GetNumberOfElements() ||
        mesh->GetNumberOfPoints() == 0 ||
        !IsMedianScalar(attrs->GetElement(index), mesh->GetNumberOfPoints())) {
        ui->label_ScalarType->setText(QStringLiteral("没有可用的单分量点标量。可先提取分量，或将单元数据转换为点数据；数组数据须完整。"));
        return;
    }
    auto& attr = attrs->GetElement(index);
    m_ScalarIndex = index;
    m_ScalarArray = attr.pointer;
    ui->btnExecute->setEnabled(true);

    QString info = QStringLiteral("类型: %1").arg(ArrayTypeName(m_ScalarArray.get()));
    auto dataRange = attr.GetDataRange();
    if (dataRange && dataRange->GetNumberOfValues() >= 4) {
        info += QStringLiteral(" | 范围: [%1, %2]").arg(dataRange->GetValue(2)).arg(dataRange->GetValue(3));
    }
    ui->label_ScalarType->setText(info);
}

void igQtMedianFilterWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    this->m_OriginDataObject = m_d;

    m_Generated = false;
    m_MedianFilter = iGame::MedianFilter::New();
    m_ResultMesh = iGame::StructuredMesh::New();
    if (m_OriginDataObject) { m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_Median"); }
    QPointer<igQtMedianFilterWidget> guard(this);
    m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [guard]() { if (guard) guard->m_Generated = false; });

    // 2D 数据第三维固定为 1
    auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(m_OriginDataObject);
    if (mesh) {
        igIndex dims[3]{1, 1, 1};
        std::copy(mesh->GetDimensionSize(), mesh->GetDimensionSize() + 3, dims);
        ui->spinBox_KernelZ->setEnabled(dims[2] > 1);
        if (dims[2] <= 1) { ui->spinBox_KernelZ->setValue(1); }
    }

    InitScalarList();
}

void igQtMedianFilterWidget::MedianFilterExecute() {
    UpdateScalarType();
    if (!m_OriginDataObject) { return; }
    if (!m_ScalarArray) {
        QMessageBox::warning(this, tr("中值滤波"), tr("请先选择一个单分量标量属性。"));
        return;
    }
    auto inputMesh = iGame::DynamicCast<iGame::StructuredMesh>(m_OriginDataObject);
    if (!inputMesh) {
        QMessageBox::warning(this, tr("中值滤波"), tr("中值滤波仅支持结构化网格 (StructuredMesh)。"));
        return;
    }

    int kx = ui->spinBox_KernelX->value();
    int ky = ui->spinBox_KernelY->value();
    int kz = ui->spinBox_KernelZ->value();
    igIndex dims[3]{1, 1, 1};
    std::copy(inputMesh->GetDimensionSize(), inputMesh->GetDimensionSize() + 3, dims);
    if (dims[2] <= 1) { kz = 1; }
    if ((kx & 1) == 0 || (ky & 1) == 0 || (kz & 1) == 0) {
        QMessageBox::warning(this, tr("中值滤波"), tr("核大小各维必须为奇数。"));
        return;
    }

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto progressObserver = iGame::ProgressObserver::Instance();
    progressObserver->UpdateText("中值滤波中");
    progressObserver->UpdateProgress(0.0);

    if (!m_MedianFilter) { m_MedianFilter = iGame::MedianFilter::New(); }
    m_MedianFilter->SetProgressRange(0.0, 1.0);
    m_MedianFilter->SetInput(m_OriginDataObject);
    m_MedianFilter->SetAttributeByIndex(m_ScalarIndex);
    m_MedianFilter->SetKernelSize(kx, ky, kz);
    if (!m_MedianFilter->Execute()) {
        progressObserver->UpdateText("");
        progressObserver->UpdateProgress(1.0);
        const QString reason = QString::fromStdString(m_MedianFilter->GetMessage());
        QMessageBox::warning(this, tr("中值滤波"), reason.isEmpty() ? tr("中值滤波执行失败，请检查输入。") : reason);
        return;
    }

    auto out = iGame::DynamicCast<iGame::StructuredMesh>(m_MedianFilter->GetOutput());
    if (!out) {
        progressObserver->UpdateText("");
        progressObserver->UpdateProgress(1.0);
        QMessageBox::warning(this, tr("中值滤波"), tr("中值滤波执行失败：输出无效。"));
        return;
    }

    igIndex extent[6]{0, 0, 0, 0, 0, 0};
    std::copy(out->GetExtent(), out->GetExtent() + 6, extent);
    m_ResultMesh->SetExtent(extent);
    igIndex outDims[3]{1, 1, 1};
    std::copy(out->GetDimensionSize(), out->GetDimensionSize() + 3, outDims);
    m_ResultMesh->SetDimensionSize(outDims);
    m_ResultMesh->SetPoints(out->GetPoints());
    m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
    m_ResultMesh->GenStructuredCellConnectivities();

    progressObserver->UpdateText("");
    progressObserver->UpdateProgress(1.0);

    // 按中值后的标量着色（单分量标量，维度 0）
    m_ResultMesh->ViewCloudPicture(scene, out->GetAttributeIndex(), 0);

    if (m_Generated) {
        UpdateMedianModel(m_ResultMesh);
    } else {
        DrawMedianModel(m_ResultMesh);
        m_Generated = true;
    }
}
