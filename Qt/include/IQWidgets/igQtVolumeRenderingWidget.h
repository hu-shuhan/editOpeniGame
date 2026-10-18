#pragma once

// ============================================================================
// igQtVolumeRenderingWidget —「体绘制」参数面板
//
// 与 resample_to_image 等标准 filter 面板一样挂在左侧工具面板的 Tab 里，
// 开启体绘制时自动打开：
//   - 信息：视口大小（逻辑像素）、渲染分辨率（设备像素，即 compute 实际派发
//     的分辨率）、线程组派发数、体数据维度与纹理显存、采样密度、每帧采样数；
//   - 参数：采样步数（含“按体数据推荐”）、不透明度缩放、质量预设、
//     是否立即生效，以及 应用 / 重置 / 关闭。
//
// 参数只在 compute 光线步进（VolumeRenderingMode::Raycast）下生效；旧的
// 片元链表实现（非结构化网格/不支持 compute）会在状态行里说明，控件置灰。
// ============================================================================

#include <IQCore/igQtExportModule.h>

#include <QWidget>

#include <iGameScene.h>

class QLabel;
class QTimer;

namespace Ui {
class igQtVolumeRendering;
}

class IG_QT_MODULE_EXPORT igQtVolumeRenderingWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtVolumeRenderingWidget(QWidget* parent = nullptr);
    ~igQtVolumeRenderingWidget() override;

    /// 重新读取当前场景的体绘制状态并刷新界面（打开面板时调用；面板打开期间
    /// 也会定时刷新，保证视口/分辨率随窗口缩放变化）。
    void refresh();

signals:
    void closeRequested();

private slots:
    void onStepsValueChanged(int steps);
    void onOpacityValueChanged(double opacity);
    void onPresetChanged(int index);
    void onRecommendClicked();
    void onApplyClicked();
    void onResetClicked();

private:
    void initUI();
    void initConnections();

    /// 把当前控件的步数/不透明度写进场景（仅 compute 光线步进有效）
    void applyQuality();
    /// 用场景值回填控件（m_updating 抑制递归应用）
    void fillQualityFromScene(const iGame::Scene::VolumeRenderingInfo& info);
    /// 刷新信息区与状态行
    void updateReadout(const iGame::Scene::VolumeRenderingInfo& info,
                       bool hasScene);
    /// 刷新采样密度提示（依赖 updateReadout 缓存的体数据维度）
    void updateStepsHint(int steps);
    /// 按体数据最长轴推荐步数（约 1.5 采样/体素）；体数据未构建时返回 0
    int recommendedSteps() const;

    iGame::Scene::Pointer currentScene() const;

    Ui::igQtVolumeRendering* ui{nullptr};
    QTimer* m_refreshTimer{nullptr};
    /// 程序化回填控件时为 true，用来抑制 valueChanged 触发的应用
    bool m_updating{false};
    /// 关闭“立即生效”后用户改了参数但还没点应用：此时定时刷新不回填控件
    bool m_pendingEdits{false};
    /// 最近一次读到的体数据维度（采样密度提示用）
    int m_volumeDimension[3] = {0, 0, 0};
    /// 当前面板编辑的体层（-1 = 当前选中模型不是体层）：由模型树选中项决定
    int m_currentLayer = -1;
};
