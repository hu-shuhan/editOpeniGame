// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Convert/TestPointSetToOctree.cpp
#include <string>
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <Convert/iGamePointSetToOctreeFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>

int main(int argc, char** argv) {
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
    if (!filter->Execute()) return 1;

    /* 以点的形式显示八叉树输出结果 */
    auto res = filter->GetOutput();
    if (res == nullptr) return 1;
    if (argc > 1 && std::string(argv[1]) == "--no-render") return 0;
    if (res != nullptr) {
        scene->AddModel(res);
        auto resDraw = iGame::DynamicCast<iGame::DrawObject>(res);
        resDraw->SetViewStyle(IG_POINTS);
        resDraw->SetPointSize(5);
    }

    /* 启动窗口 */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    if (!(argc > 1 && std::string(argv[1]) == "--no-render")) window->Show();
}
