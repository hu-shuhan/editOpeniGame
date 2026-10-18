#include "IQWidgets/igQtVolumeRenderingWidget.h"

#include "ui_igQtVolumeRendering.h"

#include <iGameSceneManager.h>

#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kDispatchGroupSize = 8; // 与 VolumeRaymarch.comp 的 local_size 一致
// 每步跨过的体素数超过这个值就认为该方向欠采样（会看到条带/漏采）
constexpr double kUnderSamplingVoxelsPerStep = 2.0;
constexpr double kRecommendedSamplesPerVoxel = 1.5;

bool IsSpinBoxEditing(QWidget* spinBox) {
    if (spinBox == nullptr) { return false; }
    if (spinBox->hasFocus()) { return true; }
    QWidget* focus = QApplication::focusWidget();
    return focus != nullptr && spinBox->isAncestorOf(focus);
}

void SetLabelText(QLabel* label, const QString& text) {
    if (label != nullptr && label->text() != text) { label->setText(text); }
}

QString FormatCount(double value) {
    if (value >= 1.0e8) {
        return QStringLiteral("%1 亿").arg(value / 1.0e8, 0, 'f', 2);
    }
    if (value >= 1.0e4) {
        return QStringLiteral("%1 万").arg(value / 1.0e4, 0, 'f', 2);
    }
    return QString::number(value, 'f', 0);
}

QString FormatMB(double bytes) {
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

} // namespace

igQtVolumeRenderingWidget::igQtVolumeRenderingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtVolumeRendering) {
    ui->setupUi(this);
    initUI();
    initConnections();
    refresh();
}

igQtVolumeRenderingWidget::~igQtVolumeRenderingWidget() { delete ui; }

void igQtVolumeRenderingWidget::initUI() {
    ui->label_StepsHint->setStyleSheet(QStringLiteral("color: #9a9a9a;"));
    ui->label_Status->setStyleSheet(QStringLiteral("color: #9a9a9a;"));
    // 输入过程不要逐键回写/被定时刷新覆盖：只在回车或失焦时提交
    ui->spinBox_Steps->setKeyboardTracking(false);
    ui->doubleSpinBox_Opacity->setKeyboardTracking(false);
    ui->doubleSpinBox_ShellOpacity->setKeyboardTracking(false);

    // 面板打开期间定时刷新：视口大小/DPR 会随窗口缩放、跨屏拖动变化，
    // 帧率也要跟着动（400ms 足够跟手，又不会太吵）
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(400);
    connect(m_refreshTimer, &QTimer::timeout, this,
            &igQtVolumeRenderingWidget::refresh);
    m_refreshTimer->start();
}

void igQtVolumeRenderingWidget::initConnections() {
    connect(ui->spinBox_Steps, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVolumeRenderingWidget::onStepsValueChanged);
    connect(ui->doubleSpinBox_Opacity,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &igQtVolumeRenderingWidget::onOpacityValueChanged);
    connect(ui->comboBox_Preset, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &igQtVolumeRenderingWidget::onPresetChanged);
    connect(ui->pushButton_Recommend, &QPushButton::clicked, this,
            &igQtVolumeRenderingWidget::onRecommendClicked);
    connect(ui->pushButton_Apply, &QPushButton::clicked, this,
            &igQtVolumeRenderingWidget::onApplyClicked);
    connect(ui->pushButton_Reset, &QPushButton::clicked, this,
            &igQtVolumeRenderingWidget::onResetClicked);
    connect(ui->pushButton_Close, &QPushButton::clicked, this,
            [this]() { emit closeRequested(); });
    // 掩膜 / 车壳层（作用于“当前体层”）：0=关闭 1=挖空 2=实体车壳
    connect(ui->comboBox_MaskMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int mode) {
                if (m_updating || m_currentLayer < 0) { return; }
                auto scene = this->currentScene();
                if (scene == nullptr) { return; }
                scene->SetVolumeLayerMaskMode(
                        m_currentLayer,
                        static_cast<iGame::Scene::VolumeMaskMode>(mode));
                // 结果由定时刷新回填（重建时会往日志里写掩膜来源与占比）
            });
    // 车壳实体模式的外壳不透明度（当前体层，只改 uniform，不重建体数据）
    connect(ui->doubleSpinBox_ShellOpacity,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double opacity) {
                if (m_updating || m_currentLayer < 0) { return; }
                auto scene = this->currentScene();
                if (scene == nullptr) { return; }
                scene->SetVolumeLayerShellOpacity(
                        m_currentLayer,
                        static_cast<float>(opacity));
            });
    // 启用 / 停用当前体层
    connect(ui->checkBox_CurrentLayerEnabled, &QCheckBox::toggled, this,
            [this](bool checked) {
                if (m_updating || m_currentLayer < 0) { return; }
                auto scene = this->currentScene();
                if (scene == nullptr) { return; }
                scene->SetVolumeLayerEnabled(m_currentLayer, checked);
            });
}

iGame::Scene::Pointer igQtVolumeRenderingWidget::currentScene() const {
    return iGame::SceneManager::Instance()->GetCurrentScene();
}

void igQtVolumeRenderingWidget::refresh() {
    auto scene = this->currentScene();
    if (scene == nullptr) {
        updateReadout(iGame::Scene::VolumeRenderingInfo{}, false);
        return;
    }

    const iGame::Scene::VolumeRenderingInfo info =
            scene->GetVolumeRenderingInfo();
    updateReadout(info, true);

    // 关掉“立即生效”时用户可能正改着数，别用场景值覆盖他的编辑
    if (!m_pendingEdits) { fillQualityFromScene(info); }
    updateStepsHint(ui->spinBox_Steps->value());
}

void igQtVolumeRenderingWidget::fillQualityFromScene(
        const iGame::Scene::VolumeRenderingInfo& info) {
    m_updating = true;
    // 正在输入时不要用场景值覆盖用户输入（定时刷新会每 400ms 触发一次）
    if (info.stepCount > 0 && !IsSpinBoxEditing(ui->spinBox_Steps)) {
        ui->spinBox_Steps->setValue(
                std::clamp(info.stepCount, ui->spinBox_Steps->minimum(),
                           ui->spinBox_Steps->maximum()));
    }
    float layerOpacity = info.layer0Opacity;
    if (m_currentLayer == 1) { layerOpacity = info.layer1Opacity; }
    if (layerOpacity > 0.0f &&
        !IsSpinBoxEditing(ui->doubleSpinBox_Opacity)) {
        ui->doubleSpinBox_Opacity->setValue(
                std::clamp(static_cast<double>(layerOpacity),
                           ui->doubleSpinBox_Opacity->minimum(),
                           ui->doubleSpinBox_Opacity->maximum()));
    }
    m_updating = false;
}

void igQtVolumeRenderingWidget::updateReadout(
        const iGame::Scene::VolumeRenderingInfo& info, bool hasScene) {
    for (int axis = 0; axis < 3; ++axis) {
        m_volumeDimension[axis] = info.volumeDimension[axis];
    }

    // 参数只在 compute 光线步进下生效：其余情况置灰，避免“调了没反应”
    const bool raycast = hasScene && info.mode == iGame::Scene::VolumeRenderingMode::Raycast;
    for (QWidget* widget : std::initializer_list<QWidget*>{
                 ui->spinBox_Steps, ui->doubleSpinBox_Opacity,
                 ui->pushButton_Recommend, ui->comboBox_Preset,
                 ui->checkBox_LiveUpdate, ui->pushButton_Apply}) {
        widget->setEnabled(raycast);
    }
    // ---- 当前体层：由模型树当前选中的模型决定（与 ColorMapper 一致）----
    m_currentLayer = hasScene ? info.currentLayer : -1;
    {
        QString text = QStringLiteral("当前体层：-");
        if (m_currentLayer == 0) {
            text = QStringLiteral("当前体层：层0 - %1")
                           .arg(QString::fromStdString(info.layer0Name));
        } else if (m_currentLayer == 1) {
            text = QStringLiteral("当前体层：层1 - %1")
                           .arg(QString::fromStdString(info.layer1Name));
        } else if (hasScene) {
            text = QStringLiteral(
                    "当前体层：-（在模型树中选中一个体层模型）");
        }
        SetLabelText(ui->label_CurrentVolumeLayer, text);
    }

    const bool hasCurrentLayer = raycast && m_currentLayer >= 0;
    int currentMaskMode = 0;
    bool currentMaskAvailable = false;
    float currentShellOpacity = 0.9f;
    bool currentEnabled = true;
    if (m_currentLayer == 0) {
        currentMaskMode = info.layer0MaskMode;
        currentMaskAvailable = info.layer0MaskAvailable;
        currentShellOpacity = info.layer0ShellOpacity;
        currentEnabled = info.layer0Enabled;
    } else if (m_currentLayer == 1) {
        currentMaskMode = info.layer1MaskMode;
        currentMaskAvailable = info.layer1MaskAvailable;
        currentShellOpacity = info.layer1ShellOpacity;
        currentEnabled = info.layer1Enabled;
    }
    ui->doubleSpinBox_Opacity->setEnabled(hasCurrentLayer);
    {
        const QSignalBlocker blocker(ui->comboBox_MaskMode);
        ui->comboBox_MaskMode->setCurrentIndex(
                std::clamp(currentMaskMode, 0,
                           ui->comboBox_MaskMode->count() - 1));
    }
    ui->comboBox_MaskMode->setEnabled(hasCurrentLayer && currentMaskAvailable);
    {
        const QSignalBlocker blocker(ui->doubleSpinBox_ShellOpacity);
        if (!IsSpinBoxEditing(ui->doubleSpinBox_ShellOpacity)) {
            ui->doubleSpinBox_ShellOpacity->setValue(
                    std::clamp(static_cast<double>(currentShellOpacity),
                               ui->doubleSpinBox_ShellOpacity->minimum(),
                               ui->doubleSpinBox_ShellOpacity->maximum()));
        }
    }
    ui->doubleSpinBox_ShellOpacity->setEnabled(hasCurrentLayer &&
                                               currentMaskAvailable &&
                                               currentMaskMode == 2);
    {
        const QSignalBlocker blocker(ui->checkBox_CurrentLayerEnabled);
        ui->checkBox_CurrentLayerEnabled->setChecked(currentEnabled);
    }
    ui->checkBox_CurrentLayerEnabled->setEnabled(hasCurrentLayer);

    if (!hasScene) {
        for (QLabel* label : std::initializer_list<QLabel*>{
                     ui->label_ViewportValue, ui->label_ResolutionValue,
                     ui->label_DispatchValue, ui->label_DimensionValue,
                     ui->label_MemoryValue, ui->label_DensityValue,
                     ui->label_SampleValue, ui->label_FpsValue,
                     ui->label_FrameTimeValue}) {
            SetLabelText(label, QStringLiteral("-"));
        }
        SetLabelText(ui->label_Status, QStringLiteral("当前没有可用场景。"));
        return;
    }

    // ---- 视口大小 与 对应的渲染分辨率 ----
    const int vw = info.viewportWidth;
    const int vh = info.viewportHeight;
    const int rw = info.renderWidth;
    const int rh = info.renderHeight;
    SetLabelText(ui->label_ViewportValue,
                 QStringLiteral("%1 × %2 px").arg(vw).arg(vh));
    if (rw > 0 && rh > 0) {
        const double dpr = vw > 0 ? static_cast<double>(rw) / vw : 1.0;
        SetLabelText(ui->label_ResolutionValue,
                     QStringLiteral("%1 × %2 px（设备像素，DPR ×%3）")
                             .arg(rw)
                             .arg(rh)
                             .arg(dpr, 0, 'f', 2));
        SetLabelText(ui->label_DispatchValue,
                     QStringLiteral("%1 × %2 个 %3×%3 线程组")
                             .arg((rw + kDispatchGroupSize - 1) /
                                  kDispatchGroupSize)
                             .arg((rh + kDispatchGroupSize - 1) /
                                  kDispatchGroupSize)
                             .arg(kDispatchGroupSize));
    } else {
        SetLabelText(ui->label_ResolutionValue, QStringLiteral("-"));
        SetLabelText(ui->label_DispatchValue, QStringLiteral("-"));
    }

    // ---- 体数据规模与纹理显存（R16F + 可选 R32F 掩膜） ----
    const double nx = static_cast<double>(info.volumeDimension[0]);
    const double ny = static_cast<double>(info.volumeDimension[1]);
    const double nz = static_cast<double>(info.volumeDimension[2]);
    const double voxels = nx * ny * nz;
    if (voxels > 0.0) {
        SetLabelText(ui->label_DimensionValue,
                     QStringLiteral("%1 × %2 × %3（%4 体素）")
                             .arg(info.volumeDimension[0])
                             .arg(info.volumeDimension[1])
                             .arg(info.volumeDimension[2])
                             .arg(FormatCount(voxels)));
        QString memory = FormatMB(voxels * 2.0) + QStringLiteral("（R16F）");
        if (info.maskEnabled) {
            memory += QStringLiteral(" + ") + FormatMB(voxels * 4.0) +
                      QStringLiteral("（R32F 掩膜）");
        }
        SetLabelText(ui->label_MemoryValue, memory);
    } else {
        SetLabelText(ui->label_DimensionValue,
                     QStringLiteral("-（尚未构建体数据）"));
        SetLabelText(ui->label_MemoryValue, QStringLiteral("-"));
    }

    // ---- 每帧采样数：渲染分辨率像素数 × 步数（上界，提前终止会更少） ----
    const double pixels = static_cast<double>(rw) * static_cast<double>(rh);
    if (pixels > 0.0 && info.stepCount > 0) {
        SetLabelText(ui->label_SampleValue,
                     QStringLiteral("≈ %1 次/帧（%2 × %3 px × %4 步）")
                             .arg(FormatCount(pixels * info.stepCount))
                             .arg(rw)
                             .arg(rh)
                             .arg(info.stepCount));
    } else {
        SetLabelText(ui->label_SampleValue, QStringLiteral("-"));
    }

    // ---- 帧率与 GPU 帧时间 ----
    // 渲染是事件驱动的：空闲不重绘时按“空闲”显示，而不是误导性的 0 FPS
    if (!info.active) {
        SetLabelText(ui->label_FpsValue, QStringLiteral("空闲（无重绘）"));
    } else if (info.fps > 0.0) {
        SetLabelText(ui->label_FpsValue,
                     QStringLiteral("%1 FPS").arg(info.fps, 0, 'f', 1));
    } else {
        SetLabelText(ui->label_FpsValue, QStringLiteral("统计中…"));
    }
    SetLabelText(ui->label_FrameTimeValue,
                 info.gpuFrameTimeMs > 0.0
                         ? QStringLiteral("%1 ms（GPU，整帧）")
                                   .arg(info.gpuFrameTimeMs, 0, 'f', 1)
                         : QStringLiteral("-"));

    // ---- 状态行 ----
    QString status;
    switch (info.mode) {
        case iGame::Scene::VolumeRenderingMode::Raycast:
            status = QStringLiteral(
                    "当前：compute 光线步进（结构化网格），体层 %1，参数立即生效。")
                             .arg(info.layerCount);
            if (info.layerCount >= 2) {
                status += QStringLiteral(" 层0：%1，层1：%2。")
                                  .arg(QString::fromStdString(info.layer0Name))
                                  .arg(QString::fromStdString(info.layer1Name));
            }
            if (info.maskAvailable) {
                const int invalidPercent =
                        static_cast<int>(info.maskInvalidRatio * 100.0f + 0.5f);
                QString modeText;
                if (info.maskMode == 1) {
                    modeText = QStringLiteral("已挖空");
                } else if (info.maskMode == 2) {
                    modeText = QStringLiteral("实体（不透明度 %1）")
                                       .arg(info.shellOpacity, 0, 'f', 2);
                } else {
                    modeText = QStringLiteral("未启用");
                }
                status += QStringLiteral(" 掩膜可用：无效体素 %1%，当前%2。")
                                  .arg(invalidPercent)
                                  .arg(modeText);
            } else {
                status += QStringLiteral(
                        " 数据里没有 vtkValidPointMask/vtkGhostType，"
                        "掩膜不可用。");
            }
            break;
        case iGame::Scene::VolumeRenderingMode::Fragment:
            status = QStringLiteral(
                    "当前：旧的片元链表实现（非结构化网格或 compute 不可用），"
                    "步数与不透明度缩放不生效。");
            break;
        default:
            status = QStringLiteral(
                    "体绘制未开启：勾选工具栏「体渲染」后这些参数才会生效。");
            break;
    }
    SetLabelText(ui->label_Status, status);
}

int igQtVolumeRenderingWidget::recommendedSteps() const {
    const int longest =
            std::max(m_volumeDimension[0],
                     std::max(m_volumeDimension[1], m_volumeDimension[2]));
    if (longest <= 1) { return 0; }
    const double raw = longest * kRecommendedSamplesPerVoxel;
    // 对齐到 8 的倍数，便于和 /8 线程组、抖动的观感对齐
    const int rounded = static_cast<int>(std::ceil(raw / 8.0)) * 8;
    return std::clamp(rounded, 64, 4096);
}

void igQtVolumeRenderingWidget::updateStepsHint(int steps) {
    if (steps <= 0 || m_volumeDimension[0] <= 0 ||
        m_volumeDimension[1] <= 0 || m_volumeDimension[2] <= 0) {
        SetLabelText(ui->label_StepsHint,
                     QStringLiteral("体数据尚未构建，暂无采样密度参考。"));
        SetLabelText(ui->label_DensityValue, QStringLiteral("-"));
        return;
    }

    const double perStep[3] = {
            static_cast<double>(m_volumeDimension[0]) / steps,
            static_cast<double>(m_volumeDimension[1]) / steps,
            static_cast<double>(m_volumeDimension[2]) / steps};
    const int longest =
            std::max(m_volumeDimension[0],
                     std::max(m_volumeDimension[1], m_volumeDimension[2]));
    const double worst = static_cast<double>(longest) / steps;
    const int recommended = recommendedSteps();

    SetLabelText(ui->label_StepsHint,
                 QStringLiteral(
                         "每步跨体素：x %1 / y %2 / z %3（轴平行视线）；"
                         "> %4 会看到条带或漏采，推荐 ≥ %5 步")
                         .arg(perStep[0], 0, 'f', 2)
                         .arg(perStep[1], 0, 'f', 2)
                         .arg(perStep[2], 0, 'f', 2)
                         .arg(kUnderSamplingVoxelsPerStep, 0, 'f', 0)
                         .arg(recommended > 0 ? QString::number(recommended)
                                              : QStringLiteral("-")));

    QString density = QStringLiteral("%1 体素/步").arg(worst, 0, 'f', 2);
    if (longest > 0 && recommended > 0 && steps < recommended) {
        density += QStringLiteral("（偏粗，建议 ≥ %1 步）").arg(recommended);
        ui->label_DensityValue->setStyleSheet(QStringLiteral("color: #d0a040;"));
    } else {
        ui->label_DensityValue->setStyleSheet(QString());
    }
    SetLabelText(ui->label_DensityValue, density);
}

void igQtVolumeRenderingWidget::applyQuality() {
    auto scene = this->currentScene();
    if (scene == nullptr) { return; }

    // 步数是视图级参数；不透明度写到“当前体层”
    scene->SetVolumeRaycastingQuality(
            ui->spinBox_Steps->value(),
            scene->GetVolumeLayerOpacityScale(0));
    if (m_currentLayer >= 0) {
        scene->SetVolumeLayerOpacityScale(
                m_currentLayer,
                static_cast<float>(ui->doubleSpinBox_Opacity->value()));
    }
    m_pendingEdits = false;
    // 参数改动后采样密度提示与每帧采样数都要跟着变
    updateStepsHint(ui->spinBox_Steps->value());
    updateReadout(scene->GetVolumeRenderingInfo(), true);
}

void igQtVolumeRenderingWidget::onStepsValueChanged(int steps) {
    if (m_updating) { return; }
    // 值被手动改过就不再声称是某个预设
    if (ui->comboBox_Preset->currentIndex() != 0) {
        const QSignalBlocker blocker(ui->comboBox_Preset);
        ui->comboBox_Preset->setCurrentIndex(0);
    }
    updateStepsHint(steps);
    if (ui->checkBox_LiveUpdate->isChecked()) {
        applyQuality();
    } else {
        m_pendingEdits = true;
    }
}

void igQtVolumeRenderingWidget::onOpacityValueChanged(double opacity) {
    if (m_updating) { return; }
    if (ui->comboBox_Preset->currentIndex() != 0) {
        const QSignalBlocker blocker(ui->comboBox_Preset);
        ui->comboBox_Preset->setCurrentIndex(0);
    }
    if (ui->checkBox_LiveUpdate->isChecked()) {
        applyQuality();
    } else {
        m_pendingEdits = true;
    }
}

void igQtVolumeRenderingWidget::onPresetChanged(int index) {
    if (m_updating || index <= 0) { return; }

    int steps = ui->spinBox_Steps->value();
    double opacity = ui->doubleSpinBox_Opacity->value();
    switch (index) {
        case 1: // 快速预览
            steps = 128;
            opacity = 0.0005;
            break;
        case 2: // 标准
            steps = 512;
            opacity = 0.0010;
            break;
        case 3: { // 精细：按体数据推荐步数
            const int recommended = recommendedSteps();
            steps = recommended > 0 ? recommended : 1024;
            opacity = 0.0015;
            break;
        }
        default:
            return;
    }

    m_updating = true;
    ui->spinBox_Steps->setValue(
            std::clamp(steps, ui->spinBox_Steps->minimum(),
                       ui->spinBox_Steps->maximum()));
    ui->doubleSpinBox_Opacity->setValue(
            std::clamp(opacity, ui->doubleSpinBox_Opacity->minimum(),
                       ui->doubleSpinBox_Opacity->maximum()));
    m_updating = false;

    updateStepsHint(ui->spinBox_Steps->value());
    if (ui->checkBox_LiveUpdate->isChecked()) {
        applyQuality();
    } else {
        m_pendingEdits = true;
    }
}

void igQtVolumeRenderingWidget::onRecommendClicked() {
    const int recommended = recommendedSteps();
    if (recommended <= 0) {
        SetLabelText(ui->label_StepsHint,
                     QStringLiteral("体数据尚未构建，无法按体素推荐步数。"));
        return;
    }
    ui->spinBox_Steps->setValue(recommended); // 触发 valueChanged → 按需应用
}

void igQtVolumeRenderingWidget::onApplyClicked() { applyQuality(); }

void igQtVolumeRenderingWidget::onResetClicked() {
    m_updating = true;
    ui->spinBox_Steps->setValue(128);
    ui->doubleSpinBox_Opacity->setValue(0.001);
    ui->comboBox_Preset->setCurrentIndex(0);
    m_updating = false;

    updateStepsHint(ui->spinBox_Steps->value());
    applyQuality(); // 重置总是立即写回场景，避免“点了没反应”
}
