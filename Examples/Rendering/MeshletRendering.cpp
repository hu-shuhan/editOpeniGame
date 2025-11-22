#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void MeshletRendering() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);

    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (dataObj != nullptr) {
        drawObj->SetAccelerationOption(true);
        scene->AddModel(dataObj);
    } else {
        igError("Error reading the file");
    }

    // Reset the camera view based on the model's bounding sphere
    scene->ResetCameraView(); // Adjust the camera position and settings to focus on the model
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
    MeshletRendering();
    return 0;
}
