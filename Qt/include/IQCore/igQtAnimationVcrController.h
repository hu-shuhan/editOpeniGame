//
// Created by m_ky on 2024/2/27.
//

#ifndef IGAMEVIEW_LITE_ANIMATIONVCRCONTROLLER_H
#define IGAMEVIEW_LITE_ANIMATIONVCRCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <vector>
#include "igQtExportModule.h"
#include "igQtAnimationOutputCache.h"

class IG_QT_MODULE_EXPORT igQtAnimationVcrController : public QObject {
Q_OBJECT
public:
    explicit igQtAnimationVcrController(QObject *parent = nullptr);

    ~igQtAnimationVcrController() override;


private:
    int keyframe_sum{0};
    int stepSize{1};

    int current_keyframe_index{0};
    bool isForward{false};
    bool isLoop{false};
    QTimer* timer;

    bool isInterpolateMode{false};

    std::vector<std::pair<int, float>> interpolate_sequence;  //  1. keyframe_0_idx, 2. interpolate t

public:

    void initController(int keyframeSum, int step_size);

    // 当前帧在“源时间序列”中的下标（0 基）。供「转换当前帧」等功能读取。
    int currentKeyframeIndex() const { return current_keyframe_index; }
    int frameCount() const { return keyframe_sum; }
    bool frameRequest(int index, igQtAnimationFrameRequest& request) const;

    // 静默定位到某一帧：只刷新面板显示（滑条/关键帧树），不发出重新读帧的信号。
    // 用于读取器已经把某一帧的场数据挂到对象上时（ODB 默认最后一帧）对齐时间轴。
    void setCurrentKeyframeSilently(int idx);

    void onTick();

    void onPlay(bool);

    void onPause();

    void onLoop(bool);

    void setStripe(int);

    void setKeyframe_sum(int);

    void setInterpolateMode(bool);
private:

    void startTimer();

    void stopTimer();

    void clearInfo();

signals:
    void updateAnimationComponentsTimeStap(int keyframe_idx);

    void timeStepChanged_snap(int keyframe_idx);

    void timeStepChanged_interpolate(int keyframe_0, float t);

    void finishPlaying();

public slots:

    void updateCurrentKeyframe(int);

    void onFirstFrame();

    void onLastFrame();

    void onPreviousFrame();

    void onNextFrame();

    void updateInterpolate(std::vector<std::pair<int, float>>& seq);
};


#endif //IGAMEVIEW_LITE_ANIMATIONVCRCONTROLLER_H
