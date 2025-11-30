#include <Convert/iGameConvertToVolumeMeshFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>

int main() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/StreamTest.vtk";

    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }

    auto filter = iGame::ConvertToVolumeMeshFilter::New();
    filter->SetInput(obj);

    if (filter->Execute()) { obj = filter->GetOutput(); }

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
