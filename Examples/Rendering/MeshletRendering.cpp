#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void MeshletRendering() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/mazewheel.obj";
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj != nullptr) {
        iGame::SmartPointer<iGame::Meshleter> meshleter = nullptr;
        if (dataObj->GetDataObjectType() == IG_SURFACE_MESH) {
            meshleter = iGame::SurfaceMeshMeshleter::New();
            meshleter->SetInput(dataObj);
            scene->AddModel(meshleter);
        } else {
            igError(std::format("Input is not surface mesh({})", dataObj->GetDataObjectType()));
        }
    } else {
        igError("Error reading the file");
    }

    // Change the display style to wireframe and surface mode
    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (drawObj) {
        // Set the display style to combine wireframe and surface modes for the object
        drawObj->SetViewStyle(IG_SURFACE); // Combined mode: Wireframe + Surface
    } else {
        igError("Not a drawable object"); // Error if the object is not drawable
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

int main() { MeshletRendering(); }
