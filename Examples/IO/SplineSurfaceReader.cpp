//
// Created by Sumzeek on 11/26/2024.
//

#pragma once

#include "Spline XML/iGameSplineVolumeReader.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"

static void ImportSplineVolume() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/Bridge.xml";

    // Note: Since XML files may have various formats, it is necessary to explicitly specify the Reader type based on the file content.
    iGame::SplineVolumeReader::Pointer reader = iGame::SplineVolumeReader::New();
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
    ImportSplineVolume();
    return 0;
}
