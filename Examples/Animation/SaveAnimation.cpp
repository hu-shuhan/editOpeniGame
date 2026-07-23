//
// Created by m_ky on 2024/11/26.
//

/**
 * @class   TestAnimation
 * @brief   TestAnimation's brief
 */
#include <Deformation/iGameStressDeformationFilter.h>
#include <future>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameScalarsToColors.h>
#include <iGameThreadPool.h>
#include <iostream>
#include <string>

#include <FFMPEG/iGameFFMPEGVideoWriter.h>

namespace {
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

void PlayAnimation(iGame::DataObject::Pointer obj, iGame::Scene* scene, int keyframe_idx) {
    using namespace iGame;

    if (obj == nullptr || obj->GetTimeFrames()->GetArrays().empty()) return;
    iGame::DrawObject::Pointer currentDrawObject = iGame::DynamicCast<iGame::DrawObject>(obj);
    currentDrawObject->GetTimeFrames()->EnableCache();
    currentDrawObject->UpdateAnimation(keyframe_idx);
    if (obj->GetDeformationData()->GetEnableStatus()) {
        StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
        deformFilter->SetInput(currentDrawObject);
        if (!deformFilter->Execute()) std::cout << " error \n";
    }

    // 固定整段动画颜色范围，避免每帧按当帧 min/max 重置
    LockAnimationColorRange(currentDrawObject.get());

    scene->MakeCurrent();
    currentDrawObject->SetViewStyle(currentDrawObject->GetViewStyle());

    if (currentDrawObject->GetAttributeIndex() != -1) {
        currentDrawObject->ViewCloudPicture(scene, currentDrawObject->GetAttributeIndex());
    }
    scene->DoneCurrent();
    scene->Draw();
}
void SaveAnimationToMP4(iGame::Scene* currentScene, iGame::DataObject ::Pointer currentObject,
                        const std::string& outputPath) {
    using namespace iGame;
    if (currentScene->GetCurrentModel() == nullptr) {
        std::cout << "error\n";
        return;
    }
    auto drawObj = DynamicCast<DrawObject>(currentObject);
    // 导出前先播第 0 帧建立 mapper 范围，再锁定
    PlayAnimation(currentObject, currentScene, 0);
    LockAnimationColorRange(drawObj.get());

    size_t timeStepSize = currentObject->GetTimeFrames()->GetTimeNum();
    std::cout << "time step size: " << timeStepSize << std::endl;
    int width = 1920, height = 1080;
    VideoInputInfo inputInfo;
    inputInfo.width = 1920;
    inputInfo.height = 1080;
    inputInfo.bit_rate = 1000000;
    inputInfo.frame_rate = 1;
    for (int i = 0; i < timeStepSize; i++) {
        PlayAnimation(currentObject, currentScene, i);
        auto tmp = currentScene->CaptureScreen(0, 0, width, height, GLFramebuffer::Type::RGBA, true);
        inputInfo.bytes_per_line = width * 4;
        std::cout << i << " " << tmp.size() << "\n";
        inputInfo.raw_image_data.emplace_back(tmp);
    }
    UnlockAnimationColorRange(drawObj.get());

    FFMPEGVideoWriter::Pointer videoWriter = FFMPEGVideoWriter::New();
    inputInfo.output_path = outputPath;
    videoWriter->SetVideoInputInfo(inputInfo);

    bool sc = videoWriter->SaveMP4();
    if (sc) {
        std::cout << "Success to save\n";
    } else {
        std::cout << "Fail to save\n";
    }
}

void SaveAnimationToGIF(iGame::Scene* currentScene, iGame::DataObject ::Pointer currentObject,
                        const std::string& outputPath) {
    using namespace iGame;
    if (currentScene->GetCurrentModel() == nullptr) {
        std::cout << "error\n";
        return;
    }
    auto drawObj = DynamicCast<DrawObject>(currentObject);
    PlayAnimation(currentObject, currentScene, 0);
    LockAnimationColorRange(drawObj.get());

    size_t timeStepSize = currentObject->GetTimeFrames()->GetTimeNum();
    std::cout << "time step size: " << timeStepSize << std::endl;
    int width = 1920, height = 1080;
    VideoInputInfo inputInfo;
    inputInfo.width = 1920;
    inputInfo.height = 1080;
    inputInfo.bit_rate = 1000000;
    inputInfo.frame_rate = 1;
    for (int i = 0; i < timeStepSize; i++) {
        PlayAnimation(currentObject, currentScene, i);

        auto tmp = currentScene->CaptureScreen(0, 0, width, height, GLFramebuffer::Type::RGBA, true);
        inputInfo.bytes_per_line = width * 4;
        std::cout << i << " " << tmp.size() << "\n";
        inputInfo.raw_image_data.emplace_back(tmp);
    }
    UnlockAnimationColorRange(drawObj.get());

    FFMPEGVideoWriter::Pointer videoWriter = FFMPEGVideoWriter::New();
    inputInfo.output_path = outputPath;
    videoWriter->SetVideoInputInfo(inputInfo);

    bool sc = videoWriter->SaveGIF();
    if (sc) {
        std::cout << "Success to save\n";
    } else {
        std::cout << "Fail to save\n";
    }
}


int main(int argn, char** args) {
    auto scene = iGame::Scene::New();
    auto obj = iGame::FileIO::ReadFile("./Models/CAD11/_frames.pvd");
    std::cout << "size : " << obj->GetTimeFrames()->GetArrays().size() << std::endl;
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
    } else {
        scene->AddModel(obj);
    }
    /* Scene rendering needs to be done in the OpenGL context provided by GLFW,
     * so the window needs to be created first */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetScene(scene);
    window->SetSize(1920, 1080);
    SaveAnimationToMP4(scene, obj, "./AnimationExample.mp4");
    SaveAnimationToGIF(scene, obj, "./AnimationExample.gif");
}
