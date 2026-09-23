#include <FeatureExtraction/iGameFeatureEdgesFilter.h>

#include <iGameFileIO.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <cstddef>
#include <iostream>
#include <string>

namespace {

struct FeatureEdgesTestCase {
    const char* name;
    const char* fileName;
    std::size_t expectedEdgeCount;
};

bool RunFeatureEdgesTest(
    const FeatureEdgesTestCase& testCase) {
    const std::string fileName =
        testCase.fileName;

    std::cout
        << "Running test: "
        << testCase.name
        << std::endl;

    auto input =
        iGame::FileIO::ReadFile(
            fileName);

    if (input == nullptr) {
        std::cerr
            << "Failed to read input file: "
            << fileName
            << std::endl;
        return false;
    }

    auto surfaceInput =
        DynamicCast<iGame::SurfaceMesh>(
            input);

    if (surfaceInput == nullptr) {
        auto unstructuredInput =
            DynamicCast<iGame::UnstructuredMesh>(
                input);

        if (unstructuredInput != nullptr) {
            std::cerr
                << "Input mesh is an UnstructuredMesh."
                << std::endl;
            std::cerr
                << "Please extract the surface mesh first."
                << std::endl;
        }
        else {
            std::cerr
                << "Input mesh type is invalid."
                << std::endl;
        }

        return false;
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
        return false;
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
        return false;
    }

    auto output =
        DynamicCast<iGame::UnstructuredMesh>(
            filter->GetOutput());

    if (output == nullptr) {
        std::cerr
            << "FeatureEdgesFilter output is invalid."
            << std::endl;
        return false;
    }

    const auto edgeCount =
        output->GetNumberOfCells();

    std::cout
        << "Output edge count: "
        << edgeCount
        << std::endl;

    if (edgeCount != testCase.expectedEdgeCount) {
        std::cerr
            << "Unexpected output edge count. Expected "
            << testCase.expectedEdgeCount
            << ", got "
            << edgeCount
            << std::endl;
        return false;
    }

    const int edgeTypeIndex =
        output->GetAttributeSet()
        ->GetAttributeIndex(
            "Edge Types");

    if (edgeTypeIndex < 0) {
        std::cerr
            << "Edge Types cell attribute is missing."
            << std::endl;
        return false;
    }

    const int edgeIdsIndex =
        output->GetAttributeSet()
        ->GetAttributeIndex(
            "Edge Ids");

    if (edgeIdsIndex < 0) {
        std::cerr
            << "Edge Ids cell attribute is missing."
            << std::endl;
        return false;
    }

    std::cout
        << "Edge Types attribute index: "
        << edgeTypeIndex
        << std::endl;

    std::cout
        << "Edge Ids attribute index: "
        << edgeIdsIndex
        << std::endl;

    std::cout
        << "FeatureEdgesFilter test passed for "
        << testCase.name
        << "."
        << std::endl;

    return true;
}

}  // namespace

int main() {
    const FeatureEdgesTestCase testCases[] = {
        {
            "FeatureEdges_Cube",
            "./Models/FeatureEdges_Cube.vtk",
            12
        },
        {
            "FeatureEdges_NonManifold",
            "./Models/FeatureEdges_NonManifold.vtk",
            7
        }
    };

    for (const auto& testCase : testCases) {
        if (!RunFeatureEdgesTest(testCase)) {
            std::cerr
                << "FeatureEdgesFilter test failed for "
                << testCase.name
                << "."
                << std::endl;
            return 1;
        }
    }

    std::cout
        << "ALL FEATURE EDGES TESTS PASSED"
        << std::endl;

    return 0;
}
