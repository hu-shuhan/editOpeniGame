//
// Created by m_ky on 2024/3/19.
//

#pragma once
#include "igQtAnimationTreeWidget.h"
#include <IQCore/igQtExportModule.h>
class IG_QT_MODULE_EXPORT igQtAnimationTreeWidget_interpolate : public igQtAnimationTreeWidget{
    Q_OBJECT

public:
    explicit igQtAnimationTreeWidget_interpolate(QWidget *parent = nullptr);

    void initAnimationTreeWidget(std::vector<float> &timeSeq) override;

    void updateData();



    bool updateInterpolateData(float _start, float _end, int keyframeNum);
signals:

    void updateVcrControllerInterpolateData(std::vector<std::pair<int, float>>&);

    void updateComponentsKeyframeSum(int);

public slots:
    void updateCurrentKeyframe(int idx);

protected:

    void updateInterpolateSequence(int num);

protected:
    float startTime;
    float endTime;
    std::vector<float> interpolate_timeSequence;
};


