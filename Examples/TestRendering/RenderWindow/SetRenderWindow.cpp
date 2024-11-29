#include <iostream>
#include <Core/iGameScene.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>
#include <iGameFileIO.h>
int main(){
    /* init scene*/
    auto scene = iGame::Scene::New();
    /* Read the file Test and put it into the scene */
    const std::string fileName = ".\\Models\\Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    DynamicCast<iGame::DrawObject>(obj)->AddViewStyle(IG_WIREFRAME);
    if(obj != nullptr){
        scene->AddModel(obj);
    } else {
        std::cout << "Read ERROR!\n";
    }

    /* Launch window Settings */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto basicInteractor = iGame::Interactor::New();
    basicInteractor->Initialize(scene);
    window->SetInteractor(basicInteractor);

    /* show single window */
    window->Show();

    return 0;
}
