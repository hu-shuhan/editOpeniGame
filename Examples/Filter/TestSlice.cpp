#include <Slice/iGameSliceFilter.h>
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
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    iGame::UnstructuredMesh::Pointer mesh = DynamicCast<iGame::UnstructuredMesh>(obj);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }
    auto input = obj;
    //新建切割的filter
    auto filter = iGame::SliceFilter::New();
    //设置输入
    filter->SetInput(input);
    auto bound = input->GetBoundingBox();
    auto ori = (bound.min + bound.max) / 2;
    float n[3] = {0, 1, 0};
    float o[3] = {ori[0], ori[1], ori[2]};
    //设置切割的平面
    filter->SetPlane(o, n);
    //执行切割
    filter->Execute();
    //返回结果
    auto res = filter->GetOutput();
    (DynamicCast<iGame::DrawObject>(res))->SetViewStyle(IG_SURFACE);
    (DynamicCast<iGame::DrawObject>(res))->ViewCloudPicture(scene, 0);
    if (res != nullptr) { scene->AddModel(res); }
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