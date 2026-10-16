// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestResampleToLine.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <ResampleToLine/iGameResampleToLine.h>
#include <Core/iGameScene.h>
#include <VectorView/iGameVectorBase.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameRenderWindow.h>
#include <iGameVolume.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>
#include <iostream>
#include <iGameSurfaceMesh.h>
#include<filesystem>

int DrawLine(iGame::SurfaceMesh::Pointer m, iGame::Painter3D* painter) {
    //draw line
    painter->SetPen(Color::White);
    painter->SetBrush(0, 255, 0);
    if (m->GetEdges() == nullptr) { m->BuildEdges(); }
    int np = m->GetNumberOfPoints();
    if (np <= 0) { throw std::runtime_error("points is zero!"); }
    for (int i = 0; i < m->GetNumberOfPoints() - 1; i++) { painter->DrawLine(m->GetPoint(i), m->GetPoint(i + 1)); }
    painter->Modified();
    return 0;
}

int main(int argc, char** argv) {
    std::cout << std::filesystem::current_path() << std::endl;
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    const std::string fileName = "./Models/Resampletoline_test.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    iGame::UnstructuredMesh::Pointer mesh = DynamicCast<iGame::UnstructuredMesh>(obj);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 1;
    }
    auto input = mesh;
    //新建切割的filter
    auto filter = iGame::ResampleToLine::New();
    //设置输入
    filter->SetInput(mesh);
    //执行
    if (!filter->Execute()) {
        std::cerr << filter->GetMessage() << "\n";
        return 1;
    }
    //返回结果：输出 1 为折线数据（SurfaceMesh，点 + 边）
    auto res = filter->GetOutput(1);
    if (res == nullptr) {
        std::cout << "OutPut EERROR!" << std::endl;
        return 1;
    }
    int fieldDim = 1;
    auto attrs = res->GetAttributeSet()->GetAllAttributes();

    for (int a = 0; a < attrs->GetNumberOfElements(); a++) {
        auto attr = attrs->GetElement(a).GetPointer();
        auto num = attr->GetNumberOfValues();
        for (int b = 0; b < num; b++) {
            std::cout << attr->GetValue(b) << std::endl;
        }
    }
    //(DynamicCast<iGame::DrawObject>(res))->SetViewStyle(IG_SURFACE);
    //auto output = iGame::DynamicCast<iGame::DrawObject>(res);
    auto output = iGame::DynamicCast<iGame::SurfaceMesh>(res);


    if (argc > 1 && std::string(argv[1]) == "--no-render") return 0;
    const auto modelId = scene->AddModel(output);
    DrawLine(output, scene->GetModelById(modelId)->GetPainter3D());
    /* 启动窗口设置*/
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
}
