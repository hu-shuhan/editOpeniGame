/**
 * @class   igQtShadingWidget
 * @brief   着色与材质设置面板（常驻窗口）
 *
 * 窗口形式与 igQtColorManagerWidget 一致：独立无边框顶层窗口，可拖动、常驻置顶，
 * 由「可视化 → 着色与材质设置」菜单项控制显示/隐藏。参数改动立即写回当前
 * Scene 并请求重绘，因此没有「确定 / 取消」，只提供「恢复默认」。
 *
 * 面板内容：
 *   - 着色模型：PBR / Blinn-Phong 二选一；
 *   - PBR 材质：金属度、粗糙度、环境遮蔽强度（对应 PBR.frag 的
 *     uMetallic / uRoughness / uAoStrength）。环境遮蔽是「法线朝向」的简易半球
 *     近似，不是屏幕空间 AO；色带模型由着色器强制关闭；
 *   - Blinn-Phong 高光：高光强度、高光指数（对应 BlinnPhong.frag 的
 *     uSpecularStrength / uShininess）。
 *   - normal 来源：当前模型的三分量点属性或 none，仅设置法线数据输入。
 *
 * 材质参数保存在 Scene 上，属于【全局】设置（所有模型共用一套），与项目中
 * DrawObject 上的默认颜色/线宽等逐对象参数不同。
 * normal 来源保存在 DrawObject 上，不修改法线平滑开关。
 */

#pragma once

#include <IQCore/igQtExportModule.h>

#include <QWidget>

class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QEvent;
class QLabel;
class QPaintEvent;
class QPushButton;
class QRadioButton;
class QSlider;
class QSpinBox;

class IG_QT_MODULE_EXPORT igQtShadingWidget : public QWidget {
    Q_OBJECT
public:
    explicit igQtShadingWidget(QWidget* parent = nullptr);
    ~igQtShadingWidget() override;

    /** 从当前场景回填控件状态（窗口首次显示或场景切换后调用）。 */
    void syncFromScene();
    void syncNormalSource();

signals:
    /** 着色模型或参数发生变更后发出，主窗口据此刷新渲染视图。 */
    void shadingChanged();

public slots:
    /** 材质参数恢复默认值，保留当前模型的 normal 来源。 */
    void resetToDefaults();

protected:
    void paintEvent(QPaintEvent* event) override;
    void changeEvent(QEvent* e) override;

private slots:
    void onShadingModeChanged();
    void onPbrValueChanged();
    void onBlinnPhongValueChanged();

private:
    void buildUi();
    /** 把界面上的取值写回当前场景并请求重绘；无场景时静默返回。 */
    void applyToScene();

    QRadioButton* m_PbrRadio{nullptr};
    QRadioButton* m_BlinnPhongRadio{nullptr};

    QSlider* m_MetallicSlider{nullptr};
    QDoubleSpinBox* m_MetallicSpin{nullptr};
    QSlider* m_RoughnessSlider{nullptr};
    QDoubleSpinBox* m_RoughnessSpin{nullptr};
    QSlider* m_AoSlider{nullptr};
    QDoubleSpinBox* m_AoSpin{nullptr};

    QSlider* m_SpecularSlider{nullptr};
    QDoubleSpinBox* m_SpecularSpin{nullptr};
    QSlider* m_ShininessSlider{nullptr};
    QSpinBox* m_ShininessSpin{nullptr};

#ifndef __EMSCRIPTEN__
    QCheckBox* m_EnvironmentEnabled{nullptr};
    QDoubleSpinBox* m_EnvironmentIntensity{nullptr};
    QDoubleSpinBox* m_EnvironmentRotation{nullptr};
#endif

    // 用于抑制「滑块 <-> 数值框」联动时的递归触发。
    bool m_Syncing{false};
    QComboBox* m_NormalSourceCombo{nullptr};
    QLabel* m_NormalSourceHint{nullptr};
};
