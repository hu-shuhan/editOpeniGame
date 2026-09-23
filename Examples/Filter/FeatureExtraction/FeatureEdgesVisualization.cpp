#include <FeatureExtraction/iGameFeatureEdgesFilter.h>

#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <iostream>
#include <string>

int main() {
    const std::string fileName =
        "./Models/FeatureEdges_Cube.vtk";

    std::cout
        << "Input file: "
        << fileName
        << std::endl;

    auto input =
        iGame::FileIO::ReadFile(
            fileName);

    if (input == nullptr) {
        std::cerr
            << "Failed to read input file: "
            << fileName
            << std::endl;
        return 1;
    }

    auto inputSurface =
        DynamicCast<iGame::SurfaceMesh>(
            input);

    if (inputSurface == nullptr) {
        auto inputUnstructured =
            DynamicCast<iGame::UnstructuredMesh>(
                input);

        if (inputUnstructured != nullptr) {
            std::cerr
                << "Input mesh is an UnstructuredMesh."
                << std::endl;

            std::cerr
                << "Please extract the surface mesh "
                << "first, then run FeatureEdgesFilter."
                << std::endl;
        }
        else {
            std::cerr
                << "Input mesh type is invalid."
                << std::endl;
        }

        return 1;
    }

    std::cout
        << "Input mesh type: SurfaceMesh"
        << std::endl;

    std::cout
        << "Input point count: "
        << inputSurface->GetNumberOfPoints()
        << std::endl;

    std::cout
        << "Input face count: "
        << inputSurface->GetNumberOfFaces()
        << std::endl;

    if (inputSurface->GetNumberOfPoints() == 0 ||
        inputSurface->GetNumberOfFaces() == 0) {
        std::cerr
            << "Input surface mesh is empty."
            << std::endl;
        return 1;
    }

    auto filter =
        iGame::FeatureEdgesFilter::New();

    filter->SetInput(
        inputSurface);

    filter->SetFeatureAngle(
        30.0);

    filter->SetBoundaryEdges(
        true);

    filter->SetFeatureEdges(
        true);

    filter->SetNonManifoldEdges(
        true);

    filter->SetManifoldEdges(
        false);

    if (!filter->Execute()) {
        std::cerr
            << "FeatureEdgesFilter execution failed."
            << std::endl;
        return 1;
    }

    auto output =
        filter->GetOutput();

    if (output == nullptr) {
        std::cerr
            << "FeatureEdgesFilter output is null."
            << std::endl;
        return 1;
    }

    auto outputMesh =
        DynamicCast<iGame::UnstructuredMesh>(
            output);

    if (outputMesh == nullptr) {
        std::cerr
            << "FeatureEdgesFilter output is invalid."
            << std::endl;
        return 1;
    }

    std::cout
        << "Output point count: "
        << outputMesh->GetNumberOfPoints()
        << std::endl;

    std::cout
        << "Output cell count: "
        << outputMesh->GetNumberOfCells()
        << std::endl;

    if (outputMesh->GetNumberOfCells() == 0) {
        std::cerr
            << "No feature edges were extracted."
            << std::endl;
        return 1;
    }

    const int edgeTypeIndex =
        output->GetAttributeSet()
        ->GetAttributeIndex(
            "Edge Types");

    if (edgeTypeIndex < 0) {
        std::cerr
            << "Edge Types cell attribute is missing."
            << std::endl;
        return 1;
    }

    const int edgeIdsIndex =
        output->GetAttributeSet()
        ->GetAttributeIndex(
            "Edge Ids");

    if (edgeIdsIndex < 0) {
        std::cerr
            << "Edge Ids cell attribute is missing."
            << std::endl;
        return 1;
    }

    std::cout
        << "Edge Types attribute index: "
        << edgeTypeIndex
        << std::endl;

    std::cout
        << "Edge Ids attribute index: "
        << edgeIdsIndex
        << std::endl;

    auto scene =
        iGame::Scene::New();

    scene->AddModel(
        inputSurface);

    scene->AddModel(
        output);

    auto inputDrawObject =
        DynamicCast<iGame::DrawObject>(
            inputSurface);

    auto outputDrawObject =
        DynamicCast<iGame::DrawObject>(
            output);

    if (inputDrawObject == nullptr ||
        outputDrawObject == nullptr) {
        std::cerr
            << "Input or output is not drawable."
            << std::endl;
        return 1;
    }

    inputDrawObject->
        ConvertToDrawableData();

    inputDrawObject->SetViewStyle(
        IG_SURFACE);

    inputDrawObject->SetDefaultColor(
        igm::vec3{
            0.70f,
            0.70f,
            0.70f
        });

    inputDrawObject->SetTransparency(
        0.45f);

    outputDrawObject->
        ConvertToDrawableData();

    outputDrawObject->SetViewStyle(
        IG_WIREFRAME);

    outputDrawObject->SetLineWidth(
        4.0f);

    outputDrawObject->SetAlwaysOnTop(
        true);

    outputDrawObject->
        ViewCloudPicture(
            scene,
            edgeTypeIndex,
            0);

    auto window =
        iGame::RenderWindow::New();

    window->SetSize(
        1280,
        720);

    window->SetScene(
        scene);

    auto interactor =
        iGame::Interactor::New();

    interactor->Initialize(
        scene);

    interactor->CreateDefaultStyle();

    window->SetInteractor(
        interactor);

    scene->ResetCameraView();

    window->Show();

    return 0;
}
