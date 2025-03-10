#include <iGameFileIO.h>
#include <iGameMeshCodec/iGameMeshLoomEncoder.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameSmartPointer.h>
#include <iGameSurfaceMesh.h>

int main() {
    const std::string sourceFileName = "./Models/Tet_Plane.vtk";
    const std::string encodedFileName = "./Models/comp.igc";

    // encoder test
    iGame::DataObject::Pointer sourceDataObj = iGame::FileIO::ReadFile(sourceFileName);
    
    iGame::UIControlParams params = iGame::MeshLoomEncoder::GenUiControlParams(sourceDataObj);
    
    auto encoder = new iGame::MeshLoomEncoder(encodedFileName, sourceDataObj, params);
    encoder->Execute();
}