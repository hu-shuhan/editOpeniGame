#include <Convert/iGameConvertToVertexFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iostream>

int main() {
    const std::string fileName = "./Models/AIGen_Tet_TwistedRod.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }

    auto filter = iGame::ConvertToVertexFilter::New();
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "Execute ERROR!\n";
        return 0;
    }

    auto out = DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        std::cout << "Output ERROR!\n";
        return 0;
    }
    out->AddViewStyle(IG_POINTS);

    auto in = DynamicCast<iGame::PointSet>(obj);
    
    //输出输入点集和输出顶点单元网格的点数和单元数
    std::cout << "input  points: " << in->GetNumberOfPoints() << "\n";
    std::cout << "output points: " << out->GetNumberOfPoints()
              << ", cells: " << out->GetNumberOfCells() << "\n";

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
