#include <iostream>
#include <filesystem>
#include <iGameVolume.h>
#include <Core/iGameScene.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>
#include <iGameFileIO.h>
#include <Contour/iGameContourFilter.h>

int main() {
    auto scene = iGame::Scene::New();
    // dianfengshan.vtk 未随仓库提供；改用 Models 内已有、带点标量的网格
    const std::string fileName = "./Models/Tet_Plane.vtk";
    std::cerr << "[testContourLine] cwd=" << std::filesystem::current_path().string()
              << " file=" << fileName
              << " exists=" << std::filesystem::exists(fileName) << "\n"
              << std::flush;

    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << "[testContourLine] FAIL: ReadFile returned null\n" << std::flush;
        return 1;
    }

    auto filter = iGame::ContourFilter::New();
    int index = 0;
    auto pointAttibutes = obj->GetAttributeSet()->GetAllPointAttributes();
    if (pointAttibutes == nullptr || index >= pointAttibutes->GetNumberOfElements()) {
        std::cerr << "[testContourLine] FAIL: invalid point attributes\n" << std::flush;
        return 1;
    }

    auto& attr = pointAttibutes->GetElement(index);
    auto range = attr.GetDataRange();
    auto array = attr.pointer;
    int dimension = 0;
    std::vector<double> values;
    // 在标量范围内取三档等值
    double value = range->GetValue(dimension * 2 + 2) * 2 / 3 + range->GetValue(dimension * 2 + 3) / 3;
    values.push_back(value);
    value = range->GetValue(dimension * 2 + 2) / 3 + range->GetValue(dimension * 2 + 3) * 2 / 3;
    values.push_back(value);
    value = range->GetValue(dimension * 2 + 2) / 2 + range->GetValue(dimension * 2 + 3) / 2;
    values.push_back(value);

    filter->SetInput(obj);
    filter->SetIsoScalarData(array, values, dimension);
    filter->Execute();
    auto res = filter->GetOutput();
    if (res == nullptr) {
        std::cerr << "[testContourLine] FAIL: ContourFilter output null\n" << std::flush;
        return 1;
    }

    scene->AddModel(res);
    auto draw = DynamicCast<iGame::DrawObject>(res);
    draw->ConvertToDrawableData();
    draw->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
    draw->ViewCloudPicture(scene, index, dimension);

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    std::cerr << "[testContourLine] Show() — close window to exit\n" << std::flush;
    window->Show();
    return 0;
}
