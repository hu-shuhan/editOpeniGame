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
                auto render_pos_set = pointset->GetRenderPoints();
                auto pointMap = pointset->GetPointMap();
                auto attribute_set = pointset->GetAttributeSet()->GetAttribute(deform_var, IG_SCALAR).pointer;
                for(int i = 0, j = 0; i < pointMap->GetNumberOfValues(); i ++, j += 3){
                    int new_idx = pointMap->GetValue(i);
                    if(new_idx == -1) continue;
                    int k = new_idx * 3;
                    render_pos_set->SetValue(k + 0, pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0));
                    render_pos_set->SetValue(k + 1, pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1));
                    render_pos_set->SetValue(k + 2, pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
                }
                render_pos_set->Modified();
            }
        }
    }
    iGame::SceneManager::Instance()->GetCurrentScene()->Update();

    return true;
}

