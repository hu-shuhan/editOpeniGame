#include <IQWidgets/igQtExtractLocationWidget.h>

#include <DataProcessing/ExtractLocation/iGameExtractLocationFilter.h>
#include <IQComponents/Dialog/igQtDarkFramelessMessage.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>

#include <Interactor/iGameInteractor.h>
#include <iGameAttributeSet.h>
#include <iGameScene.h>
#include <iGameSelection.h>

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>
#include <type_traits>

using namespace iGame;

namespace {

template<typename TArray>
QString TypedValueText(const typename TArray::Pointer& array, IGsize element, int component) {
    using TValue = std::remove_cv_t<std::remove_pointer_t<decltype(array->RawPointer())>>;
    const TValue value = array->RawPointer(element)[component];
    if constexpr (std::is_integral_v<TValue> && std::is_signed_v<TValue>)
        return QString::number(static_cast<qlonglong>(value));
    if constexpr (std::is_integral_v<TValue> && std::is_unsigned_v<TValue>)
        return QString::number(static_cast<qulonglong>(value));
    return QString::number(static_cast<double>(value), 'g', std::is_same_v<TValue, double> ? 17 : 9);
}

template<typename TArray>
QString TypedRangeText(const typename TArray::Pointer& array, int component) {
    using TValue = std::remove_cv_t<std::remove_pointer_t<decltype(array->RawPointer())>>;
    if (array->GetNumberOfElements() == 0) return {};
    TValue minimum = array->RawPointer(0)[component];
    TValue maximum = minimum;
    for (IGsize index = 1; index < array->GetNumberOfElements(); ++index) {
        const TValue value = array->RawPointer(index)[component];
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }
    auto valueText = [](TValue value) {
        if constexpr (std::is_integral_v<TValue> && std::is_signed_v<TValue>)
            return QString::number(static_cast<qlonglong>(value));
        if constexpr (std::is_integral_v<TValue> && std::is_unsigned_v<TValue>)
            return QString::number(static_cast<qulonglong>(value));
        return QString::number(static_cast<double>(value), 'g', std::is_same_v<TValue, double> ? 17 : 9);
    };
    return QStringLiteral("[%1, %2]").arg(valueText(minimum), valueText(maximum));
}

QString ArrayValueText(const ArrayObject::Pointer& array, IGsize element, int component) {
#define IG_TYPED_VALUE(ArrayType) \
    if (auto typed = DynamicCast<ArrayType>(array)) return TypedValueText<ArrayType>(typed, element, component)
    IG_TYPED_VALUE(FloatArray);
    IG_TYPED_VALUE(DoubleArray);
    IG_TYPED_VALUE(IntArray);
    IG_TYPED_VALUE(UnsignedIntArray);
    IG_TYPED_VALUE(CharArray);
    IG_TYPED_VALUE(UnsignedCharArray);
    IG_TYPED_VALUE(ShortArray);
    IG_TYPED_VALUE(UnsignedShortArray);
    IG_TYPED_VALUE(LongLongArray);
    IG_TYPED_VALUE(UnsignedLongLongArray);
#undef IG_TYPED_VALUE
    return QString::number(array->GetElementValue(element, component), 'g', 17);
}

QString ArrayRangeText(const ArrayObject::Pointer& array, int component) {
#define IG_TYPED_RANGE(ArrayType) \
    if (auto typed = DynamicCast<ArrayType>(array)) return TypedRangeText<ArrayType>(typed, component)
    IG_TYPED_RANGE(FloatArray);
    IG_TYPED_RANGE(DoubleArray);
    IG_TYPED_RANGE(IntArray);
    IG_TYPED_RANGE(UnsignedIntArray);
    IG_TYPED_RANGE(CharArray);
    IG_TYPED_RANGE(UnsignedCharArray);
    IG_TYPED_RANGE(ShortArray);
    IG_TYPED_RANGE(UnsignedShortArray);
    IG_TYPED_RANGE(LongLongArray);
    IG_TYPED_RANGE(UnsignedLongLongArray);
#undef IG_TYPED_RANGE
    return {};
}

} // namespace

igQtExtractLocationWidget::igQtExtractLocationWidget(igQtModelDrawWidget* rendererWidget,
                                                     igQtModelDialogWidget* modelTreeWidget,
                                                     Model::Pointer sourceModel,
                                                     QWidget* parent)
        : QDialog(parent), m_RendererWidget(rendererWidget), m_ModelTreeWidget(modelTreeWidget),
          m_SourceModel(sourceModel) {
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);
    setWindowModality(Qt::NonModal);
    setWindowTitle(QStringLiteral("提取指定位置数据"));
    setMinimumWidth(620);
    resize(700, 650);

    if (!m_RendererWidget || !m_ModelTreeWidget || !m_SourceModel) return;
    m_InputMesh = DynamicCast<UnstructuredMesh>(m_SourceModel->GetDataObject());
    if (m_InputMesh.IsNull()) return;
    const auto box = m_InputMesh->GetBoundingBox();
    m_BoundsCenter = (box.min + box.max) * 0.5;
    buildUi();
    createQueryPoint();
}

igQtExtractLocationWidget::~igQtExtractLocationWidget() { cleanupQueryPoint(); }

void igQtExtractLocationWidget::buildUi() {
    setStyleSheet(QStringLiteral(R"(
        QDialog { background: #1f2024; color: #ececec; }
        QLabel { color: #e7e7e7; font-size: 13px; }
        QGroupBox { color: #f0f0f0; font-size: 14px; font-weight: 600;
                    border: 1px solid #565b66; border-radius: 5px; margin-top: 10px; padding-top: 8px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QDoubleSpinBox { min-height: 26px; color: #f4f4f4; background: #292b30;
                         border: 1px solid #686d78; border-radius: 4px; padding: 1px 6px; font-size: 13px; }
        QDoubleSpinBox:focus { border: 1px solid #3296ff; }
        QPushButton { min-height: 28px; color: #f4f4f4; background: #343942;
                      border: 1px solid #697181; border-radius: 4px; padding: 2px 10px; font-size: 13px; }
        QPushButton:hover { background: #465365; }
        QPushButton:pressed { background: #2a7bc8; }
        QTableWidget { color: #eeeeee; background: #25272c; alternate-background-color: #2c2f35;
                       border: 1px solid #565b66; gridline-color: #4c5059; font-size: 14px; }
        QTableWidget::item:selected { background: #2a78be; color: white; }
        QHeaderView::section { color: #f2f2f2; background: #373b43; border: 0;
                               border-right: 1px solid #555a64; border-bottom: 1px solid #555a64;
                               padding: 5px; font-size: 14px; font-weight: 600; }
        QComboBox { min-height: 30px; color: #f4f4f4; background: #292b30;
                    border: 1px solid #686d78; border-radius: 4px; padding: 2px 8px; font-size: 14px; }
        QComboBox QAbstractItemView { color: #f4f4f4; background: #292b30; selection-background-color: #2a78be; }
    )"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(6);
    auto* description = new QLabel(
            QStringLiteral("查找第一个包含指定位置的单元（语义与 ParaView Extract Cell At Location 一致）。\n"
                           "保留 Data Arrays，添加 64 位原始点/单元 ID；不支持的单元类型会安全提示。"), this);
    description->setWordWrap(true);
    description->setStyleSheet(QStringLiteral("color: #d5d9df;"));
    layout->addWidget(description);

    auto makeSpinBox = [this](double value) {
        auto* spin = new QDoubleSpinBox(this);
        spin->setDecimals(8);
        spin->setRange(-1.0e12, 1.0e12);
        spin->setValue(value);
        return spin;
    };
    m_XSpin = makeSpinBox(m_BoundsCenter[0]);
    m_YSpin = makeSpinBox(m_BoundsCenter[1]);
    m_ZSpin = makeSpinBox(m_BoundsCenter[2]);
    auto* positionLayout = new QGridLayout;
    positionLayout->setHorizontalSpacing(6);
    positionLayout->addWidget(new QLabel(QStringLiteral("X"), this), 0, 0);
    positionLayout->addWidget(m_XSpin, 0, 1);
    positionLayout->addWidget(new QLabel(QStringLiteral("Y"), this), 0, 2);
    positionLayout->addWidget(m_YSpin, 0, 3);
    positionLayout->addWidget(new QLabel(QStringLiteral("Z"), this), 0, 4);
    positionLayout->addWidget(m_ZSpin, 0, 5);
    layout->addLayout(positionLayout);

    m_ShowPointCheck = new QCheckBox(QStringLiteral("显示查询点"), this);
    m_ShowPointCheck->setChecked(true);
    m_DragAxisCombo = new QComboBox(this);
    m_DragAxisCombo->addItem(QStringLiteral("X 轴"), 1);
    m_DragAxisCombo->addItem(QStringLiteral("Y 轴"), 2);
    m_DragAxisCombo->addItem(QStringLiteral("Z 轴"), 3);
    auto* centerButton = new QPushButton(QStringLiteral("中心定位 (Center on Bounds)"), this);
    auto* queryControls = new QHBoxLayout;
    queryControls->addWidget(m_ShowPointCheck);
    queryControls->addSpacing(8);
    queryControls->addWidget(new QLabel(QStringLiteral("拖动轴"), this));
    queryControls->addWidget(m_DragAxisCombo);
    queryControls->addWidget(centerButton, 1);
    layout->addLayout(queryControls);
    connect(centerButton, &QPushButton::clicked, this, [this]() {
        m_XSpin->setValue(m_BoundsCenter[0]);
        m_YSpin->setValue(m_BoundsCenter[1]);
        m_ZSpin->setValue(m_BoundsCenter[2]);
    });

    auto* dataGroup = new QGroupBox(QStringLiteral("Data Arrays（提取结果）"), this);
    auto* dataLayout = new QVBoxLayout(dataGroup);
    auto* dataHint = new QLabel(QStringLiteral("执行后显示输出数组的名称、附着位置、维数、元素数和范围。"), dataGroup);
    dataHint->setWordWrap(true);
    dataLayout->addWidget(dataHint);
    m_DataTable = new QTableWidget(0, 5, dataGroup);
    m_DataTable->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("位置"),
                                            QStringLiteral("类型"), QStringLiteral("元素数"),
                                            QStringLiteral("范围")});
    m_DataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_DataTable->horizontalHeader()->setStretchLastSection(true);
    m_DataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_DataTable->setAlternatingRowColors(true);
    m_DataTable->setMinimumHeight(145);
    dataLayout->addWidget(m_DataTable);
    layout->addWidget(dataGroup);

    auto* findGroup = new QGroupBox(QStringLiteral("查找数据"), this);
    auto* findLayout = new QVBoxLayout(findGroup);
    auto* findRow = new QHBoxLayout;
    m_FieldCombo = new QComboBox(findGroup);
    m_FieldCombo->addItem(QStringLiteral("请先执行提取"), -1);
    auto* findButton = new QPushButton(QStringLiteral("查看命中值"), findGroup);
    findRow->addWidget(m_FieldCombo, 1);
    findRow->addWidget(findButton);
    findLayout->addLayout(findRow);
    m_FindResultLabel = new QLabel(QStringLiteral("选择输出数组后，可查看提取结果中的首个命中点/单元元组。"), findGroup);
    m_FindResultLabel->setWordWrap(true);
    m_FindResultLabel->setStyleSheet(QStringLiteral("color: #bdc7d3;"));
    findLayout->addWidget(m_FindResultLabel);
    layout->addWidget(findGroup);
    connect(findButton, &QPushButton::clicked, this, [this]() { showSelectedArrayValue(); });

    m_StatusLabel = new QLabel(QStringLiteral("提示：选择坐标轴后，用左键拖动白色查询点。"), this);
    m_StatusLabel->setWordWrap(true);
    m_StatusLabel->setStyleSheet(QStringLiteral("color: #9fc5ec;"));
    layout->addWidget(m_StatusLabel);
    auto* executeButton = new QPushButton(QStringLiteral("执行"), this);
    layout->addWidget(executeButton);
    connect(executeButton, &QPushButton::clicked, this, [this]() { executeFilter(); });
}

void igQtExtractLocationWidget::createQueryPoint() {
    auto scene = m_RendererWidget->GetScene();
    if (!scene) return;
    m_QueryPoint = PointSet::New();
    m_QueryPoint->SetName("__ExtractLocation_QueryPoint");
    m_QueryPoint->AddPoint(m_BoundsCenter);
    m_QueryPoint->SetViewStyle(IG_POINTS);
    m_QueryPoint->SetPointSize(12.0f);
    m_QueryPointModelId = scene->AddModel(m_QueryPoint);
    m_QueryPointAdded = true;
    auto queryPointModel = scene->GetModelById(m_QueryPointModelId);

    connect(m_XSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { syncQueryPointFromControls(); });
    connect(m_YSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { syncQueryPointFromControls(); });
    connect(m_ZSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double) { syncQueryPointFromControls(); });
    connect(m_DragAxisCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, scene](int) {
        scene->GetInteractor()->SetDragPointConstraintAxis(m_DragAxisCombo->currentData().toInt());
    });
    connect(m_ShowPointCheck, &QCheckBox::toggled, this, [this, queryPointModel](bool visible) {
        if (queryPointModel) queryPointModel->SetVisibility(visible);
        m_RendererWidget->update();
    });

    if (queryPointModel) {
        QPointer<igQtExtractLocationWidget> guard(this);
        auto selection = m_QueryPoint->GetSelection(queryPointModel.GetPointer());
        selection->_SetSelectionCallBackEvent_(
                "ExtractLocationQueryPoint",
                [guard](IGenum type, const std::vector<igIndex>&, Selection::Operate) {
                    if (type != IG_DRAGPOINT || !guard) return;
                    QTimer::singleShot(0, guard, [guard]() {
                        if (!guard || guard->m_QueryPoint.IsNull()) return;
                        const auto& point = guard->m_QueryPoint->GetPoint(0);
                        guard->m_XSpin->setValue(point[0]);
                        guard->m_YSpin->setValue(point[1]);
                        guard->m_ZSpin->setValue(point[2]);
                    });
                });
        scene->SetCurrentModel(queryPointModel);
        m_RendererWidget->ChangeInteractorStyle(Interactor::DragPointStyle);
        scene->GetInteractor()->SetDragPointConstraintAxis(m_DragAxisCombo->currentData().toInt());
        scene->SetCurrentModel(m_SourceModel);
    }
}

void igQtExtractLocationWidget::cleanupQueryPoint() {
    if (!m_QueryPointAdded || !m_RendererWidget) return;
    auto scene = m_RendererWidget->GetScene();
    if (scene) {
        scene->GetInteractor()->RequestBasicStyle();
        scene->RemoveModel(m_QueryPointModelId);
    }
    m_QueryPointAdded = false;
    m_QueryPoint = nullptr;
    m_RendererWidget->update();
}

void igQtExtractLocationWidget::syncQueryPointFromControls() {
    if (m_QueryPoint.IsNull()) return;
    m_QueryPoint->SetPoint(0, Point(m_XSpin->value(), m_YSpin->value(), m_ZSpin->value()));
    m_QueryPoint->ForceReConvertToDrawableData();
    m_RendererWidget->update();
}

void igQtExtractLocationWidget::executeFilter() {
    if (m_InputMesh.IsNull()) {
        showMessage(QStringLiteral("输入模型已不可用，请关闭面板后重新选择模型。"));
        return;
    }
    auto filter = ExtractLocationFilter::New();
    filter->SetInput(m_InputMesh);
    filter->SetLocation(m_XSpin->value(), m_YSpin->value(), m_ZSpin->value());
    if (!filter->Execute()) {
        showMessage(QString::fromStdString(filter->GetLastError()));
        return;
    }
    m_LatestOutput = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    if (m_LatestOutput.IsNull()) {
        showMessage(QStringLiteral("过滤器未生成有效输出。"));
        return;
    }
    m_ModelTreeWidget->addDataObjectToModelTree(m_LatestOutput, Algorithm);
    m_RendererWidget->update();
    refreshArrayTable();
    m_FindResultLabel->setText(QStringLiteral("请选择一个数组，再点击“查看命中值”。"));
    m_StatusLabel->setText(QStringLiteral("提取完成：命中 %1 个单元，输出已加入模型树。")
                                   .arg(filter->GetExtractedCellIds().size()));
}

void igQtExtractLocationWidget::refreshArrayTable() {
    m_DataTable->setRowCount(0);
    m_FieldCombo->clear();
    if (m_LatestOutput.IsNull()) return;
    auto attributes = m_LatestOutput->GetAttributeSet()->GetAllAttributes();
    for (int attributeIndex = 0; attributeIndex < attributes->GetNumberOfElements(); ++attributeIndex) {
        auto& attribute = attributes->GetElement(attributeIndex);
        if (attribute.isDeleted || !attribute.pointer) continue;
        auto array = attribute.pointer;
        QStringList ranges;
        for (int component = 0; component < array->GetDimension(); ++component)
            ranges << ArrayRangeText(array, component);
        const int row = m_DataTable->rowCount();
        m_DataTable->insertRow(row);
        m_DataTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(array->GetName())));
        m_DataTable->setItem(row, 1, new QTableWidgetItem(attribute.attachmentType == IG_POINT
                                                                 ? QStringLiteral("点数据")
                                                                 : QStringLiteral("单元数据")));
        m_DataTable->setItem(row, 2, new QTableWidgetItem(array->GetDimension() == 1
                                                                 ? QStringLiteral("标量")
                                                                 : QStringLiteral("向量")));
        m_DataTable->setItem(row, 3, new QTableWidgetItem(QString::number(array->GetNumberOfElements())));
        m_DataTable->setItem(row, 4, new QTableWidgetItem(ranges.join(QStringLiteral(" "))));
        m_FieldCombo->addItem(QString::fromStdString(array->GetName()), attributeIndex);
    }
    if (m_FieldCombo->count() == 0) m_FieldCombo->addItem(QStringLiteral("输出中没有可查询数组"), -1);
}

void igQtExtractLocationWidget::showSelectedArrayValue() {
    const int attributeIndex = m_FieldCombo->currentData().toInt();
    if (m_LatestOutput.IsNull() || attributeIndex < 0) {
        m_FindResultLabel->setText(QStringLiteral("请先在网格内部选择位置并执行提取。"));
        return;
    }
    auto attributes = m_LatestOutput->GetAttributeSet()->GetAllAttributes();
    if (attributeIndex >= attributes->GetNumberOfElements()) {
        m_FindResultLabel->setText(QStringLiteral("所选数组已不可用，请重新执行提取。"));
        return;
    }
    auto& attribute = attributes->GetElement(attributeIndex);
    if (attribute.isDeleted || !attribute.pointer || attribute.pointer->GetNumberOfElements() == 0) {
        m_FindResultLabel->setText(QStringLiteral("该数组没有命中元素：指定位置可能不在任何单元内部。"));
        return;
    }
    QStringList values;
    for (int component = 0; component < attribute.pointer->GetDimension(); ++component)
        values << ArrayValueText(attribute.pointer, 0, component);
    m_FindResultLabel->setText(QStringLiteral("%1（%2）的第一个命中%3元组：[%4]")
                                       .arg(QString::fromStdString(attribute.pointer->GetName()))
                                       .arg(attribute.pointer->GetDimension() == 1
                                                    ? QStringLiteral("标量") : QStringLiteral("向量"))
                                       .arg(attribute.attachmentType == IG_POINT
                                                    ? QStringLiteral("点") : QStringLiteral("单元"))
                                       .arg(values.join(QStringLiteral(", "))));
}

void igQtExtractLocationWidget::showMessage(const QString& text, bool information) {
    igQtShowDarkFramelessMessage(this, QStringLiteral("提取指定位置数据"), text, information);
}

void igQtExtractLocationWidget::closeEvent(QCloseEvent* event) {
    cleanupQueryPoint();
    QDialog::closeEvent(event);
}
