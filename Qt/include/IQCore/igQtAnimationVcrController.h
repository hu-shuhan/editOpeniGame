//
// Created by m_ky on 2024/2/27.
//

#ifndef IGAMEVIEW_LITE_ANIMATIONVCRCONTROLLER_H
#define IGAMEVIEW_LITE_ANIMATIONVCRCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <vector>
#include "igQtExportModule.h"

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
    bool isPlaying{false};
    QTimer* timer;

    bool isInterpolateMode{false};

    std::vector<std::pair<int, float>> interpolate_sequence;  //  1. keyframe_0_idx, 2. interpolate t

public:

    void initController(int keyframeSum, int step_size);

    void onTick();

    void onPlay(bool);

    void onPause();

    void onFramePresented();

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
