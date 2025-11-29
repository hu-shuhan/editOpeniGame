#include <IGC/iGameIGCWriter.h>
#include <iGameFileIO.h>
#include <iGameRenderWindow.h>
#include <iGameSmartPointer.h>
#include <string>

int main() {
    const std::string sourceFileName = "./Models/Quad_Plane_Tensor.vtk";
    const std::string encodedFileName = "./Models/comp.igc";

    // encoder test
    iGame::DataObject::Pointer sourceDataObj = iGame::FileIO::ReadFile(sourceFileName);

    auto writer = iGame::IGCWriter::New();
    writer->SetUIControlParams(iGame::MeshEncoderFilter::GenUiControlParams(sourceDataObj));

    writer->WriteToFile(sourceDataObj, encodedFileName);

    return 0;
}
