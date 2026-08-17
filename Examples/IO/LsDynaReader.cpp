#include <LsDyna/iGameLsDynaReader.h>
#include <iGameDrawObject.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iostream>

// Hardcoded attribute name to visualize (edit as needed).
// LS-DYNA d3plot 转换后的字段名取决于 lsdyna_to_pvd_converter 的输出，
// 若找不到该字段，程序会打印所有可用字段，请按打印结果修改此值。
static const std::string kAttributeName = "Displacement";

// Find the attribute index by its name, return -1 if not found
static int findAttributeIndex(iGame::DrawObject* drawObj, const std::string& name) {
    if (!drawObj || !drawObj->GetAttributeSet()) { return -1; }
    auto attrs = drawObj->GetAttributeSet()->GetAllAttributes();
    for (int i = 0; i < attrs->GetNumberOfElements(); ++i) {
        if (attrs->GetElement(i).pointer->GetName() == name) { return i; }
    }
    return -1;
}

// Print all available attribute names
static void listAttributes(iGame::DrawObject* drawObj) {
    if (!drawObj || !drawObj->GetAttributeSet()) { return; }
    auto attrs = drawObj->GetAttributeSet()->GetAllAttributes();
    for (int i = 0; i < attrs->GetNumberOfElements(); ++i) {
        std::cout << "  [" << i << "] " << attrs->GetElement(i).pointer->GetName() << "\n";
    }
}

int main() {
    auto lsDynaReader = iGame::LsDynaReader::New();
    auto scene = iGame::Scene::New();

    // 默认数据路径：把 LS-DYNA d3plot 文件族（d3plot, d3plot01, d3plot02, ...）
    // 放到 Examples/Models 下（或在下方改成你自己的绝对路径）。
    // 也可参考 test/Format Validation Test/lsdyna/Bolt_B_Explicit/ 中的测试数据。
    std::string filePath = "./Models/Bolt_B_Explicit/d3plot";

    lsDynaReader->SetFilePath(filePath);
    lsDynaReader->Execute();
    auto obj = lsDynaReader->GetOutput(0);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
    } else {
        scene->AddModel(obj);

        auto drawObj = DynamicCast<iGame::DrawObject>(obj);
        if (drawObj) {
            int attrIndex = findAttributeIndex(drawObj.GetPointer(), kAttributeName);
            if (attrIndex >= 0) {
                drawObj->ViewCloudPicture(scene.GetPointer(), attrIndex, -1);
                std::cout << "Visualizing attribute: " << kAttributeName
                          << " (index " << attrIndex << ")\n";
            } else {
                std::cout << "Attribute not found: " << kAttributeName
                          << ". Available attributes:\n";
                listAttributes(drawObj.GetPointer());
            }
        }
    }

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
