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
    if(dataObject->GetDeformationData()->GetAutoComputeStatus()) {
        CalculateIdealDSF();
    }
    const std::string& deform_var = dataObject->GetDeformationData()->GetDeformationAttributeName();
    float deform_x = dataObject->GetDeformationData()->GetScaleFactorX();
    float deform_y = dataObject->GetDeformationData()->GetScaleFactorY();
    float deform_z = dataObject->GetDeformationData()->GetScaleFactorZ();
//        std::cout << deform_var << ' ' << deform_x << ' ' << deform_y << ' '<< deform_z << ' ' << '\n';

    /* Process SubDataObject's Deformation. */
    if(dataObject->HasSubDataObject()){
        for(auto it = dataObject->SubDataObjectIteratorBegin(); it != dataObject->SubDataObjectIteratorEnd(); ++ it){
            auto pointset = DynamicCast<iGame::PointSet>(it->second);
            ArrayObject::Pointer attribute_set{nullptr};
            if(pointset == nullptr || (attribute_set = pointset->GetAttributeSet()->GetAttribute(deform_var).pointer) ==
                                              nullptr ) return false;

            auto render_pos_set = pointset->GetRenderPoints();
            auto pointMap = pointset->GetPointMap();
            /* Not process Model Geometry Surface Filter, means that the points rendered are raw points
             * So we don't need to use pointMap.*/
            if(pointMap == nullptr && render_pos_set->GetNumberOfValues() == attribute_set->GetNumberOfValues()){
                if(pointset->GetPoints()->ConvertToArray() == render_pos_set){
                    FloatArray::Pointer newCopy = FloatArray::New();
                    newCopy->DeepCopy(render_pos_set);
                    render_pos_set = newCopy;
                    pointset->SetRenderPoints(render_pos_set);
                }

                for(int i = 0, j = 0; i < pointset->GetNumberOfPoints(); i ++, j += 3){
                    render_pos_set->SetValue(j + 0, pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0));
                    render_pos_set->SetValue(j + 1, pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1));
                    render_pos_set->SetValue(j + 2, pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
                }
            }
            /* Process the Model Geometry Surface Filter, means that the points renderer are not raw points
             * So we need to use pointMap.*/
            else if(pointMap != nullptr && render_pos_set->GetNumberOfValues() != attribute_set->GetNumberOfValues()){
                if(render_pos_set->GetNumberOfValues() == 0){
                    for(int i = 0, j = 0; i < pointMap->GetNumberOfValues(); i ++, j += 3){
                        int new_idx = pointMap->GetValue(i);
                        if(new_idx == -1) continue;
                        render_pos_set->AddValue(pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0));
                        render_pos_set->AddValue(pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1));
                        render_pos_set->AddValue(pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
                    }
                } else {
                    for(int i = 0, j = 0; i < pointMap->GetNumberOfValues(); i ++, j += 3){
                        int new_idx = pointMap->GetValue(i);
                        if(new_idx == -1) continue;
                        int k = new_idx * 3;
                        render_pos_set->SetValue(k + 0, pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0));
                        render_pos_set->SetValue(k + 1, pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1));
                        render_pos_set->SetValue(k + 2, pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
                    }
                }

            }
            render_pos_set->Modified();
        }
    }
    /* Process DataObject itself 's Deformation. */
    {
        auto pointset = DynamicCast<iGame::PointSet>(dataObject);
        if(nullptr != pointset){
            auto render_pos_set = pointset->GetRenderPoints();
            auto pointMap = pointset->GetPointMap();
            ArrayObject::Pointer attribute_set{nullptr};
            if(pointset == nullptr || (attribute_set = pointset->GetAttributeSet()->GetAttribute(deform_var).pointer) ==
                                      nullptr ) return false;
            /* Not process Model Geometry Surface Filter, means that the points rendered are raw points
             * So we don't need to use pointMap.*/
            if(pointMap == nullptr && render_pos_set->GetNumberOfValues() == attribute_set->GetNumberOfValues()){
                if(pointset->GetPoints()->ConvertToArray() == render_pos_set){
                    FloatArray::Pointer newCopy = FloatArray::New();
                    newCopy->DeepCopy(render_pos_set);
                    render_pos_set = newCopy;
                    pointset->SetRenderPoints(render_pos_set);
                }
                for(int i = 0, j = 0; i < pointset->GetNumberOfPoints(); i ++, j += 3){
                    render_pos_set->SetValue(j + 0, pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0));
                    render_pos_set->SetValue(j + 1, pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1));
                    render_pos_set->SetValue(j + 2, pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
                }
            }
            /* Process the Model Geometry Surface Filter, means that the points renderer are not raw points
             * So we need to use pointMap.*/
            else if(pointMap != nullptr && render_pos_set->GetNumberOfValues() != attribute_set->GetNumberOfValues()){
                for(int i = 0, j = 0; i < pointMap->GetNumberOfValues(); i ++, j += 3){
                    int new_idx = pointMap->GetValue(i);
                    if(new_idx == -1) continue;
                    int k = new_idx * 3;
                    render_pos_set->SetValue(k + 0, pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0));
                    render_pos_set->SetValue(k + 1, pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1));
                    render_pos_set->SetValue(k + 2, pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
                }
            }
            render_pos_set->Modified();
        }
    }

    return true;
}

bool iGame::StressDeformationFilter::CalculateIdealDSF() {
    auto dataObject = this->GetInput(0);
    if(nullptr == dataObject) {
        return false;
    }
    const std::string& deform_var = dataObject->GetDeformationData()->GetDeformationAttributeName();
    dataObject->GetDeformationData()->SetAutoCompute(true);

    float U_max = FLT_MIN;
    if(dataObject->HasSubDataObject()){
        for(auto it = dataObject->SubDataObjectIteratorBegin(); it != dataObject->SubDataObjectIteratorEnd(); ++ it){
            auto uset = it->second->GetAttributeSet()->GetAttribute(deform_var).pointer;
            if(uset == nullptr) return false;
            for(int i = 0; i < uset->GetNumberOfValues(); i += 3){
                float x = uset->GetValue(i), y = uset->GetValue(i + 1), z = uset->GetValue(i + 2);
                U_max = std::max(U_max, std::sqrt(x * x + y * y + z * z));
            }
        }
    }

    auto attribute_set = dataObject->GetAttributeSet()->GetAttribute(deform_var).pointer;
    if(attribute_set == nullptr) return false;
    for(int i = 0; i < attribute_set->GetNumberOfValues(); i += 3){
        float x = attribute_set->GetValue(i), y = attribute_set->GetValue(i + 1), z = attribute_set->GetValue(i + 2);
        U_max = std::max(U_max, std::sqrt(x * x + y * y + z * z));
    }
    if(U_max == FLT_MIN || U_max == 0) return false;
    auto Ds = dataObject->GetBoundingBox().max - dataObject->GetBoundingBox().min;
    float D_max = std::cbrt(Ds[0] * Ds[1] * Ds[2]);
    std::cout << "max_offset : " << U_max << '\n';
    std::cout << "max_D : " << D_max << '\n';
    std::cout << "res : " << m_K_factor * D_max / U_max << '\n';
    dataObject->GetDeformationData()->SetScaleFactors(m_K_factor * D_max / U_max);
    return true;
}

