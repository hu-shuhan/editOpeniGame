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
#include <iGameThreadPool.h>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static int TestAnimation(int keyframe_idx) {
    const std::string fileName = "./Models/CAD11/_frames.pvd";

    std::cerr << "[testAnimation] cwd = " << fs::current_path().string() << "\n"
              << std::flush;
    std::cerr << "[testAnimation] looking for: " << fileName << "\n" << std::flush;
    if (!fs::exists(fileName)) {
        std::cerr << "[testAnimation] FAIL: file not found under Working directory.\n"
                  << std::flush;
        return 2;
    }
    std::cerr << "[testAnimation] file found\n" << std::flush;

    auto scene = iGame::Scene::New();
    std::cerr << "[testAnimation] Scene::New OK\n" << std::flush;

    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << "[testAnimation] FAIL: ReadFile returned null\n" << std::flush;
        return 3;
    }
    scene->AddModel(obj);
    std::cerr << "[testAnimation] AddModel OK\n" << std::flush;

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    std::cerr << "[testAnimation] RenderWindow::New OK\n" << std::flush;
    window->SetSize(1920, 1080);
    std::cerr << "[testAnimation] SetScene begin\n" << std::flush;
    window->SetScene(scene);
    std::cerr << "[testAnimation] SetScene OK\n" << std::flush;
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    std::cerr << "[testAnimation] Interactor OK\n" << std::flush;

    using namespace iGame;
    auto timeFrames = obj->GetTimeFrames();
    if (!timeFrames) {
        std::cerr << "[testAnimation] FAIL: GetTimeFrames() is null\n" << std::flush;
        return 4;
    }
    const size_t timeNum = timeFrames->GetTimeNum();
    std::cerr << "[testAnimation] timeFrames OK, timeNum=" << timeNum
              << ", requested keyframe=" << keyframe_idx << "\n"
              << std::flush;
    if (timeNum == 0) {
        std::cerr << "[testAnimation] FAIL: no time frames\n" << std::flush;
        return 7;
    }
    // 无 timestep 的 PVD（仅 part=）会合并成 1 个时间帧；硬编码 index=5 会越界崩。
    unsigned int frameIndex = static_cast<unsigned int>(keyframe_idx);
    if (frameIndex >= timeNum) {
        std::cerr << "[testAnimation] WARN: keyframe " << frameIndex
                  << " out of range [0," << (timeNum - 1)
                  << "], clamp to 0\n"
                  << std::flush;
        frameIndex = 0;
    }
    auto currentFrame = timeFrames->GetTargetTimeFrame(frameIndex);
    auto frameData = currentFrame.GetMetaData();
    auto currentDrawObject = DynamicCast<DrawObject>(obj);
    if (!currentDrawObject) {
        std::cerr << "[testAnimation] FAIL: not a DrawObject\n" << std::flush;
        return 5;
    }
    std::cerr << "[testAnimation] using frameIndex=" << frameIndex
              << " frameType=" << static_cast<int>(currentFrame.GetFrameType())
              << "\n"
              << std::flush;

    if (currentFrame.GetFrameType() == StreamingType::MultiSubFiles) {
        if (!frameData) {
            std::cerr << "[testAnimation] FAIL: frame meta null\n" << std::flush;
            return 6;
        }
        // PVD 读取时通常已加载第 0 帧子文件；勿重复读完整套（可能上千个 vts）。
        if (frameIndex == 0 && currentDrawObject->HasSubDataObject()) {
            std::cerr << "[testAnimation] skip reload, subData already present\n"
                      << std::flush;
        } else {
            const auto n = frameData->GetNumberOfElements();
            std::cerr << "[testAnimation] MultiSubFiles n=" << n << "\n"
                      << std::flush;
            std::vector<std::future<iGame::DataObject::Pointer>> tasks;
            std::vector<iGame::DataObject::Pointer> results(n);
            for (IGsize i = 0; i < n; i++) {
                tasks.emplace_back(iGame::ThreadPool::Instance()->Commit(
                        [i, &results](const std::string& subFile) {
                            auto res = FileIO::ReadFile(subFile);
                            results[i] = res;
                            return res;
                        },
                        frameData->GetElement(i)));
            }
            currentDrawObject->ClearSubDataObject();
            for (auto& task: tasks) { task.get(); }
            for (auto& subObj: results) {
                currentDrawObject->AddSubDataObject(subObj);
            }
            std::cerr << "[testAnimation] MultiSubFiles loaded\n" << std::flush;
        }
    }

    std::cerr << "[testAnimation] deformation check\n" << std::flush;
    if (currentDrawObject->GetDeformationData() &&
        currentDrawObject->GetDeformationData()->GetEnableStatus()) {
        StressDeformationFilter::Pointer deformFilter =
                iGame::StressDeformationFilter::New();
        deformFilter->SetInput(currentDrawObject);
        if (!deformFilter->Execute()) {
            std::cerr << "[testAnimation] WARN: deformation filter failed\n"
                      << std::flush;
        }
    }

    std::cerr << "[testAnimation] update range / view\n" << std::flush;
    currentDrawObject->UpdateSubDataObjectDataRange();
    currentDrawObject->SetViewStyle(currentDrawObject->GetViewStyle());

    if (currentDrawObject->GetAttributeIndex() != -1) {
        std::cerr << "[testAnimation] ViewCloudPicture attr="
                  << currentDrawObject->GetAttributeIndex() << "\n"
                  << std::flush;
        currentDrawObject->ViewCloudPicture(
                scene, currentDrawObject->GetAttributeIndex());
    }

    std::cerr << "[testAnimation] Show() — close window to exit\n" << std::flush;
    window->Show();
    std::cerr << "[testAnimation] Show() returned OK\n" << std::flush;
    return 0;
}

int main(int argc, char** args) {
    (void) argc;
    (void) args;
    const int code = TestAnimation(5);
    if (code != 0) {
        std::cerr << "[testAnimation] exit code=" << code << "\n" << std::flush;
    }
    return code;
}
