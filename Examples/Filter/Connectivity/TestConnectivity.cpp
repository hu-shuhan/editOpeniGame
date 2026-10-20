// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
// 连通区域（Connectivity）GUI 示例：
// 读取一个含 3 个互不连通面片的模型，编号所有连通区域并按 RegionId 着色显示。
// 固定相对路径自动运行。
#include <Connectivity/iGameConnectivityFilter.h>

#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameType.h>

#include <iostream>
#include <string>

int main() {
    const std::string fileName = "./Models/AIGen_Surface_ThreeIslands.obj";
    auto dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj == nullptr) {
        igError("Error reading the file");
        return 0;
    }

    auto scene = iGame::Scene::New();

    auto filter = iGame::ConnectivityFilter::New();
    filter->SetInput(dataObj);
    filter->SetExtractionMode(iGame::ConnectivityFilter::ALL_REGIONS);
    filter->SetColorRegions(true);
    if (!filter->Execute()) {
        std::cerr << "Connectivity failed: " << filter->GetMessage() << "\n";
        return 1;
    }

    auto output = filter->GetOutput();
    if (output == nullptr) {
        std::cerr << "Connectivity has no output\n";
        return 1;
    }
    scene->AddModel(output);

    // 按 RegionId 着色显示编号结果（维度 0 = 单分量）。
    if (auto outDraw = iGame::DynamicCast<iGame::DrawObject>(output)) {
        outDraw->SetViewStyle(IG_SURFACE);
        if (auto attrs = output->GetAttributeSet()) {
            const int idx = attrs->GetAttributeIndex(iGame::ConnectivityFilter::RegionIdName);
            if (idx >= 0) { outDraw->ViewCloudPicture(scene, idx, 0); }
        }
    }

    std::cout << "Connectivity (ALL_REGIONS): " << filter->GetNumberOfExtractedRegions()
              << " regions\n";
    const auto& sizes = filter->GetRegionSizes();
    std::cout << "region sizes:";
    for (auto s : sizes) { std::cout << " " << s; }
    std::cout << "\n" << std::flush;

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
