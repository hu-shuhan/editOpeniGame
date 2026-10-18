#include <IQWidgets/igQtTextureMapToPlaneWidget.h>

#include <DataProcessing/TextureMapToPlane/iGameTextureMapToPlaneFilter.h>
#include <IQComponents/Dialog/igQtDarkFramelessMessage.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>

using namespace iGame;

igQtTextureMapToPlaneWidget::igQtTextureMapToPlaneWidget(igQtModelDrawWidget* rendererWidget,
                                                         igQtModelDialogWidget* modelTreeWidget,
                                                         Model::Pointer sourceModel,
                                                         QWidget* parent)
        : QDialog(parent), m_RendererWidget(rendererWidget), m_ModelTreeWidget(modelTreeWidget),
          m_SourceModel(sourceModel) {
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);
    setWindowModality(Qt::NonModal);
    setWindowTitle(QStringLiteral("生成平面纹理坐标"));
    setMinimumWidth(560);

    if (!m_RendererWidget || !m_ModelTreeWidget || !m_SourceModel) return;
    m_Input = DynamicCast<PointSet>(m_SourceModel->GetDataObject());
    if (m_Input.IsNull() || m_Input->GetNumberOfPoints() == 0) {
        m_Input = nullptr;
        return;
    }
    buildUi();
}

void igQtTextureMapToPlaneWidget::buildUi() {
    setStyleSheet(QStringLiteral(R"(
        QDialog { background: #1f2024; color: #ececec; }
        QLabel, QCheckBox { color: #e7e7e7; font-size: 13px; }
        QGroupBox { color: #f0f0f0; font-size: 14px; font-weight: 600;
                    border: 1px solid #565b66; border-radius: 5px; margin-top: 10px; padding-top: 8px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QDoubleSpinBox, QComboBox { min-height: 27px; color: #f4f4f4; background: #292b30;
                                   border: 1px solid #686d78; border-radius: 4px; padding: 1px 6px; }
        QComboBox QAbstractItemView { color: #f4f4f4; background: #292b30;
                                      selection-background-color: #2a78be; }
        QPushButton { min-height: 29px; color: #f4f4f4; background: #343942;
                      border: 1px solid #697181; border-radius: 4px; padding: 2px 10px; }
        QPushButton:hover { background: #465365; }
        QPushButton:pressed { background: #2a7bc8; }
        QGroupBox:disabled, QGroupBox:disabled QLabel { color: #858a93; }
    )"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(8);
    auto* description = new QLabel(
            QStringLiteral("为输入网格生成二维 Texture Coordinates。自动模式拟合平面并将 S、T 归一化到 "
                           "[0, 1]；手动模式使用 Origin、Point1、Point2 定义映射轴。"), this);
    description->setWordWrap(true);
    description->setStyleSheet(QStringLiteral("color: #d5d9df;"));
    layout->addWidget(description);

    m_AutomaticCheck = new QCheckBox(QStringLiteral("自动平面生成 (Automatic Plane Generation)"), this);
    m_AutomaticCheck->setChecked(true);
    layout->addWidget(m_AutomaticCheck);

    m_ManualGroup = new QGroupBox(QStringLiteral("手动平面参数"), this);
    auto* manualLayout = new QVBoxLayout(m_ManualGroup);
    auto* presetRow = new QHBoxLayout;
    presetRow->addWidget(new QLabel(QStringLiteral("平面预设"), m_ManualGroup));
    m_PresetCombo = new QComboBox(m_ManualGroup);
    m_PresetCombo->addItem(QStringLiteral("XY 平面"), 0);
    m_PresetCombo->addItem(QStringLiteral("XZ 平面"), 1);
    m_PresetCombo->addItem(QStringLiteral("YZ 平面"), 2);
    m_PresetCombo->addItem(QStringLiteral("自定义"), 3);
    presetRow->addWidget(m_PresetCombo, 1);
    manualLayout->addLayout(presetRow);

    auto* coordinates = new QGridLayout;
    coordinates->setHorizontalSpacing(6);
    coordinates->addWidget(new QLabel(QStringLiteral("参数"), m_ManualGroup), 0, 0);
    coordinates->addWidget(new QLabel(QStringLiteral("X"), m_ManualGroup), 0, 1);
    coordinates->addWidget(new QLabel(QStringLiteral("Y"), m_ManualGroup), 0, 2);
    coordinates->addWidget(new QLabel(QStringLiteral("Z"), m_ManualGroup), 0, 3);
    const QStringList rowNames{QStringLiteral("Origin"), QStringLiteral("Point1"), QStringLiteral("Point2")};
    for (int row = 0; row < 3; ++row) {
        coordinates->addWidget(new QLabel(rowNames[row], m_ManualGroup), row + 1, 0);
        for (int axis = 0; axis < 3; ++axis) {
            auto* spin = new QDoubleSpinBox(m_ManualGroup);
            spin->setDecimals(8);
            spin->setRange(-1.0e12, 1.0e12);
            m_PlaneControls[row][axis] = spin;
            coordinates->addWidget(spin, row + 1, axis + 1);
        }
    }
    manualLayout->addLayout(coordinates);
    layout->addWidget(m_ManualGroup);

    m_StatusLabel = new QLabel(QStringLiteral("输出数组：Texture Coordinates（点数据，2 分量，float）"), this);
    m_StatusLabel->setWordWrap(true);
    m_StatusLabel->setStyleSheet(QStringLiteral("color: #9fc5ec;"));
    layout->addWidget(m_StatusLabel);
    auto* executeButton = new QPushButton(QStringLiteral("执行并生成独立输出节点"), this);
    layout->addWidget(executeButton);

    connect(m_AutomaticCheck, &QCheckBox::toggled, this,
            [this](bool checked) { setManualControlsEnabled(!checked); });
    connect(m_PresetCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int index) { applyPlanePreset(m_PresetCombo->itemData(index).toInt()); });
    connect(executeButton, &QPushButton::clicked, this, [this]() { executeFilter(); });

    applyPlanePreset(0);
    setManualControlsEnabled(false);
}

void igQtTextureMapToPlaneWidget::applyPlanePreset(int preset) {
    if (preset == 3 || m_Input.IsNull()) return;
    const auto bounds = m_Input->GetBoundingBox();
    Point minimum{static_cast<float>(bounds.min[0]), static_cast<float>(bounds.min[1]),
                  static_cast<float>(bounds.min[2])};
    Point maximum{static_cast<float>(bounds.max[0]), static_cast<float>(bounds.max[1]),
                  static_cast<float>(bounds.max[2])};
    for (int axis = 0; axis < 3; ++axis) {
        if (maximum[axis] - minimum[axis] <= std::numeric_limits<float>::epsilon())
            maximum[axis] = minimum[axis] + 1.0f;
    }
    const Point center = (minimum + maximum) * 0.5f;
    Point origin;
    Point point1;
    Point point2;
    if (preset == 0) {
        origin = {minimum[0], minimum[1], center[2]};
        point1 = {maximum[0], minimum[1], center[2]};
        point2 = {minimum[0], maximum[1], center[2]};
    } else if (preset == 1) {
        origin = {minimum[0], center[1], minimum[2]};
        point1 = {maximum[0], center[1], minimum[2]};
        point2 = {minimum[0], center[1], maximum[2]};
    } else {
        origin = {center[0], minimum[1], minimum[2]};
        point1 = {center[0], maximum[1], minimum[2]};
        point2 = {center[0], minimum[1], maximum[2]};
    }
    setControlPoint(0, origin);
    setControlPoint(1, point1);
    setControlPoint(2, point2);
}

void igQtTextureMapToPlaneWidget::setManualControlsEnabled(bool enabled) {
    m_ManualGroup->setEnabled(enabled);
}

Point igQtTextureMapToPlaneWidget::controlPoint(int row) const {
    return {static_cast<float>(m_PlaneControls[row][0]->value()),
            static_cast<float>(m_PlaneControls[row][1]->value()),
            static_cast<float>(m_PlaneControls[row][2]->value())};
}

void igQtTextureMapToPlaneWidget::setControlPoint(int row, const Point& point) {
    for (int axis = 0; axis < 3; ++axis) m_PlaneControls[row][axis]->setValue(point[axis]);
}

void igQtTextureMapToPlaneWidget::executeFilter() {
    if (m_Input.IsNull()) {
        showMessage(QStringLiteral("输入模型已不可用，请关闭面板后重新选择模型。"));
        return;
    }
    auto filter = TextureMapToPlaneFilter::New();
    filter->SetInput(m_Input);
    filter->SetAutomaticPlaneGeneration(m_AutomaticCheck->isChecked());
    if (!m_AutomaticCheck->isChecked()) {
        filter->SetOrigin(controlPoint(0));
        filter->SetPoint1(controlPoint(1));
        filter->SetPoint2(controlPoint(2));
    }
    if (!filter->Execute()) {
        showMessage(QString::fromStdString(filter->GetLastError()));
        return;
    }
    auto output = filter->GetOutput();
    auto coordinates = filter->GetTextureCoordinates();
    if (output.IsNull() || coordinates.IsNull()) {
        showMessage(QStringLiteral("未生成有效输出。"));
        return;
    }
    m_ModelTreeWidget->addDataObjectToModelTree(output, Algorithm);
    m_RendererWidget->update();

    double sMin = std::numeric_limits<double>::max();
    double sMax = std::numeric_limits<double>::lowest();
    double tMin = std::numeric_limits<double>::max();
    double tMax = std::numeric_limits<double>::lowest();
    for (IGsize pointId = 0; pointId < coordinates->GetNumberOfElements(); ++pointId) {
        const float* tuple = coordinates->RawPointer(pointId);
        sMin = std::min(sMin, static_cast<double>(tuple[0]));
        sMax = std::max(sMax, static_cast<double>(tuple[0]));
        tMin = std::min(tMin, static_cast<double>(tuple[1]));
        tMax = std::max(tMax, static_cast<double>(tuple[1]));
    }
    m_StatusLabel->setText(QStringLiteral("完成：%1 个纹理坐标；S 范围 [%2, %3]，T 范围 [%4, %5]。输出已加入模型树。")
                                   .arg(coordinates->GetNumberOfElements())
                                   .arg(sMin, 0, 'g', 8).arg(sMax, 0, 'g', 8)
                                   .arg(tMin, 0, 'g', 8).arg(tMax, 0, 'g', 8));
}

void igQtTextureMapToPlaneWidget::showMessage(const QString& text, bool information) {
    igQtShowDarkFramelessMessage(this, QStringLiteral("生成平面纹理坐标"), text, information);
}
