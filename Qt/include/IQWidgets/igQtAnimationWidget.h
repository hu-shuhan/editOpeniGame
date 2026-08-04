/**
 * @class   igQtAnimationWidget
 * @brief   igQtAnimationWidget's brief
 */

#pragma once
#include <ui_Animation.h>
#include <IQCore/igQtExportModule.h>
#include <iGameDataObject.h>
class igQtAnimationVcrController;
class IG_QT_MODULE_EXPORT igQtAnimationWidget : public QWidget{

    Q_OBJECT

public:
    igQtAnimationWidget(QWidget* parent = nullptr);

    // 检查动画是否正在播放（用于阻止播放期间重新初始化组件）
    bool IsPlaying() const { return m_IsAnimationPlaying; }

    // 声明「至少需要缓存多少帧」。
    // 逐帧计算出的派生属性（如涡量）只存在于帧对象上，一旦缓存被清空，
    // 下次切帧会从磁盘重新读出不含该属性的新对象，既丢结果又会造成父子属性数不一致。
    // 设置后 initAnimationComponents 不再把缓存重置为 0。
    void setPreferredCacheNum(int n);

    // 开启 / 关闭「播放时按需计算涡量」。
    // 开启后每次切帧都会检查当前帧是否已有 vorticities：
    // 命中缓存（帧对象上已带该属性）则直接复用，否则同步计算完再继续渲染该帧。
    void setVortexAutoCompute(bool enabled, const std::string& sourceAttrName = std::string());
    bool isVortexAutoCompute() const { return m_VortexAutoCompute; }

    // 确保 obj 的「当前帧」已有 vorticities：已存在则直接返回（不产生任何开销，
    // 也不会触碰进度条），不存在才真正计算，并把该属性登记到父容器的
    // AttributeSet（模型树 / 云图靠它寻址）。
    // frameIndexForDisplay 仅用于进度条文字，传 -1 表示不显示帧号。
    // 返回 vorticities 在父容器中的索引；失败返回 -1。
    int ensureVortexForCurrentFrame(iGame::DataObject::Pointer obj, const std::string& sourceAttrName,
                                    int frameIndexForDisplay = -1);

public slots:
    void initAnimationComponents();

    bool saveAnimation();

    void ClearAnimationVCRInfo();
private slots:
    void playAnimation_snap(unsigned int keyframe_idx);
    void playAnimation_interpolate(int keyframe_0, float t);
    void btnPlay_finishLoop();
    void updateAnimationComponentsKeyframeSum(int keyframeSum);
    void changeAnimationMode();
    void onCacheNumChanged(int cacheNum);  // 缓存数量变化槽函数


signals:
    void UpdateScene();
    void AnimationFrameChanged();  // Signal when animation frame changes, triggers scalar UI update

    void PlayAnimation_snap(int keyframe_idx);

    void PlayAnimation_interpolate(int keyframe_0, float t);


private:
    Ui::Animation* ui;
    igQtAnimationVcrController* VcrController;
    bool m_IsAnimationPlaying{false}; // 动画播放状态标记
    int m_PreferredCacheNum{0};       // 外部声明的最小缓存帧数，见 setPreferredCacheNum
    bool m_VortexAutoCompute{false};  // 播放时按需补算涡量，见 setVortexAutoCompute
    std::string m_VortexSourceAttr;   // 计算涡量所用的源矢量属性名
    // 上次绑定按需计算的模型；用于「切换模型时自动关闭」，
    // 而在同一模型上选属性（同样会触发 initAnimationComponents）时保持开启
    iGame::DataObject* m_VortexBoundModel{nullptr};
};
