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
    const std::string fileName = "H:/iGameProject9/editOpeniGame/Examples/Models/dfs.vtk";
    auto dataObj = iGame::FileIO::ReadFile(fileName);

    // If model is triangle mesh
    auto triangulation = iGame::Triangulation::New();
    triangulation->SetInput(dataObj);
    triangulation->Execute();
    auto obj = triangulation->GetOutput();

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
    auto basicInteractor = iGame::Interactor::New();
    basicInteractor->Initialize(scene);
    window->SetInteractor(basicInteractor);

    // Start the render loop
    window->Show();
}