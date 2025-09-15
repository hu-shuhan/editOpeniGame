#pragma once

#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void SetScalarField() {
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

        // Visualize the object as a point cloud with the specified settings
        drawObj->ViewCloudPicture(scene, 1, -1); // Render the point cloud with the given parameters
        auto attributes = drawObj->GetAttributeSet()->GetAllAttributes();
        for (int i = 0; i < attributes->GetNumberOfElements(); i++) {
            std::cout << "scalar " << i << "======\n";
            std::cout << attributes->GetElement(i).pointer->GetName() << '\n';
            std::cout << attributes->GetElement(i).type << '\n';
            std::cout << attributes->GetElement(i).attachmentType << '\n';
            std::cout << attributes->GetElement(i).pointer->GetDimension() << '\n';
            std::cout << attributes->GetElement(i).GetDataRange()->GetValue(0) << ' '
                      << attributes->GetElement(i).GetDataRange()->GetValue(1) << '\n';
        }
    } else {
        igError("Not a drawable object"); // Error if the object is not drawable
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
    SetScalarField();
    return 0;
}
