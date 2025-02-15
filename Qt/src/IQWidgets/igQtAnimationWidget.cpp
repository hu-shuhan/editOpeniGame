//
// Created by m_ky on 2024/4/22.
//
#include <iGameFileIO.h>
#include <iGameSceneManager.h>
#include <iGameThreadPool.h>

#include <IQComponents/igQtAnimationTreeWidget_interpolate.h>
#include <IQComponents/igQtAnimationTreeWidget_snap.h>
#include <IQCore/igQtAnimationVcrController.h>
#include <IQCore/igQtOpenGLWidgetManager.h>
#include <IQWidgets/igQtAnimationWidget.h>
#include <QAbstractButton>
#include <QFileDialog>
#include <QMessageBox>
#include <Deformation/iGameStressDeformationFilter.h>

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

#include <iGameType.h>
#include <iGamePointSet.h>
#include <Abaqus/iGameODBReader.h>
void igQtAnimationWidget::playAnimation_snap(unsigned int keyframe_idx) {
    using namespace iGame;
    auto currentScene = SceneManager::Instance()->GetCurrentScene();
    if(currentScene->GetCurrentModel() == nullptr ) return;
    auto currentDrawObject = DynamicCast<DrawObject>(
            currentScene->GetCurrentModel()->GetDataObject());
    if (currentDrawObject == nullptr ||
        currentDrawObject->GetTimeFrames()->GetArrays().empty())
        return;

    currentDrawObject->GetTimeFrames()->EnableCache();
    auto timeFrameType = currentDrawObject->GetTimeFrames()->GetTargetFrameType(keyframe_idx);
    auto timeFrameData = currentDrawObject->GetTimeFrames()->GetTargetTimeFrameData(keyframe_idx);
    if(timeFrameType == StreamingType::MultiSubFiles){
        currentDrawObject->ClearSubDataObject();
        for(auto& subObj : timeFrameData){
            auto subDataObj = DynamicCast<iGame::DataObject>(subObj);
            if(subDataObj){
                currentDrawObject->AddSubDataObject(subDataObj);
            }
        }
    } else if(timeFrameType == StreamingType::SingleFieldAttributes){
        auto attributeSet = DynamicCast<iGame::AttributeSet>(timeFrameData[0]);
        if(attributeSet){
            currentDrawObject->SetAttributeSet(attributeSet);
            DynamicCast<iGame::PointSet>(currentDrawObject)->GetPoints()->Modified();
            currentDrawObject->ConvertToDrawableData();
        }
    }

//    auto& currentFrame = currentDrawObject->GetTimeFrames()->GetTargetTimeFrame(keyframe_idx);
//    auto frameData = currentFrame.GetMetaData();

//    /* If the timeframe data store MultiSubFile's Path, the job is to Parse the sub File. */
//    if(currentFrame.GetFrameType() == StreamingType::MultiSubFiles)
//    {
//        std::vector<std::future<iGame::DataObject::Pointer>> tasks;
//        std::vector<iGame::DataObject::Pointer> results(frameData->GetNumberOfElements());
//        for (int i = 0; i < frameData->GetNumberOfElements(); i++) {
//            tasks.emplace_back(iGame::ThreadPool::Instance()->Commit(
//                    [i, &results](const std::string& fileName) {
//                        auto res = FileIO::ReadFile(fileName);
//                        results[i] = res;
//                        return res;
//                    },
//                    frameData->GetElement(i)));
//        }
////        for(int i = 0; i < frameData->GetNumberOfElements(); i ++){
////            results[i] = FileIO::ReadFile(frameData->GetElement(i));
////        }
//        currentDrawObject->ClearSubDataObject();
//        for (auto& task: tasks) {
//            task.get();
//        }
//        for(auto& subObj : results){
//            currentDrawObject->AddSubDataObject(subObj);
//        }
//        results.clear();
//    } /* If the timeframe data store SingleField Attributes' Path,
//    *   the job is to Parse the target File's Field Attribute replace of the original one. */
//    else if(currentFrame.GetFrameType() == StreamingType::SingleFieldAttributes)
//    {
//        const auto& filePath = currentFrame.GetMetaData()->GetElement(0);
//        auto fileType = FileIO::GetFileType(filePath);
//        if(fileType == FileIO::FileType::ODB){
//            #if defined(AbqSDK_ENABLE)
//            ODBReader::Pointer reader = ODBReader::New();
//            auto attributeSet = reader->ReadOdbFieldData(filePath, keyframe_idx);
//            currentDrawObject->SetAttributeSet(attributeSet);
//            DynamicCast<iGame::PointSet>(currentDrawObject)->GetPoints()->Modified();
//            currentDrawObject->ConvertToDrawableData();
//            #endif
//        }
//    }




    /* If obj has the deformation var and is enabled.
     * Make sure every timeStep have the deformation scale factor. */

    if(currentDrawObject->GetDeformationData()->GetEnableStatus()){
        StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
        deformFilter->SetInput(currentDrawObject);
        if(!deformFilter->Execute()) std::cout << " error \n";
    }


    /* process Object's scalar range*/
    currentDrawObject->UpdateSubDataObjectDataRange();

    currentScene->MakeCurrent();
    currentDrawObject->SetViewStyle(currentDrawObject->GetViewStyle());

    if (currentDrawObject->GetAttributeIndex() != -1) {
        currentDrawObject->ViewCloudPicture(
                currentScene, currentDrawObject->GetAttributeIndex());
    }
    currentScene->DoneCurrent();

    Q_EMIT UpdateScene();
}

void igQtAnimationWidget::playAnimation_interpolate(int keyframe_0, float t) {


    using namespace iGame;
    auto currentScene = SceneManager::Instance()->GetCurrentScene();
    auto currentDrawObject = DynamicCast<DrawObject>(
            currentScene->GetCurrentModel()->GetDataObject());
    if (currentDrawObject == nullptr
    ||  currentDrawObject->GetTimeFrames()->GetArrays().empty()
    ||  keyframe_0 + 1 == currentDrawObject->GetTimeFrames()->GetArrays().size())
        return;
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

    if (currentDrawObject->GetAttributeIndex() != -1) {
        currentDrawObject->ViewCloudPicture(
                currentScene, currentDrawObject->GetAttributeIndex());
    }
    currentScene->DoneCurrent();

    Q_EMIT UpdateScene();

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

    auto& timeArrays = iGame::SceneManager::Instance()
                              ->GetCurrentScene()
                              ->GetCurrentModel()
                              ->GetDataObject()
                              ->GetTimeFrames()
                              ->GetArrays();
    if (timeArrays.empty()) return;
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

    ui->lineEditKeyframeNum->setText(
            QString("%1").arg(static_cast<int>(timeValues.size())));
    ui->lineEditStartTime->setText(
            QString::asprintf("%.f", *timeValues.begin()));
    ui->lineEditEndTime->setText(
            QString::asprintf("%.20f", *(timeValues.end() - 1)));
    connect(ui->SliderAnimationTrack, &QSlider::sliderMoved, VcrController,
            &igQtAnimationVcrController::updateCurrentKeyframe);
}

//#include <fstream>
//#include <windows.h>

#include <FFMPEG/iGameFFMPEGVideoWriter.h>
#include <IQComponents/Dialog/igQtVideoOptionDialog.h>
#include <qDebug>
bool igQtAnimationWidget::saveAnimation() {
#if defined(FFMPEG_ENABLE)
    using namespace iGame;
    auto currentScene = SceneManager::Instance()->GetCurrentScene();
    if (currentScene->GetCurrentModel() == nullptr ||
        currentScene->GetCurrentModel()->GetDataObject()->GetTimeFrames()->GetArrays().empty()) {
        QMessageBox::information(this, "", "请导入带时间帧的文件");
        return false;
    }
    auto currentObject = currentScene->GetCurrentModel()->GetDataObject();
    size_t timeStepSize = currentObject->GetTimeFrames()->GetTimeNum();

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
    dialog.setWindowTitle("Save Animation Option.");
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
    for(int i = 0; i < timeStepSize; i ++)
    {
        this->playAnimation_snap(i);
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
        QMessageBox::information(this, "", "保存成功");
    } else {
        QMessageBox::information(this, "", "保存失败");
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
