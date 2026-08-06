#include "Core/Interactor/iGameInteractor.h"
#include "Core/RenderWindow/iGameMultiRenderWindowManager.h"
#include "Core/RenderWindow/iGameRenderWindow.h"
#include "Core/iGameScene.h"
#include "iGameDrawObject.h"
#include "iGameFileIO.h"
#include <iostream>

#include "Deformation/iGameStressDeformationFilter.h"
int main() {
    iGame::StressDeformationFilter::Pointer filter = iGame::StressDeformationFilter::New();
    const std::string fileName = "./Models/sukong_Step-1_10.vtu";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << "Failed to read " << fileName << '\n';
        return 1;
    }

    auto drawObject = DynamicCast<iGame::DrawObject>(obj);
    if (drawObject == nullptr) {
        std::cerr << "The input object is not drawable.\n";
        return 1;
    }

    // The first conversion extracts the renderable SurfaceMesh.
    drawObject->ConvertToDrawableData();

    // The extracted SurfaceMesh owns a separate position array. Initialize it
    // before deformation so GetRenderPoints() does not return an empty array.
    auto renderableObject = drawObject->GetRenderableObject();
    if (renderableObject != nullptr && renderableObject != drawObject) {
        renderableObject->ConvertToDrawableData();
    }

    obj->GetDeformationData()->SetAttributeName("U");
    filter->SetInput(obj);
    if (!filter->CalculateIdealDSF()) {
        std::cerr << "Failed to calculate the deformation scale factor for attribute U.\n";
        return 1;
    }
    if (!filter->Execute()) {
        std::cerr << "Deformation failed.\n";
        return 1;
    }
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    scene->AddModel(obj);

    // Color by displacement magnitude so the selected field is visible.
    const int deformationAttribute = obj->GetAttributeSet()->GetAttributeIndex("U");
    if (deformationAttribute >= 0) {
        drawObject->ViewCloudPicture(scene, deformationAttribute, -1);
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
