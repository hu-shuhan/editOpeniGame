// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Convert/TestResampleToImage.cpp
#include <string>
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <Convert/iGameResampleToImageFilter.h>
#include <ModelSurface/iGameModelGeometryFilter.h>
#include <iGameArrayObject.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameInteractor.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameStructuredMesh.h>
#include <iGameSurfaceMesh.h>

#include <chrono>

int main(int argc, char** argv) {
    /* 创建场景 */
    auto scene = iGame::Scene::New();

    /* 读取网格文件作为重采样输入 */
    const std::string fileName = "./Models/ResampleCubeVector.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 1;
    }

    /* Resample To Image：把网格上的点场重采样到规则图像网格。
       分辨率 64x64x64（与 VTK 对比时的采样一致），UseInputBounds=true 默认取输入包围盒。 */
    auto filter = iGame::ResampleToImageFilter::New();
    filter->SetInput(obj);
    filter->SetSamplingDimensions(64, 64, 64);
    auto t0 = std::chrono::steady_clock::now();
    filter->Execute();
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "[ResampleToImage] filterTime="
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << "ms\n";

    auto resMesh = iGame::DynamicCast<iGame::StructuredMesh>(filter->GetOutput());
    if (resMesh == nullptr) {
        std::cout << "Output ERROR!\n";
        return 1;
    }

    /* ---- 定位输出属性 ---- */
    iGame::ArrayObject::Pointer maskArr;
    iGame::ArrayObject::Pointer fieldArr;
    IGenum fieldType = IG_SCALAR;
    int fieldDim = 1;
    {
        auto attrs = resMesh->GetAttributeSet()->GetAllAttributes();
        for (IGsize a = 0; a < attrs->GetNumberOfElements(); ++a) {
            auto& attr = attrs->GetElement(a);
            if (attr.isDeleted || attr.pointer == nullptr) continue;
            if (attr.pointer->GetName() == "vtkValidPointMask") {
                maskArr = attr.pointer;
            } else if (attr.pointer->GetName() == "field") {
                fieldArr = attr.pointer;
                fieldType = attr.type;
                fieldDim = attr.pointer->GetDimension();
            }
        }
    }

    /* ---- 诊断输出：便于与 VTK vtkResampleToImage 结果逐项核对 ---- */
    const IGsize nPts = resMesh->GetNumberOfPoints();
    const IGsize nCells = resMesh->GetNumberOfCells();
    const iGame::Point& p0 = resMesh->GetPoint(0);
    const iGame::Point& p1 = resMesh->GetPoint(1);
    const iGame::Point& py = resMesh->GetPoint(64);     // 格点 (0,1,0)
    const iGame::Point& pz = resMesh->GetPoint(4096);   // 格点 (0,0,1)  (64x64=4096)
    std::cout << "[ResampleToImage] points=" << nPts << " cells=" << nCells << "\n";
    std::cout << "[ResampleToImage] origin=(" << p0[0] << ", " << p0[1] << ", " << p0[2] << ")\n";
    std::cout << "[ResampleToImage] spacing=(" << (p1[0] - p0[0]) << ", " << (py[1] - p0[1])
              << ", " << (pz[2] - p0[2]) << ")\n";

    IGsize validCount = 0;
    for (IGsize p = 0; p < nPts; ++p) {
        if (maskArr != nullptr && maskArr->GetElementValue(p, 0) != 0.0) ++validCount;
    }
    std::cout << "[ResampleToImage] validPoints=" << validCount << "/" << nPts << "\n";

    /* 打印第一个有效格点的插值场（用于与 VTK 比对非零插值） */
    for (IGsize p = 0; p < nPts; ++p) {
        if (maskArr != nullptr && maskArr->GetElementValue(p, 0) != 0.0) {
            if (fieldArr != nullptr) {
                std::cout << "[ResampleToImage] firstValid grid idx=" << p << " field=(";
                for (int d = 0; d < fieldDim; ++d) {
                    if (d) std::cout << ", ";
                    std::cout << fieldArr->GetElementValue(p, d);
                }
                std::cout << ")\n";
            }
            break;
        }
    }

    std::cout << std::flush;

    /* ---- 验证 ghost 空白化：抽壳面数应远大于 6（否则是立方体外表面）---- */
    {
        auto geom = iGame::ModelGeometryFilter::New();
        iGame::SurfaceMesh::Pointer surf = iGame::SurfaceMesh::New();
        if (geom->Execute(resMesh, surf) && surf != nullptr) {
            std::cout << "[surface] faces=" << surf->GetNumberOfFaces()
                      << " points=" << surf->GetNumberOfPoints() << "\n";
            std::cout << std::flush;
        }
    }

    /* ---- 显示整幅图像（262144 点 / 250047 单元）----
       渲染时 ModelGeometryFilter 读取 "vtkGhostType" 单元数组做空白化，抽出有效单元表面，
       从而显示成缺角立方体形状（输入模型挖掉一个角部小立方体），与 VTK vtkResampleToImage
       的 ghost 空白化行为一致。 */
    if (argc > 1 && std::string(argv[1]) == "--no-render") return 0;
    scene->AddModel(resMesh);
    auto outDraw = iGame::DynamicCast<iGame::DrawObject>(resMesh);
    if (outDraw != nullptr) {
        outDraw->SetViewStyle(IG_SURFACE);
    }

    /* 启动窗口 */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    if (!(argc > 1 && std::string(argv[1]) == "--no-render")) window->Show();
}
