// Source: dayuwan77/igamevis at eccac729b57aeacbe9312d7d5189f6990bb4eebd.
// Integration gap: e7ec6571 imported the filter but omitted its example.
// Preserve the numerical/attribute checks below and reject invalid inputs;
// visual examples support --no-render so CI requires a real exit status.
// Integration commit: test: add examples for first-batch standard filters
// Find it: git log --diff-filter=A --format="%h %s" -- Examples/Filter/FeatureExtraction/FeatureEdges.cpp
#include <FeatureExtraction/iGameFeatureEdgesFilter.h>

#include <iGameFileIO.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <iostream>
#include <string>

int main(
    int argc,
    char** argv) {
    if (argc != 2) {
        std::cerr
            << "Usage: testFeatureEdges.exe "
            << "<model-file>"
            << std::endl;
        return 1;
    }

    const std::string fileName =
        argv[1];

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

    std::cout
        << "Input mesh loaded successfully."
        << std::endl;

    auto surfaceInput =
        DynamicCast<iGame::SurfaceMesh>(
            input);

    if (surfaceInput == nullptr) {
        auto unstructuredInput =
            DynamicCast<iGame::UnstructuredMesh>(
                input);

        if (unstructuredInput != nullptr) {
            std::cerr
                << "Input mesh is an "
                << "UnstructuredMesh."
                << std::endl;

            std::cerr
                << "Please extract the surface mesh "
                << "first, then run this test."
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
        << "Input points: "
        << surfaceInput->GetNumberOfPoints()
        << std::endl;

    std::cout
        << "Input faces: "
        << surfaceInput->GetNumberOfFaces()
        << std::endl;

    if (surfaceInput->GetNumberOfPoints() == 0 ||
        surfaceInput->GetNumberOfFaces() == 0) {
        std::cerr
            << "Input surface mesh is empty."
            << std::endl;
        return 1;
    }

    auto filter =
        iGame::FeatureEdgesFilter::New();

    filter->SetInput(
        surfaceInput);

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
        DynamicCast<iGame::UnstructuredMesh>(
            filter->GetOutput());

    if (output == nullptr) {
        std::cerr
            << "FeatureEdgesFilter output is invalid."
            << std::endl;
        return 1;
    }

    const auto edgeCount =
        output->GetNumberOfCells();

    std::cout
        << "FeatureEdgesFilter finished."
        << std::endl;

    std::cout
        << "Output edge count: "
        << edgeCount
        << std::endl;

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

    if (edgeCount == 0) {
        std::cerr
            << "Unexpected output edge count. "
            << "Expected a positive count, got "
            << edgeCount
            << std::endl;
        return 1;
    }

    std::cout
        << "FeatureEdgesFilter test passed."
        << std::endl;

    return 0;
}
