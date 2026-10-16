// Source: dayuwan77/igamevis at eccac729b57aeacbe9312d7d5189f6990bb4eebd.
// Integration gap: e7ec6571 imported the filter but omitted its example.
// Preserve the numerical/attribute checks below and reject invalid inputs;
// visual examples support --no-render so CI requires a real exit status.
// Integration commit: test: add examples for first-batch standard filters
// Find it: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Convert/TestConvertToVertex.cpp
#include <Convert/iGameConvertToVertexFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iostream>

int main(int argc, char** argv) {
    const bool noRender = argc > 1 && std::string(argv[1]) == "--no-render";
    const std::string fileName = "./Models/AIGen_Tet_TwistedRod.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 1;
    }

    auto filter = iGame::ConvertToVertexFilter::New();
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "Execute ERROR!\n";
        return 1;
    }

    auto out = DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        std::cout << "Output ERROR!\n";
        return 1;
    }
    out->AddViewStyle(IG_POINTS);

    auto in = DynamicCast<iGame::PointSet>(obj);

    //输出输入点集和输出顶点单元网格的点数和单元数
    std::cout << "input  points: " << in->GetNumberOfPoints() << "\n";
    std::cout << "output points: " << out->GetNumberOfPoints()
              << ", cells: " << out->GetNumberOfCells() << "\n";

    // Regression: a successful Execute alone did not validate the vertex mapping.
    // Check one independent vertex per input point and unchanged coordinates.
    if (out->GetNumberOfPoints() != in->GetNumberOfPoints() ||
        out->GetNumberOfCells() != in->GetNumberOfPoints() || out.get() == obj.get()) return 1;
    for (IGsize i = 0; i < out->GetNumberOfCells(); ++i) {
        const igIndex* ids = nullptr;
        if (out->GetCells()->GetCellIds(i, ids) != 1 || ids[0] != i ||
            out->GetCellTypes()->GetValue(i) != iGame::IG_VERTEX) return 1;
        for (int d = 0; d < 3; ++d) {
            if (out->GetPoint(i)[d] != in->GetPoint(i)[d]) return 1;
        }
    }
    if (noRender) return 0;

    /* 创建场景，显示转换后的顶点单元网格 */
    auto scene = iGame::Scene::New();
    //scene->AddModel(in);
    scene->AddModel(out);

    auto model = scene->GetCurrentModel();
    model->ViewCloudPicture(0, -1);

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
    return 0;
}
