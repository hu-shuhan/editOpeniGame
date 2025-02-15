//
// Created by m_ky on 2024/11/26.
//

/**
 * @class   TestAnimation
 * @brief   TestAnimation's brief
 */
#include <iGameRenderWindow.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iostream>
#include <string>
#include <Deformation/iGameStressDeformationFilter.h>
#include <iGameThreadPool.h>
#include <future>

#include <FFMPEG/iGameFFMPEGVideoWriter.h>
void PlayAnimation(iGame::DataObject::Pointer obj, iGame::Scene* scene,int keyframe_idx){
    using namespace iGame;

//    auto currentDrawObject = iGame
    auto currentFrame = obj->GetTimeFrames()->GetTargetTimeFrame(keyframe_idx);
    auto frameData = currentFrame.GetMetaData();
    {
        std::vector<std::future<iGame::DataObject::Pointer>> tasks;
        std::vector<iGame::DataObject::Pointer> results(frameData->GetNumberOfElements());
        for (int i = 0; i < frameData->GetNumberOfElements(); i++) {
            tasks.emplace_back(iGame::ThreadPool::Instance()->Commit(
                    [i, &results](const std::string& fileName) {
                        auto res = FileIO::ReadFile(fileName);
                        results[i] = res;
                        return res;
                    },
                    frameData->GetElement(i)));
        }
        obj->ClearSubDataObject();
        for (auto& task: tasks) {
            task.get();
        }
        for(auto& subObj : results){
            obj->AddSubDataObject(subObj);
        }
    }
    /* If obj has the deformation var and is enabled.
     * Make sure every timeStep have the deformation scale factor. */

    if(obj->GetDeformationData()->GetEnableStatus()){
        StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
        deformFilter->SetInput(obj);
        if(!deformFilter->Execute()) std::cout << " error \n";
    }


    /* process Object's scalar range*/
    obj->UpdateSubDataObjectDataRange();
    auto curDrawOBj = iGame::DynamicCast<DrawObject>(obj);
    curDrawOBj->SetViewStyle(curDrawOBj->GetViewStyle());

    if (curDrawOBj->GetAttributeIndex() != -1) {
        curDrawOBj->ViewCloudPicture(
                scene, curDrawOBj->GetAttributeIndex());
    }
    scene->Draw();

}
void SaveAnimationToMP4(iGame::Scene* currentScene ,const std::string& outputPath){
    auto currentObject = currentScene->GetCurrentModel()->GetDataObject();
    size_t timeStepSize = currentObject->GetTimeFrames()->GetTimeNum();

    int ratio = currentScene->GetCamera()->GetDevicePixelRatio();
    auto wh = currentScene->GetCamera()->GetViewPort();
    int width = wh[0] / ratio, height = wh[1] / ratio;


    iGame::VideoInputInfo inputInfo;
    inputInfo.output_path = outputPath;
    inputInfo.width = width;
    inputInfo.height = height;
    inputInfo.bit_rate = 1000000;
    inputInfo.frame_rate = 1;
    /* RGBA Type , means that one pixel's size is 4 byte.*/
    inputInfo.bytes_per_line = width * 4;

    for(size_t i = 0; i < timeStepSize; i ++){
        PlayAnimation(currentObject, currentScene, i);
        std::vector<uint8_t> currentFrameBuffer = currentScene->CaptureScreen(0, 0, width, height, RGBA, true);
        inputInfo.raw_image_data.emplace_back(currentFrameBuffer);
    }
    iGame::FFMPEGVideoWriter::Pointer videoWriter = iGame::FFMPEGVideoWriter::New();
    videoWriter->SetVideoInputInfo(inputInfo);
    if(videoWriter->SaveMP4()){
        std::cout << "Save complete\n";
    }
}

int main(int argn, char** args){
    auto scene = iGame::Scene::New();
    auto obj = iGame::FileIO::ReadFile("./Models/CAD11/_frames.pvd");
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
    SaveAnimationToMP4(scene, "./AnimationExample.mp4");
}