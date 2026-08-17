#include <Ansys/iGameAnsysReader.h>
#include <iGameDrawObject.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iostream>

// Hardcoded attribute name to visualize (edit as needed)
static const std::string kAttributeName = "Temperature";

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
    auto ansysReader = iGame::AnsysReader::New();
    auto scene = iGame::Scene::New();
    std::string filePath = "./Models/housing_thermal.rth";
    ansysReader->SetFilePath(filePath);
    ansysReader->Execute();
    auto obj = ansysReader->GetOutput(0);
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
