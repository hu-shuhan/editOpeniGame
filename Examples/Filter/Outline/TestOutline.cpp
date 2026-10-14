// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/Outline/TestOutline.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
#include <Outline/iGameOutlineFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <string>
#include <utility>

namespace {

bool Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

bool NearlyEqual(double lhs, double rhs) {
    return std::abs(lhs - rhs) <= 1.0e-5 * std::max(1.0, std::abs(rhs));
}

bool CheckOutline(iGame::UnstructuredMesh* output, const iGame::BoundingBox& bounds) {
    if (!Check(output != nullptr, "output is UnstructuredMesh")) return false;
    if (!Check(output->GetNumberOfPoints() == 8, "outline has 8 points")) return false;
    if (!Check(output->GetNumberOfCells() == 12, "outline has 12 cells")) return false;

    // 检查八种角点组合
    std::set<int> corners;
    for (IGsize i = 0; i < 8; ++i) {
        const auto& point = output->GetPoint(i);
        int corner = 0;
        for (int axis = 0; axis < 3; ++axis) {
            const bool atMin = NearlyEqual(point[axis], bounds.min[axis]);
            const bool atMax = NearlyEqual(point[axis], bounds.max[axis]);
            if (!Check(atMin || atMax, "point lies on a bounding-box corner")) return false;
            if (atMax) corner |= 1 << axis;
        }
        corners.insert(corner);
    }
    const bool fullBox = bounds.max[0] > bounds.min[0] &&
                         bounds.max[1] > bounds.min[1] &&
                         bounds.max[2] > bounds.min[2];
    if (fullBox && !Check(corners.size() == 8, "all 8 corners are present")) return false;

    // 检查边的唯一性、方向和连接数
    std::set<std::pair<igIndex, igIndex>> edges;
    int degree[8]{};
    for (IGsize i = 0; i < 12; ++i) {
        if (!Check(output->GetCellType(i) == iGame::IG_LINE, "cell type is IG_LINE")) return false;
        const igIndex* ids = nullptr;
        if (!Check(output->GetCellPointIds(i, ids) == 2, "line has 2 endpoints")) return false;
        if (!Check(ids[0] >= 0 && ids[0] < 8 && ids[1] >= 0 && ids[1] < 8 && ids[0] != ids[1],
                   "endpoint indices are valid")) return false;
        if (!Check(edges.emplace(std::min(ids[0], ids[1]), std::max(ids[0], ids[1])).second,
                   "line is unique")) return false;
        ++degree[ids[0]];
        ++degree[ids[1]];
        int changedAxes = 0;
        for (int axis = 0; axis < 3; ++axis) {
            if (!NearlyEqual(output->GetPoint(ids[0])[axis], output->GetPoint(ids[1])[axis])) {
                ++changedAxes;
            }
        }
        if (!Check(fullBox ? changedAxes == 1 : changedAxes <= 1,
                   "line follows a bounding-box edge")) return false;
    }
    for (int count : degree) {
        if (!Check(count == 3, "each corner joins 3 edges")) return false;
    }
    return true;
}

bool TestEmptyAndDegenerateInput() {
    using namespace iGame;

    auto filter = OutlineFilter::New();
    if (!Check(!filter->Execute() && !filter->GetOutput(), "missing input fails")) return false;

    auto input = PointSet::New();
    input->AddPoint(Point(-1.0, -2.0, 0.0));
    input->AddPoint(Point(3.0, 4.0, 0.0));
    filter->SetInput(input);
    if (!Check(filter->Execute(), "planar input executes")) return false;
    auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    if (!CheckOutline(output, input->GetBoundingBox())) return false;

    auto point = PointSet::New();
    point->AddPoint(Point(1.0, 2.0, 3.0));
    filter->SetInput(point);
    if (!Check(filter->Execute(), "single-point input executes")) return false;
    output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    if (!CheckOutline(output, point->GetBoundingBox())) return false;

    filter->SetInput(PointSet::New());
    return Check(!filter->Execute() && !filter->GetOutput(), "empty input clears previous output");
}

}

int main(int argc, char* argv[]) {
    using namespace iGame;

    const bool checkOnly = argc == 2 && std::string(argv[1]) == "--check";
    if (argc > 1 && !checkOnly) {
        std::cerr << "Usage: testOutline [--check]\n";
        return 1;
    }
    if (!TestEmptyAndDegenerateInput()) return 1;

    auto object = FileIO::ReadFile("Models/Quad_Bicycle.vtk");
    auto input = DynamicCast<UnstructuredMesh>(object);
    if (!Check(input != nullptr && input->GetNumberOfPoints() > 0, "bicycle model loads")) return 1;

    const auto bounds = input->GetBoundingBox();
    const auto pointCount = input->GetNumberOfPoints();
    const auto cellCount = input->GetNumberOfCells();
    const auto firstPoint = input->GetPoint(0);
    auto filter = OutlineFilter::New();
    filter->SetInput(input);
    if (!filter->Execute()) {
        std::cerr << "FAILED: " << filter->GetMessage() << '\n';
        return 1;
    }
    auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    if (!CheckOutline(output, bounds)) return 1;
    if (!Check(output != input && output->GetPoints() != input->GetPoints() &&
               output->GetCells() != input->GetCells(), "output owns independent geometry")) return 1;
    if (!Check(input->GetNumberOfPoints() == pointCount && input->GetNumberOfCells() == cellCount &&
               input->GetPoint(0) == firstPoint && input->GetBoundingBox() == bounds,
               "input geometry is preserved")) return 1;

    std::cout << "Outline checks passed: Quad_Bicycle.vtk, 8 points, 12 lines.\n";
    if (checkOnly) return 0;

    // 同时显示自行车和包围盒
    input->SetViewStyle(IG_SURFACE);
    output->SetLineColor(igm::vec3(1.0f, 1.0f, 1.0f));
    output->SetLineWidth(2.0f);
    auto scene = Scene::New();
    scene->AddModel(input);
    scene->AddModel(output);

    auto window = RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);
    auto interactor = Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
    return 0;
}
