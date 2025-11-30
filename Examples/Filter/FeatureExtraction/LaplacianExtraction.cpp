#pragma once

#include "FeatureExtraction/iGameLaplacianFilter.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameUnstructuredMesh.h"

static void LaplacianExtract() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/Driver/driver-1.vtk";
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj != nullptr) {
        scene->AddModel(dataObj);
    } else {
        igError("Error reading the file");
    }

    // Change the display style to wireframe and surface mode
    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (!drawObj) {
        igError("Not a drawable object"); // Error if the object is not drawable
    }

    // Select a scalar to extract features
    drawObj->ViewCloudPicture(scene, 2);

    // Execute vortexFilter
    iGame::LaplacianFilter::Pointer filter = iGame::LaplacianFilter::New();
    filter->SetInput(drawObj);
    filter->Execute();

    // The extracted features are added to the original model as a scalar
    int d = drawObj->GetAttributeSet()->GetNumberOfAttributes();
    drawObj->ViewCloudPicture(scene, d - 1);

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

int main() { LaplacianExtract(); }
