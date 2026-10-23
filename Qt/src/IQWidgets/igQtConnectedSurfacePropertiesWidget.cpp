#include <IQWidgets/igQtConnectedSurfacePropertiesWidget.h>

#include <DataProcessing/ConnectedSurfaceProperties/iGameConnectedSurfacePropertiesFilter.h>
#include <IQComponents/Dialog/igQtDarkFramelessMessage.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

using namespace iGame;

namespace {

QString NumberText(double value) {
    return std::isfinite(value) ? QString::number(value, 'g', 17) : QStringLiteral("NaN");
}

} // namespace

igQtConnectedSurfacePropertiesWidget::igQtConnectedSurfacePropertiesWidget(
        igQtModelDrawWidget* rendererWidget, igQtModelDialogWidget* modelTreeWidget,
        Model::Pointer sourceModel, QWidget* parent)
        : QDialog(parent), m_RendererWidget(rendererWidget), m_ModelTreeWidget(modelTreeWidget),
          m_SourceModel(sourceModel) {
    setModal(false);
    setWindowModality(Qt::NonModal);
    setObjectName(QStringLiteral("ConnectedSurfacePropertiesWidget"));
    setWindowTitle(QStringLiteral("计算连通表面属性"));
    setMinimumWidth(520);

    setSourceModel(sourceModel);
    if (!m_RendererWidget || !m_ModelTreeWidget || m_InputMesh.IsNull()) return;
    buildUi();
    setFixedSize(600, 300);
}

igQtConnectedSurfacePropertiesWidget::~igQtConnectedSurfacePropertiesWidget() {
    detachOutputObservers();
}

bool igQtConnectedSurfacePropertiesWidget::setSourceModel(Model::Pointer sourceModel) {
    m_SourceModel = sourceModel;
    m_InputMesh = m_SourceModel
            ? DynamicCast<SurfaceMesh>(m_SourceModel->GetDataObject())
            : SurfaceMesh::Pointer{};
    if (m_NumberOfObjectsLabel) m_NumberOfObjectsLabel->setText(QStringLiteral("—"));
    if (m_AllValidLabel) {
        m_AllValidLabel->setText(QStringLiteral("—"));
        m_AllValidLabel->setStyleSheet(QString{});
    }
    if (m_TotalAreaLabel) m_TotalAreaLabel->setText(QStringLiteral("—"));
    if (m_TotalVolumeLabel) m_TotalVolumeLabel->setText(QStringLiteral("—"));
    if (m_StatusLabel) m_StatusLabel->setText(QStringLiteral("等待执行；结果将生成独立节点。"));
    return !m_InputMesh.IsNull();
}

void igQtConnectedSurfacePropertiesWidget::buildUi() {
    setStyleSheet(QStringLiteral(R"(
        QDialog { background: #1f2024; color: #ececec; }
        QLabel, QCheckBox { color: #e7e7e7; font-size: 12px; }
        QGroupBox { color: #f0f0f0; font-size: 13px; font-weight: 600;
                    border: 1px solid #565b66; border-radius: 4px; margin-top: 8px; padding-top: 6px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QLineEdit { min-height: 23px; color: #f4f4f4; background: #292b30;
                    border: 1px solid #686d78; border-radius: 4px; padding: 1px 6px; font-size: 12px; }
        QLineEdit:disabled { color: #8f949e; background: #25262a; }
        QPushButton { min-height: 26px; color: #f4f4f4; background: #343942;
                      border: 1px solid #697181; border-radius: 4px; padding: 1px 9px; font-size: 12px; }
        QPushButton:hover { background: #465365; }
        QPushButton:pressed { background: #2a7bc8; }
    )"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 7, 10, 9);
    layout->setSpacing(4);

    auto* description = new QLabel(
            QStringLiteral("按共享边识别连通表面，计算逐面面积/体积贡献及逐对象有效性、面积、体积和质心。"), this);
    description->setWordWrap(true);
    description->setStyleSheet(QStringLiteral("color: #d5d9df;"));
    description->setToolTip(
            QStringLiteral("计算方式与 ParaView Connected Surface Properties 一致。只有闭合且每条边恰由"
                           "两个面使用的对象才会标记为有效。"));
    layout->addWidget(description);

    auto* optionsGroup = new QGroupBox(QStringLiteral("参数"), this);
    auto* optionsLayout = new QHBoxLayout(optionsGroup);
    optionsLayout->setContentsMargins(8, 7, 8, 5);
    optionsLayout->setSpacing(6);
    m_SkipIdentificationCheck = new QCheckBox(
            QStringLiteral("跳过识别/有效性检查"), optionsGroup);
    m_SkipIdentificationCheck->setToolTip(
            QStringLiteral("Skip Object Identification：仅在已有同长度的 64 位整数单元数组时使用。"));
    m_ObjectIdsNameEdit = new QLineEdit(QStringLiteral("ObjectIds"), optionsGroup);
    m_ObjectIdsNameEdit->setEnabled(false);
    m_ObjectIdsNameEdit->setToolTip(
            QStringLiteral("数组缺失、类型不符或长度不一致时改用自动识别。"));
    optionsLayout->addWidget(m_SkipIdentificationCheck);
    optionsLayout->addWidget(new QLabel(QStringLiteral("ID 数组"), optionsGroup));
    optionsLayout->addWidget(m_ObjectIdsNameEdit, 1);
    layout->addWidget(optionsGroup);
    connect(m_SkipIdentificationCheck, &QCheckBox::toggled, m_ObjectIdsNameEdit, &QLineEdit::setEnabled);

    auto* summaryGroup = new QGroupBox(QStringLiteral("汇总结果"), this);
    auto* summaryLayout = new QGridLayout(summaryGroup);
    summaryLayout->setContentsMargins(8, 7, 8, 5);
    summaryLayout->setHorizontalSpacing(18);
    summaryLayout->setVerticalSpacing(2);
    m_NumberOfObjectsLabel = new QLabel(QStringLiteral("—"), summaryGroup);
    m_AllValidLabel = new QLabel(QStringLiteral("—"), summaryGroup);
    m_TotalAreaLabel = new QLabel(QStringLiteral("—"), summaryGroup);
    m_TotalVolumeLabel = new QLabel(QStringLiteral("—"), summaryGroup);
    const QString summaryTitles[4]{QStringLiteral("对象数"), QStringLiteral("全部有效"),
                                   QStringLiteral("总面积"), QStringLiteral("有效总体积")};
    QLabel* summaryValues[4]{m_NumberOfObjectsLabel, m_AllValidLabel,
                             m_TotalAreaLabel, m_TotalVolumeLabel};
    for (int column = 0; column < 4; ++column) {
        auto* title = new QLabel(summaryTitles[column], summaryGroup);
        title->setStyleSheet(QStringLiteral("color: #aeb4bd;"));
        summaryValues[column]->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 600;"));
        summaryLayout->addWidget(title, 0, column);
        summaryLayout->addWidget(summaryValues[column], 1, column);
        summaryLayout->setColumnStretch(column, 1);
    }
    layout->addWidget(summaryGroup);

    m_StatusLabel = new QLabel(QStringLiteral("等待执行；结果将生成独立节点。"), this);
    m_StatusLabel->setWordWrap(true);
    m_StatusLabel->setStyleSheet(QStringLiteral("color: #77bfff;"));
    layout->addWidget(m_StatusLabel);
    auto* executeButton = new QPushButton(QStringLiteral("执行并生成输出"), this);
    layout->addWidget(executeButton);
    connect(executeButton, &QPushButton::clicked, this,
            [this]() { executeFilter(); });
}

void igQtConnectedSurfacePropertiesWidget::executeFilter() {
    if (m_InputMesh.IsNull()) {
        showMessage(QStringLiteral("输入模型已不可用，请关闭面板后重新选择表面网格。"));
        return;
    }

    auto filter = ConnectedSurfacePropertiesFilter::New();
    filter->SetInput(m_InputMesh);
    filter->SetSkipObjectIdentification(m_SkipIdentificationCheck->isChecked());
    filter->SetObjectIdsArrayName(m_ObjectIdsNameEdit->text().trimmed().toStdString());
    if (!filter->Execute()) {
        showMessage(QString::fromStdString(filter->GetLastError()));
        return;
    }
    m_LatestOutput = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (m_LatestOutput.IsNull()) {
        showMessage(QStringLiteral("未生成有效输出。"));
        return;
    }

    m_ModelTreeWidget->addDataObjectToModelTree(m_LatestOutput, Algorithm);
    const auto output = m_LatestOutput;
    OutputBinding binding;
    binding.output = output;
    binding.source = m_SourceModel;
    binding.observerTag = output->AddObserver(
            Command::DeleteEvent,
            [this, deletedOutput = output.GetPointer()]() { handleOutputDeleted(deletedOutput); });
    m_OutputBindings.push_back(std::move(binding));

    if (auto scene = m_RendererWidget->GetScene(); scene && m_SourceModel) {
        scene->ChangeModelVisibility(m_SourceModel, false);
        m_ModelTreeWidget->RefreshModelTreeVisibilityIcons();
    }
    m_RendererWidget->update();
    if (m_OutputReadyCallback) m_OutputReadyCallback(m_LatestOutput.GetPointer());

    m_NumberOfObjectsLabel->setText(QString::number(filter->GetNumberOfObjects()));
    m_AllValidLabel->setText(filter->GetAllValid() ? QStringLiteral("是") : QStringLiteral("否"));
    m_AllValidLabel->setStyleSheet(filter->GetAllValid()
            ? QStringLiteral("color: #78d994; font-size: 13px; font-weight: 600;")
            : QStringLiteral("color: #ffb36b; font-size: 13px; font-weight: 600;"));
    m_TotalAreaLabel->setText(NumberText(filter->GetTotalArea()));
    m_TotalVolumeLabel->setText(NumberText(filter->GetTotalVolume()));

    QString modeText = QStringLiteral("自动识别");
    if (m_SkipIdentificationCheck->isChecked()) {
        modeText = filter->GetUsedSuppliedObjectIds()
                ? QStringLiteral("使用现有 64 位 ObjectIds")
                : QStringLiteral("指定数组不可用，已改用自动识别");
    }
    QString validityHint;
    if (!filter->GetAllValid()) {
        validityHint = QStringLiteral(" | 开放/非流形：体积和质心可能无实体意义");
    }
    m_StatusLabel->setText(
            QStringLiteral("完成：%1 个对象 | %2 | 详细属性已显示在“查找数据”表格%3")
                    .arg(filter->GetNumberOfObjects()).arg(modeText, validityHint));
}

void igQtConnectedSurfacePropertiesWidget::detachOutputObservers() {
    for (auto& binding : m_OutputBindings) {
        if (binding.output && binding.observerTag) {
            binding.output->RemoveObserver(binding.observerTag);
        }
    }
    m_OutputBindings.clear();
}

void igQtConnectedSurfacePropertiesWidget::handleOutputDeleted(SurfaceMesh* output) {
    auto binding = std::find_if(
            m_OutputBindings.begin(), m_OutputBindings.end(),
            [output](const OutputBinding& candidate) {
                return candidate.output.GetPointer() == output;
            });
    if (binding == m_OutputBindings.end()) return;

    const auto source = binding->source;
    const bool wasLatestOutput = m_LatestOutput.GetPointer() == output;
    binding->observerTag = 0;
    m_OutputBindings.erase(binding);
    if (wasLatestOutput) {
        m_LatestOutput = m_OutputBindings.empty()
                ? SurfaceMesh::Pointer{}
                : m_OutputBindings.back().output;
    }

    const bool sourceStillHasOutput = std::any_of(
            m_OutputBindings.begin(), m_OutputBindings.end(),
            [&source](const OutputBinding& candidate) { return candidate.source == source; });
    if (!sourceStillHasOutput && source) {
        if (auto scene = m_RendererWidget->GetScene()) {
            const auto models = scene->GetModelList();
            bool sourceIsInScene = false;
            if (models) {
                for (auto it = models->Begin(); it != models->End(); ++it)
                    if (it->second == source) { sourceIsInScene = true; break; }
            }
            if (sourceIsInScene) scene->ChangeModelVisibility(source, true);
        }
        m_ModelTreeWidget->RefreshModelTreeVisibilityIcons();
    }
    if (wasLatestOutput && m_OutputReadyCallback) {
        auto replacement = m_LatestOutput
                ? static_cast<DataObject*>(m_LatestOutput.GetPointer())
                : (source ? source->GetDataObject().GetPointer() : nullptr);
        m_OutputReadyCallback(replacement);
    }
    m_RendererWidget->update();
}

void igQtConnectedSurfacePropertiesWidget::showMessage(const QString& text, bool information) {
    igQtShowDarkFramelessMessage(this, QStringLiteral("计算连通表面属性"), text, information);
}
