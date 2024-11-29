//
// Created by Sumzeek on 11/26/2024.
//

#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void ResetCameraView() {
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

    // Reset the camera view based on the model's bounding sphere
    scene->ResetCameraView(); // Adjust the camera position and settings to focus on the model

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
