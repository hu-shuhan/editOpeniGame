//
// Created by m_ky on 2024/4/22.
//

#include <IQComponents/igQtAnimationTreeWidget_interpolate.h>
#include <IQComponents/igQtAnimationTreeWidget_snap.h>
#include <IQCore/igQtAnimationVcrController.h>
#include <IQWidgets/igQtAnimationWidget.h>
#include <QAbstractButton>

#include <qdebug.h>

#include <iGameFileIO.h>
#include <iGameSceneManager.h>
#include <iGameThreadPool.h>

/**
 * @class   igQtAnimationWidget
 * @brief   igQtAnimationWidget's brief
 */
igQtAnimationWidget::igQtAnimationWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::Animation) {
    ui->setupUi(this);
    VcrController = new igQtAnimationVcrController(this);

    connect(VcrController, &igQtAnimationVcrController::timeStepChanged_snap,
            this, &igQtAnimationWidget::playAnimation_snap);
    connect(VcrController,
            &igQtAnimationVcrController::timeStepChanged_interpolate, this,
            &igQtAnimationWidget::playAnimation_interpolate);
    connect(VcrController,
            &igQtAnimationVcrController::updateAnimationComponentsTimeStap,
            ui->treeWidget_snap,
            &igQtAnimationTreeWidget_snap::updateCurrentKeyframe);
    connect(VcrController,
            &igQtAnimationVcrController::updateAnimationComponentsTimeStap,
            ui->treeWidget_interpolate,
            &igQtAnimationTreeWidget_interpolate::updateCurrentKeyframe);
    connect(VcrController,
            &igQtAnimationVcrController::updateAnimationComponentsTimeStap,
            ui->SliderAnimationTrack, &QSlider::setValue);
    connect(VcrController, &igQtAnimationVcrController::finishPlaying, this,
            &igQtAnimationWidget::btnPlay_finishLoop);
    connect(ui->btnFirstFrame, &QPushButton::clicked, VcrController,
            &igQtAnimationVcrController::onFirstFrame);
    connect(ui->btnLastFrame, &QPushButton::clicked, VcrController,
            &igQtAnimationVcrController::onLastFrame);
    connect(ui->btnPreviousFrame, &QPushButton::clicked, VcrController,
            &igQtAnimationVcrController::onPreviousFrame);
    connect(ui->btnNextFrame, &QPushButton::clicked, VcrController,
            &igQtAnimationVcrController::onNextFrame);
    connect(ui->treeWidget_snap,
            &igQtAnimationTreeWidget_snap::keyframedChanged, VcrController,
            &igQtAnimationVcrController::updateCurrentKeyframe);
    connect(ui->treeWidget_interpolate,
            &igQtAnimationTreeWidget_interpolate::keyframedChanged,
            VcrController, &igQtAnimationVcrController::updateCurrentKeyframe);
    connect(ui->treeWidget_interpolate,
            &igQtAnimationTreeWidget_interpolate::
                    updateVcrControllerInterpolateData,
            VcrController, &igQtAnimationVcrController::updateInterpolate);
    connect(ui->treeWidget_interpolate,
            &igQtAnimationTreeWidget_interpolate::updateComponentsKeyframeSum,
            this, &igQtAnimationWidget::updateAnimationComponentsKeyframeSum);
    connect(ui->rbtnSnapTimeMode, SIGNAL(toggled(bool)), this,
            SLOT(changeAnimationMode()));


    connect(ui->btnPlayOrPause, &QPushButton::toggled, this, [&](bool checked) {
        if (checked) {
            if (ui->btnReverseOrPause->isChecked()) {
                ui->btnPlayOrPause->setChecked(false);
                ui->btnReverseOrPause->setChecked(false);
            } else {
                VcrController->onPlay(true);
                ui->btnPlayOrPause->setIcon(
                        QIcon(":/Ticon/Icons/VcrPause.svg"));
            }
        } else {
            VcrController->onPause();
            ui->btnPlayOrPause->setIcon(QIcon(":/Ticon/Icons/VcrPlay.svg"));
        }
    });
    connect(ui->btnReverseOrPause, &QPushButton::toggled, this,
            [&](bool checked) {
                if (checked) {
                    if (ui->btnPlayOrPause->isChecked())
                        ui->btnReverseOrPause->setChecked(false),
                                ui->btnPlayOrPause->setChecked(false);
                    else {
                        VcrController->onPlay(false);
                        ui->btnReverseOrPause->setIcon(
                                QIcon(":/Ticon/Icons/VcrPause.svg"));
                    }
                } else {
                    VcrController->onPause();
                    ui->btnReverseOrPause->setIcon(
                            QIcon(":/Ticon/Icons/VcrReverse.svg"));
                }
            });
    connect(ui->btnLoop, &QPushButton::toggled, this, [&](bool checked) {
        VcrController->onLoop(checked);
        if (checked) {
            ui->btnLoop->setIcon(QIcon(":/Ticon/Icons/VcrLoop.svg"));
        } else {
            ui->btnLoop->setIcon(QIcon(":/Ticon/Icons/VcrDisabledLoop.svg"));
        }
    });
    connect(ui->btnApplyAnimationOperation, &QPushButton::clicked, this,
            [&](bool checked) {
                VcrController->setStripe(ui->spinBoxAnimationStride->value());
                ui->treeWidget_interpolate->updateInterpolateData(
                        ui->lineEditStartTime->text().toFloat(),
                        ui->lineEditEndTime->text().toFloat(),
                        ui->lineEditKeyframeNum->text().toInt());
            });

    auto* validator = new QIntValidator(
            2, 999, this); // 限制关键帧输入范围为2到999，根据需要修改
    auto* LineValidator =
            new QRegExpValidator(QRegExp("^[0-9]*\\.?[0-9]*$"), this);
    ui->lineEditStartTime->setValidator(LineValidator);
    ui->lineEditEndTime->setValidator(LineValidator);
    ui->lineEditKeyframeNum->setValidator(validator);
    //    ui->treeWidget_interpolate->header()->show();
    ui->treeWidget_interpolate->hide();


    //  Init the Animation Components if  model have the time value.
    //    std::vector<float> timevalue{1.0, 2.0, 3.0, 4.0};
    std::vector<float> timevalue{};
    if (timevalue.empty() || timevalue.size() == 1) return;
    VcrController->initController(static_cast<int>(timevalue.size()), 1);
    ui->treeWidget_snap->initAnimationTreeWidget(timevalue);
    ui->treeWidget_interpolate->initAnimationTreeWidget(timevalue);
    ui->SliderAnimationTrack->setMaximum(static_cast<int>(timevalue.size()) -
                                         1);
    ui->SliderAnimationTrack->setMinimum(0);
    ui->SliderAnimationTrack->setValue(0);

    ui->lineEditKeyframeNum->setText(
            QString("%1").arg(static_cast<int>(timevalue.size())));
    ui->lineEditStartTime->setText(
            QString::asprintf("%.f", *timevalue.begin()));
    ui->lineEditEndTime->setText(
            QString::asprintf("%.20f", *(timevalue.end() - 1)));
    connect(ui->SliderAnimationTrack, &QSlider::sliderMoved, VcrController,
            &igQtAnimationVcrController::updateCurrentKeyframe);
}

void igQtAnimationWidget::playAnimation_snap(int keyframe_idx) {
    using namespace iGame;
    auto currentScene = SceneManager::Instance()->GetCurrentScene();
    auto currentDrawObject = DynamicCast<DrawObject>(
            currentScene->GetCurrentModel()->GetDataObject());
    if (currentDrawObject == nullptr ||
        currentDrawObject->GetTimeFrames()->GetArrays().empty())
        return;
    auto& frameSubFiles = currentDrawObject->GetTimeFrames()
                                  ->GetTargetTimeFrame(keyframe_idx)
                                  .SubFileNames;
    {
        std::vector<std::future<iGame::DataObject::Pointer>> tasks;

        for (int i = 0; i < frameSubFiles->GetNumberOfElements(); i++) {
            tasks.emplace_back(iGame::ThreadPool::Instance()->Commit(
                    [](const std::string& fileName) {
                        return FileIO::ReadFile(fileName);
                    },
                    frameSubFiles->GetElement(i)));
        }
        currentDrawObject->ClearSubDataObject();

        for (auto& task: tasks) {
            currentDrawObject->AddSubDataObject(task.get());
        }
    }


    //    for(int i = 0; i < frameSubFiles->Size(); i ++){
    //        auto sub = FileIO::ReadFile(frameSubFiles->GetElement(i));
    //        currentObject->AddSubDataObject(sub);
    //    }

    /* process Object's scalar range*/
    IGsize scalar_size = currentDrawObject->GetAttributeSet()
                                 ? currentDrawObject->GetAttributeSet()
                                           ->GetAllAttributes()
                                           ->GetNumberOfElements()
                                 : 0;
    for (IGsize k = 0; k < scalar_size; k++) {
        auto& par_range =
                currentDrawObject->GetAttributeSet()->GetAttribute(k).dataRange;
        //        float range_max = FLT_MIN;
        //        float range_min = FLT_MAX;
        //        for(auto it = currentObject->SubDataObjectIteratorBegin(); it != currentObject->SubDataObjectIteratorEnd(); ++ it){
        //            const auto& range = it->second->GetAttributeSet()->GetAttribute(k).dataRange;
        //            range_max = std::max(range_max, range.second);
        //            range_min = std::min(range_min, range.first );
        //        }
        for (auto it = currentDrawObject->SubDataObjectIteratorBegin();
             it != currentDrawObject->SubDataObjectIteratorEnd(); ++it) {
            auto& range =
                    it->second->GetAttributeSet()->GetAttribute(k).dataRange;
            range.second = par_range.second;
            range.first = par_range.first;
        }
        //        std::cout << "range : " << range_max << ' ' << range_min << '\n';
    }
    currentScene->MakeCurrent();

    currentDrawObject->SetViewStyle(currentDrawObject->GetViewStyle());
    currentDrawObject->ConvertToDrawableData();

    //    for(auto it = currentObject->SubDataObjectIteratorBegin(); it != currentObject->SubDataObjectIteratorEnd(); ++ it){
    //        it->second->SetViewStyle(currentObject->GetViewStyle());
    //        it->second->ConvertToDrawableData();
    //    }
    if (currentDrawObject->GetAttributeIndex() != -1) {
        currentDrawObject->ViewCloudPicture(
                currentScene, currentDrawObject->GetAttributeIndex());
    }
    //    else if(frameSubFiles->Size() == 1){
    //        auto newDataObject = FileIO::ReadFile(frameSubFiles->GetElement(0));
    //        newDataObject->SetTimeFrames(currentObject->GetTimeFrames());
    //
    //        currentScene->RemoveCurrentModel();
    //        currentScene->GetCurrentModel()->SetDataObject(newDataObject);
    //        currentScene->AddModel(currentScene->CreateModel(newDataObject));
    //        newDataObject->SwitchToCurrentTimeframe(keyframe_idx);
    //        newDataObject->SetScalarRange(currentObject->GetScalarRange());
    //        newDataObject->ConvertToDrawableData();
    //        newDataObject->ViewCloudPicture(currentScene, 0);
    //    }
    currentScene->DoneCurrent();

    Q_EMIT UpdateScene();
}

void igQtAnimationWidget::playAnimation_interpolate(int keyframe_0, float t) {
    Q_EMIT PlayAnimation_interpolate(keyframe_0, t);
}

void igQtAnimationWidget::btnPlay_finishLoop() {
    ui->btnPlayOrPause->setChecked(false);
    ui->btnReverseOrPause->setChecked(false);
}

void igQtAnimationWidget::updateAnimationComponentsKeyframeSum(
        int keyframeSum) {
    VcrController->setKeyframe_sum(keyframeSum);
    ui->SliderAnimationTrack->setValue(0);
    ui->SliderAnimationTrack->setMaximum(keyframeSum - 1);
}

void igQtAnimationWidget::changeAnimationMode() {
    if (ui->rbtnSnapTimeMode->isChecked()) {
        ui->treeWidget_interpolate->hide();
        ui->treeWidget_snap->show();
        updateAnimationComponentsKeyframeSum(
                ui->treeWidget_snap->getKeyframeSize());
        VcrController->setInterpolateMode(false);
    } else {
        ui->treeWidget_snap->hide();
        ui->treeWidget_interpolate->show();
        updateAnimationComponentsKeyframeSum(
                ui->treeWidget_interpolate->getKeyframeSize());
        VcrController->setInterpolateMode(true);
    }
}

void igQtAnimationWidget::initAnimationComponents() {
    if (iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel() ==
                nullptr ||
        iGame::SceneManager::Instance()
                        ->GetCurrentScene()
                        ->GetCurrentModel()
                        ->GetDataObject()
                        ->GetTimeFrames() == nullptr)
        return;

    auto timeArrays = iGame::SceneManager::Instance()
                              ->GetCurrentScene()
                              ->GetCurrentModel()
                              ->GetDataObject()
                              ->GetTimeFrames()
                              ->GetArrays();
    if (timeArrays.empty()) return;
    std::vector<float> timeValues;
    timeValues.reserve(timeArrays.size());
    for (auto& timeArray: timeArrays) timeValues.push_back(timeArray.timeValue);
    VcrController->initController(static_cast<int>(timeValues.size()), 1);
    ui->treeWidget_snap->initAnimationTreeWidget(timeValues);
    ui->treeWidget_interpolate->initAnimationTreeWidget(timeValues);
    ui->SliderAnimationTrack->setMaximum(static_cast<int>(timeValues.size()) -
                                         1);
    ui->SliderAnimationTrack->setMinimum(0);
    ui->SliderAnimationTrack->setValue(0);

    ui->lineEditKeyframeNum->setText(
            QString("%1").arg(static_cast<int>(timeValues.size())));
    ui->lineEditStartTime->setText(
            QString::asprintf("%.f", *timeValues.begin()));
    ui->lineEditEndTime->setText(
            QString::asprintf("%.20f", *(timeValues.end() - 1)));
    connect(ui->SliderAnimationTrack, &QSlider::sliderMoved, VcrController,
            &igQtAnimationVcrController::updateCurrentKeyframe);
}
