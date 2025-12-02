#include "Core/Interactor/iGameInteractor.h"
#include "Core/RenderWindow/iGameMultiRenderWindowManager.h"
#include "Core/RenderWindow/iGameRenderWindow.h"
#include "Core/iGameScene.h"
#include "iGameFileIO.h"
#include <iostream>

#include "Deformation/iGameStressDeformationFilter.h"
int main() {
    iGame::StressDeformationFilter::Pointer filter = iGame::StressDeformationFilter::New();
    const std::string fileName = "./Models/sukong_Step-1_2.vtu";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);

    //    obj->GetDeformationData()->SetAttributeName("U");
    //    DynamicCast<iGame::DrawObject>(obj)->ConvertToDrawableData();
    //    filter->SetInput(obj);
    //    filter->CalculateIdealDSF();
    //    filter->Execute();
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    if (obj != nullptr) {
        scene->AddModel(obj);
    } else {
        std::cout << "error\n";
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
