#include <IsoVolume/iGameIsoVolumeFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameRenderWindow.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <iostream>

/* TestIsoVolume: 提取标量值落在 [lower, upper] 区间内的体网格
 * 输入: ./Models/ClipTest_Plane_UnstructuredGrid.vtk (带标量场的四面体网格)
 * 区间取标量范围的 [1/3, 2/3], 打印输出点数/单元数验证, 并弹窗可视化展示结果 */
int main() {

    /* 读取模型 */
    const std::string fileName = "./Models/ClipTest_Plane_UnstructuredGrid.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 1;
    }

    /* 取第一个点标量数组, 区间取数据范围的 [1/3, 2/3] */
    auto attrs = obj->GetAttributeSet()->GetAllPointAttributes();
    if (attrs == nullptr || attrs->GetNumberOfElements() == 0) {
        std::cout << "No point attributes ERROR!\n";
        return 1;
    }
    auto& attr = attrs->GetElement(0);
    auto range = attr.GetDataRange();
    auto array = attr.pointer;
    /* GetDataRange 布局: [0]/[1]=向量模长范围, [2]/[3]=分量0范围, [4]/[5]=分量1范围...
     * 测试数据 test_1 是 3 分量向量, 这里取分量 0 的范围作为标量范围 */
    double smin = range->GetValue(2);
    double smax = range->GetValue(3);
    double lower = smin + (smax - smin) / 3.0;
    double upper = smin + (smax - smin) * 2.0 / 3.0;

    /* 等值面之间的体提取 */
    auto filter = iGame::IsoVolumeFilter::New();
    filter->SetInput(obj);
    filter->SetIsoScalarData(array, lower, upper, 0);
    filter->Execute();

    /* 打印结果 */
    auto res = filter->GetOutput();
    if (res == nullptr) {
        std::cout << "Output NULL ERROR!\n";
        return 1;
    }
    unsigned long long np = 0, nc = 0;
    if (auto m = iGame::DynamicCast<iGame::UnstructuredMesh>(res)) {
        np = m->GetNumberOfPoints();
        nc = m->GetNumberOfCells();
    } else if (auto m = iGame::DynamicCast<iGame::SurfaceMesh>(res)) {
        np = m->GetNumberOfPoints();
        nc = m->GetNumberOfFaces();
    } else if (auto m = iGame::DynamicCast<iGame::VolumeMesh>(res)) {
        np = m->GetNumberOfPoints();
        nc = m->GetNumberOfVolumes();
    }
    std::cout << "IsoVolume range = [" << lower << ", " << upper << "]\n";
    std::cout << "Input  scalar range = [" << smin << ", " << smax << "]\n";
    std::cout << "Output points = " << np << ", cells = " << nc << "\n";
    std::cout << "TestIsoVolume DONE\n";

    /* 可视化展示 (参照 TestClip / TestSlice 标准写法) */
    auto scene = iGame::Scene::New();
    auto draw = iGame::DynamicCast<iGame::DrawObject>(res);
    if (draw != nullptr) {
        draw->SetViewStyle(IG_SURFACE);
        draw->ViewCloudPicture(scene, 0);
    }
    if (res != nullptr) { scene->AddModel(res); }
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
