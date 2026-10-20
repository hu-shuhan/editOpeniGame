// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
#include <DataProcessing/iGameDecimatePolylineFilter.h>

#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameUnstructuredMesh.h>

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    const bool visualize = argc == 2 && std::string(argv[1]) == "--visualize";
    if (argc > 2 || (argc == 2 && !visualize)) {
        std::cerr << "Usage: testDecimatePolyline.exe [--visualize]\n";
        return 1;
    }

    const std::string fileName = "Models/DecimatePolyline_Curve.vtk";
    auto input = iGame::FileIO::ReadFile(fileName);
    if (!input || !iGame::DecimatePolylineFilter::CanProcessInput(input)) {
        std::cerr << "Failed to read a supported polyline: " << fileName << '\n';
        return 1;
    }

    auto filter = iGame::DecimatePolylineFilter::New();
    filter->SetInput(input);
    filter->SetTargetReduction(0.5);
    if (!filter->Execute()) {
        std::cerr << "DecimatePolylineFilter execution failed.\n";
        return 1;
    }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (!output || output->GetNumberOfPoints() != 75 || output->GetNumberOfCells() != 1 ||
        output->GetCellType(0) != iGame::IG_POLY_LINE) {
        std::cerr << "Unexpected output topology.\n";
        return 1;
    }

    const auto& attribute = output->GetAttributeSet()->GetAttribute("OriginalPointId");
    auto originalIds = iGame::DynamicCast<iGame::IntArray>(attribute.pointer);
    const std::vector<int> paraViewPointIds{
            0,   2,   4,   6,   8,   10,  12,  14,  16,  20,  22,  24,  25,
            27,  29,  33,  35,  37,  39,  40,  41,  42,  44,  46,  48,  52,
            54,  56,  58,  59,  61,  63,  65,  69,  71,  73,  75,  77,  79,
            83,  85,  87,  89,  90,  91,  92,  94,  96,  98,  102, 104, 106,
            108, 110, 112, 114, 118, 120, 122, 123, 125, 127, 129, 133, 135,
            137, 139, 140, 141, 142, 143, 144, 146, 148, 150};
    if (!originalIds || originalIds->GetNumberOfElements() != paraViewPointIds.size()) {
        std::cerr << "OriginalPointId was not copied correctly.\n";
        return 1;
    }

    for (IGsize i = 0; i < paraViewPointIds.size(); ++i) {
        if (originalIds->GetValue(i) != paraViewPointIds[i]) {
            std::cerr << "Result differs from ParaView at output point " << i << ".\n";
            return 1;
        }
    }

    auto chainedFilter = iGame::DecimatePolylineFilter::New();
    chainedFilter->SetInput(output);
    chainedFilter->SetTargetReduction(0.5);
    if (!iGame::DecimatePolylineFilter::CanProcessInput(output) || !chainedFilter->Execute()) {
        std::cerr << "The filter cannot process its own output.\n";
        return 1;
    }

    auto chainedOutput = iGame::DynamicCast<iGame::UnstructuredMesh>(chainedFilter->GetOutput());
    if (!chainedOutput || chainedOutput->GetNumberOfPoints() != 37 ||
        chainedOutput->GetNumberOfCells() != 1 ||
        chainedOutput->GetCellType(0) != iGame::IG_POLY_LINE) {
        std::cerr << "Unexpected chained output topology.\n";
        return 1;
    }

    std::cout << "DecimatePolyline: 151 points -> " << output->GetNumberOfPoints()
              << " -> " << chainedOutput->GetNumberOfPoints()
              << " points; ParaView reference matched and output is reusable.\n";

    if (visualize) {
        auto drawObject = iGame::DynamicCast<iGame::DrawObject>(output);
        drawObject->SetShellRenderingOption(false);
        drawObject->SetViewStyle(IG_POINTS | IG_WIREFRAME);
        drawObject->SetPointSize(7.0f);
        drawObject->SetLineWidth(3.0f);

        auto scene = iGame::Scene::New();
        scene->AddModel(output);

        auto window = iGame::RenderWindow::New();
        window->SetSize(1280, 720);
        window->SetScene(scene);

        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);

        scene->ResetCameraView();
        window->Show();
    }

    return 0;
}
