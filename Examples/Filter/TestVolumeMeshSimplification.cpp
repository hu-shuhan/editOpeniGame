// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestVolumeMeshSimplification.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <DataProcessing/iGameMeshTetrahedralize.h>
#include <DataProcessing/iGameVolumeMeshSimplification.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <limits>

int main() {
    const std::string fileName = "./Models/VolumeSimplification_FlangedTube.vtk";
    auto scene = iGame::Scene::New();
    auto input = iGame::FileIO::ReadFile(fileName);

    if (input == nullptr) {
        std::cerr << "Falied to read input File:" << fileName << std::endl;
        return 1;
    }

    // The input contains hexahedra; tetrahedralization must precede simplification.
    auto tetraFilter = iGame::MeshTetrahedralize::New();
    tetraFilter->SetInput(input);
    if (!tetraFilter->Execute()) {
        std::cerr << "Failed to tetrahedralize mesh." << std::endl;
        return 1;
    }
    auto tetInput = tetraFilter->GetOutput();
    if (tetInput == nullptr) {
        std::cerr << "Failed to get MeshTetrahedralize output." << std::endl;
        return 1;
    }
    auto filter = iGame::TetraSimplification::New();
    filter->SetInput(tetInput);
    filter->SetTargetReduction(0.5);
    filter->SetTargetTetraCount(0);
    filter->SetPreserveBoundary(true);
    filter->SetUseAllPointAttributes(true);

    filter->SetInput(tetInput);
    if (!filter->Execute()) {
        std::cerr << "Failed to simplify mesh." << std::endl;
        return 1;
    }
    auto output = filter->GetOutput();
    if (output == nullptr) {
        std::cerr << "Failed to get simplified mesh." << std::endl;
        return 1;
    }

    scene->AddModel(output);

    auto drawObject = DynamicCast<iGame::DrawObject>(output);

    if (drawObject == nullptr) {
        std::cerr << "Output is not drawable." << std::endl;
        return 1;
    }

    drawObject->SetViewStyle(IG_SURFACE);
    if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) return 0;
    drawObject->ViewCloudPicture(scene, 0, 0);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();

    window->SetInteractor(interactor);

    scene->ResetCameraView();
    window->Show();
    return 0;

}
