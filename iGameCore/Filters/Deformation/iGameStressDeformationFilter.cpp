//
// Created by m_ky on 2024/10/16.
//

/**
 * @class   iGameStressDeformationFilter
 * @brief   iGameStressDeformationFilter's brief
 */

#include "iGameStressDeformationFilter.h"
#include "iGamePointSet.h"
#include <iGameSceneManager.h>

iGame::StressDeformationFilter::StressDeformationFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool iGame::StressDeformationFilter::Execute() {
    auto dataObject = this->GetInput(0);
    if(nullptr == dataObject) {
        return false;
    }
    if(dataObject->GetDeformationData()->m_enable_dsf){
        std::string deform_var = dataObject->GetDeformationData()->m_deformation_attribute_name;
        float deform_x = dataObject->GetDeformationData()->m_deformation_scale_factor_x;
        float deform_y = dataObject->GetDeformationData()->m_deformation_scale_factor_y;
        float deform_z = dataObject->GetDeformationData()->m_deformation_scale_factor_z;
//        std::cout << deform_var << ' ' << deform_x << ' ' << deform_y << ' '<< deform_z << ' ' << '\n';
        if(dataObject->HasSubDataObject()){
            for(auto it = dataObject->SubDataObjectIteratorBegin(); it != dataObject->SubDataObjectIteratorEnd(); ++ it){
                auto pointset = DynamicCast<iGame::PointSet>(it->second);
                if(nullptr == pointset) return false;
                auto uset = pointset->GetAttributeSet()->GetAttribute(deform_var, IG_SCALAR).pointer;
                for(int i = 0, j = 0; i < pointset->GetNumberOfPoints(); i ++, j += 3){
                    pointset->SetPoint(i, pointset->GetPoint(i) + (Vector3d
                            (deform_x * uset->GetValue(j + 0),
                             deform_y * uset->GetValue(j + 1),
                             deform_z * uset->GetValue(j + 2))));
                }
                pointset->Modified();
            }
        }
        {
            auto pointset = DynamicCast<iGame::PointSet>(dataObject);
            if(nullptr != pointset){
                auto pset = pointset->GetRenderPoints();
                auto uset = pointset->GetAttributeSet()->GetAttribute(deform_var, IG_SCALAR).pointer;
//                std::cout << "Points : " << pointset->GetPoint(599999)[0] << '\n';
                for(int i = 0, j = 0; i < pointset->GetNumberOfPoints(); i ++, j += 3){
//                    std::cout << uset->GetValue(j + 0) << ' ' << uset->GetValue(j + 1) << ' ' <<uset->GetValue(j + 2) << '\n' ;
                    pset->SetValue(j + 0, pointset->GetPoint(i)[0] + deform_x * uset->GetValue(j + 0));
                    pset->SetValue(j + 1, pointset->GetPoint(i)[1] + deform_y * uset->GetValue(j + 1));
                    pset->SetValue(j + 2, pointset->GetPoint(i)[2] + deform_z * uset->GetValue(j + 2));
//                    pointset->SetPoint(i, pointset->GetPoint(i) + (Vector3d
//                          (deform_x * uset->GetValue(j + 0),
//                           deform_y * uset->GetValue(j + 1),
//                           deform_z * uset->GetValue(j + 2))));
                }
//                pointset->Modified();
                pset->Modified();
            }
        }
    }
    iGame::SceneManager::Instance()->GetCurrentScene()->Update();

    return true;
}

