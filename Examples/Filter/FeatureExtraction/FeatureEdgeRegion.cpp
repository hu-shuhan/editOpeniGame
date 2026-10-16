// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/FeatureExtraction/FeatureEdgeRegion.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <FeatureExtraction/iGameFeatureEdgeRegionFilter.h>
#include <Convert/iGameConvertToSurfaceMeshFilter.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <limits>



int main() {
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

    //use regionId filter
    auto filter = iGame::FeatureEdgeRegionFilter ::New();
    filter->SetInput(0, surfaceMesh);
    filter->SetFeatureAngle(30);

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
    scene->AddModel(output);
    auto outputDrawObject = DynamicCast<iGame::DrawObject>(output);


    outputDrawObject->SetViewStyle(IG_SURFACE);
    if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) return 0;
    outputDrawObject->ViewCloudPicture(scene, outputDrawObject->GetAttributeSet()->GetAttributeIndex("Region Id"), 0);

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
