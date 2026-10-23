// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
// ============================================================================
// Histogram 可视化测试（OpenGL 窗口；写法与 Examples/Filter/Convert/TestResampleToImage.cpp 一致）
//
//   1) 读取 Models/Convert_Quad_Bicycle.vtk（自行车模型；也可自动定位 Examples/Models 下的同一文件）
//   2) 对点数组 test_1（3 分量，取分量 0）做 30 bin 直方图（与 ParaView 6.2 / VTK vtkExtractHistogram 对照过）
//   3) 3D 视图显示模型并用被统计的数组着色
//   4) 用 Scene::GetPainter2D() 在窗口右上角叠加紫色（#984EA3）柱状图 —— 对应 ParaView
//      运行 Histogram 后在模型上出现的那张图；控制台同时打印 bin_extents / bin_values
//
// 需要图形环境。运行时可加 --bins <n> 调整分箱数。
// ============================================================================
#include <Histogram/iGameHistogramFilter.h>
#include <iGameAttributeSet.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameHistogramData.h>
#include <iGameInteractor.h>
#include <iGamePainter2D.h>
#include <iGamePen.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

/// 依次在「当前工作目录 / exe 同目录 / Examples/Models / 仓库相对路径」里定位模型。
std::string ResolveModelPath(const char* executable, const std::string& relative) {
    std::vector<std::string> candidates;
    candidates.push_back(relative);
    candidates.push_back("Examples/" + relative);
    candidates.push_back("../Examples/" + relative);
    candidates.push_back("../../Examples/" + relative);
    if (executable != nullptr) {
        const std::string exePath(executable);
        const size_t separator = exePath.find_last_of("/\\");
        if (separator != std::string::npos) {
            candidates.push_back(exePath.substr(0, separator + 1) + relative);
        }
    }
    for (const auto& candidate : candidates) {
        std::ifstream file(candidate);
        if (file.good()) { return candidate; }
    }
    return candidates.front();
}

/// 找名为 `name` 的属性下标（优先指定附着位置，找不到返回 -1）。
int FindAttribute(iGame::DataObject::Pointer object, const std::string& name, IGenum attachmentType,
                  bool requireAttachment) {
    auto attributeSet = object->GetAttributeSet();
    if (!attributeSet) { return -1; }
    for (IGsize i = 0; i < attributeSet->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributeSet->GetAttribute(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) { continue; }
        if (attribute.pointer->GetName() != name) { continue; }
        if (requireAttachment && attribute.attachmentType != attachmentType) { continue; }
        return static_cast<int>(i);
    }
    return -1;
}

} // namespace

int main(int argc, char** argv) {
    int bins = 30;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--bins") { bins = std::max(2, std::atoi(argv[i + 1])); }
    }

    /* ---- 1. 读取模型 ---- */
    const std::string modelPath = ResolveModelPath(argc > 0 ? argv[0] : nullptr, "Models/Convert_Quad_Bicycle.vtk");
    iGame::DataObject::Pointer model = iGame::FileIO::ReadFile(modelPath);
    if (model == nullptr) {
        std::cout << "[Histogram] Read ERROR: " << modelPath << "\n";
        return 1;
    }
    std::cout << "[Histogram] model=" << modelPath
              << " attributes=" << model->GetAttributeSet()->GetNumberOfAttributes() << "\n";

    /* ---- 2. 执行直方图 ---- */
    const int arrayIndex = FindAttribute(model, "test_1", IG_POINT, true);
    const int effectiveIndex = arrayIndex >= 0 ? arrayIndex : model->GetAttributeIndex();
    auto filter = iGame::HistogramFilter::New();
    filter->SetInput(model);
    filter->SetAttributeIndex(effectiveIndex);
    filter->SetNumberOfBins(bins);
    filter->SetComponent(0);
    if (!filter->Execute()) {
        std::cout << "[Histogram] ERROR: " << filter->GetMessage() << "\n";
        return 1;
    }
    auto histogram = iGame::DynamicCast<iGame::HistogramData>(filter->GetOutput(0));
    if (histogram == nullptr) {
        std::cout << "[Histogram] Output ERROR!\n";
        return 1;
    }

    /* ---- 3. 控制台打印 bin_extents / bin_values ---- */
    const int numberOfBins = histogram->GetNumberOfBins();
    auto extents = histogram->GetBinExtents();
    auto values = histogram->GetBinValues();
    double maxCount = 0.0;
    std::cout << "[Histogram] array=" << (arrayIndex >= 0 ? "test_1 (point data, component 0)" : "current attribute")
              << " bins=" << numberOfBins << " range=[" << histogram->GetBinMinimum() << ", "
              << histogram->GetBinMaximum() << "]\n";
    for (int i = 0; i < numberOfBins; ++i) {
        const double count = values->GetElementValue(i, 0);
        maxCount = std::max(maxCount, count);
        std::cout << "[Histogram] bin " << i << " extent=" << extents->GetElementValue(i, 0) << " count=" << count
                  << "\n";
    }

    /* ---- 4. 场景：模型 + 被统计数组着色 ---- */
    auto scene = iGame::Scene::New();
    scene->AddModel(model);
    auto drawObject = iGame::DynamicCast<iGame::DrawObject>(model);
    if (drawObject) {
        drawObject->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
        if (effectiveIndex >= 0) { drawObject->ViewCloudPicture(scene, effectiveIndex, -1); }
    }

    /* ---- 5. 2D 叠加紫色直方图（ParaView 图表 Set1 紫 #984EA3）---- */
    const unsigned int windowWidth = 1280;
    const unsigned int windowHeight = 800;
    const unsigned int margin = 20;
    const unsigned int boxWidth = 440;
    const unsigned int boxHeight = 260;
    const unsigned int boxLeft = windowWidth - margin - boxWidth;
    const unsigned int boxBottom = windowHeight - margin - boxHeight;

    auto painter = scene->GetPainter2D();
    if (painter && numberOfBins > 0 && maxCount > 0.0) {
        // 背板
        painter->SetBrush(0x12, 0x12, 0x12);
        painter->SetPen(0x70, 0x70, 0x70);
        painter->DrawRect(iGame::Vector2ui{boxLeft, boxBottom},
                          iGame::Vector2ui{boxLeft + boxWidth, boxBottom + boxHeight});

        // 柱子
        const double barWidth = static_cast<double>(boxWidth - 2) / numberOfBins;
        painter->SetPen(iGame::Pen::Style::NoPen);
        painter->SetBrush(0x98, 0x4E, 0xA3);
        for (int i = 0; i < numberOfBins; ++i) {
            const double count = values->GetElementValue(i, 0);
            const double barHeight = (count / maxCount) * (boxHeight - 2);
            const unsigned int x0 = boxLeft + static_cast<unsigned int>(i * barWidth) + 1;
            const unsigned int x1 = boxLeft + static_cast<unsigned int>((i + 1) * barWidth);
            const unsigned int y1 = boxBottom + 1 + static_cast<unsigned int>(barHeight);
            if (x1 > x0 && y1 > boxBottom + 1) {
                painter->DrawRect(iGame::Vector2ui{x0, boxBottom + 1}, iGame::Vector2ui{x1, y1});
            }
        }

        // 基线
        painter->SetPen(0xC8, 0xC8, 0xC8);
        painter->SetBrush(iGame::Brush::Style::NoBrush);
        painter->DrawLine(iGame::Vector2ui{boxLeft + 1, boxBottom + 1},
                          iGame::Vector2ui{boxLeft + boxWidth - 1, boxBottom + 1});
    }

    /* ---- 6. 窗口 ---- */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(static_cast<int>(windowWidth), static_cast<int>(windowHeight));
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
    return 0;
}
