// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
// ============================================================================
// AppendArcLength 可视化测试（OpenGL 窗口；写法与 Examples/Filter/Convert/TestResampleToImage.cpp 一致）
//
//   1) 程序内生成一条 201 点螺旋折线（r=10、高 20、2 圈；弧长有解析解，便于核对）
//   2) 执行 AppendArcLengthFilter
//   3) 把输出加入场景，用 arc_length 作为标量场着色显示（窗口可直接旋转/缩放查看）
//   4) 控制台打印首/末点弧长与解析总长，便于和 VTK vtkAppendArcLength 对照
//
// 需要图形环境；不需要任何模型文件。
// ============================================================================
#include <AppendArcLength/iGameAppendArcLengthFilter.h>
#include <iGameAttributeSet.h>
#include <iGameCellType.h>
#include <iGameDrawObject.h>
#include <iGameInteractor.h>
#include <iGamePointSet.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameUnstructuredMesh.h>

#include <cmath>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

int main() {
    constexpr double kPi = 3.14159265358979323846;
    const int segments = 200;
    const double radius = 10.0;
    const double height = 20.0;
    const int turns = 2;

    /* ---- 1. 生成螺旋折线 ---- */
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->SetName("helix");
    for (int i = 0; i <= segments; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(segments);
        const double angle = 2.0 * kPi * turns * t;
        mesh->AddPoint(iGame::Point{static_cast<float>(radius * std::cos(angle)),
                                    static_cast<float>(radius * std::sin(angle)),
                                    static_cast<float>(height * t)});
    }
    std::vector<igIndex> ids(static_cast<size_t>(segments) + 1);
    std::iota(ids.begin(), ids.end(), 0);
    mesh->AddCell(ids.data(), static_cast<int>(ids.size()), iGame::IG_POLY_LINE);

    /* ---- 2. 执行过滤器 ---- */
    auto filter = iGame::AppendArcLengthFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) {
        std::cout << "[AppendArcLength] ERROR: " << filter->GetMessage() << "\n";
        return 1;
    }
    iGame::DataObject::Pointer output = filter->GetOutput(0);
    if (output == nullptr) {
        std::cout << "[AppendArcLength] Output ERROR!\n";
        return 1;
    }

    /* ---- 3. 打印弧长并与解析解对照 ---- */
    int arcIndex = -1;
    iGame::ArrayObject::Pointer arcArray;
    auto attributes = output->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto& attribute = attributes->GetElement(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) { continue; }
        if (attribute.pointer->GetName() == "arc_length") {
            arcArray = attribute.pointer;
            arcIndex = static_cast<int>(i);
        }
    }
    const double analytic = std::sqrt(std::pow(2.0 * kPi * radius * turns, 2.0) + height * height);
    if (arcArray) {
        const IGsize count = arcArray->GetNumberOfValues();
        const double first = arcArray->GetElementValue(0, 0);
        const double last = arcArray->GetElementValue(count - 1, 0);
        std::cout << "[AppendArcLength] points=" << mesh->GetNumberOfPoints() << " arc_length dims="
                  << arcArray->GetDimension() << "\n";
        std::cout << "[AppendArcLength] first=" << first << " last=" << last << " analytic=" << analytic
                  << " diff=" << std::fabs(last - analytic) << "\n";
    } else {
        std::cout << "[AppendArcLength] arc_length array NOT FOUND!\n";
    }

    /* ---- 4. 场景 + 标量场着色 + 窗口 ---- */
    auto scene = iGame::Scene::New();
    scene->AddModel(output);
    auto drawObject = iGame::DynamicCast<iGame::DrawObject>(output);
    if (drawObject) {
        drawObject->SetViewStyle(IG_WIREFRAME | IG_SURFACE);
        if (arcIndex >= 0) {
            drawObject->ViewCloudPicture(scene, arcIndex, -1); // 用 arc_length 着色
        }
    }

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1280, 800);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
    return 0;
}
