//
// Created by m_ky on 2024/4/22.
//
#include <iGameFileIO.h>
#include <iGameSceneManager.h>

#include <IQComponents/igQtAnimationTreeWidget_interpolate.h>
#include <IQComponents/igQtAnimationTreeWidget_snap.h>
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQCore/igQtAnimationFilterAdapters.h>
#include <IQCore/igQtAnimationFrameSource.h>
#include <IQCore/igQtAnimationVcrController.h>
#include <IQCore/igQtOpenGLWidgetManager.h>
#include <IQWidgets/igQtAnimationWidget.h>
#include <IQWidgets/igQtRenderWidget.h>
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QLineEdit>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <IQComponents/Dialog/igQtDarkFramelessMessage.h>
#include <Deformation/iGameStressDeformationFilter.h>
#include <FeatureExtraction/iGameVortexFilter.h>
#include <Animation/iGameAttrDiff.h>
#include <iGameProgressObserver.h>
#include <iGameAttributeSet.h>
#include <iGameDrawObject.h>
#include <algorithm>
#include <iostream>

/**
 * @class   igQtAnimationWidget
 * @brief   igQtAnimationWidget's brief
 */
igQtAnimationWidget::igQtAnimationWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::Animation) {
    ui->setupUi(this);

    igQtPanelTheme::attachDeep(this);
    VcrController = new igQtAnimationVcrController(this);
    ui->SliderAnimationTrack->installEventFilter(this);

    QString registrationError;
    if (!igQtRegisterBuiltinAnimationFilters(
                m_AnimationFilterManager, &registrationError)) {
        std::cout << "[Animation][Filter] "
                  << registrationError.toStdString() << std::endl;
    }
    ui->comboBoxAnimationFilter->clear();
    ui->comboBoxAnimationFilter->addItem(QStringLiteral("添加 Filter..."),
                                         QString());
    for (const auto& id : m_AnimationFilterManager.filterIds()) {
        const auto* descriptor = m_AnimationFilterManager.descriptor(id);
        if (descriptor) {
            ui->comboBoxAnimationFilter->addItem(descriptor->displayName, id);
        }
    }

    connect(ui->comboBoxAnimationFilter,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &igQtAnimationWidget::onAnimationFilterChanged);
    connect(ui->btnAnimationFilterParameters, &QPushButton::clicked,
            this, &igQtAnimationWidget::openAnimationFilterParameters);
    connect(ui->btnAnimationFilterAdd, &QPushButton::clicked, this,
            &igQtAnimationWidget::addSelectedFilterToPipeline);
    connect(ui->btnAnimationFilterRemove, &QPushButton::clicked, this,
            &igQtAnimationWidget::removeSelectedPipelineStep);
    connect(ui->btnAnimationFilterUp, &QPushButton::clicked, this,
            [this]() { moveSelectedPipelineStep(true); });
    connect(ui->btnAnimationFilterDown, &QPushButton::clicked, this,
            [this]() { moveSelectedPipelineStep(false); });
    connect(ui->btnAnimationFilterClear, &QPushButton::clicked, this,
            &igQtAnimationWidget::clearAnimationPipeline);
    connect(ui->listWidgetAnimationPipeline, &QListWidget::currentRowChanged,
            this, &igQtAnimationWidget::onPipelineSelectionChanged);
    onAnimationFilterChanged(ui->comboBoxAnimationFilter->currentIndex());

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
    connect(ui->SliderAnimationTrack, &QSlider::actionTriggered, this,
            [this](int) {
                VcrController->updateCurrentKeyframe(
                        ui->SliderAnimationTrack->sliderPosition());
            });
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
    auto* validator = new QIntValidator(
            2, 999, this); // 限制关键帧输入范围为2到999，根据需要修改
    auto* LineValidator =
            new QRegExpValidator(QRegExp("^[0-9]*\\.?[0-9]*$"), this);
    ui->lineEditStartTime->setValidator(LineValidator);
    ui->lineEditEndTime->setValidator(LineValidator);
    ui->lineEditKeyframeNum->setValidator(validator);

    auto applyAnimationOperation = [this]() {
        // 原始时间模式不生成新的时间序列，应用操作没有意义。
        if (!ui->rbtnInterpolateTimeMode->isChecked()) return;

        const float start = ui->lineEditStartTime->text().toFloat();
        const float end = ui->lineEditEndTime->text().toFloat();
        const int frameCount = ui->lineEditKeyframeNum->text().toInt();

        VcrController->setStripe(ui->spinBoxAnimationStride->value());
        const bool applied = ui->treeWidget_interpolate->updateInterpolateData(
                start, end, frameCount);

        // 只有有效输入才保存，便于切回插值模式时恢复上一次设置。
        if (applied) {
            invalidateAnimationOutputs();
            m_InterpolateStartTime = start;
            m_InterpolateEndTime = end;
            m_InterpolateFrameCount = frameCount;
        }
    };

    connect(ui->btnApplyAnimationOperation, &QPushButton::clicked, this,
            [applyAnimationOperation](bool) { applyAnimationOperation(); });
    connect(ui->lineEditKeyframeNum, &QLineEdit::returnPressed, this,
            applyAnimationOperation);
    connect(ui->lineEditStartTime, &QLineEdit::returnPressed, this,
            applyAnimationOperation);
    connect(ui->lineEditEndTime, &QLineEdit::returnPressed, this,
            applyAnimationOperation);

    // 默认是原始时间模式；此时插值参数只作为信息显示，不允许编辑。
    updateAnimationModeControls();
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

igQtAnimationWidget::~igQtAnimationWidget() {
    restoreAnimationFilterSource();
    delete ui;
}

bool igQtAnimationWidget::eventFilter(QObject* watched, QEvent* event) {
    auto* slider = ui->SliderAnimationTrack;
    if (watched == slider && event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QStyleOptionSlider option;
            option.initFrom(slider);
            option.orientation = slider->orientation();
            option.minimum = slider->minimum();
            option.maximum = slider->maximum();
            option.sliderPosition = slider->sliderPosition();
            option.sliderValue = slider->value();
            option.singleStep = slider->singleStep();
            option.pageStep = slider->pageStep();
            option.tickPosition = slider->tickPosition();
            option.tickInterval = slider->tickInterval();
            option.upsideDown = slider->invertedAppearance();
            if (slider->layoutDirection() == Qt::RightToLeft) {
                option.upsideDown = !option.upsideDown;
            }

            const QRect handleRect = slider->style()->subControlRect(
                    QStyle::CC_Slider, &option, QStyle::SC_SliderHandle,
                    slider);

            // Move the handle under the cursor before QSlider processes the
            // press. This provides click-to-seek without consuming the event:
            // QSlider then sees a handle press and keeps its native dragging
            // behavior instead of applying the default pageStep (10).
            const int handleLength = handleRect.width();
            const int span = qMax(0, slider->width() - handleLength);
            const int position = mouseEvent->pos().x() - handleLength / 2;
            const int value = QStyle::sliderValueFromPosition(
                    slider->minimum(), slider->maximum(), position, span,
                    option.upsideDown);

            slider->setValue(value);
            VcrController->updateCurrentKeyframe(value);
        }
    }

    return QWidget::eventFilter(watched, event);
}

#include <iGameType.h>
#include <iGamePointSet.h>
#include <Abaqus/iGameODBReader.h>
void igQtAnimationWidget::playAnimation_snap(unsigned int frame) {
    displayAnimationFrame({static_cast<int>(frame), false, 0, static_cast<int>(frame)});
}

void igQtAnimationWidget::playAnimation_interpolate(int frame, float weight) {
    if (displayAnimationFrame({frame, true, weight, VcrController->currentKeyframeIndex()})) Q_EMIT PlayAnimation_interpolate(frame, weight);
}

int igQtAnimationWidget::animationOutputFrameCount() const { return VcrController->frameCount(); }

bool igQtAnimationWidget::renderAnimationOutputFrame(int index, bool exporting) {
    igQtAnimationFrameRequest request;
    if (!VcrController->frameRequest(index, request)) {
        m_AnimationFrameError = QStringLiteral("输出帧编号无效。"); return false;
    }
    return displayAnimationFrame(request, exporting);
}

bool igQtAnimationWidget::bindAnimationSource() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto model = scene ? scene->GetCurrentModel() : nullptr;
    auto object = model ? model->GetDataObject() : nullptr;
    if (model == m_AnimationFilterSourceModel && object &&
        (object == m_AnimationFilterSourceObject || object == m_AnimationDisplayedObject)) return true;
    restoreAnimationFilterSource();
    if (!object || !object->PeekTimeFrames() || object->PeekTimeFrames()->GetArrays().empty()) return false;
    m_AnimationFilterSourceModel = model;
    m_AnimationFilterSourceObject = object;
    object->PeekTimeFrames()->DisableCache();
    return true;
}

void igQtAnimationWidget::invalidateAnimationOutputs() {
    auto scene = m_AnimationFilterSourceModel ? m_AnimationFilterSourceModel->GetScene() : nullptr;
    if (scene) scene->MakeCurrent();
    m_AnimationOutputCache.clear();
    if (scene) scene->DoneCurrent();
}

bool igQtAnimationWidget::displayAnimationFrame(const igQtAnimationFrameRequest& request, bool exporting) {
    using namespace iGame;
    QScopedValueRollback<bool> playing(m_IsAnimationPlaying, true);
    m_AnimationFrameError.clear();
    if (!bindAnimationSource()) {
        m_AnimationFrameError = QStringLiteral("没有动画源数据。"); return false;
    }
    auto scene = SceneManager::Instance()->GetCurrentScene();
    auto model = scene->GetCurrentModel();
    auto previous = DynamicCast<DrawObject>(model->GetDataObject());
    auto source = m_AnimationFilterSourceObject;
    scene->MakeCurrent();
    source->PeekTimeFrames()->DisableCache();
    const auto result = m_AnimationOutputCache.resolve(request, [&] {
        igQtAnimationFilterResult output;
        DataObject::DeferDrawableConversionScope cpu;
        igQtAnimationFrameContext context;
        if (!igQtLoadAnimationFrame(source, request, context, output.error)) return output;
        context.exporting = exporting;
        context.outputFrameIndex = request.outputFrame;
        if (m_VortexAutoCompute && !m_VortexSourceAttr.empty() &&
            ensureVortexForCurrentFrame(context.input, m_VortexSourceAttr, request.sourceFrame) < 0) {
            output.error = QStringLiteral("当前帧涡量计算失败。"); return output;
        }
        if (m_DiffAutoCompute && !m_DiffSourceAttr.empty() &&
            EnsureTimeDifferenceForCurrentFrame(context.input, m_DiffSourceAttr, request.sourceFrame) < 0) {
            output.error = QStringLiteral("当前帧差值计算失败。"); return output;
        }
        QString error;
        if (!igQtExecuteAnimationPipeline(m_AnimationFilterManager, m_AnimationPipeline, context, output, &error)) {
            output.success = false; output.error = error;
        }
        if (output.success && !DynamicCast<DrawObject>(output.output)) {
            output.success = false; output.error = QStringLiteral("Pipeline 输出不可显示。");
        }
        return output;
    });
    if (!result.success || !result.output) {
        m_AnimationFrameError = result.error;
        scene->DoneCurrent(); VcrController->onPause();
        ui->labelAnimationFilterSummary->setText(m_AnimationFrameError);
        std::cout << "[Animation][Pipeline] " << result.error.toStdString() << std::endl;
        return false;
    }
    auto display = DynamicCast<DrawObject>(result.output);
    // Time metadata is shared; source/intermediate numerical arrays are not.
    display->SetTimeFrames(source->PeekTimeFrames());
    display->SetColorMapper(previous ? previous->GetColorMapper() : source->GetColorMapper());
    source->SetColorMapper(display->GetColorMapper());
    if (previous) {
        display->SetDeformationData(previous->GetDeformationData());
        if (m_AnimationPipeline.isEmpty() || previous == m_AnimationDisplayedObject)
            display->SetViewStyle(previous->GetViewStyle());
        display->SetVisibility(previous->GetVisibility());
        display->SetTransparency(previous->GetTransparency());
        display->SetDefaultColor(previous->GetDefaultColor());
        display->SetLineColor(previous->GetLineColor());
        display->SetPointSize(previous->GetPointSize());
        display->SetLineWidth(previous->GetLineWidth());
    }
    m_AnimationPipelineDisplayAttribute = result.displayAttribute;
    m_AnimationPipelineDisplayDimension = result.displayDimension;
    // Filter preferences initialize a new pipeline. Once displayed, the user's
    // current field/component (including solid color) wins on subsequent frames.
    if (previous && previous == m_AnimationDisplayedObject &&
        m_DisplayedPipelineRevision == m_AnimationPipelineRevision) {
        m_AnimationPipelineDisplayAttribute.clear();
        m_AnimationPipelineDisplayDimension = -1;
    }
    if (m_AnimationPipelineDisplayAttribute.isEmpty() && previous && previous->GetAttributeIndex() >= 0) {
        auto attrs = previous->GetAttributeSet();
        if (attrs && previous->GetAttributeIndex() < attrs->GetNumberOfAttributes()) {
            auto field = attrs->GetAttribute(previous->GetAttributeIndex()).pointer;
            if (field) m_AnimationPipelineDisplayAttribute = QString::fromStdString(field->GetName());
            m_AnimationPipelineDisplayDimension = previous->GetAttributeDimension();
        }
    }
    if (previous && previous != display && previous->GetAttributeSet()) {
        // Copy view range settings locally. FixAttributeRange scans every source
        // frame, which would defeat a final-output cache hit.
        std::function<void(DataObject::Pointer)> syncRanges = [&](DataObject::Pointer object) {
            if (object->HasSubDataObject())
                for (auto it = object->SubDataObjectIteratorBegin(); it != object->SubDataObjectIteratorEnd(); ++it)
                    syncRanges(it->second);
            auto attrs = object->GetAttributeSet();
            for (IGsize i = 0; attrs && i < attrs->GetNumberOfAttributes(); ++i) {
                auto& target = attrs->GetAttribute(i);
                if (!target.pointer) continue;
                const int index = previous->GetAttributeSet()->GetAttributeIndex(target.pointer->GetName());
                if (index < 0) continue;
                const auto& setting = previous->GetAttributeSet()->GetAttribute(index);
                const bool wasLocked = target.rangeLocked;
                target.rangeLocked = setting.rangeLocked;
                target.rangeMode = setting.rangeMode;
                target.rangeLockedDimension = setting.rangeLockedDimension;
                target.runningMin = setting.runningMin; target.runningMax = setting.runningMax;
                target.runningRangeValid = setting.runningRangeValid;
                if (setting.rangeLocked && setting.dataRange) {
                    auto range = DoubleArray::New(); range->DeepCopy(setting.dataRange); target.dataRange = range;
                } else if (wasLocked) {
                    target.UpdateAllDataRange();
                }
            }
        };
        syncRanges(display);
    }
    // Initialize this animation's coloring with the existing expand-only mode.
    // Subsequent frames inherit the selected mode; do not override an explicit
    // per-frame/global choice on every tick or scan source frames for a range.
    if ((!m_AnimationDisplayedObject || m_DisplayedPipelineRevision != m_AnimationPipelineRevision) &&
        !m_AnimationPipelineDisplayAttribute.isEmpty()) {
        auto attrs = display->GetAttributeSet();
        const int index = attrs->GetAttributeIndex(m_AnimationPipelineDisplayAttribute.toStdString());
        if (index >= 0) {
            auto& attr = attrs->GetAttribute(index);
            if (attr.rangeMode == AttributeSet::RangeMode::PerFrame && !attr.rangeLocked) {
                attr.rangeMode = AttributeSet::RangeMode::ExpandOnly;
                attr.rangeLocked = true;
                attr.rangeLockedDimension = m_AnimationPipelineDisplayDimension;
                attr.runningRangeValid = false;
            }
        }
    }
    display->RefreshAnimationOutputRanges();
    if (m_AnimationPipelineDisplayAttribute.isEmpty()) display->ViewCloudPicture(scene, -1);
    applyPipelineDisplaySettings(scene, display, source);
    // Rendering settings remain live on hits; data filters do not run again.
    if (display->GetDeformationData()->GetEnableStatus()) display->ForceReConvertToDrawableData();
    display->SetViewStyle(display->GetViewStyle());
    std::function<void(DrawObject::Pointer)> prepare = [&](DrawObject::Pointer object) {
        object->ConvertToDrawableData();
        auto renderable = object->GetRenderableObject();
        if (renderable && renderable != object) renderable->ConvertToDrawableData();
        if (object->HasSubDataObject())
            for (auto it = object->SubDataObjectIteratorBegin(); it != object->SubDataObjectIteratorEnd(); ++it)
                if (auto child = DynamicCast<DrawObject>(it->second)) prepare(child);
    };
    prepare(display);
    if (display->GetDeformationData()->GetEnableStatus()) {
        auto deform = StressDeformationFilter::New(); deform->SetInput(display); deform->Execute();
    }
    model->SetDataObject(display);
    m_AnimationDisplayedObject = display;
    m_DisplayedPipelineRevision = m_AnimationPipelineRevision;
    m_DisplayedSourceFrame = request.sourceFrame;
    previous = nullptr; // Release an uncached old frame while its GL context is current.
    scene->DoneCurrent();
    ui->comboBoxCurrentAnimation->blockSignals(true);
    ui->comboBoxCurrentAnimation->setCurrentIndex(request.outputFrame);
    ui->comboBoxCurrentAnimation->blockSignals(false);
    Q_EMIT AnimationDataChanged();
    Q_EMIT UpdateScene();
    Q_EMIT AnimationFrameChanged();
    return true;
}
void igQtAnimationWidget::onAnimationFilterChanged(int index) {
    const QString filterId = index >= 0
                                     ? ui->comboBoxAnimationFilter
                                               ->itemData(index).toString()
                                     : QString();
    ui->btnAnimationFilterAdd->setEnabled(
            m_AnimationFilterManager.contains(filterId));
    updateAnimationFilterSummary();
}

void igQtAnimationWidget::onPipelineSelectionChanged() {
    const int row = ui->listWidgetAnimationPipeline->currentRow();
    const int count = ui->listWidgetAnimationPipeline->count();
    ui->btnAnimationFilterParameters->setEnabled(row >= 0);
    ui->btnAnimationFilterRemove->setEnabled(row >= 0);
    ui->btnAnimationFilterUp->setEnabled(row > 0);
    ui->btnAnimationFilterDown->setEnabled(row >= 0 && row < count - 1);
}

void igQtAnimationWidget::addSelectedFilterToPipeline() {
    const QString filterId = selectedAnimationFilterId();
    if (!m_AnimationFilterManager.contains(filterId)) return;

    igQtAnimationPipelineStep step;
    step.filterId = filterId;
    m_AnimationPipeline.push_back(step);
    ++m_AnimationPipelineRevision;
    invalidateAnimationOutputs();
    updateAnimationFilterSummary();
    ui->listWidgetAnimationPipeline->setCurrentRow(
            static_cast<int>(m_AnimationPipeline.size()) - 1);
}

void igQtAnimationWidget::removeSelectedPipelineStep() {
    const int row = ui->listWidgetAnimationPipeline->currentRow();
    if (row < 0 || row >= static_cast<int>(m_AnimationPipeline.size())) return;

    m_AnimationPipeline.removeAt(row);
    ++m_AnimationPipelineRevision;
    invalidateAnimationOutputs();
    updateAnimationFilterSummary();
}

void igQtAnimationWidget::moveSelectedPipelineStep(bool up) {
    const int row = ui->listWidgetAnimationPipeline->currentRow();
    const int target = row + (up ? -1 : 1);
    if (row < 0 || row >= static_cast<int>(m_AnimationPipeline.size()) ||
        target < 0 || target >= static_cast<int>(m_AnimationPipeline.size())) {
        return;
    }

    std::swap(m_AnimationPipeline[row], m_AnimationPipeline[target]);
    ++m_AnimationPipelineRevision;
    invalidateAnimationOutputs();
    updateAnimationFilterSummary();
    ui->listWidgetAnimationPipeline->setCurrentRow(target);
}

void igQtAnimationWidget::clearAnimationPipeline() {
    if (m_AnimationPipeline.isEmpty()) return;
    m_AnimationPipeline.clear();
    ++m_AnimationPipelineRevision;
    invalidateAnimationOutputs();
    updateAnimationFilterSummary();
}

void igQtAnimationWidget::openAnimationFilterParameters() {
    const int row = ui->listWidgetAnimationPipeline->currentRow();
    if (row < 0 || row >= static_cast<int>(m_AnimationPipeline.size())) return;

    const auto& step = m_AnimationPipeline.at(row);
    const QString filterId = step.filterId;
    const auto* descriptor = m_AnimationFilterManager.descriptor(filterId);
    if (!descriptor) return;

    VcrController->onPause();
    QString error;
    igQtAnimationDataInfo input;
    igQtAnimationFilterParameterSchema schema;
    if (!animationPipelineInputInfo(row, input, error) ||
        !m_AnimationFilterManager.parameterSchema(filterId, input, schema, error)) {
        QMessageBox::warning(this, QStringLiteral("动画 Filter"), error);
        return;
    }

    auto* dialog = new igQtFilterDialogDockWidget(this, true);
    dialog->setObjectName(QStringLiteral("animationFilterParameters"));
    dialog->setProperty("pipelineRow", row);
    dialog->setFilterTitle(
            QStringLiteral("动画 Filter 参数 - %1").arg(descriptor->displayName));
    dialog->setFilterDescription(schema.empty()
            ? QStringLiteral("此步骤无需参数，将在每个动画帧上执行。")
            : QStringLiteral("可选字段来自前序步骤。请填写等值或上下限；参数将在每个动画帧上生效。"));

    const QVariantMap existing = step.parameters;
    QMap<QString, int> widgetIds;
    for (const auto& parameter : schema) {
        const QVariant initial = existing.contains(parameter.key)
                                         ? existing.value(parameter.key)
                                         : parameter.defaultValue;
        int widgetId = -1;
        switch (parameter.type) {
            case igQtAnimationFilterParameterType::Boolean:
                widgetId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        parameter.title,
                        initial.toBool() ? QStringLiteral("true")
                                         : QStringLiteral("false"));
                break;
            case igQtAnimationFilterParameterType::Choice: {
                std::vector<QString> choices(parameter.choices.cbegin(),
                                             parameter.choices.cend());
                widgetId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX,
                        parameter.title, choices);
                if (auto* combo = qobject_cast<QComboBox*>(
                            dialog->getWidget(widgetId))) {
                    const int initialIndex = combo->findText(initial.toString());
                    if (initialIndex >= 0) combo->setCurrentIndex(initialIndex);
                    else if (existing.contains(parameter.key)) {
                        // Keep stale choices visible for correction; never silently pick another field.
                        combo->addItem(initial.toString());
                        combo->setCurrentIndex(combo->count() - 1);
                        combo->setToolTip(QStringLiteral("原参数不在当前上游选项中，请重新选择。"));
                    }
                }
                break;
            }
            case igQtAnimationFilterParameterType::String:
            case igQtAnimationFilterParameterType::Integer:
            case igQtAnimationFilterParameterType::Double:
                widgetId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        parameter.title, initial.toString());
                break;
        }
        if (widgetId >= 0) {
            widgetIds.insert(parameter.key, widgetId);
            dialog->getWidget(widgetId)->setObjectName("animationParam_" + parameter.key);
        }
    }

    auto* parameterError = new QLabel(dialog);
    parameterError->setObjectName(QStringLiteral("animationParameterError"));
    parameterError->setWordWrap(true);
    parameterError->setStyleSheet(QStringLiteral("color: #e0a050;"));
    dialog->addRowWidget(parameterError);
    dialog->setApplyFunctor(
            [this, dialog, parameterError, row, filterId, schema, widgetIds,
             revision = m_AnimationPipelineRevision, source = animationFilterInput()]() mutable {
                parameterError->clear();
                if (revision != m_AnimationPipelineRevision || source != animationFilterInput() ||
                    row >= m_AnimationPipeline.size() || m_AnimationPipeline[row].filterId != filterId) {
                    parameterError->setText(QStringLiteral("流程或输入已改变，请关闭并重新打开参数窗口。"));
                    return;
                }
                QVariantMap values;
                for (const auto& parameter : schema) {
                    const int widgetId = widgetIds.value(parameter.key, -1);
                    QWidget* widget = dialog->getWidget(widgetId);
                    if (!widget) continue;

                    switch (parameter.type) {
                        case igQtAnimationFilterParameterType::Boolean: {
                            auto* check = qobject_cast<QCheckBox*>(widget);
                            values.insert(parameter.key,
                                          check && check->isChecked());
                            break;
                        }
                        case igQtAnimationFilterParameterType::Choice: {
                            auto* combo = qobject_cast<QComboBox*>(widget);
                            values.insert(parameter.key,
                                          combo ? combo->currentText() : QString());
                            break;
                        }
                        case igQtAnimationFilterParameterType::Integer: {
                            bool ok = false;
                            const int value = qobject_cast<QLineEdit*>(widget)
                                                      ->text().toInt(&ok);
                            values.insert(parameter.key,
                                          ok ? QVariant(value) : QVariant(QString()));
                            break;
                        }
                        case igQtAnimationFilterParameterType::Double: {
                            bool ok = false;
                            const double value = qobject_cast<QLineEdit*>(widget)
                                                         ->text().toDouble(&ok);
                            values.insert(parameter.key,
                                          ok ? QVariant(value) : QVariant(QString()));
                            break;
                        }
                        case igQtAnimationFilterParameterType::String:
                            values.insert(parameter.key,
                                          qobject_cast<QLineEdit*>(widget)->text());
                            break;
                    }
                }

                QString error;
                igQtAnimationDataInfo input, output;
                if (!animationPipelineInputInfo(row, input, error) ||
                    !m_AnimationFilterManager.describeOutput(filterId, input, values, output, error)) {
                    parameterError->setText(error);
                    return;
                }
                if (row < 0 || row >= static_cast<int>(m_AnimationPipeline.size())) {
                    return;
                }
                m_AnimationPipeline[row].parameters = values;
                revision = ++m_AnimationPipelineRevision;
                invalidateAnimationOutputs();
                updateAnimationFilterSummary();
            });

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

QString igQtAnimationWidget::selectedAnimationFilterId() const {
    return ui->comboBoxAnimationFilter->currentData().toString();
}

iGame::DataObject::Pointer igQtAnimationWidget::animationFilterInput() const {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto model = scene ? scene->GetCurrentModel() : nullptr;
    if (!model) return nullptr;
    if (m_AnimationFilterSourceModel && m_AnimationFilterSourceObject &&
        m_AnimationFilterSourceModel.GetPointer() == model.GetPointer() &&
        (model->GetDataObject() == m_AnimationFilterSourceObject || model->GetDataObject() == m_AnimationDisplayedObject)) {
        return m_AnimationFilterSourceObject;
    }
    return model->GetDataObject();
}

bool igQtAnimationWidget::animationPipelineInputInfo(
        int row, igQtAnimationDataInfo& input, QString& error) const {
    igQtAnimationDataInfo source;
    if (!igQtDescribeAnimationData(animationFilterInput(), source, error)) return false;
    return igQtDescribeAnimationPipelineInput(m_AnimationFilterManager, m_AnimationPipeline,
                                             row, source, input, error);
}

void igQtAnimationWidget::updateAnimationFilterSummary() {
    const int previousRow = ui->listWidgetAnimationPipeline->currentRow();
    ui->listWidgetAnimationPipeline->clear();
    igQtAnimationDataInfo upstream;
    QString descriptionError;
    bool described = igQtDescribeAnimationData(animationFilterInput(), upstream, descriptionError);
    for (int i = 0; i < static_cast<int>(m_AnimationPipeline.size()); ++i) {
        const auto& step = m_AnimationPipeline.at(i);
        const auto* descriptor = m_AnimationFilterManager.descriptor(step.filterId);
        QString text = QStringLiteral("%1. %2")
                               .arg(i + 1)
                               .arg(descriptor ? descriptor->displayName
                                               : step.filterId);
        igQtAnimationDataInfo output;
        QString rowError = descriptionError;
        const bool valid = described && m_AnimationFilterManager.describeOutput(
                step.filterId, upstream, step.parameters, output, rowError);
        if (valid) {
            if (!step.parameters.isEmpty()) text += QStringLiteral("（参数已设置）");
            upstream = std::move(output);
        } else {
            text += described ? QStringLiteral("（需配置）") : QStringLiteral("（上游未就绪）");
            if (described) descriptionError = QStringLiteral("第 %1 步未就绪：%2").arg(i + 1).arg(rowError);
            described = false;
        }
        ui->listWidgetAnimationPipeline->addItem(text);
        ui->listWidgetAnimationPipeline->item(i)->setToolTip(valid ? QString() : rowError);
        if (!valid) ui->listWidgetAnimationPipeline->item(i)->setForeground(QColor(220, 150, 60));
    }
    if (previousRow >= 0 &&
        previousRow < ui->listWidgetAnimationPipeline->count()) {
        ui->listWidgetAnimationPipeline->setCurrentRow(previousRow);
    }

    if (m_AnimationPipeline.isEmpty()) {
        ui->labelAnimationFilterSummary->setText(
                QStringLiteral("空 Pipeline：输出源帧"));
    } else {
        ui->labelAnimationFilterSummary->setText(
                QStringLiteral("%1 步 Filter").arg(m_AnimationPipeline.size()));
    }
    onPipelineSelectionChanged();
}

void igQtAnimationWidget::applyPipelineDisplaySettings(
        iGame::Scene* scene,
        iGame::DataObject::Pointer displayObject,
        iGame::DataObject::Pointer sourceObject) {
    if (m_AnimationPipelineDisplayAttribute.isEmpty() || !displayObject || !scene) {
        return;
    }
    if (sourceObject) {
        displayObject->SetColorMapper(sourceObject->GetColorMapper());
    }
    auto drawObject = iGame::DynamicCast<iGame::DrawObject>(displayObject);
    auto attrSet = displayObject->GetAttributeSet();
    if (drawObject && attrSet) {
        const int index = attrSet->GetAttributeIndex(
                m_AnimationPipelineDisplayAttribute.toStdString());
        if (index >= 0) {
            // SetAttributeIndex alone bypasses the color/dirty flags and makes
            // ViewCloudPicture's unchanged-selection guard skip initialization.
            drawObject->ViewCloudPicture(scene, index,
                                         m_AnimationPipelineDisplayDimension);
        }
    }
    // Output blocks were attached before assigning the source color mapper.
    // Share it explicitly and resolve the display field in each block by name.
    if (displayObject->HasSubDataObject()) {
        for (auto it = displayObject->SubDataObjectIteratorBegin();
             it != displayObject->SubDataObjectIteratorEnd(); ++it) {
            applyPipelineDisplaySettings(scene, it->second, sourceObject);
        }
    }
}

void igQtAnimationWidget::restoreAnimationFilterSource() {
    auto scene = m_AnimationFilterSourceModel ? m_AnimationFilterSourceModel->GetScene() : nullptr;
    if (scene) scene->MakeCurrent();
    m_AnimationOutputCache.clear();
    if (m_AnimationFilterSourceModel && m_AnimationFilterSourceObject &&
        m_AnimationFilterSourceModel->GetDataObject() == m_AnimationDisplayedObject) {
        m_AnimationFilterSourceModel->SetDataObject(m_AnimationFilterSourceObject);
    }
    m_AnimationFilterSourceModel = nullptr;
    m_AnimationFilterSourceObject = nullptr;
    m_AnimationDisplayedObject = nullptr;
    m_AnimationInitializedSource = nullptr;
    if (scene) scene->DoneCurrent();
}

void igQtAnimationWidget::btnPlay_finishLoop() {
    ui->btnPlayOrPause->setChecked(false);
    ui->btnReverseOrPause->setChecked(false);
}

void igQtAnimationWidget::updateAnimationComponentsKeyframeSum(
        int keyframeSum) {
    VcrController->setKeyframe_sum(keyframeSum);
    ui->SliderAnimationTrack->setValue(0);
    ui->SliderAnimationTrack->setMaximum(std::max(0, keyframeSum - 1));
    QSignalBlocker blocked(ui->comboBoxCurrentAnimation);
    ui->comboBoxCurrentAnimation->clear();
    for (int i = 0; i < keyframeSum; ++i) ui->comboBoxCurrentAnimation->addItem(QString::number(i + 1));
    updateCacheChoices(keyframeSum);
}

void igQtAnimationWidget::updateAnimationModeControls() {
    const bool interpolateMode = ui->rbtnInterpolateTimeMode->isChecked();

    ui->label_19->setText(interpolateMode ? QStringLiteral("关键帧数量")
                                          : QStringLiteral("原始帧数"));
    ui->lineEditKeyframeNum->setEnabled(interpolateMode);
    ui->lineEditStartTime->setEnabled(interpolateMode);
    ui->lineEditEndTime->setEnabled(interpolateMode);
    ui->label_19->setEnabled(interpolateMode);
    ui->label_20->setEnabled(interpolateMode);
    ui->label_21->setEnabled(interpolateMode);
    ui->btnApplyAnimationOperation->setEnabled(interpolateMode);

    if (interpolateMode) {
        if (m_InterpolateFrameCount > 0) {
            ui->lineEditKeyframeNum->setText(
                    QString::number(m_InterpolateFrameCount));
            ui->lineEditStartTime->setText(
                    QString::number(m_InterpolateStartTime, 'g', 10));
            ui->lineEditEndTime->setText(
                    QString::number(m_InterpolateEndTime, 'g', 10));
        }
    } else {
        ui->lineEditKeyframeNum->setText(
                QString::number(m_SourceFrameCount));
        ui->lineEditStartTime->setText(
                QString::number(m_SourceStartTime, 'g', 10));
        ui->lineEditEndTime->setText(
                QString::number(m_SourceEndTime, 'g', 10));
    }
}

void igQtAnimationWidget::changeAnimationMode() {
    invalidateAnimationOutputs();
    if (ui->rbtnSnapTimeMode->isChecked()) {
        // 切换前保存插值模式的设置，切回时恢复。
        if (m_InterpolateFrameCount > 0) {
            m_InterpolateStartTime = ui->lineEditStartTime->text().toFloat();
            m_InterpolateEndTime = ui->lineEditEndTime->text().toFloat();
            m_InterpolateFrameCount = ui->lineEditKeyframeNum->text().toInt();
        }

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
    updateAnimationModeControls();
}

void igQtAnimationWidget::onCacheNumChanged(int count) {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene) scene->MakeCurrent();
    m_AnimationOutputCache.setCapacity(count);
    auto source = animationFilterInput();
    if (source && source->PeekTimeFrames()) source->PeekTimeFrames()->DisableCache();
    if (scene) scene->DoneCurrent();
}

void igQtAnimationWidget::updateCacheChoices(int frameCount) {
    const int capacity = std::min(m_AnimationOutputCache.capacity(), frameCount);
    QSignalBlocker blocked(ui->comboBox_AnimationCacheNum);
    ui->comboBox_AnimationCacheNum->clear();
    for (int i = 0; i <= frameCount; ++i) ui->comboBox_AnimationCacheNum->addItem(QString::number(i));
    ui->comboBox_AnimationCacheNum->setCurrentIndex(capacity);
    ui->comboBox_AnimationCacheNum->setToolTip(QStringLiteral("最多缓存 N 个 Pipeline 最终输出；0 关闭缓存。"));
    onCacheNumChanged(capacity);
}
void igQtAnimationWidget::setVortexAutoCompute(bool enabled, const std::string& sourceAttrName) {
    using namespace iGame;
    invalidateAnimationOutputs();
    m_VortexAutoCompute = enabled;
    if (!sourceAttrName.empty()) { m_VortexSourceAttr = sourceAttrName; }
    if (!enabled) {
        m_VortexSourceAttr.clear();
        m_VortexBoundModel = nullptr;
        return;
    }
    // 记住绑定到哪个模型，供 initAnimationComponents 判断是否发生了模型切换
    auto scene = SceneManager::Instance()->GetCurrentScene();
    if (scene && scene->GetCurrentModel()) {
        m_VortexBoundModel = animationFilterInput().GetPointer();
    }
}

int igQtAnimationWidget::ensureVortexForCurrentFrame(iGame::DataObject::Pointer obj,
                                                     const std::string& sourceAttrName,
                                                     int frameIndexForDisplay) {
    using namespace iGame;
    if (!obj || sourceAttrName.empty()) return -1;

    static const std::string kOutName = "vorticities";

    // 1) 幂等检查：当前帧的每个子对象是否都已带 vorticities。
    //    缓存命中时帧对象上还留着上次算好的结果，直接复用，避免重复计算。
    bool needCompute = false;
    if (obj->HasSubDataObject()) {
        for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); ++it) {
            if (!it->second) continue;
            auto a = it->second->GetAttributeSet();
            if (!a || a->GetAttributeIndex(kOutName) < 0) {
                needCompute = true;
                break;
            }
        }
    } else {
        auto a = obj->GetAttributeSet();
        needCompute = (!a || a->GetAttributeIndex(kOutName) < 0);
    }

    // 2) 缺失才算。同步执行——调用方（切帧回调）需要等它算完再渲染本帧。
    //    命中缓存时这里整段跳过，不做计算也不触碰进度条
    //    （进度条收到 100% 会自动复位，每帧无条件写会导致它反复闪动）。
    if (needCompute) {
        auto progressObserver = ProgressObserver::Instance();
        progressObserver->UpdateText(
                frameIndexForDisplay >= 0
                        ? QStringLiteral("计算涡量 第 %1 帧").arg(frameIndexForDisplay + 1).toStdString()
                        : std::string("计算涡量"));

        auto filter = VortexFilter::New();
        filter->SetInput(obj);
        filter->SetAttributeByName(sourceAttrName);
        const bool ok = filter->Execute();

        progressObserver->UpdateText("");
        progressObserver->UpdateProgress(1.0);

        if (!ok) {
            std::cout << "[Vortex] frame compute failed: " << filter->GetMessage() << std::endl;
            return -1;
        }
    }

    // 3) 父容器登记：vorticities 是写进子对象的，而模型树与云图按父容器的属性下标寻址
    //    （见 DataObject::ReCollectSubDataObjectDataRange），必须补一条同名同维占位。
    int parIdx = -1;
    auto parentAttr = obj->GetAttributeSet();
    if (!parentAttr) return -1;
    parIdx = parentAttr->GetAttributeIndex(kOutName);

    if (parIdx < 0 && obj->HasSubDataObject()) {
        auto firstSub = obj->SubDataObjectIteratorBegin()->second;
        auto subAttr = firstSub ? firstSub->GetAttributeSet() : nullptr;
        int subIdx = subAttr ? subAttr->GetAttributeIndex(kOutName) : -1;
        if (subIdx >= 0) {
            auto& sa = subAttr->GetAttribute(subIdx);
            const int vdim = sa.pointer->GetDimension();

            DoubleArray::Pointer placeholder = DoubleArray::New();
            placeholder->SetName(kOutName);
            placeholder->SetDimension(vdim);

            DoubleArray::Pointer range = DoubleArray::New();
            range->SetDimension(2);
            range->Resize(vdim + 1);

            parIdx = static_cast<int>(parentAttr->AddScalar(sa.attachmentType, placeholder, range));
        }
    }

    // 4) 用本帧实际数值刷新父容器值域，否则色条范围是空的
    if (parIdx >= 0 && obj->HasSubDataObject()) {
        obj->ReCollectSubDataObjectDataRange();
        obj->UpdateSubDataObjectDataRange();
    }

    if (needCompute && parIdx >= 0) {
        if (auto drawObj = DynamicCast<DrawObject>(obj)) { drawObj->ForceReConvertToDrawableData(); }
    }

    return parIdx;
}

void igQtAnimationWidget::setPreferredCacheNum(int count) {
    m_PreferredCacheNum = std::max(0, count);
    onCacheNumChanged(m_PreferredCacheNum);
    if (ui->comboBox_AnimationCacheNum->count() > m_PreferredCacheNum) {
        QSignalBlocker blocked(ui->comboBox_AnimationCacheNum);
        ui->comboBox_AnimationCacheNum->setCurrentIndex(m_PreferredCacheNum);
    }
}

void igQtAnimationWidget::initAnimationComponents() {
    if (m_IsAnimationPlaying) return;
    if (!bindAnimationSource()) { ClearAnimationVCRInfo(); return; }
    auto source = m_AnimationFilterSourceObject;
    if (m_AnimationInitializedSource == source.GetPointer()) {
        updateAnimationFilterSummary(); return;
    }
    m_AnimationInitializedSource = source.GetPointer();
    if (m_VortexBoundModel != source.GetPointer()) {
        m_VortexAutoCompute = false; m_VortexSourceAttr.clear(); m_VortexBoundModel = nullptr;
    }
    if (m_DiffBoundModel != source.GetPointer()) {
        m_DiffAutoCompute = false; m_DiffSourceAttr.clear(); m_DiffBoundModel = nullptr;
    }
    std::vector<float> times;
    for (auto& frame : source->PeekTimeFrames()->GetArrays()) times.push_back(frame.GetTimeValue());
    m_SourceFrameCount = static_cast<int>(times.size());
    m_SourceStartTime = m_InterpolateStartTime = times.front();
    m_SourceEndTime = m_InterpolateEndTime = times.back();
    m_InterpolateFrameCount = m_SourceFrameCount;
    VcrController->initController(m_SourceFrameCount, ui->spinBoxAnimationStride->value());
    ui->treeWidget_snap->initAnimationTreeWidget(times);
    ui->treeWidget_interpolate->initAnimationTreeWidget(times);
    onCacheNumChanged(std::min(m_PreferredCacheNum, m_SourceFrameCount));
    updateCacheChoices(m_SourceFrameCount);
    updateAnimationComponentsKeyframeSum(m_SourceFrameCount);
    connect(ui->SliderAnimationTrack, &QSlider::sliderMoved, VcrController,
            &igQtAnimationVcrController::updateCurrentKeyframe, Qt::UniqueConnection);
    connect(ui->comboBoxCurrentAnimation, QOverload<int>::of(&QComboBox::currentIndexChanged),
            VcrController, &igQtAnimationVcrController::updateCurrentKeyframe, Qt::UniqueConnection);
    updateAnimationModeControls();
    updateAnimationFilterSummary();
}
void igQtAnimationWidget::ClearAnimationVCRInfo() {
    invalidateAnimationOutputs();
    m_SourceFrameCount = 1;
    m_SourceStartTime = 0.0f;
    m_SourceEndTime = 0.0f;
    m_InterpolateStartTime = 0.0f;
    m_InterpolateEndTime = 0.0f;
    m_InterpolateFrameCount = 1;
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
    updateAnimationModeControls();
}

//#include <fstream>
//#include <windows.h>

#include <FFMPEG/iGameFFMPEGVideoWriter.h>
#include <IQComponents/Dialog/igQtVideoOptionDialog.h>
#include <QDebug>
bool igQtAnimationWidget::saveAnimation() {
#if defined(FFMPEG_ENABLE)
    using namespace iGame;
    if (!bindAnimationSource()) {
        igQtShowDarkFramelessMessage(this, QStringLiteral("保存动画"),
                                   QStringLiteral("请导入带时间帧的文件"), true);
        return false;
    }
    initAnimationComponents();
    VcrController->onPause();
    ui->btnPlayOrPause->setChecked(false);
    ui->btnReverseOrPause->setChecked(false);
    auto* renderer = igQtOpenGLManager::Instance()->getRenderWidget();
    if (!renderer) return false;
    const QStringList formats{"Mp4 File(*.mp4)", "GIF File(*.gif)", "PNG Files(*.png)"};
    QString selected;
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("保存动画"), "", formats.join(";;"), &selected);
    if (path.isEmpty()) return false;
    igQtVideoOptionDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return false;
    auto input = dialog.getInput();
    if (input.width <= 0 || input.height <= 0 || input.frame_rate <= 0) return false;
    const int format = formats.indexOf(selected);
    if (format < 0) return false;
    const QString suffix = format == 0 ? ".mp4" : format == 1 ? ".gif" : ".png";
    if (!path.endsWith(suffix, Qt::CaseInsensitive)) path += suffix;
    const QSize oldSize = renderer->size();
    const qreal pixelRatio = renderer->devicePixelRatioF();
    renderer->resize(qRound(input.width / pixelRatio), qRound(input.height / pixelRatio));
    const int previousFrame = VcrController->currentKeyframeIndex();
    const QFileInfo file(path);
    bool ok = true;
    QString error;
    for (int frame = 0; frame < animationOutputFrameCount(); ++frame) {
        if (!renderAnimationOutputFrame(frame, true)) {
            error = m_AnimationFrameError; ok = false; break;
        }
        QImage image = renderer->grabFramebuffer();
        if (image.isNull()) { error = QStringLiteral("读取动画画面失败。"); ok = false; break; }
        if (image.size() != QSize(input.width, input.height))
            image = image.scaled(input.width, input.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        if (format == 2) {
            if (!image.save(file.path() + "/" + file.completeBaseName() + QString("_%1.png").arg(frame))) {
                error = QStringLiteral("写入 PNG 帧失败。"); ok = false; break;
            }
        } else {
            image = image.convertToFormat(QImage::Format_RGBA8888);
            input.bytes_per_line = image.bytesPerLine();
            input.raw_image_data.emplace_back(image.constBits(), image.constBits() + image.sizeInBytes());
        }
    }
    renderer->resize(oldSize);
    renderAnimationOutputFrame(previousFrame);
    if (ok && format != 2) {
        auto writer = FFMPEGVideoWriter::New();
        input.output_path = path.toStdString(); writer->SetVideoInputInfo(input);
        ok = format == 0 ? writer->SaveMP4() : writer->SaveGIF();
    }
    igQtShowDarkFramelessMessage(this, QStringLiteral("保存动画"),
        ok ? QStringLiteral("保存成功") : (error.isEmpty() ? QStringLiteral("保存失败") : error), ok);
    return ok;
#else
    return false;
#endif
}
std::string igQtAnimationWidget::GetDiffOutputName(const std::string& sourceAttrName) const {
    static const char* modeSuffix[] = {"", "_abs", "_rel"};
    const int mode = (m_DiffMode >= 0 && m_DiffMode <= 2) ? m_DiffMode : 0;
    return sourceAttrName + "_diff" + modeSuffix[mode];
}

int igQtAnimationWidget::currentFrameIndex() const {
    // VcrController 记录的是“源时间序列”的下标：数据转换要作用于当前帧而不是第一帧。
    if (m_AnimationDisplayedObject) return m_DisplayedSourceFrame;
    if (VcrController != nullptr) { return VcrController->currentKeyframeIndex(); }
    if (ui && ui->comboBoxCurrentAnimation) { return ui->comboBoxCurrentAnimation->currentIndex(); }
    return 0;
}

void igQtAnimationWidget::SetDiffMode(int mode) {
    if (m_DiffMode != mode) { m_DiffMode = mode; invalidateAnimationOutputs(); }
}

void igQtAnimationWidget::SetDiffAutoCompute(bool enabled, const std::string& sourceAttrName) {
    using namespace iGame;
    invalidateAnimationOutputs();
    m_DiffAutoCompute = enabled;
    if (!sourceAttrName.empty()) { m_DiffSourceAttr = sourceAttrName; }
    if (!enabled) {
        m_DiffSourceAttr.clear();
        m_DiffBoundModel = nullptr;
        return;
    }
    auto scene = SceneManager::Instance()->GetCurrentScene();
    if (scene && scene->GetCurrentModel()) {
        m_DiffBoundModel = animationFilterInput().GetPointer();
    }
}

int igQtAnimationWidget::EnsureTimeDifferenceForCurrentFrame(iGame::DataObject::Pointer obj,
                                                             const std::string& sourceAttrName, int frameIndex) {
    using namespace iGame;
    if (!obj || sourceAttrName.empty() || frameIndex < 0) return -1;
    const std::string outputName = GetDiffOutputName(sourceAttrName);

    bool needCompute = false;
    if (obj->HasSubDataObject()) {
        for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); it++) {
            if (!it->second) continue;
            auto attr = it->second->GetAttributeSet();
            if (!attr || attr->GetAttributeIndex(outputName) < 0) {
                needCompute = true;
                break;
            }
        }
    } else {
        auto attr = obj->GetAttributeSet();
        needCompute = (!attr || attr->GetAttributeIndex(outputName) < 0);
    }

    if (needCompute) {
        auto progressObserver = ProgressObserver::Instance();
        progressObserver->UpdateText("计算属性差值 第 " + std::to_string(frameIndex + 1) + " 帧");

        auto filter = iGameAttrDiff::New();
        filter->SetInput(obj);
        filter->SetAttributeByName(sourceAttrName);
        filter->SetFrameIndex(frameIndex);
        filter->SetDiffMode(m_DiffMode);
        filter->SetOutputName(outputName);
        const bool ok = filter->Execute();
        progressObserver->UpdateText("");
        progressObserver->UpdateProgress(1.0);
        if (!ok) {
            std::cout << "[AttrDiff] frame compute failed: " << filter->GetMessage() << std::endl;
            return -1;
        }
    }

    auto parentAttr = obj->GetAttributeSet();
    if (!parentAttr) return -1;
    return parentAttr->GetAttributeIndex(outputName);
}

void igQtAnimationWidget::changeEvent(QEvent* e) {
    if (e && e->type() == QEvent::StyleChange) {
        igQtPanelTheme::refreshDeep(this);
    }
    QWidget::changeEvent(e);
}
