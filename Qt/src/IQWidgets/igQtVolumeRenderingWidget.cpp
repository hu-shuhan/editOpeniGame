#include "IQWidgets/igQtVolumeRenderingWidget.h"

#include "ui_igQtVolumeRendering.h"

#include <iGameSceneManager.h>

#include <QComboBox>
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

    // 面板打开期间定时刷新：视口大小/DPR 会随窗口缩放、跨屏拖动变化
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(700);
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
    if (info.stepCount > 0) {
        ui->spinBox_Steps->setValue(
                std::clamp(info.stepCount, ui->spinBox_Steps->minimum(),
                           ui->spinBox_Steps->maximum()));
    }
    if (info.opacityScale > 0.0f) {
        ui->doubleSpinBox_Opacity->setValue(
                std::clamp(static_cast<double>(info.opacityScale),
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

    if (!hasScene) {
        for (QLabel* label : std::initializer_list<QLabel*>{
                     ui->label_ViewportValue, ui->label_ResolutionValue,
                     ui->label_DispatchValue, ui->label_DimensionValue,
                     ui->label_MemoryValue, ui->label_DensityValue,
                     ui->label_SampleValue}) {
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

    // ---- 体数据规模与纹理显存（R16F + 可选 R8 掩膜） ----
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
            memory += QStringLiteral(" + ") + FormatMB(voxels) +
                      QStringLiteral("（R8 掩膜）");
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

    // ---- 状态行 ----
    QString status;
    switch (info.mode) {
        case iGame::Scene::VolumeRenderingMode::Raycast:
            status = QStringLiteral(
                    "当前：compute 光线步进（结构化网格），参数立即生效。");
            if (info.maskEnabled) {
                status += QStringLiteral(" 已启用体数据掩膜。");
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

    scene->SetVolumeRaycastingQuality(ui->spinBox_Steps->value(),
                                      static_cast<float>(
                                              ui->doubleSpinBox_Opacity->value()));
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
            opacity = 0.05;
            break;
        case 2: // 标准
            steps = 512;
            opacity = 0.10;
            break;
        case 3: { // 精细：按体数据推荐步数
            const int recommended = recommendedSteps();
            steps = recommended > 0 ? recommended : 1024;
            opacity = 0.15;
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
    ui->doubleSpinBox_Opacity->setValue(0.1);
    ui->comboBox_Preset->setCurrentIndex(0);
    m_updating = false;

    updateStepsHint(ui->spinBox_Steps->value());
    applyQuality(); // 重置总是立即写回场景，避免“点了没反应”
}
