//
// Created by Sumzeek on 11/26/2024.
//

#pragma once

#include "Spline XML/iGameSplineSurfaceReader.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"

static void ImportSplineSurface() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/Bridge.xml";

    // Note: Since XML files may have various formats, it is necessary to explicitly specify the Reader type based on the file content.
    iGame::SplineSurfaceReader::Pointer reader = iGame::SplineSurfaceReader::New();
    reader->SetFilePath(fileName);
    reader->Execute();
    iGame::DataObject::Pointer dataObj = reader->GetOutput();

    if (dataObj != nullptr) {
        scene->AddModel(dataObj);
    } else {
        igError("Error reading the file");
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
    ImportSplineSurface();
    return 0;
}
