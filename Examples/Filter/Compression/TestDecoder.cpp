#include <iGameInteractor.h>
#include <iGameMeshCodec/iGameMeshLoomDecoder.h>
#include <iGameRenderWindow.h>

int main() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/comp.igc";
    
    auto decoder = new iGame::MeshLoomDecoder(fileName);
    iGame::DataObject::Pointer obj = decoder->Execute();
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