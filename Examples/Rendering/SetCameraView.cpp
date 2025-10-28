#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void SetCameraView() {
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

    // Change the display style to wireframe and surface mode
    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (drawObj) {
        // Set the display style to combine wireframe and surface modes for the object
        drawObj->SetViewStyle(IG_WIREFRAME | IG_SURFACE); // Combined mode: Wireframe + Surface
    } else {
        igError("Not a drawable object"); // Error if the object is not drawable
    }

    // Change the camera view angle to various predefined perspectives
    // The following functions adjust the camera's view to different orientations:

    // scene->ResetCameraViewToPositiveX(); // Look at the scene from the positive X-axis direction
    // scene->ResetCameraViewToNegativeX(); // Look at the scene from the negative X-axis direction
    // scene->ResetCameraViewToPositiveY(); // Look at the scene from the positive Y-axis direction
    // scene->ResetCameraViewToNegativeY(); // Look at the scene from the negative Y-axis direction
    // scene->ResetCameraViewToPositiveZ(); // Look at the scene from the positive Z-axis direction
    scene->ResetCameraViewToNegativeZ(); // Look at the scene from the negative Z-axis direction
    // scene->ResetCameraViewToIsometric(); // Set the camera to an isometric view (typically used for 3D models)

    scene->RotateNinetyClockwise(); // Rotate the camera view 90 degrees clockwise
    // scene->RotateNinetyCounterClockwise(); // Rotate the camera view 90 degrees counterclockwise

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
    SetCameraView();
    return 0;
}
