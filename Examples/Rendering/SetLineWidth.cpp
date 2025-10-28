#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void SetLineWidth() {
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

    // Set the display style and line width for the object
    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (drawObj) {
        // Set the rendering style to combine wireframe and surface display
        drawObj->SetViewStyle(IG_WIREFRAME | IG_SURFACE);
        // Set the line width for wireframe rendering
        drawObj->SetLineWidth(10); // Set the line width to 10 for wireframe lines
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
    SetLineWidth();
    return 0;
}
