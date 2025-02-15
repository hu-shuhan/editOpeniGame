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
#include <iGameScene.h>
#include <iGameThreadPool.h>
#include <future>

void TestAnimation(int keyframe_idx){
    /* 创建场景*/
    auto scene = iGame::Scene::New();
//    const std::string fileName = "./Models/CAD11/_frames.pvd";
    const std::string fileName = "C:\\Users\\m_ky\\Desktop\\Resource\\xml\\pvd\\CAD11/_frames.pvd";
    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
    } else {
        scene->AddModel(obj);
    }
    /* Launch window Settings */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);



    using namespace iGame;
    auto currentFrame = obj->GetTimeFrames()->GetTargetTimeFrame(keyframe_idx);
    auto frameData = currentFrame.GetMetaData();
    auto currentDrawObject = DynamicCast<DrawObject>(
            obj);
    /* If the timeframe data store MultiSubFile's Path, the job is to Parse the sub File. */
    if(currentFrame.GetFrameType() == StreamingType::MultiSubFiles)
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
        currentDrawObject->ClearSubDataObject();
        for (auto& task: tasks) {
            task.get();
        }
        for(auto& subObj : results){
            currentDrawObject->AddSubDataObject(subObj);
        }
    }
    /* If obj has the deformation var and is enabled.
     * Make sure every timeStep have the deformation scale factor. */

    if(currentDrawObject->GetDeformationData()->GetEnableStatus()){
        StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
        deformFilter->SetInput(currentDrawObject);
        if(!deformFilter->Execute()) std::cout << " error \n";
    }


    /* process Object's scalar range*/
    currentDrawObject->UpdateSubDataObjectDataRange();
    currentDrawObject->SetViewStyle(currentDrawObject->GetViewStyle());

    if (currentDrawObject->GetAttributeIndex() != -1) {
        currentDrawObject->ViewCloudPicture(
                scene, currentDrawObject->GetAttributeIndex());
    }
    /* show single window */
    window->Show();

}


int main(int argc, char** args){
    TestAnimation(5);
}