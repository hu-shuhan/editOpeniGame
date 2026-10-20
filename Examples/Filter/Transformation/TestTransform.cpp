// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
// 回归恢复（待提交）：恢复旧可视化示例时覆盖了轴心、点云和负缩放的数值检查。
// 保留真实模型渲染，并先执行原有四组检查及真实模型逐点坐标检查；失败返回非 0。
// --check / --no-render 跳过渲染窗口，便于自动回归；默认检查通过后显示结果。
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <filesystem>
#include <string>

// 通用几何变换（Transform）用例：
//   ① 轴心 pivot：绕自身中心旋转时包围盒中心不变；不设轴心时绕坐标原点旋转
//   ② 点云（IG_POINT_SET）输入也可以变换
//   ③ 负缩放（镜像）：绕轴心镜像后包围盒不变，点翻到对侧
// 输入用代码构造的立方体点云（中心 (10,20,30)，边长 (2,4,6)），不依赖外部模型文件。
#include <Transformation/iGameTransformFilter.h>

#include <iGameBoundingBox.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>

#include <cmath>
#include <iostream>

namespace {

bool Check(bool ok, const char* what) {
    if (!ok) { std::cerr << "FAILED: " << what << std::endl; }
    return ok;
}

bool Nearly(double a, double b, double eps = 1.0e-4) { return std::abs(a - b) < eps; }

iGame::PointSet::Pointer MakeCubePointSet() {
    auto pointSet = iGame::PointSet::New();
    auto points = iGame::Points::New();
    points->Resize(8);

    int index = 0;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                points->SetPoint(index++,
                                 10.0f + (i ? 1.0f : -1.0f),
                                 20.0f + (j ? 2.0f : -2.0f),
                                 30.0f + (k ? 3.0f : -3.0f));
            }
        }
    }

    pointSet->SetPoints(points);
    pointSet->SetName("Cube");
    return pointSet;
}

bool GetCenter(const iGame::DataObject::Pointer& object, double center[3]) {
    if (object == nullptr) { return false; }
    const iGame::BoundingBox& bbox = object->GetBoundingBox();
    for (int i = 0; i < 3; ++i) { center[i] = 0.5 * (bbox.min[i] + bbox.max[i]); }
    return true;
}

bool GetLength(const iGame::DataObject::Pointer& object, double length[3]) {
    if (object == nullptr) { return false; }
    const iGame::BoundingBox& bbox = object->GetBoundingBox();
    for (int i = 0; i < 3; ++i) { length[i] = bbox.max[i] - bbox.min[i]; }
    return true;
}

// ③ 点云输入 + 恒等变换
bool TestPointSetSupport() {
    auto input = MakeCubePointSet();
    auto filter = iGame::TransformFilter::New();
    filter->SetInput(input);
    if (!Check(filter->Execute(), "point set input should be transformed")) { return false; }

    auto output = filter->GetOutput();
    double center[3]{};
    if (!Check(GetCenter(output, center), "output should have a bounding box")) { return false; }

    const bool sameType = Check(output->GetDataObjectType() == IG_POINT_SET,
                                "point set output should stay a point set");
    const bool unchanged = Check(Nearly(center[0], 10.0) && Nearly(center[1], 20.0) && Nearly(center[2], 30.0),
                                 "identity transform should keep the point set unchanged");
    return sameType && unchanged;
}

// ① 轴心：绕自身中心旋转（中心不动），并与"绕原点旋转"对照
bool TestPivot() {
    auto input = MakeCubePointSet();

    auto withPivot = iGame::TransformFilter::New();
    withPivot->SetInput(input);
    withPivot->SetPivot(10.0f, 20.0f, 30.0f);
    withPivot->SetRotation(0.0f, 0.0f, 90.0f);
    if (!Check(withPivot->Execute(), "rotation with pivot should succeed")) { return false; }

    double center[3]{};
    double length[3]{};
    GetCenter(withPivot->GetOutput(), center);
    GetLength(withPivot->GetOutput(), length);
    const bool pivotOk = Check(Nearly(center[0], 10.0) && Nearly(center[1], 20.0) && Nearly(center[2], 30.0),
                               "rotation with pivot should keep the bounding box center")
                      && Check(Nearly(length[0], 4.0) && Nearly(length[1], 2.0) && Nearly(length[2], 6.0),
                               "rotation with pivot should swap the x/y extents");

    // 不设轴心（默认原点）：中心 (10,20,30) 旋转 90° 后应落到 (-20,10,30)
    auto withoutPivot = iGame::TransformFilter::New();
    withoutPivot->SetInput(input);
    withoutPivot->SetRotation(0.0f, 0.0f, 90.0f);
    if (!Check(withoutPivot->Execute(), "rotation without pivot should succeed")) { return false; }
    GetCenter(withoutPivot->GetOutput(), center);
    const bool originOk = Check(Nearly(center[0], -20.0) && Nearly(center[1], 10.0) && Nearly(center[2], 30.0),
                                "without pivot the rotation is about the world origin");

    return pivotOk && originOk;
}

// 向后兼容：默认轴心即为原点
bool TestDefaultPivotIsOrigin() {
    auto input = MakeCubePointSet();

    auto implicitOrigin = iGame::TransformFilter::New();
    implicitOrigin->SetInput(input);
    implicitOrigin->SetTranslation(3.0f, 0.0f, 0.0f);
    implicitOrigin->SetRotation(0.0f, 0.0f, 90.0f);
    if (!Check(implicitOrigin->Execute(), "default pivot transform should succeed")) { return false; }

    auto explicitOrigin = iGame::TransformFilter::New();
    explicitOrigin->SetInput(input);
    explicitOrigin->SetPivot(0.0f, 0.0f, 0.0f);
    explicitOrigin->SetTranslation(3.0f, 0.0f, 0.0f);
    explicitOrigin->SetRotation(0.0f, 0.0f, 90.0f);
    if (!Check(explicitOrigin->Execute(), "explicit origin pivot transform should succeed")) { return false; }

    double centerA[3]{};
    double centerB[3]{};
    GetCenter(implicitOrigin->GetOutput(), centerA);
    GetCenter(explicitOrigin->GetOutput(), centerB);

    const bool same = Check(Nearly(centerA[0], centerB[0]) && Nearly(centerA[1], centerB[1]) &&
                                    Nearly(centerA[2], centerB[2]),
                            "default pivot should behave like pivot at the origin");
    const bool expected = Check(Nearly(centerA[0], -17.0) && Nearly(centerA[1], 10.0) && Nearly(centerA[2], 30.0),
                                "rotation is about the origin when pivot is not set");
    return same && expected;
}

// ③ 负缩放（镜像）：绕中心镜像后包围盒不变，点应翻到对侧
bool TestNegativeScale() {
    auto input = MakeCubePointSet();

    auto filter = iGame::TransformFilter::New();
    filter->SetInput(input);
    filter->SetPivot(10.0f, 20.0f, 30.0f);
    filter->SetScale(-1.0f, 1.0f, 1.0f);
    if (!Check(filter->Execute(), "negative scale should succeed")) { return false; }

    auto output = filter->GetOutput();
    double center[3]{};
    GetCenter(output, center);
    const bool centerOk = Check(Nearly(center[0], 10.0) && Nearly(center[1], 20.0) && Nearly(center[2], 30.0),
                                "mirroring about the pivot should keep the bounding box center");

    auto outputPointSet = iGame::DynamicCast<iGame::PointSet>(output);
    if (!Check(outputPointSet.IsNotNull(), "output should be a point set")) { return false; }

    // 点 0 原本在 x = 9（中心左侧 1），镜像后应在 x = 11
    const iGame::Point& point = outputPointSet->GetPoint(0);
    const bool mirrored = Check(Nearly(point[0], 11.0),
                                "negative scale should mirror points across the pivot");
    return centerOk && mirrored;
}

} // namespace

bool RunRegressionChecks() {
    bool pass = true;
    pass = TestPointSetSupport() && pass;
    pass = TestPivot() && pass;
    pass = TestNegativeScale() && pass;
    pass = TestDefaultPivotIsOrigin() && pass;

    std::cout << "Transform (pivot / point set / mirror) : " << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

std::string FindVisualizationModel(const char* executable) {
    namespace fs = std::filesystem;
    const fs::path exeDir = fs::absolute(executable).parent_path();
    const fs::path candidates[] = {
        fs::path("Models/Transform_Complex.vtk"),
        fs::path("Examples/Models/Transform_Complex.vtk"),
        exeDir / "Models/Transform_Complex.vtk",
        exeDir.parent_path() / "Models/Transform_Complex.vtk",
        fs::path(__FILE__).parent_path().parent_path().parent_path() / "Models/Transform_Complex.vtk"
    };
    for (const auto& candidate : candidates) {
        if (fs::is_regular_file(candidate)) return candidate.string();
    }
    return "Models/Transform_Complex.vtk";
}

int main(int argc, char** argv)
{
    bool noRender = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--check" || arg == "--no-render") noRender = true;
        else { std::cerr << "Usage: testTransform [--check|--no-render]\n"; return 1; }
    }
    if (!RunRegressionChecks()) return 1;

    const std::string fileName = FindVisualizationModel(argv[0]);
    auto input = iGame::FileIO::ReadFile(fileName);

    if (input == nullptr) {
        std::cout << "Read ERROR!\n";
        return 1;
    }

    auto filter = iGame::TransformFilter::New();
    filter->SetInput(input);

    filter->SetTranslation(10.0f, 0.0f, 0.0f);
    filter->SetRotation(0.0f, 0.0f, 45.0f);
    filter->SetScale(1.5f, 1.5f, 1.5f);

    if (!filter->Execute()) {
        std::cout << "Transform ERROR!\n";
        return 1;
    }

    auto output = filter->GetOutput();
    if (output == nullptr) {
        std::cout << "Output ERROR!\n";
        return 1;
    }

    // Check every model point against the analytic T * Rz(45) * S(1.5) result.
    auto inPoints = iGame::DynamicCast<iGame::PointSet>(input);
    auto outPoints = iGame::DynamicCast<iGame::PointSet>(output);
    if (!inPoints || !outPoints || inPoints->GetNumberOfPoints() != outPoints->GetNumberOfPoints()) {
        std::cerr << "FAILED: transformed model point count\n";
        return 1;
    }
    const double factor = 1.5 / std::sqrt(2.0);
    for (IGsize i = 0; i < inPoints->GetNumberOfPoints(); ++i) {
        const auto& a = inPoints->GetPoint(i);
        const auto& b = outPoints->GetPoint(i);
        if (!Check(Nearly(b[0], 10.0 + factor * (a[0] - a[1])) &&
                   Nearly(b[1], factor * (a[0] + a[1])) && Nearly(b[2], 1.5 * a[2]),
                   "visualization model coordinates must match T * R * S")) return 1;
    }
    std::cout << "Transform_Complex model coordinates: PASS\n";
    if (noRender) return 0;

    auto scene = iGame::Scene::New();
    scene->AddModel(output);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();

    window->SetInteractor(interactor);
    window->Show();

    return 0;
}