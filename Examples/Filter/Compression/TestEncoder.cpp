#include <iGameFileIO.h>
#include <IGC/iGameIGCWriter.h>
#include <iGameRenderWindow.h>
#include <iGameSmartPointer.h>

int main() {
    const std::string sourceFileName = "./Models/Tet_Plane.vtk";
    const std::string encodedFileName = "./Models/comp.igc";

    // encoder test
    iGame::DataObject::Pointer sourceDataObj = iGame::FileIO::ReadFile(sourceFileName);
    
    auto writer = iGame::IGCWriter::New();
    writer->SetUIControlParams(iGame::MeshEncoder::GenUiControlParams(sourceDataObj));

    writer->WriteToFile(sourceDataObj, encodedFileName);

    return 0;
}