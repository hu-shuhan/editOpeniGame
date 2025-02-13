//
// Created by Sumzeek on 11/26/2024.
//

#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void SetVolumeRendering() {
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

    // Set the display style and render the first scalar data
    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (drawObj) {
        // Set the display style to surface mode
        drawObj->SetViewStyle(IG_SURFACE); // Display the object as a surface

        // Visualize the object as a point cloud with the specified settings
        drawObj->ViewCloudPicture(scene, 0, -1); // Render the point cloud with the given parameters
        drawObj->SetTransparency(0.2f);
    } else {
        igError("The object is not drawable"); // Error if the object is not drawable
    }

    // Change the scene rendering mode to volume rendering
    // Note: Volume rendering is currently suitable only for small-scale data and small screens.
    // Rendering larger datasets or using full-screen mode may cause rendering errors or performance issues.
    scene->SetVolumeRendering(true); // Enable volume rendering for 3D visualization

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
    SetVolumeRendering();
}