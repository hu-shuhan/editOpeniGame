#include "IQWidgets/igQtShadingWidget.h"

#include "IQCore/igQtFramelessWidget.h"
#include <IQWidgets/igQtRenderWidget.h>

#include <iGameSceneManager.h>
#include <iGameModel.h>
#include <iGameDrawObject.h>

#include <QComboBox>
#include <QCheckBox>
#include <QAbstractItemView>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QTimer>

namespace {
// 与 PBR.frag / BlinnPhong.frag 的 uniform 默认值以及 Scene 的成员默认值保持一致。
constexpr double kDefaultMetallic = 0.0;
constexpr double kDefaultRoughness = 0.35;
// 与 Scene::m_SurfaceAo / PBR.frag 的 uAoStrength 默认值保持一致。
// 1.0 = 全强度（默认打开）；0.0 = 关闭，画面与该参数加入前完全一致。
constexpr double kDefaultAo = 1.0;
constexpr double kDefaultSpecular = 0.0;
constexpr int kDefaultShininess = 32;

// 浮点参数滑块的细分：0..kSliderSteps 线性映射到 [lo, hi]。
constexpr int kSliderSteps = 1000;

// 粗糙度 0..1 与着色器一一对应（PBR.frag 已去掉内部下限，与 ParaView 语义一致）：
// 0.00 = 最光滑（镜面高光消失），1.00 = 完全哑光。
// 注意 (0, 0.045] 这一段 GGX 峰值 1/(π·roughness⁴) 极大，高光会变成亚像素白点，
// 交互浏览时可能看到闪烁，这是这种"解析直接光 + 无抗锯齿"做法的固有代价。
constexpr double kRoughnessMin = 0.0;
constexpr double kRoughnessMax = 1.0;
constexpr int kShininessMin = 1;
constexpr int kShininessMax = 100;

double SliderToValue(int raw, double lo, double hi) {
    return lo + (hi - lo) * (static_cast<double>(raw) / kSliderSteps);
}

int ValueToSlider(double value, double lo, double hi) {
    if (hi <= lo) { return 0; }
    double t = (value - lo) / (hi - lo);
    if (t < 0.0) { t = 0.0; }
    if (t > 1.0) { t = 1.0; }
    return static_cast<int>(t * kSliderSteps + 0.5);
}

// 面板基础 QSS。这里的十六进制取值刻意使用项目调色板的角色 token
// （见 igQtRenderWidget 的 kTokenRoles），这样 igQtPanelTheme::attach 能在
// 浅色主题下自动重映射，不需要为两套主题各写一份。
QString PanelBaseQss() {
    return QStringLiteral(
            "QWidget { background: transparent; color: #D4D4D4; font-size: 12px; }"
            "QLabel#shadingTitleLabel { color: #E0E0E0; font-size: 13px;"
            " font-weight: 600; }"
            "QLabel#shadingHintLabel { color: #858585; }"
            "QGroupBox { border: 1px solid #3C3C3C; border-radius: 6px;"
            " margin-top: 14px; padding-top: 8px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px;"
            " padding: 0 4px; color: #D4D4D4; }"
            "QRadioButton { color: #D4D4D4; spacing: 6px; }"
            "QDoubleSpinBox, QSpinBox, QComboBox { background-color: #252526; color: #D4D4D4;"
            " border: 1px solid #3C3C3C; border-radius: 4px; min-height: 24px;"
            " padding: 0 4px; }"
            "QDoubleSpinBox:focus, QSpinBox:focus, QComboBox:focus { border: 1px solid #0E639C; }"
            "QComboBox QAbstractItemView { background-color: #252526; color: #D4D4D4;"
            " selection-background-color: #0E639C; }"
            "QPushButton { background-color: #2D2D30; color: #D4D4D4;"
            " border: 1px solid #3C3C3C; border-radius: 4px; min-height: 26px;"
            " padding: 0 14px; }"
            "QPushButton:hover { background-color: #3A3A3D; }"
            "QSlider::groove:horizontal { height: 4px; background: #3C3C3C;"
            " border-radius: 2px; }"
            "QSlider::sub-page:horizontal { background: #0E639C; border-radius: 2px; }"
            "QSlider::handle:horizontal { width: 12px; margin: -5px 0;"
            " border-radius: 6px; background: #D4D4D4; }");
}
} // namespace

igQtShadingWidget::igQtShadingWidget(QWidget* parent) : QWidget(parent) {
    // 与 igQtColorManagerWidget 相同的窗口形式：独立、无边框、常驻置顶。
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowTitle(QStringLiteral("着色与材质设置"));
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);
    setStyleSheet(PanelBaseQss());

    buildUi();

    // 无边框窗口的拖动沿用项目现成的 igQtFramelessWidget（与色条编辑窗口同一套外壳）。
    //
    // 只给叶子型的装饰标签设「鼠标穿透」：组件的拖动是在“事件送达本窗口”时启动的，
    // 而 QLabel 默认会吞掉按下事件，设了穿透后才能用标题/说明文字拖动窗口。
    //
    // 注意：绝对不能给 QGroupBox 这类容器设 WA_TransparentForMouseEvents——该属性
    // 的语义是「本控件及其所有子控件都不接收鼠标事件」，设到容器上会让组框里的
    // 滑块 / 数值框 / 单选框全部点不动，并且事件穿透到底层窗口被当成"空白区拖动"。
    for (QLabel* label : findChildren<QLabel*>()) {
        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    // 无边框外壳：可拖动，内容固定几行所以不允许拉伸。
    auto* frameless = new igQtFramelessWidget(this);
    frameless->setWidget(this);
    frameless->setPadding(6);
    frameless->setMoveEnable(true);
    frameless->setResizeEnable(false);

    igQtPanelTheme::attach(this);

    setFixedWidth(380);
    adjustSize();
    setFixedHeight(sizeHint().height());

    syncFromScene();
    // Refresh metadata while open, including filter edits and removed models.
    auto* normalRefresh = new QTimer(this);
    normalRefresh->setInterval(500);
    connect(normalRefresh, &QTimer::timeout, this, [this]() {
        if (isVisible() && !m_NormalSourceCombo->view()->isVisible()) syncNormalSource();
    });
    normalRefresh->start();
}

igQtShadingWidget::~igQtShadingWidget() {}

void igQtShadingWidget::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(8);

    auto* title = new QLabel(QStringLiteral("着色与材质设置"), this);
    title->setObjectName(QStringLiteral("shadingTitleLabel"));
    root->addWidget(title);

    // ---------- 着色模型 ----------
    auto* modeBox = new QGroupBox(QStringLiteral("着色模型"), this);
    auto* modeLayout = new QHBoxLayout(modeBox);
    modeLayout->setContentsMargins(12, 4, 12, 4);
    modeLayout->setSpacing(20);
    m_PbrRadio = new QRadioButton(QStringLiteral("PBR"), modeBox);
    m_BlinnPhongRadio = new QRadioButton(QStringLiteral("Blinn-Phong"), modeBox);
    m_PbrRadio->setChecked(true);
    modeLayout->addWidget(m_PbrRadio);
    modeLayout->addWidget(m_BlinnPhongRadio);
    modeLayout->addStretch(1);
    root->addWidget(modeBox);

#ifndef __EMSCRIPTEN__
    auto* environmentBox = new QGroupBox(QStringLiteral("环境照明"), this);
    auto* environmentGrid = new QGridLayout(environmentBox);
    environmentGrid->setContentsMargins(12, 4, 12, 6);
    environmentGrid->setHorizontalSpacing(10);
    environmentGrid->setVerticalSpacing(8);
    m_EnvironmentEnabled = new QCheckBox(QStringLiteral("启用 Qwantani Noon"), environmentBox);
    m_EnvironmentEnabled->setObjectName(QStringLiteral("environmentEnabled"));
    m_EnvironmentEnabled->setChecked(true);
    environmentGrid->addWidget(m_EnvironmentEnabled, 0, 0, 1, 2);
    m_EnvironmentBackgroundVisible = new QCheckBox(QStringLiteral("显示环境贴图背景"), environmentBox);
    m_EnvironmentBackgroundVisible->setObjectName(QStringLiteral("environmentBackgroundVisible"));
    m_EnvironmentBackgroundVisible->setToolTip(QStringLiteral("仅控制背景显示，与环境照明独立；共用旋转角度。"));
    m_EnvironmentIntensity = new QDoubleSpinBox(environmentBox);
    m_EnvironmentIntensity->setObjectName(QStringLiteral("environmentIntensity"));
    m_EnvironmentIntensity->setRange(0.0, 4.0);
    m_EnvironmentIntensity->setDecimals(2);
    m_EnvironmentIntensity->setSingleStep(0.1);
    m_EnvironmentIntensity->setValue(1.0);
    environmentGrid->addWidget(new QLabel(QStringLiteral("强度"), environmentBox), 1, 0);
    environmentGrid->addWidget(m_EnvironmentIntensity, 1, 1);
    m_EnvironmentRotation = new QDoubleSpinBox(environmentBox);
    m_EnvironmentRotation->setObjectName(QStringLiteral("environmentRotation"));
    m_EnvironmentRotation->setRange(0.0, 359.9);
    m_EnvironmentRotation->setDecimals(1);
    m_EnvironmentRotation->setSingleStep(5.0);
    m_EnvironmentRotation->setSuffix(QStringLiteral("°"));
    m_EnvironmentRotation->setWrapping(true);
    environmentGrid->addWidget(new QLabel(QStringLiteral("旋转"), environmentBox), 2, 0);
    environmentGrid->addWidget(m_EnvironmentRotation, 2, 1);
    auto* environmentHint = new QLabel(QStringLiteral("影响模型的照明与反射；PBR 模式下同样作用于云图。"), environmentBox);
    environmentHint->setWordWrap(true);
    environmentHint->setObjectName(QStringLiteral("shadingHintLabel"));
    environmentGrid->addWidget(m_EnvironmentBackgroundVisible, 3, 0, 1, 2);
    environmentGrid->addWidget(environmentHint, 4, 0, 1, 2);
    root->addWidget(environmentBox);
    connect(m_EnvironmentEnabled, &QCheckBox::toggled, this, [this](bool enabled) {
        m_EnvironmentIntensity->setEnabled(enabled);
        m_EnvironmentRotation->setEnabled(enabled || m_EnvironmentBackgroundVisible->isChecked());
        if (!m_Syncing) applyToScene();
    });
    connect(m_EnvironmentBackgroundVisible, &QCheckBox::toggled, this, [this](bool visible) {
        m_EnvironmentRotation->setEnabled(visible || m_EnvironmentEnabled->isChecked());
        if (!m_Syncing) applyToScene();
    });
    connect(m_EnvironmentIntensity, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double) { if (!m_Syncing) applyToScene(); });
    connect(m_EnvironmentRotation, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double) { if (!m_Syncing) applyToScene(); });
#endif

    auto* normalBox = new QGroupBox(QStringLiteral("normal（当前模型）"), this);
    auto* normalLayout = new QVBoxLayout(normalBox);
    normalLayout->setContentsMargins(12, 4, 12, 6);
    m_NormalSourceCombo = new QComboBox(normalBox);
    m_NormalSourceCombo->setObjectName(QStringLiteral("normalSourceCombo"));
    m_NormalSourceCombo->setToolTip(QStringLiteral("normal 来源：三分量数值点属性，元素数须与模型点数一致；none 表示不传入 normal。"));
    normalLayout->addWidget(m_NormalSourceCombo);
    m_NormalSourceHint = new QLabel(normalBox);
    m_NormalSourceHint->setObjectName(QStringLiteral("normalSourceHint"));
    m_NormalSourceHint->setWordWrap(true);
    m_NormalSourceHint->setMinimumHeight(54);
    normalLayout->addWidget(m_NormalSourceHint);
    root->addWidget(normalBox);
    connect(m_NormalSourceCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        auto* scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto draw = model ? DynamicCast<iGame::DrawObject>(model->GetDataObject()) : nullptr;
        if (draw && draw->SetNormalSource(m_NormalSourceCombo->itemData(index).toString().toStdString())) {
            scene->Update();
            emit shadingChanged();
        }
        syncNormalSource();
    });

    // ---------- PBR 材质 ----------
    auto* pbrBox = new QGroupBox(QStringLiteral("PBR 材质"), this);
    auto* pbrGrid = new QGridLayout(pbrBox);
    pbrGrid->setContentsMargins(12, 4, 12, 6);
    pbrGrid->setHorizontalSpacing(10);
    pbrGrid->setVerticalSpacing(8);
    pbrGrid->setColumnStretch(1, 1);

    m_MetallicSlider = new QSlider(Qt::Horizontal, pbrBox);
    m_MetallicSlider->setRange(0, kSliderSteps);
    m_MetallicSpin = new QDoubleSpinBox(pbrBox);
    m_MetallicSpin->setRange(0.0, 1.0);
    m_MetallicSpin->setSingleStep(0.01);
    m_MetallicSpin->setDecimals(2);
    m_MetallicSpin->setValue(kDefaultMetallic);
    pbrGrid->addWidget(new QLabel(QStringLiteral("金属度"), pbrBox), 0, 0);
    pbrGrid->addWidget(m_MetallicSlider, 0, 1);
    pbrGrid->addWidget(m_MetallicSpin, 0, 2);

    m_RoughnessSlider = new QSlider(Qt::Horizontal, pbrBox);
    m_RoughnessSlider->setRange(0, kSliderSteps);
    m_RoughnessSpin = new QDoubleSpinBox(pbrBox);
    m_RoughnessSpin->setRange(kRoughnessMin, kRoughnessMax);
    m_RoughnessSpin->setSingleStep(0.01);
    m_RoughnessSpin->setDecimals(2);
    m_RoughnessSpin->setValue(kDefaultRoughness);
    pbrGrid->addWidget(new QLabel(QStringLiteral("粗糙度"), pbrBox), 1, 0);
    pbrGrid->addWidget(m_RoughnessSlider, 1, 1);
    pbrGrid->addWidget(m_RoughnessSpin, 1, 2);

    // 环境遮蔽（简易半球近似）：只压暗间接光，直接光不受影响。
    m_AoSlider = new QSlider(Qt::Horizontal, pbrBox);
    m_AoSlider->setRange(0, kSliderSteps);
    m_AoSpin = new QDoubleSpinBox(pbrBox);
    m_AoSpin->setRange(0.0, 1.0);
    m_AoSpin->setSingleStep(0.01);
    m_AoSpin->setDecimals(2);
    m_AoSpin->setValue(kDefaultAo);
    m_AoSpin->setToolTip(QStringLiteral(
            "按法线朝向压暗间接光（半球近似，不是屏幕空间遮蔽）。\n"
            "0 = 关闭；普通模型与色带模型均参与。"));
    pbrGrid->addWidget(new QLabel(QStringLiteral("环境遮蔽强度"), pbrBox), 2, 0);
    pbrGrid->addWidget(m_AoSlider, 2, 1);
    pbrGrid->addWidget(m_AoSpin, 2, 2);


    root->addWidget(pbrBox);

    // ---------- Blinn-Phong 高光 ----------
    auto* blinnBox = new QGroupBox(QStringLiteral("Blinn-Phong 高光"), this);
    auto* blinnGrid = new QGridLayout(blinnBox);
    blinnGrid->setContentsMargins(12, 4, 12, 6);
    blinnGrid->setHorizontalSpacing(10);
    blinnGrid->setVerticalSpacing(8);
    blinnGrid->setColumnStretch(1, 1);

    m_SpecularSlider = new QSlider(Qt::Horizontal, blinnBox);
    m_SpecularSlider->setRange(0, kSliderSteps);
    m_SpecularSpin = new QDoubleSpinBox(blinnBox);
    m_SpecularSpin->setRange(0.0, 1.0);
    m_SpecularSpin->setSingleStep(0.01);
    m_SpecularSpin->setDecimals(2);
    m_SpecularSpin->setValue(kDefaultSpecular);
    blinnGrid->addWidget(new QLabel(QStringLiteral("高光强度"), blinnBox), 0, 0);
    blinnGrid->addWidget(m_SpecularSlider, 0, 1);
    blinnGrid->addWidget(m_SpecularSpin, 0, 2);

    m_ShininessSlider = new QSlider(Qt::Horizontal, blinnBox);
    m_ShininessSlider->setRange(kShininessMin, kShininessMax);
    m_ShininessSpin = new QSpinBox(blinnBox);
    m_ShininessSpin->setRange(kShininessMin, kShininessMax);
    m_ShininessSpin->setValue(kDefaultShininess);
    blinnGrid->addWidget(new QLabel(QStringLiteral("高光指数"), blinnBox), 1, 0);
    blinnGrid->addWidget(m_ShininessSlider, 1, 1);
    blinnGrid->addWidget(m_ShininessSpin, 1, 2);

    root->addWidget(blinnBox);

    auto* hint = new QLabel(
            QStringLiteral("材质参数为全局设置；normal 来源仅作用于当前模型。"),
            this);
    hint->setObjectName(QStringLiteral("shadingHintLabel"));
    hint->setWordWrap(true);
    root->addWidget(hint);

    // ---------- 恢复默认 ----------
    auto* buttonRow = new QHBoxLayout();
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->addStretch(1);
    auto* resetButton = new QPushButton(QStringLiteral("恢复材质默认"), this);
    buttonRow->addWidget(resetButton);
    root->addLayout(buttonRow);

    connect(resetButton, &QPushButton::clicked, this, &igQtShadingWidget::resetToDefaults);

    // 着色模型：两个单选按钮任一变化都重新取一次当前选择。
    connect(m_PbrRadio, &QRadioButton::toggled, this,
            &igQtShadingWidget::onShadingModeChanged);
    connect(m_BlinnPhongRadio, &QRadioButton::toggled, this,
            &igQtShadingWidget::onShadingModeChanged);

    // 滑块 <-> 数值框 双向联动，并用 QSignalBlocker 避免来回递归。
    connect(m_MetallicSlider, &QSlider::valueChanged, this, [this](int raw) {
        if (m_Syncing) { return; }
        {
            QSignalBlocker blocker(m_MetallicSpin);
            m_MetallicSpin->setValue(SliderToValue(raw, 0.0, 1.0));
        }
        onPbrValueChanged();
    });
    connect(m_MetallicSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_Syncing) { return; }
                {
                    QSignalBlocker blocker(m_MetallicSlider);
                    m_MetallicSlider->setValue(ValueToSlider(value, 0.0, 1.0));
                }
                onPbrValueChanged();
            });
    connect(m_RoughnessSlider, &QSlider::valueChanged, this, [this](int raw) {
        if (m_Syncing) { return; }
        {
            QSignalBlocker blocker(m_RoughnessSpin);
            m_RoughnessSpin->setValue(
                    SliderToValue(raw, kRoughnessMin, kRoughnessMax));
        }
        onPbrValueChanged();
    });
    connect(m_RoughnessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_Syncing) { return; }
                {
                    QSignalBlocker blocker(m_RoughnessSlider);
                    m_RoughnessSlider->setValue(
                            ValueToSlider(value, kRoughnessMin, kRoughnessMax));
                }
                onPbrValueChanged();
            });
    connect(m_AoSlider, &QSlider::valueChanged, this, [this](int raw) {
        if (m_Syncing) { return; }
        {
            QSignalBlocker blocker(m_AoSpin);
            m_AoSpin->setValue(SliderToValue(raw, 0.0, 1.0));
        }
        onPbrValueChanged();
    });
    connect(m_AoSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_Syncing) { return; }
                {
                    QSignalBlocker blocker(m_AoSlider);
                    m_AoSlider->setValue(ValueToSlider(value, 0.0, 1.0));
                }
                onPbrValueChanged();
            });

    connect(m_SpecularSlider, &QSlider::valueChanged, this, [this](int raw) {
        if (m_Syncing) { return; }
        {
            QSignalBlocker blocker(m_SpecularSpin);
            m_SpecularSpin->setValue(SliderToValue(raw, 0.0, 1.0));
        }
        onBlinnPhongValueChanged();
    });
    connect(m_SpecularSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (m_Syncing) { return; }
                {
                    QSignalBlocker blocker(m_SpecularSlider);
                    m_SpecularSlider->setValue(ValueToSlider(value, 0.0, 1.0));
                }
                onBlinnPhongValueChanged();
            });
    connect(m_ShininessSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_Syncing) { return; }
        {
            QSignalBlocker blocker(m_ShininessSpin);
            m_ShininessSpin->setValue(value);
        }
        onBlinnPhongValueChanged();
    });
    connect(m_ShininessSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int value) {
                if (m_Syncing) { return; }
                {
                    QSignalBlocker blocker(m_ShininessSlider);
                    m_ShininessSlider->setValue(value);
                }
                onBlinnPhongValueChanged();
            });
}

void igQtShadingWidget::onShadingModeChanged() {
    if (m_Syncing) { return; }
    applyToScene();
}

void igQtShadingWidget::onPbrValueChanged() {
    if (m_Syncing) { return; }
    applyToScene();
}

void igQtShadingWidget::onBlinnPhongValueChanged() {
    if (m_Syncing) { return; }
    applyToScene();
}

void igQtShadingWidget::applyToScene() {
    auto* scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene == nullptr) { return; }

    // 0 = Blinn-Phong，2 = PBR（1 = 无光照仅 Web 端使用，这里不暴露）。
    scene->SetSurfaceShadingMode(m_BlinnPhongRadio->isChecked() ? 0 : 2);
    scene->SetMaterialParameters(static_cast<float>(m_MetallicSpin->value()),
                                 static_cast<float>(m_RoughnessSpin->value()));
    scene->SetSurfaceAo(static_cast<float>(m_AoSpin->value()));
    scene->SetBlinnPhongParameters(static_cast<float>(m_SpecularSpin->value()),
                                   m_ShininessSpin->value());
#ifndef __EMSCRIPTEN__
    scene->SetEnvironmentEnabled(m_EnvironmentEnabled->isChecked());
    scene->SetEnvironmentBackgroundVisible(m_EnvironmentBackgroundVisible->isChecked());
    scene->SetEnvironmentIntensity(static_cast<float>(m_EnvironmentIntensity->value()));
    scene->SetEnvironmentRotation(static_cast<float>(m_EnvironmentRotation->value()));
#endif
    scene->Update();

    emit shadingChanged();
}

void igQtShadingWidget::syncFromScene() {
    m_Syncing = true;

    auto* scene = iGame::SceneManager::Instance()->GetCurrentScene();
    const bool blinnPhong = (scene != nullptr) && scene->GetSurfaceShadingMode() != 2;
    m_BlinnPhongRadio->setChecked(blinnPhong);
    m_PbrRadio->setChecked(!blinnPhong);

    double metallic = kDefaultMetallic;
    double roughness = kDefaultRoughness;
    double ao = kDefaultAo;
    double specular = kDefaultSpecular;
    int shininess = kDefaultShininess;
    if (scene != nullptr) {
        float m = 0.0f;
        float r = 0.0f;
        scene->GetMaterialParameters(m, r);
        metallic = m;
        roughness = r;
        ao = scene->GetSurfaceAo();

        float s = 0.0f;
        float sh = 32.0f;
        scene->GetBlinnPhongParameters(s, sh);
        specular = s;
        shininess = static_cast<int>(sh + 0.5f);
    }

    m_MetallicSpin->setValue(metallic);
    m_MetallicSlider->setValue(ValueToSlider(metallic, 0.0, 1.0));
    m_RoughnessSpin->setValue(roughness);
    m_RoughnessSlider->setValue(ValueToSlider(roughness, kRoughnessMin, kRoughnessMax));
    m_AoSpin->setValue(ao);
    m_AoSlider->setValue(ValueToSlider(ao, 0.0, 1.0));
    m_SpecularSpin->setValue(specular);
    m_SpecularSlider->setValue(ValueToSlider(specular, 0.0, 1.0));
    m_ShininessSpin->setValue(shininess);
    m_ShininessSlider->setValue(shininess);
#ifndef __EMSCRIPTEN__
    const bool environmentEnabled = scene ? scene->GetEnvironmentEnabled() : true;
    m_EnvironmentEnabled->setChecked(environmentEnabled);
    const bool backgroundVisible = scene ? scene->GetEnvironmentBackgroundVisible() : true;
    m_EnvironmentBackgroundVisible->setChecked(backgroundVisible);
    m_EnvironmentIntensity->setValue(scene ? scene->GetEnvironmentIntensity() : 1.0);
    m_EnvironmentRotation->setValue(scene ? scene->GetEnvironmentRotation() : 0.0);
    m_EnvironmentIntensity->setEnabled(environmentEnabled);
    m_EnvironmentRotation->setEnabled(environmentEnabled || backgroundVisible);
#endif

    m_Syncing = false;
    syncNormalSource();
}

void igQtShadingWidget::syncNormalSource() {
    auto* scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto model = scene ? scene->GetCurrentModel() : nullptr;
    auto draw = model ? DynamicCast<iGame::DrawObject>(model->GetDataObject()) : nullptr;
    const auto candidates = draw ? draw->GetNormalSourceCandidates() : std::vector<std::string>{};
    const auto source = draw ? draw->GetNormalSource() : std::string{};
    const QSignalBlocker blocker(m_NormalSourceCombo);
    bool rebuild = m_NormalSourceCombo->count() != static_cast<int>(candidates.size()) + 1;
    for (int i = 0; !rebuild && i < static_cast<int>(candidates.size()); ++i) {
        rebuild = m_NormalSourceCombo->itemData(i + 1).toString() != QString::fromStdString(candidates[i]);
    }
    if (rebuild) {
        m_NormalSourceCombo->clear();
        m_NormalSourceCombo->addItem(QStringLiteral("none"), QString());
        for (const auto& name : candidates) m_NormalSourceCombo->addItem(QString::fromStdString(name), QString::fromStdString(name));
    }
    m_NormalSourceCombo->setCurrentIndex(source.empty() ? 0 : m_NormalSourceCombo->findData(QString::fromStdString(source)));
    m_NormalSourceCombo->setEnabled(draw != nullptr);
    if (!draw) {
        m_NormalSourceHint->setText(QStringLiteral("请先选择一个模型。"));
    } else if (source.empty()) {
        m_NormalSourceHint->setText(QStringLiteral("未检测到或未指定 normal，当前使用面法线。可选择三分量点属性作为来源。"));
    } else {
        m_NormalSourceHint->setText(QStringLiteral("当前模型：%1\nnormal 来源：%2").arg(QString::fromStdString(model->GetDataObject()->GetName()), QString::fromStdString(source)));
    }
}

void igQtShadingWidget::resetToDefaults() {
    m_Syncing = true;
    m_PbrRadio->setChecked(true);
    m_BlinnPhongRadio->setChecked(false);
    m_MetallicSpin->setValue(kDefaultMetallic);
    m_MetallicSlider->setValue(ValueToSlider(kDefaultMetallic, 0.0, 1.0));
    m_RoughnessSpin->setValue(kDefaultRoughness);
    m_RoughnessSlider->setValue(
            ValueToSlider(kDefaultRoughness, kRoughnessMin, kRoughnessMax));
    m_AoSpin->setValue(kDefaultAo);
    m_AoSlider->setValue(ValueToSlider(kDefaultAo, 0.0, 1.0));
    m_SpecularSpin->setValue(kDefaultSpecular);
    m_SpecularSlider->setValue(ValueToSlider(kDefaultSpecular, 0.0, 1.0));
    m_ShininessSpin->setValue(kDefaultShininess);
    m_ShininessSlider->setValue(kDefaultShininess);
#ifndef __EMSCRIPTEN__
    m_EnvironmentEnabled->setChecked(true);
    m_EnvironmentBackgroundVisible->setChecked(true);
    m_EnvironmentIntensity->setValue(1.0);
    m_EnvironmentRotation->setValue(0.0);
#endif
    m_Syncing = false;

    applyToScene();
}

void igQtShadingWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRect r = this->rect().adjusted(1, 1, -1, -1);
    painter.setPen(
            QPen(igQtRenderWidget::uiRole(igQtRenderWidget::UiRole::BorderStrong), 2));
    painter.setBrush(igQtRenderWidget::uiRole(igQtRenderWidget::UiRole::PanelBg2));
    painter.drawRoundedRect(r, 10, 10);
}

void igQtShadingWidget::changeEvent(QEvent* e) {
    if (e && e->type() == QEvent::StyleChange) {
        igQtPanelTheme::refresh(this);
        update();
    }
    QWidget::changeEvent(e);
}
