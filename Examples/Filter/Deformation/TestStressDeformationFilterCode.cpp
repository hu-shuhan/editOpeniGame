#include <Deformation/iGameStressDeformationFilterCode.h>
#include "Core/Interactor/iGameInteractor.h"
#include "Core/RenderWindow/iGameMultiRenderWindowManager.h"
#include "Core/RenderWindow/iGameRenderWindow.h"
#include "iGameFileIO.h"
#include <iostream>
int main(){
    iGame::StressDeformationCodeFilter::Pointer filter = iGame::StressDeformationCodeFilter::New();
    //    const std::string fileName = "./Models/sukong_Step-1_2.vtu";
    // Any Model with Vector Attribute.
    const std::string fileName = "C:\\Users\\m_ky\\Desktop\\Resource\\xml\\pvd\\redsea\\1/1_0_30.vtu";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);

    obj->GetDeformationData()->SetAttributeName("UVW");
    filter->SetInput(obj);
    filter->CalculateIdealDSF();
    //    filter->SetScaleFactorX(0.0);
    //    filter->SetScaleFactorY(0.0);
    //    filter->SetScaleFactorZ(0.0);
    filter->Execute();

    auto res = filter->GetOutput(0);
//    std::cout << DynamicCast<iGame::PointSet>(res)->GetNumberOfPoints() << std::endl;
    auto pointset_raw = DynamicCast<iGame::PointSet>(obj);
    auto attribute_set = pointset_raw->GetAttributeSet()->GetAttribute(obj->GetDeformationData()->GetDeformationAttributeName()).pointer;
    auto pointset_new = DynamicCast<iGame::PointSet>(res);
    for(int i= 0, j = 0; i < 10; i ++, j += 3){
        std::cout << "==================" << i << std::endl;

        //  Deformation value
        std::cout << pointset_raw->GetDeformationData()->GetScaleFactorX() * attribute_set->GetValue(j + 0) << ' '
                 << pointset_raw->GetDeformationData()->GetScaleFactorY() * attribute_set->GetValue(j + 1) << ' '
                 << pointset_raw->GetDeformationData()->GetScaleFactorZ() * attribute_set->GetValue(j + 2) << std:: endl;
        // Before Deformation Point Position
        std:: cout << pointset_raw->GetPoint(i)[0] << ' ' << pointset_raw->GetPoint(i)[1] << ' ' << pointset_raw->GetPoint(i)[2] << '\n';
        // After Deformation Point Position
        std:: cout << pointset_new->GetPoint(i)[0] << ' ' << pointset_new->GetPoint(i)[1] << ' ' << pointset_new->GetPoint(i)[2] << '\n';

    }
//    iGame::DynamicCast<iGame::DrawObject>(res)->AddViewStyle(IG_WIREFRAME);
//    iGame::DynamicCast<iGame::DrawObject>(res)->AddViewStyle(IG_POINTS);
//    iGame::DynamicCast<iGame::DrawObject>(res)->AddViewStyle(IG_SURFACE);
//    iGame::DynamicCast<iGame::DrawObject>(res)->ConvertToDrawableData();
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    if (obj != nullptr) {
        scene->AddModel(res);
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