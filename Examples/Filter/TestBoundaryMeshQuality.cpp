// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestBoundaryMeshQuality.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <BoundaryMeshQuality/iGameBoundaryMeshQualityFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameRenderWindow.h>
#include <iGameSurfaceMesh.h>
#include <iostream>

// 串行展示三个 BoundaryMeshQuality 指标：
//   1. DistanceFromCellCenterToFaceCenter
//   2. DistanceFromCellCenterToFacePlane
//   3. AngleFaceNormalAndCellCenterToFaceCenterVector
//
// 注意：BoundaryMeshQualityFilter 产生一个独立的 SurfaceMesh 输出节点，
// 不修改输入对象本身。测试应从 filter->GetOutput() 获取结果，
// 然后在那个输出 mesh 上调 ViewCloudPicture。

int main() {

    auto baseScene = iGame::Scene::New();
    const std::string fileName = "./Models/Boundary_Mesh_Quality_Test.vtk";
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj != nullptr) {
        baseScene->AddModel(dataObj);
    } else {
        std::cerr << "Read ERROR!\n";
        return -1;
    }

    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (!drawObj) {
        std::cerr << "Loaded data is not a DrawObject\n";
        return -1;
    }

    iGame::BoundaryMeshQualityFilter::Pointer filter =
        iGame::BoundaryMeshQualityFilter::New();

    struct MetricEntry {
        const char* title;
        iGame::BoundaryMeshQualityFilter::BoundaryMetric metric;
    };

    const MetricEntry metrics[] = {
        {"Metric 1/3: DistanceFromCellCenterToFaceCenter",
         iGame::BoundaryMeshQualityFilter::DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER},
        {"Metric 2/3: DistanceFromCellCenterToFacePlane",
         iGame::BoundaryMeshQualityFilter::DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE},
        {"Metric 3/3: AngleFaceNormalAndCellCenterToFaceCenterVector",
         iGame::BoundaryMeshQualityFilter::ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR},
    };

    for (int i = 0; i < 3; ++i) {
        std::cout << ">>> " << metrics[i].title << "\n";

        filter->SetBoundaryMetric(metrics[i].metric);
        filter->SetInput(drawObj);
        if (!filter->Execute()) {
            std::cerr << "Filter execute failed: " << filter->GetMessage() << "\n";
            return -1;
        }

        // 从 filter 输出中取出独立的 SurfaceMesh（包含边界面 + 对应属性）
        iGame::DataObject::Pointer outputObj = filter->GetOutput();
        auto outputMesh = DynamicCast<iGame::SurfaceMesh>(outputObj);
        if (!outputMesh) {
            std::cerr << "Filter output is not a SurfaceMesh\n";
            return -1;
        }

        auto outputDrawObj = DynamicCast<iGame::DrawObject>(outputMesh);
        if (!outputDrawObj) {
            std::cerr << "Output SurfaceMesh is not a DrawObject\n";
            return -1;
        }

        // 对输出 mesh 本身调 ConvertToDrawableData，使新增属性上色
        if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) continue;
        outputDrawObj->ConvertToDrawableData();

        // 属性 index 0 即为本轮计算出的指标数组（每个边界面对应一个元素）
        const int attrIndex = 0;

        // 每个窗口使用独立 Scene，避免共享状态相互污染
        auto scene = iGame::Scene::New();
        scene->AddModel(outputDrawObj);

        iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
        window->SetSize(1920, 1080);
        window->SetScene(scene);

        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);

        // 在输出 mesh 上切到属性 index=0（即本指标数组）
        outputDrawObj->ViewCloudPicture(scene, attrIndex, -1);

        // Show() 阻塞，直到用户关闭当前窗口
        window->Show();

        std::cout << "    window closed.\n";
    }

    std::cout << "All three boundary metrics displayed. Done.\n";
    return 0;
}
