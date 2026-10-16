// Regression: b42e58d2 added this example without a CMake target; read/filter
// failures also returned success. Verify the 3x3x2 point VOI has 18 points and
// 4 hexahedra. The middle layer is warped: preserve its actual coordinates,
// not a uniform lattice. Keep interactive rendering outside --no-render.
// Fix commit: test: add examples for first-batch standard filters
// Find it: git log --format="%h %s" --grep="test: add examples for first-batch standard filters"
#include <ExtractSubset/iGameExtractSubsetFilter.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameStructuredMesh.h>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const bool noRender = argc > 1 && std::string(argv[1]) == "--no-render";

    // 读取结构化网格数据文件
    // 模型为 5×4×3 的结构化网格 (DIMENSIONS 5 4 3)
    const std::string fileName = "./Models/Structured_Volume_Test.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "读取文件失败!\n";
        return 1;
    }

    // 创建提取子集过滤器
    auto filter = iGame::ExtractSubsetFilter::New();
    filter->SetInput(obj);

    // 设置感兴趣区域 (VOI: Volume of Interest)
    // 格式: minI, maxI, minJ, maxJ, minK, maxK
    // 模型维度为 5×4×3 (含 4×3×2 = 24 个六面体单元)
    // 提取 I:[0,2] J:[0,2] K:[0,1] 区域，共 18 个点、4 个六面体单元
    filter->SetVOI(0, 2, 0, 2, 0, 1);

    // 执行提取
    if (!filter->Execute()) {
        std::cout << "过滤器执行失败!\n";
        return 1;
    }

    // 获取结果
    auto result = iGame::DynamicCast<iGame::StructuredMesh>(filter->GetOutput());
    if (!result || result->GetNumberOfPoints() != 18 || result->GetNumberOfVolumes() != 4) {
        std::cerr << "Incorrect subset dimensions or cell count\n";
        return 1;
    }
    const auto size = result->GetDimensionSize();
    if (size[0] != 3 || size[1] != 3 || size[2] != 2) {
        std::cerr << "Unexpected dimensions: " << size[0] << "," << size[1] << "," << size[2] << "\n";
        return 1;
    }
    for (int k = 0; k < 2; ++k) {
        for (int j = 0; j < 3; ++j) {
            for (int i = 0; i < 3; ++i) {
                const auto p = result->GetPoint(i + 3*j + 9*k);
                const auto expected = obj->GetPoints()->GetPoint(i + 5*j + 20*k);
                if (p[0] != expected[0] || p[1] != expected[1] || p[2] != expected[2]) {
                    std::cerr << "Point " << i << "," << j << "," << k << ": "
                              << p[0] << "," << p[1] << "," << p[2] << "\n";
                    return 1;
                }
            }
        }
    }
    if (obj->GetPoints()->GetNumberOfPoints() != 60) return 1;
    if (noRender) return 0;
    auto scene = iGame::Scene::New();
    scene->AddModel(result);

    // 启动渲染窗口
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    // 设置交互器
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    // 显示窗口
    window->Show();
}
