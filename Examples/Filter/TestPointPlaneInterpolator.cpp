// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
// 建议路径：Examples/Filter/PointPlaneInterpolator/TestPointPlaneInterpolator.cpp
// 运行测试：直接编译运行即可。如果不想弹窗渲染，可以在环境变量里设置 IGAME_EXAMPLE_NO_RENDER=1

#include <cstdlib>
#include <PointPlaneInterpolator/iGamePointPlaneInterpolatorFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGameUnstructuredMesh.h>
#include <iGameScene.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

constexpr const char* PointPlaneInterpolatorModelPath = "Models/analysis_vtu_subcase1.vtu";

bool Near(double lhs, double rhs, double tolerance = 1e-5) { return std::abs(lhs - rhs) <= tolerance; }

bool Check(bool condition, const std::string& message) {
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

iGame::PointSet::Pointer CreateSource() {
    std::cout << "Loading model: " << PointPlaneInterpolatorModelPath << '\n';
    auto dataObject = iGame::FileIO::ReadFile(PointPlaneInterpolatorModelPath);
    if (!dataObject) {
        std::cerr << "FATAL: Failed to load model. Please check if the file exists in Models/.\n";
        return nullptr;
    }
    return iGame::DynamicCast<iGame::PointSet>(dataObject);
}

// 1. 测试基础参数、面片拓扑结构以及 Voronoi 插值
bool TestParameterizedPlaneAndVoronoi() {
    std::cout << "Running parameterized-plane and Voronoi tests..." << std::endl;
    auto source = CreateSource();
    if (!Check(source != nullptr, "the PointPlaneInterpolator test model must load automatically")) { return false; }
    
    auto filter = iGame::PointPlaneInterpolatorFilter::New();
    filter->SetInput(source);
    
    auto bbox = source->GetBoundingBox();
    auto center = bbox.center();
    
    filter->SetOrigin(iGame::Point(center[0], center[1], center[2]));
    filter->SetNormal(iGame::Point(0.0, 0.0, 1.0)); // Z轴法向
    filter->SetResolution(4); // 4x4 网格
    filter->SetKernelType(iGame::PointPlaneInterpolatorFilter::VORONOI);
    
    if (!Check(filter->Execute(), "Voronoi execution")) return false;

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput()); 
    bool ok = Check(output != nullptr, "output exists");
    if (!ok) return false;

    // 验证拓扑：4x4 分辨率，点数 = 4*4=16，单元数(四边形) = 3*3=9
    ok &= Check(output->GetNumberOfPoints() == 16, "Resolution 4 creates 16 points");
    ok &= Check(output->GetNumberOfCells() == 9, "Resolution 4 creates 9 quad cells");

    // 验证属性传递（你的截图中显示有 StaticDisplacement 属性）
    auto& displacement = output->GetAttributeSet()->GetAttribute("StaticDisplacement");
    if (!displacement.IsNone()) {
        ok &= Check(displacement.pointer->GetDimension() == 3, "vector components are preserved");
    }

    // 测试改变分辨率和平面位置
    filter->SetOrigin(iGame::Point(center[0] + 10.0, center[1], center[2]));
    filter->SetNormal(iGame::Point(1.0, 0.0, 0.0)); // 沿 X 轴
    filter->SetResolution(2); // 2x2 网格
    ok &= Check(filter->Execute(), "repeated execution");
    
    auto output2 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    ok &= Check(output2->GetNumberOfPoints() == 4 && output2->GetNumberOfCells() == 1,
                "repeated execution rebuilds output with new resolution");

    return ok;
}

// 2. 测试高斯(Gaussian)和反距离(Shepard)权重
bool TestGaussianAndShepard() {
    std::cout << "Running Gaussian and Shepard tests..." << std::endl;
    auto source = CreateSource();
    if (!Check(source != nullptr, "model load")) { return false; }
    
    auto filter = iGame::PointPlaneInterpolatorFilter::New();
    filter->SetInput(source);
    
    auto bbox = source->GetBoundingBox();
    auto center = bbox.center();
    
    filter->SetOrigin(iGame::Point(center[0], center[1], center[2]));
    filter->SetNormal(iGame::Point(0.0, 1.0, 0.0));
    filter->SetResolution(2); 
    filter->SetKernelFootprint(iGame::PointPlaneInterpolatorFilter::RADIUS);
    filter->SetRadius(bbox.diag() / 4.0); // 根据模型大小动态设置半径

    filter->SetKernelType(iGame::PointPlaneInterpolatorFilter::GAUSSIAN);
    filter->SetSharpness(2.0);
    bool ok = Check(filter->Execute(), "Gaussian execution");
    
    filter->SetKernelType(iGame::PointPlaneInterpolatorFilter::SHEPARD);
    filter->SetPowerParameter(2.0);
    ok &= Check(filter->Execute(), "Shepard execution");
    
    return ok;
}

// 3. 测试空点策略以及非法参数拦截
bool TestNullPointStrategiesAndInvalidParameters() {
    std::cout << "Running null-point and validation tests..." << std::endl;
    auto source = CreateSource();
    if (!Check(source != nullptr, "model load")) { return false; }
    
    auto filter = iGame::PointPlaneInterpolatorFilter::New();
    filter->SetInput(source);
    
    // 故意把原点设在极远的地方，制造空邻域 (Null Points)
    filter->SetOrigin(iGame::Point(99999.0, 99999.0, 99999.0)); 
    filter->SetNormal(iGame::Point(0.0, 0.0, 1.0));
    filter->SetResolution(2);
    filter->SetKernelType(iGame::PointPlaneInterpolatorFilter::GAUSSIAN);
    filter->SetKernelFootprint(iGame::PointPlaneInterpolatorFilter::RADIUS);
    filter->SetRadius(1.0);
    
    // 策略1：Mask Points
    filter->SetNullPointsStrategy(iGame::PointPlaneInterpolatorFilter::MASK_POINTS);
    filter->SetNullValue(-7.0);
    bool ok = Check(filter->Execute(), "mask null-points execution");
    auto output = filter->GetOutput();
    auto mask = output->GetAttributeSet()->GetAttribute("vtkValidPointMask").pointer;
    ok &= Check(mask != nullptr && Near(mask->GetValue(0), 0.0), "invalid mask emitted");

    // 策略2：Null Value
    filter->SetNullPointsStrategy(iGame::PointPlaneInterpolatorFilter::NULL_VALUE);
    filter->SetNullValue(-9.0);
    ok &= Check(filter->Execute(), "null-value execution");

    // 策略3：Closest Point
    filter->SetNullPointsStrategy(iGame::PointPlaneInterpolatorFilter::CLOSEST_POINT);
    ok &= Check(filter->Execute(), "closest-point fallback execution");

    // 测试非法参数拦截
    filter->SetResolution(0); // 分辨率小于 2 应该失败
    ok &= Check(!filter->Execute(), "Resolution below 2 is rejected");

    filter->SetResolution(100);
    filter->SetRadius(0.0); // 半径 <= 0 应该失败
    ok &= Check(!filter->Execute(), "non-positive radius is rejected");
    
    filter->SetRadius(1.0);
    filter->SetNormal(iGame::Point(0.0, 0.0, 0.0)); // 法向为零向量应该失败
    ok &= Check(!filter->Execute(), "zero normal vector is rejected");

    return ok;
}

void VisualizeResult() {
    std::cout << "\n=== Visualization ===" << std::endl;
    auto dataObject = iGame::FileIO::ReadFile(PointPlaneInterpolatorModelPath);
    auto source = iGame::DynamicCast<iGame::PointSet>(dataObject);
    if (!source) { std::cerr << "Failed to load model.\n"; return; }

    auto bbox = source->GetBoundingBox();
    auto center = bbox.center();

    auto filter = iGame::PointPlaneInterpolatorFilter::New();
    filter->SetInput(source);
    filter->SetOrigin(iGame::Point(center[0], center[1], center[2]));
    filter->SetNormal(iGame::Point(0.0, 0.0, 1.0));
    filter->SetResolution(64);
    filter->SetKernelType(iGame::PointPlaneInterpolatorFilter::VORONOI);

    if (!filter->Execute()) {
        std::cerr << "Filter execution failed.\n";
        return;
    }

    auto planeOutput = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (!planeOutput) return;
    
    std::cout << "Interpolated plane: " << planeOutput->GetNumberOfPoints() << " points, "
              << planeOutput->GetNumberOfCells() << " quad cells" << std::endl;

    auto scene = iGame::Scene::New();

    // 渲染原始模型为半透明白色点云，方便对比
    auto srcDraw = iGame::DynamicCast<iGame::DrawObject>(source);
    if (srcDraw) {
        srcDraw->SetViewStyle(IG_POINTS);
        srcDraw->SetPointSize(2.0f);
        srcDraw->SetDefaultColor(igm::vec3{1.0f, 1.0f, 1.0f});
    }
    scene->AddModel(source);

    // 渲染生成的插值平面
    auto planeDraw = iGame::DynamicCast<iGame::DrawObject>(planeOutput);
    if (planeDraw) {
        planeDraw->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
        planeDraw->SetLineWidth(1.0f);
        planeDraw->SetLineColor(igm::vec3{0.0f, 1.0f, 1.0f});
        
        // 【关键修改 2】自动着色，找第一个点属性生成彩色云图，录视频效果极佳
        auto attrSet = planeOutput->GetAttributeSet();
        if (attrSet && attrSet->GetNumberOfAttributes() > 0) {
            for (int i = 0; i < attrSet->GetNumberOfAttributes(); ++i) {
                auto& attr = attrSet->GetAttribute(i);
                if (attr.pointer && attr.attachmentType == IG_POINT && attr.pointer->GetName() != "vtkValidPointMask") {
                    planeDraw->ViewCloudPicture(scene, i, -1);
                    break;
                }
            }
        }
    }
    scene->AddModel(planeOutput);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetTitle("PointPlaneInterpolator - analysis_vtu_subcase1 Test");
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    std::cout << "Render window opened. Close the window to exit." << std::endl;
    window->Show();
}
} // namespace

int main() {
    const bool ok = TestParameterizedPlaneAndVoronoi() && 
                    TestGaussianAndShepard() &&
                    TestNullPointStrategiesAndInvalidParameters();
                    
    if (!ok) return 1;
    
    std::cout << "PointPlaneInterpolator acceptance tests passed.\n";
    
    if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) {
        VisualizeResult();
    }
    return 0;
}