#include <iostream>
#include <Core/iGameScene.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>
#include <iGameFileIO.h>

int main() {
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj != nullptr) {
        scene->AddModel(obj);
    }
    else {
        std::cout << "Read ERROR!\n";
        return -1;
    }
    auto drawObj=DynamicCast<iGame::DrawObject>(obj);
    if (drawObj == nullptr) {
        std::cout << "Could not render this object\n";
        return -1;
    }
    auto mapper = drawObj->GetColorMapper();
    //云图展示，第一个参数为scene，第二个参数是云图数据所在的index，
    //第三个参数是展示数据的维度，-1表示长度，0表示第0维，默认为-1
    drawObj->ViewCloudPicture(scene, 0);
    //转化为渲染数据
    drawObj->ConvertToDrawableData();
    //自定义range
    mapper->SetRange(0.3, 0.6);
    //更新mapper的状态
    mapper->Modified();
    scene->Update();

    /* 启动窗口设置*/
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto basicInteractor = iGame::Interactor::New();
    basicInteractor->Initialize(scene);
    window->SetInteractor(basicInteractor);
    window->Show();

    return 0;
}