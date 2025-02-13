#include <iGameFileIO.h>
#include <iGameMeshCodec/iGameMeshEncoder.h>

<<<<<<< HEAD
int main(){
    // Read the file and add it to the scene
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    auto encoder = iGame::MeshEncoder::New();
    encoder->SetSaveFilePath("./Models/comp.igc");
    encoder->SetInput(dataObj);
=======
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
>>>>>>> 4acf973b5f55bccdf339da3b7adc38a3cb315277
    encoder->Execute();
}