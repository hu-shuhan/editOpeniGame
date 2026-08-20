#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <Contour/iGameContourFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolume.h>

namespace {

// 统计轮廓结果里线段与三角面的数量：
// 2D 单元产出 IG_LINE（等值线），3D 单元产出 IG_TRIANGLE（等值面）
void CountContourCells(const iGame::UnstructuredMesh::Pointer& mesh, int& lineNum, int& triangleNum) {
    lineNum = 0;
    triangleNum = 0;
    if (!mesh) return;
    const auto cellNum = mesh->GetNumberOfCells();
    for (auto i = decltype(cellNum){0}; i < cellNum; ++i) {
        const int dim = iGame::Cell::GetCellDimension(mesh->GetCellType(i));
        if (dim == 1) ++lineNum;
        else if (dim >= 2) ++triangleNum;
    }
}

// 对一个模型抽取轮廓，成功后把结果加进场景。expectIsoSurface 仅用于结果校验。
bool RunContour(const std::string& tag, const std::string& fileName, bool expectIsoSurface,
                iGame::Scene::Pointer scene) {
    std::cerr << "[" << tag << "] file=" << fileName
              << " exists=" << std::filesystem::exists(fileName) << "\n"
              << std::flush;

    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << "[" << tag << "] FAIL: ReadFile returned null\n" << std::flush;
        return false;
    }

    // 轮廓提取只支持点标量
    const int index = 0;
    auto pointAttributes = obj->GetAttributeSet() ? obj->GetAttributeSet()->GetAllPointAttributes() : nullptr;
    if (pointAttributes == nullptr || index >= pointAttributes->GetNumberOfElements()) {
        std::cerr << "[" << tag << "] FAIL: invalid point attributes\n" << std::flush;
        return false;
    }

    auto& attr = pointAttributes->GetElement(index);
    auto array = attr.pointer;
    auto range = attr.GetDataRange();
    if (array == nullptr || range == nullptr) {
        std::cerr << "[" << tag << "] FAIL: attribute or data range is null\n" << std::flush;
        return false;
    }

    const int dimension = 0;
    const double lo = range->GetValue(dimension * 2 + 2);
    const double hi = range->GetValue(dimension * 2 + 3);
    std::cerr << "[" << tag << "] scalar=\"" << array->GetName() << "\" dim=" << dimension
              << " range=(" << lo << ", " << hi << ")\n"
              << std::flush;

    // 在标量范围内取三档等值，一次传入合并到同一个输出网格
    std::vector<double> values;
    values.push_back(lo * 2 / 3 + hi / 3);
    values.push_back(lo / 2 + hi / 2);
    values.push_back(lo / 3 + hi * 2 / 3);

    auto filter = iGame::ContourFilter::New();
    filter->SetInput(obj);
    filter->SetIsoScalarData(array, values, dimension);
    if (!filter->Execute()) {
        std::cerr << "[" << tag << "] FAIL: ContourFilter::Execute returned false\n" << std::flush;
        return false;
    }

    auto res = filter->GetContourMesh();
    if (res == nullptr) {
        std::cerr << "[" << tag << "] FAIL: ContourFilter output null\n" << std::flush;
        return false;
    }
    if (res->GetNumberOfCells() == 0) {
        std::cerr << "[" << tag << "] FAIL: contour is empty (iso values intersect no cell)\n" << std::flush;
        return false;
    }

    int lineNum = 0, triangleNum = 0;
    CountContourCells(res, lineNum, triangleNum);
    std::cerr << "[" << tag << "] points=" << res->GetNumberOfPoints()
              << " cells=" << res->GetNumberOfCells()
              << " lines=" << lineNum << " triangles=" << triangleNum << "\n"
              << std::flush;

    if (expectIsoSurface && triangleNum == 0) {
        std::cerr << "[" << tag << "] FAIL: expected iso-surfaces but got no triangle\n" << std::flush;
        return false;
    }
    if (!expectIsoSurface && lineNum == 0) {
        std::cerr << "[" << tag << "] FAIL: expected iso-lines but got no line segment\n" << std::flush;
        return false;
    }

    res->SetName(std::filesystem::path(fileName).stem().string() + "_Contour");
    scene->AddModel(res);

    auto draw = DynamicCast<iGame::DrawObject>(res);
    draw->SetShellRenderingOption(false);
    draw->ConvertToDrawableData();
    // 等值线只有线单元，不带 IG_WIREFRAME 会什么都画不出来
    draw->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
    draw->SetLineWidth(2.5f);
    draw->ViewCloudPicture(scene, index, dimension);

    std::cerr << "[" << tag << "] OK\n" << std::flush;
    return true;
}

} // namespace

int main() {
    std::cerr << "[testContourExtraction] cwd=" << std::filesystem::current_path().string() << "\n" << std::flush;

    auto scene = iGame::Scene::New();

    // Driver/driver_1.vtk：面网格 → 等值线；streamTest.vtk：体网格 → 等值面
    // 两个模型均需自备，放到运行目录的 ./Models/ 下
    const bool okSurface = RunContour("testContourExtraction/isoline", "./Models/Driver/driver_1.vtk",
                                      /*expectIsoSurface=*/false, scene);
    const bool okVolume = RunContour("testContourExtraction/isosurface", "./Models/streamTest.vtk",
                                     /*expectIsoSurface=*/true, scene);

    if (!okSurface || !okVolume) {
        std::cerr << "[testContourExtraction] FAIL: isoline=" << okSurface << " isosurface=" << okVolume << "\n"
                  << std::flush;
        return 1;
    }

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    std::cerr << "[testContourExtraction] Show() — close window to exit\n" << std::flush;
    window->Show();
    return 0;
}
