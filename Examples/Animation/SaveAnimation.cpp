// Created by m_ky on 2024/11/26.
// Standalone MP4/GIF export example.
#include "SaveAnimation.h"
#include <Deformation/iGameStressDeformationFilter.h>
#include <iGameFileIO.h>
#include <iGameRenderWindow.h>
#include <iostream>

namespace {
class ScopedColorRangeLock {
public:
    explicit ScopedColorRangeLock(iGame::ScalarsToColors::Pointer mapper)
        : m_Mapper(mapper), m_WasStable(mapper->GetStable()) {
        m_Mapper->SetRangeStable(true);
    }
    ~ScopedColorRangeLock() { m_Mapper->SetRangeStable(m_WasStable); }
    ScopedColorRangeLock(const ScopedColorRangeLock&) = delete;
    ScopedColorRangeLock& operator=(const ScopedColorRangeLock&) = delete;

private:
    iGame::ScalarsToColors::Pointer m_Mapper;
    bool m_WasStable;
};

void PlayAnimation(iGame::DrawObject::Pointer object, iGame::Scene* scene, int frame) {
    using namespace iGame;
    const int attributeIndex = object->GetAttributeIndex();
    const int attributeDimension = object->GetAttributeDimension();
    object->GetTimeFrames()->EnableCache(1000);
    object->UpdateAnimation(frame);
    if (object->GetDeformationData()->GetEnableStatus()) {
        auto deformFilter = StressDeformationFilter::New();
        deformFilter->SetInput(object);
        if (!deformFilter->Execute()) std::cout << "Deformation failed\n";
    }

    scene->MakeCurrent();
    object->SetViewStyle(object->GetViewStyle());
    if (attributeIndex != -1) {
        // New children need the selection even if the parent's index is unchanged.
        // Keep the selected vector component instead of reverting to magnitude.
        object->ViewCloudPicture(scene, -1);
        object->ViewCloudPicture(scene, attributeIndex, attributeDimension);
    }
    scene->DoneCurrent();
    scene->Draw();
}
} // namespace

iGame::VideoInputInfo CaptureAnimationFrames(iGame::Scene* scene, iGame::DataObject::Pointer object,
                                             int width, int height) {
    using namespace iGame;
    VideoInputInfo inputInfo{};
    auto draw = DynamicCast<DrawObject>(object);
    if (!scene || !scene->GetCurrentModel() || !draw || width <= 0 || height <= 0) return inputInfo;
    const size_t frameCount = object->GetTimeFrames()->GetTimeNum();
    if (frameCount == 0) return inputInfo;

    // Establish the first frame's range before locking it. An existing user lock
    // is honored by scalar conversion and preserved, including on early exit.
    PlayAnimation(draw, scene, 0);
    ScopedColorRangeLock rangeLock(draw->GetColorMapper());
    inputInfo.width = width;
    inputInfo.height = height;
    inputInfo.bit_rate = 1000000;
    inputInfo.frame_rate = 1;
    inputInfo.bytes_per_line = width * 4;
    for (size_t frame = 0; frame < frameCount; ++frame) {
        PlayAnimation(draw, scene, static_cast<int>(frame));
        inputInfo.raw_image_data.emplace_back(
                scene->CaptureScreen(0, 0, width, height, GLFramebuffer::Type::RGBA, true));
    }
    return inputInfo;
}

void SaveAnimationToMP4(iGame::Scene* scene, iGame::DataObject::Pointer object, const std::string& outputPath) {
    auto inputInfo = CaptureAnimationFrames(scene, object);
    if (inputInfo.raw_image_data.empty()) return;
    auto writer = iGame::FFMPEGVideoWriter::New();
    inputInfo.output_path = outputPath;
    writer->SetVideoInputInfo(inputInfo);
    std::cout << (writer->SaveMP4() ? "Success to save MP4\n" : "Fail to save MP4\n");
}

void SaveAnimationToGIF(iGame::Scene* scene, iGame::DataObject::Pointer object, const std::string& outputPath) {
    auto inputInfo = CaptureAnimationFrames(scene, object);
    if (inputInfo.raw_image_data.empty()) return;
    auto writer = iGame::FFMPEGVideoWriter::New();
    inputInfo.output_path = outputPath;
    writer->SetVideoInputInfo(inputInfo);
    std::cout << (writer->SaveGIF() ? "Success to save GIF\n" : "Fail to save GIF\n");
}

#ifndef IGAME_ANIMATION_EXPORT_LIBRARY
int main(int argc, char** argv) {
    auto scene = iGame::Scene::New();
    auto obj = iGame::FileIO::ReadFile("./Models/CAD11/_frames.pvd");
    if (!obj) {
        std::cerr << "Read ERROR!\n";
        return 1;
    }
    scene->AddModel(obj);
    // Rendering needs the OpenGL context provided by the window.
    auto window = iGame::RenderWindow::New();
    window->SetScene(scene);
    window->SetSize(1920, 1080);
    SaveAnimationToMP4(scene, obj, "./AnimationExample.mp4");
    SaveAnimationToGIF(scene, obj, "./AnimationExample.gif");
}
#endif
