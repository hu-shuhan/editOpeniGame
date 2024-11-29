#pragma once

#include "iGameFileIO.h"
#include "iGameMeshCodec/iGameMeshEncoder.h"

static void Encoder() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "../Examples/Models/Tet_Plane.vtk";
    auto dataObj = iGame::FileIO::ReadFile(fileName);
    
    auto encoder = iGame::MeshEncoder::New();
    encoder->SetInput(dataObj);
    encoder->SetSaveFilePath("../Examples/Models/comp");
    encoder->Execute();
}
