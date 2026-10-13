// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/WarpByScalar/TestWarpByScalar.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
#include <WarpByScalar/iGameWarpByScalarFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameSmartPointer.h>
#include <iGameVector.h>
#include <string>
#include <vector>

int main() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/warp_grid3d_normals.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (!obj) {
        std::cout << "Failed to read file: " << fileName << std::endl;
        std::cin.get();
        return 1;
    }


    auto filter = iGame::WarpByScalar::New();
    filter->SetScalarsArrayName("height");
    filter->SetScaleFactor(1.0);
    filter->SetUseNormal(true);
    filter->SetNormal(0.0,0.0,1.0);
    filter->SetXYPlane(false);
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "Filter ERROR!\n";
        std::cin.get();
        return 0;
    }
    obj = filter->GetOutput();
    scene->AddModel(obj);
    // Set up the render window
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    // Set up the interactor
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    // Start the render loop
    window->Show();
}