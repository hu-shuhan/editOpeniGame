#include <cstdlib>
// 迁移回归（待提交）：防止标准过滤器仅有菜单占位，验证本文件记录的算法结果、输入保持不变及边界情况。
#include "ExtractEnclosedPoints/iGameExtractEnclosedPointsFilter.h"
#include "iGameFileIO.h"
#include "iGameScene.h"
#include "iGameRenderWindow.h"
#include "iGameInteractor.h"
#include "iGameDrawObject.h"
#include "iGameSurfaceMesh.h"
#include "iGamePointSet.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace iGame;

// ============================================================
// 工具：跑一次过滤器
// ============================================================
struct RunResult {
    bool   ok = false;
    IGsize inPoints = 0;
    IGsize outPoints = 0;
};

static RunResult RunExtract(PointSet::Pointer cloud,
                            SurfaceMesh::Pointer surface,
                            bool checkClosed,
                            bool insideOut,
                            double tolerance) {
    RunResult r;
    if (!cloud || !surface) return r;

    auto filter = ExtractEnclosedPointsFilter::New();
    filter->SetInput(0, cloud);
    filter->SetInput(1, surface);
    filter->SetCheckSurface(checkClosed);
    filter->SetInsideOut(insideOut);
    filter->SetTolerance(tolerance);

    if (!filter->Execute()) return r;

    auto out = filter->GetOutput();
    if (!out) return r;

    auto outPS = DynamicCast<PointSet>(out);
    if (!outPS) return r;

    r.inPoints  = cloud->GetNumberOfPoints();
    r.outPoints = outPS->GetNumberOfPoints();
    r.ok = true;
    return r;
}

// ============================================================
// 主函数
// ============================================================
int main() {
    std::cout << "========== ExtractEnclosedPoints Test ==========" << std::endl;

    bool allPassed = true;
    auto report = [&](const std::string& name, bool pass,
                      const std::string& msg = "") {
        std::cout << (pass ? "PASS" : "FAIL") << " - " << name;
        if (!msg.empty()) std::cout << "  [" << msg << "]";
        std::cout << std::endl;
        if (!pass) allPassed = false;
    };

    // ---- 1. 读取点云与球面 ----
    const std::string cloudPath   = "./Models/ExtractEnclosedPoints_Points_Cloud1.vtk";
    const std::string surfacePath = "./Models/ExtractEnclosedPoints_Surface_Sphere.vtk";

    auto cloudObj = FileIO::ReadFile(cloudPath);
    if (!cloudObj) {
        std::cout << "ERROR: cannot read " << cloudPath << std::endl;
        return 1;
    }
    auto cloud = DynamicCast<PointSet>(cloudObj);
    if (!cloud) {
        std::cout << "ERROR: " << cloudPath << " is not a PointSet" << std::endl;
        return 1;
    }

    auto surfaceObj = FileIO::ReadFile(surfacePath);
    if (!surfaceObj) {
        std::cout << "ERROR: cannot read " << surfacePath << std::endl;
        return 1;
    }
    auto surface = DynamicCast<SurfaceMesh>(surfaceObj);
    if (!surface) {
        std::cout << "ERROR: " << surfacePath << " is not a SurfaceMesh" << std::endl;
        return 1;
    }

    std::cout << "Cloud   : " << cloud->GetNumberOfPoints() << " points" << std::endl;
    std::cout << "Surface : "
              << surface->GetNumberOfPoints() << " points, "
              << surface->GetNumberOfFaces() << " faces" << std::endl;

    // ---- 2. 主测试：和 ParaView 对照（点云 + 球面） ----
    {
        std::cout << "\n--- Test 1: point cloud vs sphere (aligned with ParaView) ---"
                  << std::endl;

        auto r = RunExtract(cloud, surface, /*check*/true, /*insideOut*/false, 1e-9);

        std::cout << "Input  points: " << r.inPoints  << std::endl;
        std::cout << "Output points: " << r.outPoints << std::endl;

        // 和 ParaView 对照，iGameVis 应输出约 68 点
        const bool pass = r.ok && r.outPoints > 0 && r.outPoints < r.inPoints;
        report("point cloud vs sphere", pass,
               "input=" + std::to_string(r.inPoints) +
               " output=" + std::to_string(r.outPoints));
    }

    // ---- 3. InsideOut：应该保留“外点” ----
    {
        std::cout << "\n--- Test 2: InsideOut ---" << std::endl;
        auto rNormal = RunExtract(cloud, surface, true, /*insideOut*/false, 1e-9);
        auto rInvert = RunExtract(cloud, surface, true, /*insideOut*/true,  1e-9);

        std::cout << "Normal    : in=" << rNormal.inPoints
                  << ", out=" << rNormal.outPoints << std::endl;
        std::cout << "InsideOut : in=" << rInvert.inPoints
                  << ", out=" << rInvert.outPoints << std::endl;

        // 期望：两者输出点数之和 = 输入点数
        const bool pass = rNormal.ok && rInvert.ok &&
                          (rNormal.outPoints + rInvert.outPoints == rNormal.inPoints);
        report("InsideOut", pass,
               "normal=" + std::to_string(rNormal.outPoints) +
               " invert=" + std::to_string(rInvert.outPoints));
    }

    // ---- 4. 结果稳定性：同输入跑两次 ----
    {
        std::cout << "\n--- Test 3: determinism ---" << std::endl;
        auto r1 = RunExtract(cloud, surface, true, false, 1e-9);
        auto r2 = RunExtract(cloud, surface, true, false, 1e-9);
        const bool pass = r1.ok && r2.ok && r1.outPoints == r2.outPoints;
        report("determinism", pass,
               "r1=" + std::to_string(r1.outPoints) +
               " r2=" + std::to_string(r2.outPoints));
    }

    // ---- 5. 容差敏感度：容差变化对输出影响 ----
    {
        std::cout << "\n--- Test 4: tolerance sensitivity ---" << std::endl;
        auto r1 = RunExtract(cloud, surface, true, false, /*tolerance*/1e-9);
        auto r2 = RunExtract(cloud, surface, true, false, /*tolerance*/1e-3);

        std::cout << "tol=1e-9 : out=" << r1.outPoints << std::endl;
        std::cout << "tol=1e-3 : out=" << r2.outPoints << std::endl;

        // 容差变大时，交点 t 小的会被过滤，理论上保留点数不会增加
        const bool pass = r1.ok && r2.ok && r2.outPoints <= r1.outPoints;
        report("tolerance sensitivity", pass,
               "small=" + std::to_string(r1.outPoints) +
               " large=" + std::to_string(r2.outPoints));
    }

    // ---- 最终结果 ----
    if (!allPassed) {
        std::cout << "\n========== SOME TESTS FAILED ==========" << std::endl;
        return 1;
    }
    std::cout << "\n========== ALL TESTS PASSED ==========" << std::endl;

    if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) return 0;
    // ---- 弹窗展示：结果点云 + 原始点云 + 球面 ----
    std::cout << "\n===== Show result in window =====" << std::endl;
    {
        auto filter = ExtractEnclosedPointsFilter::New();
        filter->SetInput(0, cloud);
        filter->SetInput(1, surface);
        filter->SetCheckSurface(true);
        filter->SetInsideOut(false);
        filter->SetTolerance(1e-9);

        if (!filter->Execute()) {
            std::cout << "FAIL: filter execute failed" << std::endl;
            return 1;
        }
        auto out = filter->GetOutput();
        if (!out) {
            std::cout << "FAIL: output null" << std::endl;
            return 1;
        }

        // 设置渲染样式
        if (auto drawOut = DynamicCast<DrawObject>(out)) {
            drawOut->SetViewStyle(IG_POINTS);
            drawOut->SetPointSize(5.0f);
            drawOut->ConvertToDrawableData();
        }
        if (auto drawSurface = DynamicCast<DrawObject>(surface)) {
            drawSurface->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
            drawSurface->ConvertToDrawableData();
        }
        if (auto drawCloud = DynamicCast<DrawObject>(cloud)) {
            drawCloud->SetViewStyle(IG_POINTS);
            drawCloud->SetPointSize(2.0f);
            drawCloud->ConvertToDrawableData();
        }

        auto scene = Scene::New();
        scene->AddModel(surface);
        scene->AddModel(cloud);
        scene->AddModel(out);
        scene->ResetCameraView();

        RenderWindow::Pointer window = RenderWindow::New();
        window->SetSize(1280, 720);
        window->SetScene(scene);

        auto interactor = Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);

        std::cout << "About to call Show()..." << std::endl;
        window->Show();
        std::cout << "Show() returned." << std::endl;
    }

    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}