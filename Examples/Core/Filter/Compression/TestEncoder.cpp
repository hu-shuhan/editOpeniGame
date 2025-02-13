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

    encoder->PointQuantizedMode = iGame::QuantMode::Float;
    encoder->m_PointQuantizedBits = 16;
    encoder->m_AttrbQuantMode = iGame::QuantMode::Float;
    encoder->m_AttrbQuantizedBits = 16;
    encoder->m_errorStaMode = iGame::ErrorStatusMode::MAPE;
    std::vector<std::string> errorSta;
    encoder->m_errorStaResult = &errorSta;

    encoder->SetSaveFilePath(encodedFileName);
    encoder->SetInput(sourceDataObj);
    encoder->Execute();
}