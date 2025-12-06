//
// Created by m_ky on 2024/2/27.
//

#include "IQCore/igQtAnimationVcrController.h"
//#include "QApplication"
#include "qdebug.h"
igQtAnimationVcrController::igQtAnimationVcrController(QObject *parent) : QObject(parent) {
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &igQtAnimationVcrController::onTick);

}

igQtAnimationVcrController::~igQtAnimationVcrController() {
    stopTimer();
}

void igQtAnimationVcrController::initController(int keyframeSum, int stepsize) {
    keyframe_sum = keyframeSum;
    stepSize = stepsize;
}

void igQtAnimationVcrController::onTick() {

    int step = isForward ? stepSize : -stepSize;
    if (current_keyframe_index + step < 0 && !isForward) {
        if (isLoop) step = keyframe_sum - 1 - stepSize;
        else step = -current_keyframe_index;
    } else if (current_keyframe_index + step > keyframe_sum - 1 && isForward) {
        if (isLoop) step = stepSize - keyframe_sum;
        else step = keyframe_sum - 1 - current_keyframe_index;
//        qDebug() << "dbg " << current_keyframe_index << ' ' << step;
    }
    if (step)
    {
        current_keyframe_index += step;
        Q_EMIT this->updateAnimationComponentsTimeStap(current_keyframe_index);
        if(isInterpolateMode) Q_EMIT this->timeStepChanged_interpolate(interpolate_sequence[current_keyframe_index].first, interpolate_sequence[current_keyframe_index].second);
        else Q_EMIT this->timeStepChanged_snap(current_keyframe_index);
    }
    else Q_EMIT this->finishPlaying();
//    qDebug() << "Playing current Keyframe : " << current_keyframe_index;

}

void igQtAnimationVcrController::onPlay(bool forward) {
    isForward = forward;
    startTimer();
//    qDebug() << "play";
}

void igQtAnimationVcrController::onPause() {
    stopTimer();
//    qDebug() << "pause";
}

void igQtAnimationVcrController::onLoop(bool checked) {
    isLoop = checked;
}

void igQtAnimationVcrController::startTimer() {
    if (keyframe_sum) timer->start(100);
}

void igQtAnimationVcrController::stopTimer() {
    timer->stop();
}


void igQtAnimationVcrController::clearInfo() {
    keyframe_sum = 0;
    current_keyframe_index = 0;
}

void igQtAnimationVcrController::onFirstFrame() {
    if (current_keyframe_index) {
        current_keyframe_index = 0;
        Q_EMIT this->updateAnimationComponentsTimeStap(current_keyframe_index);
        if(isInterpolateMode) Q_EMIT this->timeStepChanged_interpolate(interpolate_sequence[current_keyframe_index].first, interpolate_sequence[current_keyframe_index].second);
        else Q_EMIT this->timeStepChanged_snap(current_keyframe_index);
    }
}

void igQtAnimationVcrController::onLastFrame() {
    if (keyframe_sum && current_keyframe_index != keyframe_sum - 1) {
        current_keyframe_index = keyframe_sum - 1;
        Q_EMIT this->updateAnimationComponentsTimeStap(current_keyframe_index);
        if(isInterpolateMode) Q_EMIT this->timeStepChanged_interpolate(interpolate_sequence[current_keyframe_index].first, interpolate_sequence[current_keyframe_index].second);
        else Q_EMIT this->timeStepChanged_snap(current_keyframe_index);
    }
}

void igQtAnimationVcrController::onPreviousFrame() {
    if (current_keyframe_index - stepSize >= 0)
    {
        current_keyframe_index -= stepSize;
        Q_EMIT this->updateAnimationComponentsTimeStap(current_keyframe_index);
        if(isInterpolateMode) Q_EMIT this->timeStepChanged_interpolate(interpolate_sequence[current_keyframe_index].first, interpolate_sequence[current_keyframe_index].second);
        else Q_EMIT this->timeStepChanged_snap(current_keyframe_index);
    }
}

void igQtAnimationVcrController::onNextFrame() {
    if (current_keyframe_index + stepSize < keyframe_sum)
    {
        current_keyframe_index += stepSize;
        Q_EMIT this->updateAnimationComponentsTimeStap(current_keyframe_index);
        if(isInterpolateMode) Q_EMIT this->timeStepChanged_interpolate(interpolate_sequence[current_keyframe_index].first, interpolate_sequence[current_keyframe_index].second);
        else Q_EMIT this->timeStepChanged_snap(current_keyframe_index);
    }
}

void igQtAnimationVcrController::updateCurrentKeyframe(int idx) {
    if(idx < 0) return ;
    current_keyframe_index = idx;
    Q_EMIT this->updateAnimationComponentsTimeStap(current_keyframe_index);
    if(isInterpolateMode) Q_EMIT this->timeStepChanged_interpolate(interpolate_sequence[current_keyframe_index].first, interpolate_sequence[current_keyframe_index].second);
    else Q_EMIT this->timeStepChanged_snap(current_keyframe_index);
}

void igQtAnimationVcrController::setStripe(int step) {
    stepSize = step;
}

void igQtAnimationVcrController::setKeyframe_sum(int keyframeSum) {
    current_keyframe_index = 0;
    keyframe_sum = keyframeSum;
}

void igQtAnimationVcrController::setInterpolateMode(bool checked) {
    isInterpolateMode = checked;
}

void igQtAnimationVcrController::updateInterpolate(std::vector<std::pair<int, float>> &seq) {
    interpolate_sequence = std::move(seq);
//    for(auto& [a, b] : interpolate_sequence)
//    {
//        std::cout << a << ' ' << b << std::endl;
//    }
}
