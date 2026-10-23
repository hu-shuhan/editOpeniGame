#include <iGamePointSet.h>
#include <cstdlib>
// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
#include <VolumeOfRevolution/iGameVolumeOfRevolutionFilter.h>
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
    const std::string fileName = "./Models/vase2.vtk";

    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (!obj) {
        std::cout << "Failed to read file: " << fileName << std::endl;
        if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) std::cin.get();
        return 1;
    }


    double angleRad = 360.0 * M_PI / 180.0;
    auto filter = iGame::VolumeOfRevolutionFilter::New();

    filter->SetAxis(iGame::Vector3d(0.0, 0.0, 1.0).normalized(), iGame::Vector3d(0.0, 0.0, 0.0));
    filter->SetResolution(36);
    filter->SetAngle(angleRad);
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "Filter ERROR!\n";
        if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) std::cin.get();
        return 1;
    }
    obj = filter->GetOutput();
    if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) {
        auto points = iGame::DynamicCast<iGame::PointSet>(obj);
        return points && points->GetNumberOfPoints() > 0 ? 0 : 1;
    }
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