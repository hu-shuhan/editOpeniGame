// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/ReverseSense/TestReverseSense.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
// 反转面朝向（ReverseSense）GUI 示例：
// 读取一个 AI 生成的曲面模型，反转其面环序并取反法向，输出一份新的 SurfaceMesh。
// 输入（线框）与输出（实体，沿 X 平移错开）并排显示，固定相对路径自动运行。
#include <ReverseSense/iGameReverseSenseFilter.h>

#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGamePointSet.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameType.h>

#include <iostream>
#include <string>

int main() {
    const std::string fileName = "./Models/AIGen_Surface_ReverseSenseDemo.obj";
    auto dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj == nullptr) {
        igError("Error reading the file");
        return 0;
    }

    auto scene = iGame::Scene::New();
    scene->AddModel(dataObj);
    if (auto inDraw = iGame::DynamicCast<iGame::DrawObject>(dataObj)) {
        inDraw->SetViewStyle(IG_WIREFRAME);
    }

    auto filter = iGame::ReverseSenseFilter::New();
    filter->SetInput(dataObj);
    filter->SetReverseCells(true);
    filter->SetReverseNormals(true);
    if (!filter->Execute()) {
        std::cerr << "ReverseSense failed: " << filter->GetMessage() << "\n";
        return 1;
    }

    auto output = filter->GetOutput();
    if (output == nullptr) {
        std::cerr << "ReverseSense has no output\n";
        return 1;
    }

    // 沿 X 平移输出，避免与输入完全重叠，便于并排对照。
    if (auto outPts = iGame::DynamicCast<iGame::PointSet>(output)) {
        const double dx = dataObj->GetBoundingBox().diag() * 1.2 + 1.0;
        auto points = outPts->GetPoints();
        for (IGsize i = 0; i < points->GetNumberOfPoints(); ++i) {
            iGame::Point p = points->GetPoint(i);
            p[0] += static_cast<float>(dx);
            points->SetPoint(i, p);
        }
    }

    scene->AddModel(output);
    if (auto outDraw = iGame::DynamicCast<iGame::DrawObject>(output)) {
        outDraw->SetViewStyle(IG_SURFACE);
    }

    std::cout << "ReverseSense applied: reverseCells=1 reverseNormals=1\n" << std::flush;

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
