// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Interpolation/TestPointVolumeInterpolator.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
// 点体积插值（PointVolumeInterpolator）GUI 示例：
// 读取一个散点云，按核函数把点属性插值到规则体网格并显示。
// 固定相对路径，无需手动输入，运行自动完成测试。
#include <Interpolation/iGamePointVolumeInterpolatorFilter.h>

#include <iGameFileIO.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameStructuredMesh.h>
#include <iGameType.h>

#include <iostream>
#include <string>

int main() {
    auto scene = iGame::Scene::New();

    // 读取散点云（POLYDATA + POINT_DATA）；读取失败时回退到 UNSTRUCTURED_GRID 版本
    std::string fileName = "./Models/AIGen_Points_ScatterCloud.vtk";
    auto dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj == nullptr) {
        fileName = "./Models/AIGen_Points_VertexCloud.vtk";
        dataObj = iGame::FileIO::ReadFile(fileName);
    }
    if (dataObj == nullptr) {
        igError("Error reading the file");
        return 0;
    }
    auto cloud = iGame::DynamicCast<iGame::PointSet>(dataObj);
    if (cloud == nullptr) {
        igError("Error: expected a point cloud (PointSet)");
        return 0;
    }
    scene->AddModel(dataObj);

    // 输入散点云：点样式、按 field 上色
    auto cloudDraw = iGame::DynamicCast<iGame::DrawObject>(dataObj);
    if (cloudDraw) {
        cloudDraw->SetViewStyle(IG_POINTS);
        cloudDraw->SetPointSize(3.0f);
        if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) cloudDraw->ViewCloudPicture(scene, 0, -1);
    }

    const auto& box = dataObj->GetBoundingBox();
    std::cout << "input cloud: " << cloud->GetNumberOfPoints() << " points\n";
    std::cout << "  bounding box: min(" << box.min[0] << ", " << box.min[1] << ", " << box.min[2]
              << ") max(" << box.max[0] << ", " << box.max[1] << ", " << box.max[2] << ")\n";

    // ==================== 点体积插值 ====================
    auto filter = iGame::PointVolumeInterpolatorFilter::New();
    filter->SetInput(cloud);
    filter->SetKernelType(iGame::PointKernelType::Gaussian);
    filter->SetKernelFootprint(iGame::PointKernelFootprint::Radius);
    filter->SetRadius(1.0);
    filter->SetSharpness(2.0);
    filter->SetNullPointsStrategy(iGame::PointNullPointsStrategy::NullValue);
    filter->SetNullValue(0.0);
    filter->SetUseInputBounds(true);
    filter->SetResolution(64, 64, 64);
    if (!filter->Execute()) {
        std::cerr << "PointVolumeInterpolator failed: " << filter->GetMessage() << "\n";
        return 1;
    }

    auto output = iGame::DynamicCast<iGame::StructuredMesh>(filter->GetOutput());
    if (output == nullptr) {
        std::cerr << "PointVolumeInterpolator has no structured output\n";
        return 1;
    }

    igIndex* dims = output->GetDimensionSize();
    IGsize hits = 0;
    if (auto attrSet = output->GetAttributeSet()) {
        const int maskIndex = attrSet->GetAttributeIndex(
                iGame::PointVolumeInterpolatorFilter::ValidPointsMaskName);
        if (maskIndex >= 0) {
            auto& maskAttr = attrSet->GetAttribute(maskIndex);
            if (maskAttr.pointer) {
                for (IGsize i = 0; i < maskAttr.pointer->GetNumberOfElements(); ++i) {
                    if (maskAttr.pointer->GetElementValue(i, 0) != 0.0) ++hits;
                }
            }
        }
    }
    std::cout << "output volume: " << dims[0] << " x " << dims[1] << " x " << dims[2]
              << ", points " << output->GetNumberOfPoints()
              << ", cells " << output->GetNumberOfCells()
              << ", valid " << hits << " / " << output->GetNumberOfPoints() << "\n";

    scene->AddModel(output);
    auto outDraw = iGame::DynamicCast<iGame::DrawObject>(output);
    if (outDraw) {
        outDraw->SetViewStyle(IG_SURFACE);
        if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) outDraw->ViewCloudPicture(scene, 0, -1); // 按插值后的 field 上色
    }

    if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) return 0;
    std::cout << std::flush;
    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
    return 0;
}
