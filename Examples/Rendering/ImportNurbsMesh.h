//
// Created by Sumzeek on 11/26/2024.
//

#pragma once

#include "Spline XML/iGameNurbsReader.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameNurbsGeometry.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

static void ImportNurbsMesh() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "../Examples/Models/circle.xml";
    //const std::string fileName = "../Examples/Models/surface.xml";
    //const std::string fileName = "../Examples/Models/hand.xml";

    // Note: Since XML files may have various formats, it is necessary to explicitly specify the Reader type based on the file content.
    NurbsReader::Pointer reader = NurbsReader::New();
    reader->SetFilePath(fileName);
    reader->Execute();
    iGame::DataObject::Pointer dataObj = reader->GetOutput();

    if (dataObj != nullptr) {
        scene->AddModel(dataObj);
    } else {
        igError("Error reading the file");
    }

    // Change the display style to surface mode
    auto nurbsObj = DynamicCast<iGame::NurbsGeometry>(dataObj);
    if (nurbsObj) {
        // Set the display style to combine wireframe and surface modes for the object
        nurbsObj->SetViewStyle(IG_WIREFRAME | IG_SURFACE); // Surface mode
        nurbsObj->SetSamples(10);
    } else {
        igError("Not a nurbs object"); // Error if the object is not drawable
    }

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
