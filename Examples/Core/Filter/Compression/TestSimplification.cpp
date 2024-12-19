#include <iGameInteractor.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iGameRenderWindow.h>
#include <SurfaceMeshFilters/iGameSimplification.h>
#include <SurfaceMeshFilters/iGameTriangulation.h>

int main(){
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/mazewheel.obj";
    auto obj = iGame::FileIO::ReadFile(fileName);

    // If model is not triangle mesh, this will occur error
    auto filter = iGame::Simplification::New();
    filter->SetTargetReduction(0.5);
    filter->SetInput(obj);
    filter->Execute();

    scene->AddModel(obj);

    // Set up the render window
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    // Set up the interactor
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    // Start the render loop
    window->Show();
}