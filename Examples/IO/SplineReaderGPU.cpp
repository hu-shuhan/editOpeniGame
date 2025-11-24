//
// Created by Sumzeek on 11/26/2024.
//

#pragma once

#include "Spline XML/iGameSplineReaderGPU.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"

static void ImportSplineFileWithGpuCompute() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/Bridge.xml";

    // Note: Since XML files may have various formats, it is necessary to explicitly specify the Reader type based on the file content.
    iGame::SplineReaderGPU::Pointer reader = iGame::SplineReaderGPU::New();
    reader->SetFilePath(fileName);
    reader->SetSurfaceRenderForVolume(true); // surface rendering
    // reader->SetSurfaceRenderForVolume(false); // volume rendering
    reader->Execute();
    iGame::DataObject::Pointer dataObj = reader->GetOutput();

    if (dataObj != nullptr) {
        scene->AddModel(dataObj);
    } else {
        igError("Error reading the file");
    }

    // Change the display style to surface mode
    auto sg = DynamicCast<iGame::SplineGeometry>(dataObj);
    if (sg) {
        // Set the display style to combine wireframe and surface modes for the object
        sg->SetViewStyle(IG_WIREFRAME | IG_SURFACE);
        sg->SetSamples(0);
    } else {
        igError("Not a spline object"); // Error if the object is not drawable
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
    ImportSplineFileWithGpuCompute();
    return 0;
}
