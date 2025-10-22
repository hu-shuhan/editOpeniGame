#include<Convert/iGameConvertToLagrangeUnstructuredMesh.h>
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
    const std::string fileName = "./Models/f6_res_ele.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    iGame::UnstructuredMesh::Pointer mesh = DynamicCast<iGame::UnstructuredMesh>(obj);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }
    auto input = obj;
    //新建转换为lagrange网格的filter
    auto filter = iGame::ConvertToLagrangeUnstructuredMesh::New();
    //设置输入
    filter->SetInput(input);
    //执行
    filter->Execute();
    //返回结果
    auto res = filter->GetOutput();
    if (res != nullptr) { scene->AddModel(res); }
    (DynamicCast<iGame::DrawObject>(res))->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
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