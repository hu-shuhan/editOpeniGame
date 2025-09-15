#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void SetViewStyle() {
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

    // Change the display style
    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (drawObj) {
        //drawObj->SetViewStyle(IG_POINTS);                             // Point mode
        //drawObj->SetViewStyle(IG_WIREFRAME);                          // Wireframe mode
        //drawObj->SetViewStyle(IG_SURFACE);                            // Surface mode
        //drawObj->SetViewStyle(IG_POINTS | IG_WIREFRAME);              // Combined mode: Points + Wireframe
        //drawObj->SetViewStyle(IG_POINTS | IG_SURFACE);                // Combined mode: Points + Surface
        //drawObj->SetViewStyle(IG_WIREFRAME | IG_SURFACE);             // Combined mode: Wireframe + Surface
        drawObj->SetViewStyle(IG_POINTS | IG_WIREFRAME | IG_SURFACE); // Combined mode: Points + Wireframe + Surface
    } else {
        igError("Not a drawable object");
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

int main() { SetViewStyle(); }
