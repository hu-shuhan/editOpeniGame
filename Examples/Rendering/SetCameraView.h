//
// Created by Sumzeek on 11/26/2024.
//

#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void SetCameraView() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "../Examples/Models/Tet_Plane.vtk";
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

    scene->LookAtPositiveX(); // Look at the scene from the positive X-axis direction
    //scene->LookAtNegativeX(); // Look at the scene from the negative X-axis direction
    //scene->LookAtPositiveY(); // Look at the scene from the positive Y-axis direction
    //scene->LookAtNegativeY(); // Look at the scene from the negative Y-axis direction
    //scene->LookAtPositiveZ(); // Look at the scene from the positive Z-axis direction
    //scene->LookAtNegativeZ(); // Look at the scene from the negative Z-axis direction
    //scene->LookAtIsometric(); // Set the camera to an isometric view (typically used for 3D models)

    scene->RotateNinetyClockwise(); // Rotate the camera view 90 degrees clockwise
    //scene->RotateNinetyCounterClockwise(); // Rotate the camera view 90 degrees counterclockwise

    // Set up the render window
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->setSize(1920, 1080);
    window->setScene(scene);

    // Set up the interactor
    auto basicInteractor = iGame::Interactor::New();
    basicInteractor->Initialize(scene);
    window->setInteractor(basicInteractor);

    // Start the render loop
    window->show();
}
