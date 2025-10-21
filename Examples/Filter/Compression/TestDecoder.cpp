#include <IGC/iGameIGCReader.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameFileIO.h>
#include <string>

int main() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/comp.igc";

    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);

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