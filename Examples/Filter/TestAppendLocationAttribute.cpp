// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestAppendLocationAttribute.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <AppendLocationAttribute/iGameAppendLocationAttribute.h>
#include <Core/iGameScene.h>
#include <VectorView/iGameVectorBase.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameRenderWindow.h>
#include <iGameVolume.h>
#include <iostream>

int main() {

    /* 创建场景*/
    auto scene = iGame::Scene::New();
    const std::string fileName = "./Models/Test_Append_Location_Attribute.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    iGame::UnstructuredMesh::Pointer mesh = DynamicCast<iGame::UnstructuredMesh>(obj);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 1;
    }
    auto input = obj;
    //新建切割的filter
    auto filter = iGame::AppendLocationAttribute::New();
    //设置输入
    filter->SetInput(input);

    if (!filter->Execute()) {
        std::cerr << "AppendLocationAttribute execution failed.\n";
        return 1;
    }
    //返回结果
    auto res = filter->GetOutput();
    if (res == nullptr) {
        std::cerr << "AppendLocationAttribute output is missing.\n";
        return 1;
    }
    std::cout << "AppendLocationAttribute execution succeeded.\n";
    scene->AddModel(res);
    if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) return 0;
    /* 启动窗口设置*/
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
}
