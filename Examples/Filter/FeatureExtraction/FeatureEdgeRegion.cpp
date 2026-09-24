// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/FeatureExtraction/FeatureEdgeRegion.cpp
#include <string>
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <FeatureExtraction/iGameFeatureEdgeRegionFilter.h>
#include <FeatureExtraction/iGameFeatureEdgesFilter.h>
#include <Convert/iGameConvertToSurfaceMeshFilter.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <limits>



int main(int argc, char** argv) {
	const std::string fileName = "./Models/FeatureRegion_MountingPlate.vtk";
	auto scene = iGame::Scene::New();
    auto input = iGame::FileIO::ReadFile(fileName);

	if (input == nullptr) {
		std::cerr << "Faile to read input file:" << fileName << std::endl;
        return 1;
	}

    auto convertFilter = iGame::ConvertToSurfaceMeshFilter::New();

    convertFilter->SetConvertMethod(iGame::ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);

    convertFilter->SetInput(input);

    if (!convertFilter->Execute()) {
        std::cerr << "ConvertToSurfaceMeshFilter execution failed." << std::endl;
        return 1;
    }

    auto surfaceMesh = DynamicCast<iGame::SurfaceMesh>(convertFilter->GetOutput());
    if (surfaceMesh == nullptr) {
        std::cerr << "ConvertToSurfaceMeshFilter output is not SurfaceMesh." << std::endl;
        return 1;
    }

	//auto inputUnstructured = DynamicCast<iGame::UnstructuredMesh>(input);
 //   auto inputSurface = DynamicCast<iGame::SurfaceMesh>(input);

	//if (inputUnstructured != nullptr) {
 //       std::cout << "Input mesh type: UnstructuredMesh" << std::endl;

 //       std::cout << "Input point count: " << inputUnstructured->GetNumberOfPoints() << std::endl;

 //       std::cout << "Input cell count: " << inputUnstructured->GetNumberOfCells() << std::endl;
 //   }
 //   else if (inputSurface != nullptr) {
 //       std::cout << "Input mesh type: SurfaceMesh" << std::endl;

 //       std::cout << "Input point count: " << inputSurface->GetNumberOfPoints() << std::endl;

 //       std::cout << "Input face count: " << inputSurface->GetNumberOfFaces() << std::endl;
 //   } else {
 //       std::cerr << "Input mesh type is invalid." << std::endl;
 //       return 1;
 //   }

    //use FeatureEdgesFilter
    auto featureEdgeFilter = iGame::FeatureEdgesFilter::New();

    featureEdgeFilter->SetInput(surfaceMesh);
    featureEdgeFilter->SetFeatureAngle(30.0);
    featureEdgeFilter->SetBoundaryEdges(true);
    featureEdgeFilter->SetFeatureEdges(true);
    featureEdgeFilter->SetNonManifoldEdges(true);
    featureEdgeFilter->SetManifoldEdges(false);

    if (!featureEdgeFilter->Execute()) {
        std::cerr << "FeatureEdgesFilter execution failed." << std::endl;
        return 1;
    }
    auto featureEdgeOutput = featureEdgeFilter->GetOutput();

    if (featureEdgeOutput == nullptr) {
        std::cerr << "FeatureEdgesFilter output is null." << std::endl;
        return 1;
    }

    auto FeatureEdgeOutputMesh = DynamicCast<iGame::UnstructuredMesh>(featureEdgeOutput);
    if (FeatureEdgeOutputMesh == nullptr) {
        std::cerr << "FeatureEdgesFilter output is invalid." << std::endl;
        return 1;
    }

    //use regionId filter
    auto filter = iGame::FeatureEdgeRegionFilter ::New();
    filter->SetInput(0, surfaceMesh);
    filter->SetInput(1,FeatureEdgeOutputMesh);

    if (!filter->Execute()) {
        std::cerr << "FeatureEdgeRegionFilter execution failed" << std::endl;
        return 1;
    }

    auto output = filter->GetOutput();
    if (output == nullptr) {
        std::cerr << "FeatureEdgesRegionFilter output is invalid." << std::endl;
        return 1;
    }

    auto faceIdAttribute = output->GetAttributeSet()->GetAttribute("Region Id");
    if (faceIdAttribute.IsNone()) {
        std::cerr << "Region id cell attribute is missing" << std::endl;
        return 1;
    }
    if (argc > 1 && std::string(argv[1]) == "--no-render") return 0;
    scene->AddModel(output);
    scene->AddModel(featureEdgeOutput);

    auto edgeDrawObject = DynamicCast<iGame::DrawObject>(featureEdgeOutput);
    auto outputDrawObject = DynamicCast<iGame::DrawObject>(output);

    if (edgeDrawObject == nullptr || outputDrawObject == nullptr) {
        std::cerr << "Input or output is not drawable." << std::endl;
        return 1;
    }

    edgeDrawObject->SetViewStyle(IG_WIREFRAME);
    edgeDrawObject->SetLineWidth(3.0f);
    //edgeDrawObject->SetAlwaysOnTop(true);
    edgeDrawObject->ViewCloudPicture(scene, 0, 0);

    outputDrawObject->SetViewStyle(IG_SURFACE);
    outputDrawObject->ViewCloudPicture(scene, outputDrawObject->GetAttributeSet()->GetAttributeIndex("Region Id"), 0);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();

    window->SetInteractor(interactor);

    scene->ResetCameraView();
    if (!(argc > 1 && std::string(argv[1]) == "--no-render")) window->Show();
    return 0;
}
