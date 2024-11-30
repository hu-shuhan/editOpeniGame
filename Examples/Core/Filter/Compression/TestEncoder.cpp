#include <iGameFileIO.h>
#include <iGameMeshCodec/iGameMeshEncoder.h>

int main(){
    // Read the file and add it to the scene
    const std::string fileName = "H:/iGameProject9/editOpeniGame/Examples/Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    auto encoder = iGame::MeshEncoder::New();
    encoder->SetSaveFilePath("H:/iGameProject9/editOpeniGame/Examples/Models/comp.igc");
    encoder->SetInput(dataObj);
    encoder->Execute();
}