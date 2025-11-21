#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameRenderWindow.h>
#include <iostream>
int main() {
    /* init scene*/
    auto scene = iGame::Scene::New();
    /* Read the file Test and put it into the scene */
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    DynamicCast<iGame::DrawObject>(obj)->AddViewStyle(IG_WIREFRAME);
    if (obj != nullptr) {
        scene->AddModel(obj);
    } else {
        std::cout << "Read ERROR!\n";
    }

    /* Launch window Settings */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    /* Multi-window Test(There is a problem with temporary rendering) */
    iGame::RenderWindow::Pointer window_2 = iGame::RenderWindow::New();
    auto scene2 = iGame::Scene::New();
    window_2->SetScene(scene2);

    auto interactor2 = iGame::Interactor::New();
    interactor2->Initialize(scene);
    interactor2->CreateDefaultStyle();
    window_2->SetInteractor(interactor2);

    iGame::DataObject::Pointer obj2 = iGame::FileIO::ReadFile(".\\Models\\StreamTest.vtk");
    scene2->AddModel(obj2);

    /* Running multiple Windows simultaneously requires registration in the MultiRenderWindowManager  */
    iGame::MultiRenderWindowManager::Instance()->Register(window);
    iGame::MultiRenderWindowManager::Instance()->Register(window_2);
    iGame::MultiRenderWindowManager::Instance()->ShowAllRegisterWindow();

    return 0;
}
