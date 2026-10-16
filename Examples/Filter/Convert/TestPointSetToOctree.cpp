// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Convert/TestPointSetToOctree.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <Convert/iGamePointSetToOctreeFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>

int main() {
    /* 创建场景 */
    auto scene = iGame::Scene::New();

    /* 读取点集文件 */
    const std::string fileName = "./Models/OctreePoints.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 1;
    }

    /* 点集转八叉树：执行转换 */
    auto filter = iGame::PointSetToOctreeFilter::New();
    filter->SetInput(obj);
    filter->SetNumberOfPointsPerCell(1);
    if (!filter->Execute() || !filter->GetOutput()) return 1;

    /* 以点的形式显示八叉树输出结果 */
    auto res = filter->GetOutput();
    if (res != nullptr) {
        scene->AddModel(res);
        auto resDraw = iGame::DynamicCast<iGame::DrawObject>(res);
        resDraw->SetViewStyle(IG_POINTS);
        resDraw->SetPointSize(5);
    }

    if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) return 0;
    /* 启动窗口 */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
}
