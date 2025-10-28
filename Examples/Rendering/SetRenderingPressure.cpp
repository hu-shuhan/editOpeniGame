#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void SetRenderingPressure() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj != nullptr) {
        scene->AddModel(dataObj);
    } else {
        igError("Error reading the file");
    }

    scene->ResetCameraView();
    scene->SetGpuUsageLimit(0.1f); // Set GPU usage limit to 10%
    scene->SetTargetFps(30.0f);    // Set target FPS to 30

    // Set the display style and point size for the object
    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (drawObj) {
        drawObj->SetViewStyle(IG_SURFACE);
    } else {
        igError("The object is not drawable");
    }

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

int main() {
    SetRenderingPressure();
    return 0;
}
