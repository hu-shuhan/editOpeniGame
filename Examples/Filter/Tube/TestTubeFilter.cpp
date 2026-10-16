// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/Tube/TestTubeFilter.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
#include <Core/iGameScene.h>
#include <Tube/iGameTube.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameSurfaceMesh.h>

#include <iostream>
#include <string>

// TubeFilter 测试程序
//   - 默认以相对路径加载 Models/tube_test.vtk（工作目录为 Examples 构建目录，构建时 Models 会拷贝到该目录）；
//   - 将线段扫掠成圆管；
//   - 在新的渲染窗口（黑框）中显示生成的 tube。
// 用法：
//   TestTubeFilter.exe                        （使用默认相对路径模型）
//   TestTubeFilter.exe Models/tube_test.vtk   （显式指定模型路径）
int main(int argc, char* argv[]) {
    std::string fileName = "./Models/tube_test.vtk";
    if (argc > 1 && argv[1] != nullptr) {
        fileName = argv[1];
    }

    std::cout << "读取模型: " << fileName << std::endl;
    auto input = iGame::FileIO::ReadFile(fileName);
    if (input == nullptr) {
        std::cout << "读取模型失败。" << std::endl;
        std::cout << "用法: TestTubeFilter.exe Models/tube_test.vtk" << std::endl;
        return 1;
    }

    // 线段 -> 圆管
    auto filter = iGame::TubeFilter::New();
    filter->SetRadius(0.05);          // 圆管半径，需与模型尺度匹配
    filter->SetNumberOfSides(12);     // 截面正多边形边数，越大越接近圆
    filter->SetCapping(true);         // 封闭首、尾端面
    filter->SetUseDefaultNormal(false); // 不使用用户给定初始法向，由程序自动选择
    filter->SetInput(input);

    std::cout << "圆管参数: 半径 Radius = " << filter->GetRadius()
              << ", 截面正多边形边数 NumberOfSides = " << filter->GetNumberOfSides()
              << std::endl;

    if (!filter->Execute()) {
        std::cout << "TubeFilter 执行失败；请确认输入为含线段（IG_LINE / IG_POLY_LINE）的 Poly Data。"
                  << std::endl;
        return 1;
    }

    auto outMesh = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput(0));
    if (outMesh == nullptr) {
        std::cout << "Filter 没有生成有效的 SurfaceMesh 输出。" << std::endl;
        return 1;
    }

    std::cout << "Tube 生成完成: 点数 " << outMesh->GetNumberOfPoints()
              << ", 面数 " << outMesh->GetNumberOfFaces() << std::endl;

    // 在新渲染窗口（黑框）中显示生成的 tube
    auto scene = iGame::Scene::New();
    scene->AddModel(outMesh);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    std::cout << "关闭渲染窗口后程序退出。" << std::endl;
    window->Show();
    return 0;
}
