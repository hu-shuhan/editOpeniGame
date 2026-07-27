//
// Created by m_ky on 2024/4/22.
//
#include <iGameFileIO.h>
#include <iGameSceneManager.h>
#include <iGameThreadPool.h>

#include <IQComponents/igQtAnimationTreeWidget_interpolate.h>
#include <IQComponents/igQtAnimationTreeWidget_snap.h>
#include <IQCore/igQtAnimationVcrController.h>
#include <IQCore/igQtAttributeDataSourceManager.h>
#include <IQCore/igQtOpenGLWidgetManager.h>
#include <IQWidgets/igQtAnimationWidget.h>
#include <DataCodec/Filter/Adapter/iGameFramePresentationBridge.h>
#include <algorithm>
#include <QAbstractButton>
#include <QDebug>
#include <QFileDialog>
#include <QMetaObject>
#include <QPointer>
#include <QThread>
#include <IQComponents/Dialog/igQtDarkFramelessMessage.h>
#include <Deformation/iGameStressDeformationFilter.h>
#include <iGameScalarsToColors.h>

namespace {
/** 动画播放/导出时锁定当前 ColorMapper 范围，避免每帧按当帧 dataRange 重置。 */
void LockAnimationColorRange(iGame::DrawObject* obj) {
    if (!obj) return;
    auto mapper = obj->GetColorMapper();
    if (!mapper || mapper->GetStable()) return;
    mapper->SetRangeStable(true);
    mapper->Modified();
}

void UnlockAnimationColorRange(iGame::DrawObject* obj) {
    if (!obj) return;
    auto mapper = obj->GetColorMapper();
    if (!mapper) return;
    mapper->SetRangeStable(false);
}
} // namespace

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
                        QIcon(":/Ticon/Icons/VcrPause.png"));
            }
        } else {
            VcrController->onPause();
            ui->btnPlayOrPause->setIcon(QIcon(":/Ticon/Icons/VcrPlay.png"));
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
                                QIcon(":/Ticon/Icons/VcrPause.png"));
                    }
                } else {
                    VcrController->onPause();
                    ui->btnReverseOrPause->setIcon(
                            QIcon(":/Ticon/Icons/VcrReverse.png"));
                }
            });
    connect(ui->btnLoop, &QPushButton::toggled, this, [&](bool checked) {
        VcrController->onLoop(checked);
        if (checked) {
            ui->btnLoop->setIcon(QIcon(":/Ticon/Icons/VcrDisabledLoop.png"));
        } else {
            ui->btnLoop->setIcon(QIcon(":/Ticon/Icons/VcrDisabledLoop.png"));
        }
    });
    connect(ui->spinBoxAnimationStride, QOverload<int>::of(&QSpinBox::valueChanged), this, [&](int val){
        VcrController->setStripe(ui->spinBoxAnimationStride->value());
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

    // 缓存数量ComboBox信号连接
    connect(ui->comboBox_AnimationCacheNum, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &igQtAnimationWidget::onCacheNumChanged);


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

#include <iGameType.h>
#include <iGamePointSet.h>
#include <Abaqus/iGameODBReader.h>
void igQtAnimationWidget::playAnimation_snap(unsigned int keyframe_idx) {
    using namespace iGame;

    if (m_SnapFrameRequestInFlight) {
        m_PendingSnapFrame = keyframe_idx;
        return;
    }

    m_IsAnimationPlaying = true;
    auto currentScene = SceneManager::Instance()->GetCurrentScene();
    if (currentScene == nullptr || currentScene->GetCurrentModel() == nullptr) {
        m_IsAnimationPlaying = false;
        btnPlay_finishLoop();
        return;
    }
    auto currentDrawObject = DynamicCast<DrawObject>(
            currentScene->GetCurrentModel()->GetDataObject());
    auto timeFrames = currentDrawObject != nullptr
        ? currentDrawObject->PeekTimeFrames()
        : StreamingData::Pointer{};
    if (currentDrawObject == nullptr || timeFrames == nullptr ||
        timeFrames->GetArrays().empty() || keyframe_idx >= timeFrames->GetTimeNum()) {
        m_IsAnimationPlaying = false;
        btnPlay_finishLoop();
        return;
    }

    const auto frameType = timeFrames->GetTargetFrameType(keyframe_idx);
    const auto generation = ++m_SnapFrameGeneration;
    m_SnapFrameRequestInFlight = true;
    QPointer<igQtAnimationWidget> self(this);
    auto* worker = QThread::create(
            [self, generation, keyframe_idx, currentDrawObject, timeFrames, frameType]() mutable {
        std::vector<iGame::Object::Pointer> frameData;
        QString error;
        try {
            frameData = timeFrames->GetTargetTimeFrameData(keyframe_idx);
            if (frameData.empty()) { error = QStringLiteral("解码结果为空"); }
        } catch (const std::exception& exception) {
            error = QString::fromUtf8(exception.what());
        } catch (...) {
            error = QStringLiteral("解码过程中发生未知错误");
        }
        if (!self) { return; }
        QMetaObject::invokeMethod(
                self,
                [self, generation, keyframe_idx, currentDrawObject, timeFrames,
                 frameType, frameData = std::move(frameData), error = std::move(error)]() mutable {
            if (!self || generation != self->m_SnapFrameGeneration) { return; }
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            auto model = scene != nullptr ? scene->GetCurrentModel() : nullptr;
            const bool modelUnchanged = model != nullptr &&
                model->GetDataObject().get() == currentDrawObject.get();
            if (error.isEmpty() && modelUnchanged) {
                self->presentAnimationFrame(
                        keyframe_idx, currentDrawObject.get(), timeFrames, frameType, frameData);
            } else {
                if (!error.isEmpty()) {
                    qWarning() << "动画帧解码失败:" << error;
                }
                self->btnPlay_finishLoop();
            }
            self->completeSnapFrameRequest();
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void igQtAnimationWidget::presentAnimationFrame(
        const unsigned int keyframeIndex,
        iGame::DataObject* dataObject,
        iGame::StreamingData::Pointer timeFrames,
        const StreamingType frameType,
        const std::vector<iGame::Object::Pointer>& frameData) {
    using namespace iGame;
    auto currentDrawObject = DynamicCast<DrawObject>(dataObject);
    auto currentScene = SceneManager::Instance()->GetCurrentScene();
    if (currentDrawObject == nullptr || currentScene == nullptr || timeFrames == nullptr) { return; }

    ui->comboBoxCurrentAnimation->blockSignals(true);
    ui->comboBoxCurrentAnimation->setCurrentIndex(static_cast<int>(keyframeIndex));
    ui->comboBoxCurrentAnimation->blockSignals(false);

    auto presentedDrawObject = currentDrawObject;
    iGame::DataCodecFramePresentationResult dataCodecPresentation;
    if (frameType == StreamingType::IGCFramePackage &&
        frameData.size() == 1u) {
        auto nextDrawObject = DynamicCast<DrawObject>(frameData.front());
        auto currentModel = currentScene->GetCurrentModel();
        if (nextDrawObject == nullptr || currentModel == nullptr ||
            currentModel->GetDataObject().get() != currentDrawObject) {
            return;
        }
        if (nextDrawObject.get() == currentDrawObject) {
            timeFrames->NotifyFramePresented(keyframeIndex);
            return;
        }
        const auto selectedAttributeIndex = currentDrawObject->GetAttributeIndex();
        const auto selectedAttributeDimension = currentDrawObject->GetAttributeDimension();
        const auto nextAttributeCount = nextDrawObject->GetAttributeSet() != nullptr
            ? nextDrawObject->GetAttributeSet()->GetNumberOfAttributes()
            : 0u;
        dataCodecPresentation = iGame::PrepareDataCodecFramePresentation(
            currentDrawObject,
            nextDrawObject.get());

        const auto currentObjectId = currentDrawObject->GetDataObjectId();
        auto currentAttributeSource =
            igQtAttributeDataSourceManager::Instance()->Source(currentObjectId);
        currentModel->SetDataObject(nextDrawObject);
        presentedDrawObject = nextDrawObject;
        if (currentAttributeSource != nullptr) {
            auto nextSource = currentAttributeSource->ForFrameObject(nextDrawObject);
            if (nextSource != nullptr) {
                igQtAttributeDataSourceManager::Instance()->RegisterSource(
                    nextDrawObject,
                    std::move(nextSource));
            }
            igQtAttributeDataSourceManager::Instance()->ReleaseSource(currentObjectId);
        }
        if (selectedAttributeIndex >= 0 &&
            static_cast<std::size_t>(selectedAttributeIndex) < nextAttributeCount) {
            presentedDrawObject->ViewCloudPicture(
                    currentScene, selectedAttributeIndex, selectedAttributeDimension, false);
        }
    } else {
        currentDrawObject->ApplyAnimationFrame(frameType, frameData);
    }

    // 固定整段动画的颜色范围（使用进入播放时 mapper 上已有的 min/max）
    LockAnimationColorRange(currentDrawObject);

    currentScene->MakeCurrent();
    presentedDrawObject->SetViewStyle(presentedDrawObject->GetViewStyle());

    if (!dataCodecPresentation.recognized || !dataCodecPresentation.drawableReady) {
        presentedDrawObject->ForceReConvertToDrawableData();
        presentedDrawObject->ConvertToDrawableData();

        auto renderableObj = presentedDrawObject->GetRenderableObject();
        if (renderableObj && renderableObj.get() != presentedDrawObject) {
            renderableObj->ForceReConvertToDrawableData();
            renderableObj->ConvertToDrawableData();
        }

        if (presentedDrawObject->HasSubDataObject()) {
            for (auto it = presentedDrawObject->SubDataObjectIteratorBegin();
                 it != presentedDrawObject->SubDataObjectIteratorEnd(); ++it) {
                auto subDrawObj = iGame::DynamicCast<iGame::DrawObject>(it->second);
                if (subDrawObj) {
                    auto subRenderableObj = subDrawObj->GetRenderableObject();
                    if (subRenderableObj && subRenderableObj.get() != subDrawObj.get()) {
                        subRenderableObj->ForceReConvertToDrawableData();
                        subRenderableObj->ConvertToDrawableData();
                    }
                }
            }
        }
    }
    
    // Apply deformation AFTER both parent and RenderableMesh have been converted
    // but BEFORE the render pass
    if(presentedDrawObject->GetDeformationData()->GetEnableStatus()){
        StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
        deformFilter->SetInput(presentedDrawObject);
        if(!deformFilter->Execute()) std::cout << " deformation error \n";
    }

    currentScene->DoneCurrent();

    // Single render pass - ConvertToDrawableData should NOT run again
    // because m_ReConvertToDrawableData was set to false by our explicit call
    Q_EMIT UpdateScene();

    Q_EMIT AnimationFrameChanged();  // Notify scalar widget to update UI
}

void igQtAnimationWidget::completeSnapFrameRequest() {
    m_SnapFrameRequestInFlight = false;
    if (m_PendingSnapFrame.has_value()) {
        const auto pendingFrame = *m_PendingSnapFrame;
        m_PendingSnapFrame.reset();
        playAnimation_snap(pendingFrame);
        return;
    }
    m_IsAnimationPlaying = ui->btnPlayOrPause->isChecked() ||
        ui->btnReverseOrPause->isChecked();
    VcrController->onFramePresented();
}

void igQtAnimationWidget::playAnimation_interpolate(int keyframe_0, float t) {
    using namespace iGame;
    
    // 设置播放状态，阻止播放期间触发initAnimationComponents
    m_IsAnimationPlaying = true;
    
    auto currentScene = SceneManager::Instance()->GetCurrentScene();
    auto currentDrawObject = DynamicCast<DrawObject>(
            currentScene->GetCurrentModel()->GetDataObject());
    if (currentDrawObject == nullptr
        ||  currentDrawObject->GetTimeFrames()->GetArrays().empty()
        ||  keyframe_0 + 1 == currentDrawObject->GetTimeFrames()->GetArrays().size()) {
        m_IsAnimationPlaying = false;
        btnPlay_finishLoop();
        return;
    }
    auto frameSubFiles_0 = currentDrawObject->GetTimeFrames()
            ->GetTargetTimeFrame(keyframe_0)
            .GetMetaData();
    std::vector<iGame::DataObject::Pointer> results_0(frameSubFiles_0->GetNumberOfElements());
    {
        std::vector<std::future<iGame::DataObject::Pointer>> tasks;
        for (int i = 0; i < frameSubFiles_0->GetNumberOfElements(); i++) {
            tasks.emplace_back(iGame::ThreadPool::Instance()->Commit(
                    [i, &results_0](const std::string& fileName) {
                        auto res = FileIO::ReadFile(fileName);
                        res->SetName(fileName);
                        results_0[i] = res;
                        return res;
                    },
                    frameSubFiles_0->GetElement(i)));
        }
        currentDrawObject->ClearSubDataObject();

        for (auto& task: tasks) {
            task.get();
        }
        for(const auto& obj : results_0){
            currentDrawObject->AddSubDataObject(obj);
        }
        currentDrawObject->UpdateSubDataObjectDataRange();
    }
    auto frameSubFiles_1 = currentDrawObject->GetTimeFrames()
            ->GetTargetTimeFrame(keyframe_0 + 1)
            .GetMetaData();
    {
        std::vector<std::future<iGame::DataObject::Pointer>> tasks;
        std::vector<iGame::DataObject::Pointer> results(frameSubFiles_1->GetNumberOfElements());
        for (int i = 0; i < frameSubFiles_1->GetNumberOfElements(); i++) {
            tasks.emplace_back(iGame::ThreadPool::Instance()->Commit(
                    [i, &results](const std::string& fileName) {
                        auto res = FileIO::ReadFile(fileName);
                        res->SetName(fileName);
                        results[i] = res;
                        return res;
                    },
                    frameSubFiles_1->GetElement(i)));
        }
        for(auto& task : tasks){
            task.get();
        }

//        auto it = currentDrawObject->SubDataObjectIteratorBegin();
//        for (int i = 0; i < tasks.size(); i ++, it ++) {
        for (int i = 0; i < tasks.size(); i ++) {
//            const auto& subObject_0 = DynamicCast<PointSet>(it->second);
            const auto& subObject_0 = DynamicCast<PointSet>(results_0[i]);
            const auto& subObject_1 = DynamicCast<PointSet>(results[i]);
            /* Process interpolate. */
            const auto& ps_0 = subObject_0->GetPoints().get();
            const auto& ps_1 = subObject_1->GetPoints().get();
            /* Process Points' interpolation. */
            for(int j = 0; j < subObject_0->GetNumberOfPoints(); j ++){
                /* p_inter = p_0 + t(p_1 - p_0)*/
                auto p_0 = ps_0->GetPoint(j);
                auto p_1 = ps_1->GetPoint(j);
                ps_0->SetPoint(j, p_0 + t * (p_1 - p_0));
//                ps_0->GetPoint(j) = ps_0->GetPoint(j) + t * (ps_1->GetPoint(j) - ps_0->GetPoint(j));
            }
            /* Process Vector's Interpolation to adapt Interpolation's Deformation Filter. */
            const auto& attributes_0 = subObject_0->GetAttributeSet()->GetAllAttributes();
            const auto& attributes_1 = subObject_1->GetAttributeSet()->GetAllAttributes();
            for(int j = 0; j < attributes_0->Size(); j ++){
                const auto& attribute_0 = attributes_0[j].RawPointer()->pointer;
                const auto& attribute_1 = attributes_1[j].RawPointer()->pointer;
                /* If attribute's dimension is minus one, means it needn't to process in Deformation. */
//                if(attribute_0->GetDimension() < 2) continue;
                int dimension = attribute_0->GetDimension();
                for(auto k = 0; k < attribute_0->GetNumberOfElements(); k ++){
                    for(int elem_idx = 0; elem_idx < dimension; elem_idx ++){
                        double val_0 = attribute_0->GetValue(k * dimension + elem_idx);
                        double val_1 = attribute_1->GetValue(k * dimension + elem_idx);
                        attribute_0->SetValue(k * dimension + elem_idx, val_0 + t * (val_1 - val_0));
                    }
                }
            }
            subObject_0->GetPoints()->Modified();
            subObject_0->GetAttributeSet()->Modified();
            subObject_0->ConvertToDrawableData();
        }
    }

    /* If obj has the deformation var and is enabled.
     * Make sure every timeStep have the deformation scale factor. */

//    StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
//    deformFilter->SetInput(currentDrawObject);
//    if(!deformFilter->Execute()) std::cout << " error \n";


    currentScene->MakeCurrent();
    currentDrawObject->SetViewStyle(currentDrawObject->GetViewStyle());

    LockAnimationColorRange(currentDrawObject.get());

    if (currentDrawObject->GetAttributeIndex() != -1) {
        currentDrawObject->ViewCloudPicture(
                currentScene, currentDrawObject->GetAttributeIndex());
    }
    currentScene->DoneCurrent();

    // Update comboBoxCurrentAnimation for interpolation (block signals to avoid recursion)
    ui->comboBoxCurrentAnimation->blockSignals(true);
    ui->comboBoxCurrentAnimation->setCurrentIndex(keyframe_0);
    ui->comboBoxCurrentAnimation->blockSignals(false);

    Q_EMIT UpdateScene();
    Q_EMIT AnimationFrameChanged();  // Notify scalar widget to update UI

    Q_EMIT PlayAnimation_interpolate(keyframe_0, t);
    
    // 恢复播放状态标记
    m_IsAnimationPlaying = ui->btnPlayOrPause->isChecked() ||
        ui->btnReverseOrPause->isChecked();
    VcrController->onFramePresented();
}

void igQtAnimationWidget::btnPlay_finishLoop() {
    ui->btnPlayOrPause->setChecked(false);
    ui->btnReverseOrPause->setChecked(false);
    // 播放结束后解除锁定，便于之后按单帧自适应；手动改过的范围可通过「重置/自定义范围」再锁
    auto currentScene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (currentScene && currentScene->GetCurrentModel()) {
        auto draw = iGame::DynamicCast<iGame::DrawObject>(
                currentScene->GetCurrentModel()->GetDataObject());
        UnlockAnimationColorRange(draw.get());
    }
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

void igQtAnimationWidget::onCacheNumChanged(int cacheNum) {
    using namespace iGame;
    auto currentScene = SceneManager::Instance()->GetCurrentScene();
    if (!currentScene || !currentScene->GetCurrentModel()) return;
    
    auto currentDrawObject = DynamicCast<DrawObject>(
            currentScene->GetCurrentModel()->GetDataObject());
    if (!currentDrawObject || !currentDrawObject->GetTimeFrames()) return;
    
    auto timeFrames = currentDrawObject->GetTimeFrames();
    
    if (cacheNum == 0) {
        // 缓存数量为0时禁用缓存
        timeFrames->DisableCache();
    } else {
        // 启用缓存并设置最大缓存帧数
        timeFrames->EnableCache(cacheNum);
        m_LastPresentedFrame = -1;
        Q_EMIT UpdateScene();
    }
}

void igQtAnimationWidget::notifyCurrentFramePresented() {
    using namespace iGame;
    auto scene = SceneManager::Instance()->GetCurrentScene();
    auto model = scene != nullptr ? scene->GetCurrentModel() : nullptr;
    auto object = model != nullptr ? model->GetDataObject() : DataObject::Pointer{};
    auto timeFrames = object != nullptr ? object->PeekTimeFrames() : StreamingData::Pointer{};
    const auto frameIndex = ui->comboBoxCurrentAnimation->currentIndex();
    if (object == nullptr || timeFrames == nullptr || frameIndex < 0 ||
        static_cast<std::size_t>(frameIndex) >= timeFrames->GetTimeNum()) {
        return;
    }
    const auto objectId = object->GetDataObjectId();
    if (m_LastPresentedObjectId == objectId && m_LastPresentedFrame == frameIndex) {
        return;
    }
    timeFrames->NotifyFramePresented(static_cast<unsigned int>(frameIndex));
    m_LastPresentedObjectId = objectId;
    m_LastPresentedFrame = frameIndex;
}

void igQtAnimationWidget::initAnimationComponents() {
    // 如果正在播放动画，跳过初始化以避免中断播放
    if (m_IsAnimationPlaying) {
        return;
    }
    
    if (iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel() ==
                nullptr ||
        iGame::SceneManager::Instance()
                        ->GetCurrentScene()
                        ->GetCurrentModel()
                        ->GetDataObject()
                        ->GetTimeFrames() == nullptr)
        return;
//    IGAME_CORE_ERROR("Init Animation");
    auto& timeArrays = iGame::SceneManager::Instance()
                              ->GetCurrentScene()
                              ->GetCurrentModel()
                              ->GetDataObject()
                              ->GetTimeFrames()
                              ->GetArrays();
    if (timeArrays.empty()) {
        ClearAnimationVCRInfo();
        return ;
    }

    std::vector<float> timeValues;
    timeValues.reserve(timeArrays.size());
    for (auto& timeArray: timeArrays) timeValues.push_back(timeArray.GetTimeValue());
    VcrController->initController(static_cast<int>(timeValues.size()), 1);
    ui->treeWidget_snap->initAnimationTreeWidget(timeValues);
    ui->treeWidget_interpolate->initAnimationTreeWidget(timeValues);
    ui->SliderAnimationTrack->setMaximum(static_cast<int>(timeValues.size()) -
                                         1);
    ui->SliderAnimationTrack->setMinimum(0);
    ui->SliderAnimationTrack->setValue(0);
    
    // 初始化缓存ComboBox: 选项 [0, 1, 2, ..., 时间帧数量]
    int frameCount = static_cast<int>(timeValues.size());
    const int defaultCacheNum = std::min(3, std::max(0, frameCount - 1));
    ui->comboBox_AnimationCacheNum->blockSignals(true);
    ui->comboBox_AnimationCacheNum->clear();
    for (int i = 0; i < frameCount; i++) {
        ui->comboBox_AnimationCacheNum->addItem(QString::number(i));
    }
    ui->comboBox_AnimationCacheNum->setCurrentIndex(defaultCacheNum);
    ui->comboBox_AnimationCacheNum->blockSignals(false);
    
    // 应用初始缓存设置
    auto currentDrawObject = iGame::DynamicCast<iGame::DrawObject>(
            iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject());
    if (currentDrawObject && currentDrawObject->GetTimeFrames()) {
        if (defaultCacheNum > 0) {
            currentDrawObject->GetTimeFrames()->EnableCache(defaultCacheNum);
        } else {
            currentDrawObject->GetTimeFrames()->DisableCache();
        }
    }

    // Populate comboBoxCurrentAnimation with frame numbers (1-based display)
    ui->comboBoxCurrentAnimation->blockSignals(true);

    if(ui->comboBoxCurrentAnimation->count() != 0){
        ui->comboBoxCurrentAnimation->clear();
    }
    for (int i = 0; i < timeValues.size(); i++) {
        ui->comboBoxCurrentAnimation->addItem(QString::number(i + 1));  // Display 1, 2, 3...
    }
    ui->comboBoxCurrentAnimation->setCurrentIndex(0);  // Start at frame 1
    ui->comboBoxCurrentAnimation->blockSignals(false);
    ui->lineEditKeyframeNum->setText(
            QString("%1").arg(static_cast<int>(timeValues.size())));
    ui->lineEditStartTime->setText(
            QString::asprintf("%.f", *timeValues.begin()));
    ui->lineEditEndTime->setText(
            QString::asprintf("%.20f", *(timeValues.end() - 1)));
    connect(ui->SliderAnimationTrack, &QSlider::sliderMoved, VcrController,
            &igQtAnimationVcrController::updateCurrentKeyframe);
    connect(ui->comboBoxCurrentAnimation, QOverload<int>::of(&QComboBox::currentIndexChanged),
            VcrController, &igQtAnimationVcrController::updateCurrentKeyframe, Qt::UniqueConnection);
}

void igQtAnimationWidget::ClearAnimationVCRInfo() {
    ++m_SnapFrameGeneration;
    m_SnapFrameRequestInFlight = false;
    m_PendingSnapFrame.reset();
    m_IsAnimationPlaying = false;
    m_LastPresentedObjectId = -1;
    m_LastPresentedFrame = -1;
    VcrController->onPause();
    VcrController->initController(1, 1);
    std::vector<float> tmpTimeSteps(1, 0.f);
    ui->treeWidget_snap->initAnimationTreeWidget(tmpTimeSteps);
    ui->treeWidget_interpolate->initAnimationTreeWidget(tmpTimeSteps);
    ui->SliderAnimationTrack->setMaximum(static_cast<int>(tmpTimeSteps.size()) -
                                         1);
    ui->SliderAnimationTrack->setMinimum(0);
    ui->SliderAnimationTrack->setValue(0);


    // Populate comboBoxCurrentAnimation with frame numbers (1-based display)
    ui->comboBoxCurrentAnimation->blockSignals(true);

    if(ui->comboBoxCurrentAnimation->count() != 0){
        ui->comboBoxCurrentAnimation->clear();
    }
    for (int i = 0; i < tmpTimeSteps.size(); i++) {
        ui->comboBoxCurrentAnimation->addItem(QString::number(i + 1));  // Display 1, 2, 3...
    }
    ui->comboBoxCurrentAnimation->setCurrentIndex(0);  // Start at frame 1
    ui->comboBoxCurrentAnimation->blockSignals(false);
    ui->lineEditKeyframeNum->setText(
            QString("%1").arg(static_cast<int>(tmpTimeSteps.size())));
    ui->lineEditStartTime->setText(
            QString::asprintf("%.f", *tmpTimeSteps.begin()));
    ui->lineEditEndTime->setText(
            QString::asprintf("%.20f", *(tmpTimeSteps.end() - 1)));
}

//#include <fstream>
//#include <windows.h>

#include <FFMPEG/iGameFFMPEGVideoWriter.h>
#include <IQComponents/Dialog/igQtVideoOptionDialog.h>
bool igQtAnimationWidget::saveAnimation() {
#if defined(FFMPEG_ENABLE)
    using namespace iGame;
    auto currentScene = SceneManager::Instance()->GetCurrentScene();
    if (currentScene->GetCurrentModel() == nullptr ||
        currentScene->GetCurrentModel()->GetDataObject()->GetTimeFrames()->GetArrays().empty()) {
        igQtShowDarkFramelessMessage(this, QStringLiteral("保存动画"),
                                     QStringLiteral("请导入带时间帧的文件"), true);
        return false;
    }
    auto currentObject = currentScene->GetCurrentModel()->GetDataObject();
    auto currentDrawObject = DynamicCast<DrawObject>(currentObject);
    auto timeFrames = currentObject->PeekTimeFrames();
    if (currentDrawObject == nullptr || timeFrames == nullptr) { return false; }
    size_t timeStepSize = timeFrames->GetTimeNum();

    igQtRenderWidget* rendererWidget =
            igQtOpenGLManager::Instance()->getRenderWidget();
    QStringList filters = {
            "Mp4 File(*.mp4)",
            "GIF File(*.gif)",
            "PNG Files(*.png)",
    };

    QString SelectedFilter;
    QString path =
            QFileDialog::getSaveFileName(nullptr, "Save Animation As ", "",
                                         filters.join(";;"), &SelectedFilter);

    igQtVideoOptionDialog dialog(this);
    int oldwidth = rendererWidget->width(),
        oldheight = rendererWidget->height();
    int ratio_pixel = rendererWidget->devicePixelRatio();
    int width = 1920, height = 1080;
    VideoInputInfo inputInfo;
    if (dialog.exec() == QDialog::Accepted) {
        inputInfo = dialog.getInput();
        width = inputInfo.width, height = inputInfo.height;
    } else
        return false;

    rendererWidget->resize(width / ratio_pixel, height / ratio_pixel);
    int selected_idx = filters.indexOf(SelectedFilter);

    switch (selected_idx) {
        case 0:
            if (!path.contains(".mp4")) path += ".mp4";
            break;
        case 1:
            if (!path.contains(".gif")) path += ".gif";
            break;
        case 2:
            if (!path.contains(".png")) path += ".png";
            break;
        default:
            break;
    }
    int ratio = currentScene->GetCamera()->GetDevicePixelRatio();
    auto wh = currentScene->GetCamera()->GetViewPort();


//    /* RGBA Type , means that one pixel's size is 4 byte.*/
//    inputInfo.bytes_per_line = width * 4;
//
//    for(size_t i = 0; i < timeStepSize; i ++){
//        this->playAnimation_snap(i);
//        std::vector<uint8_t> currentFrameBuffer = currentScene->CaptureScreen(0, 0, width, height, RGBA, true);
//        inputInfo.raw_image_data.emplace_back(currentFrameBuffer);
//    }



    QFileInfo info(path);
    auto drawObj = DynamicCast<DrawObject>(currentObject);
    // 先建立第 0 帧范围，再锁定整段导出期间的色标
    this->playAnimation_snap(0);
    LockAnimationColorRange(drawObj.get());

    for(int i = 0; i < timeStepSize; i ++)
    {
        const auto frameType = timeFrames->GetTargetFrameType(static_cast<unsigned int>(i));
        const auto frameData = timeFrames->GetTargetTimeFrameData(static_cast<unsigned int>(i));
        if (frameData.empty()) {
            rendererWidget->resize(oldwidth, oldheight);
            return false;
        }
        presentAnimationFrame(
                static_cast<unsigned int>(i),
                DynamicCast<DrawObject>(currentScene->GetCurrentModel()->GetDataObject()).get(),
                timeFrames,
                frameType,
                frameData);
        QImage image = rendererWidget->grabFramebuffer();
        if(filters.indexOf(SelectedFilter) == 2){
            qDebug() << QString(info.path() + "/" + info.baseName() + QString::asprintf("_%d.png", i));
            image.save(info.path() + "/" + info.baseName() + QString::asprintf("_%d.png", i));
        }

//        std::vector<uint8_t> tmp(image.bits(),
//                                 image.bits() + image.sizeInBytes());
//        inputInfo.bytes_per_line = image.bytesPerLine();
        auto tmp = currentScene->CaptureScreen(0, 0, width, height, GLFramebuffer::Type::RGBA, true);
        inputInfo.bytes_per_line = width * 4;
        inputInfo.raw_image_data.emplace_back(tmp);
    }
    UnlockAnimationColorRange(drawObj.get());
    rendererWidget->resize(oldwidth, oldheight);



    FFMPEGVideoWriter::Pointer videoWriter = FFMPEGVideoWriter::New();
    inputInfo.output_path = path.toStdString();
    videoWriter->SetVideoInputInfo(inputInfo);

    bool sc = false;
    switch (selected_idx) {
        case 0:
            sc = videoWriter->SaveMP4();
            break;
        case 1:
            sc = videoWriter->SaveGIF();
            break;
        case 2:
            sc = true;
            break;
        default:
            break;
    }
    if (sc) {
        igQtShowDarkFramelessMessage(this, QStringLiteral("保存动画"), QStringLiteral("保存成功"), true);
    } else {
        igQtShowDarkFramelessMessage(this, QStringLiteral("保存动画"), QStringLiteral("保存失败"));
    }
#endif


//    //    currentScene
//        int width = 1920, height = 1080;
//        currentScene->MakeCurrent();
////        currentScene->Resize()
//        auto* bits = currentScene->CaptureOffScreenBuffer(width, height);
//        currentScene->DoneCurrent();
//    //    for(auto i = 0; i )
//        FILE* pfile = fopen(file_path.toStdString().c_str(), "wb");
//        if(pfile){
//            BITMAPFILEHEADER  bfh;
//            memset(&bfh, 0, sizeof (BITMAPFILEHEADER));
//            bfh.bfType = 0x4D42;
//            bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER) + width * height * 3;
//            bfh.bfOffBits = sizeof (BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
//            fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, pfile);
//
//            BITMAPINFOHEADER  bih;
//            memset(&bih, 0, sizeof (BITMAPINFOHEADER));
//            bih.biWidth = width;
//            bih.biHeight = height;
//            bih.biBitCount = 24;
//            bih.biSize = sizeof(BITMAPINFOHEADER);
//            fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, pfile);
//
//            fwrite(bits, 1, width * height * 3, pfile);
//            fclose(pfile);
//        }
//        delete[] bits;

    return true;
}

