#include <iGameFileIO.h>
#include <iGameMeshCodec/iGameMeshEncoder.h>

#include <iGameInteractor.h>
#include <iGameMeshCodec/iGameMeshDecoder.h>
#include <iGameRenderWindow.h>

#include <iGameSmartPointer.h>
#include <iGameSurfaceMesh.h>

int main() {
    const std::string sourceFileName = "./Models/Tet_Plane.vtk";
    const std::string encodedFileName = "./Models/comp.igc";

    // encoder test
    iGame::DataObject::Pointer sourceDataObj = iGame::FileIO::ReadFile(sourceFileName);
    auto encoder = iGame::MeshEncoder::New();
    encoder->SetSaveFilePath(encodedFileName);
    encoder->SetDebugModeOn();
    encoder->SetInput(sourceDataObj);
    encoder->Execute();
}