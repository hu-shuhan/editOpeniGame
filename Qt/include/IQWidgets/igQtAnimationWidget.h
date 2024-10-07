/**
 * @class   igQtAnimationWidget
 * @brief   igQtAnimationWidget's brief
 */

#pragma once
#include <ui_Animation.h>
#include <IQCore/igQtExportModule.h>

class igQtAnimationVcrController;
class IG_QT_MODULE_EXPORT igQtAnimationWidget : public QWidget{

    Q_OBJECT

public:
    igQtAnimationWidget(QWidget* parent = nullptr);

public slots:
    void initAnimationComponents();

    bool saveAnimation();

private slots:
    void playAnimation_snap(unsigned int keyframe_idx);
    void playAnimation_interpolate(int keyframe_0, float t);
    void btnPlay_finishLoop();
    void updateAnimationComponentsKeyframeSum(int keyframeSum);
    void changeAnimationMode();


signals:
    void UpdateScene();

    void PlayAnimation_snap(int keyframe_idx);

    void PlayAnimation_interpolate(int keyframe_0, float t);

private:
    Ui::Animation* ui;
    igQtAnimationVcrController* VcrController;
};
