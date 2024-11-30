//
// Created by Sumzeek on 11/26/2024.
//

#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void SetPointSize() {
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

    // Set the display style and point size for the object
    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (drawObj) {
        // Set the view style to combine points, wireframe, and surface rendering
        drawObj->SetViewStyle(IG_POINTS); // Enable points rendering

        // Set the point size for point rendering
        drawObj->SetPointSize(5); // Set the point size to 5 for point-based rendering
    } else {
        igError("The object is not drawable");
    }

    // Set up the render window
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    // Set up the interactor
    auto basicInteractor = iGame::Interactor::New();
    basicInteractor->Initialize(scene);
    window->SetInteractor(basicInteractor);

    // Start the render loop
    window->Show();
}

int main() {
    SetPointSize();
}
