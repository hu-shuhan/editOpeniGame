//
// Created by m_ky on 2025/12/1.
//

/**
 * @class   iGameStressDeformationFilterCode
 * @brief   iGameStressDeformationFilterCode's brief
 */

#include "iGameStressDeformationFilterCode.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"

iGame::StressDeformationCodeFilter::StressDeformationCodeFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool iGame::StressDeformationCodeFilter::SetScaleFactorX(float sf_x) {
    auto dataObject = this->GetInput(0);
    if(dataObject == nullptr) {
        IGAME_CORE_WARN("[Deformation Filter] Not Setting Input DataObject");
        return false;
    }
    dataObject->GetDeformationData()->SetScaleFactorX(sf_x);
    if(dataObject->HasSubDataObject()){
        for(auto it = dataObject->SubDataObjectIteratorBegin(); it != dataObject->SubDataObjectIteratorEnd(); ++ it){
            it->second->GetDeformationData()->SetScaleFactorX(sf_x);
        }
    }
    return true;
}
bool iGame::StressDeformationCodeFilter::SetScaleFactorY(float sf_y) {
    auto dataObject = this->GetInput(0);
    if(dataObject == nullptr) {
        IGAME_CORE_WARN("[Deformation Filter] Not Setting Input DataObject");
        return false;
    }
    dataObject->GetDeformationData()->SetScaleFactorY(sf_y);
    if(dataObject->HasSubDataObject()){
        for(auto it = dataObject->SubDataObjectIteratorBegin(); it != dataObject->SubDataObjectIteratorEnd(); ++ it){
            it->second->GetDeformationData()->SetScaleFactorY(sf_y);
        }
    }
    return true;
}
bool iGame::StressDeformationCodeFilter::SetScaleFactorZ(float sf_z) {
    auto dataObject = this->GetInput(0);
    if(dataObject == nullptr) {
        IGAME_CORE_WARN("[Deformation Filter] Not Setting Input DataObject");
        return false;
    }
    dataObject->GetDeformationData()->SetScaleFactorZ(sf_z);
    if(dataObject->HasSubDataObject()){
        for(auto it = dataObject->SubDataObjectIteratorBegin(); it != dataObject->SubDataObjectIteratorEnd(); ++ it){
            it->second->GetDeformationData()->SetScaleFactorZ(sf_z);
        }
    }
    return true;
}
bool iGame::StressDeformationCodeFilter::Execute() {
    auto dataObject = this->GetInput(0);
    std::cout << "Processing=========\n";
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
    std::cout << deform_var << ' ' << deform_x << ' ' << deform_y << ' '<< deform_z << ' ' << '\n';

    auto outMesh = UnstructuredMesh::New();
    auto newPoints = Points::New();
    if (dataObject->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
        auto temp = DynamicCast<UnstructuredMesh>(dataObject);
        outMesh->SetCells(temp->GetCells(), temp->GetCellTypes());
        outMesh->SetAttributeSet(temp->GetAttributeSet());
        outMesh->SetName(temp->GetName());
        outMesh->SetPoints(newPoints);
    }
    auto pointset = DynamicCast<iGame::PointSet>(dataObject);
    if(pointset == nullptr) {
        IGAME_CORE_WARN("[Deformation Filter] Input DataObject Is NOT PointSet.");
        return false;
    }
    newPoints->DeepCopy(pointset->GetPoints());
    ArrayObject::Pointer attribute_set{nullptr};
    if(pointset == nullptr || (attribute_set = pointset->GetAttributeSet()->GetAttribute(deform_var).pointer) ==
                                       nullptr ){
        return false;
    }
    for(int i = 0, j = 0; i < newPoints->GetNumberOfPoints(); i ++, j += 3){
        newPoints->SetPoint(i,
                            newPoints->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0),
                            newPoints->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1),
                            newPoints->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
    }

    this->SetOutput(0, outMesh);
    return true;


    //    /* Process DataObject itself 's Deformation. */
    //    {
    //        auto pointset = DynamicCast<iGame::PointSet>(dataObject);
    //        if(nullptr != pointset){
    //            auto points = pointset->GetPoints();
    //            ArrayObject::Pointer attribute_set{nullptr};
    //            if(pointset == nullptr || (attribute_set = pointset->GetAttributeSet()->GetAttribute(deform_var).pointer) ==
    //                                               nullptr ){
    //                //                UpdateProgress(1.0);
    //                return false;
    //            }
    //            /* Not process Model Geometry Surface Filter, means that the points rendered are raw points
    //             * So we don't need to use pointMap.*/
    //            if(pointMap == nullptr && render_pos_set->GetNumberOfValues() == attribute_set->GetNumberOfValues()){
    //                if(pointset->GetPoints()->ConvertToArray() == render_pos_set){
    //                    FloatArray::Pointer newCopy = FloatArray::New();
    //                    newCopy->DeepCopy(render_pos_set);
    //                    render_pos_set = newCopy;
    //                    pointset->SetRenderPoints(render_pos_set);
    //                }
    //                int PointSize = pointset->GetNumberOfPoints();
    //                for(int i = 0, j = 0; i < pointset->GetNumberOfPoints(); i ++, j += 3){
    //                    //                    if(i % 100 == 0) UpdateProgress((double)i / PointSize);
    //                    render_pos_set->SetValue(j + 0, pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0));
    //                    render_pos_set->SetValue(j + 1, pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1));
    //                    render_pos_set->SetValue(j + 2, pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
    //                    newPoints->SetPoint(j / 3, pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0),
    //                                        pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1),
    //                                        pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
    //                }
    //            }
    //            /* Process the Model Geometry Surface Filter, means that the points renderer are not raw points
    //             * So we need to use pointMap.*/
    //            else if(pointMap != nullptr && render_pos_set->GetNumberOfValues() != attribute_set->GetNumberOfValues()){
    //                int displayPointSize = pointMap->GetNumberOfValues();
    //                render_pos_set->Resize(displayPointSize);
    //                for(int i = 0, j = 0; i < displayPointSize; i ++, j += 3){
    //                    //                    if(i % 100 == 0) UpdateProgress((double) i / displayPointSize);
    //                    int new_idx = pointMap->GetValue(i);
    //                    if(new_idx == -1) continue;
    //                    int k = new_idx * 3;
    //                    render_pos_set->SetValue(k + 0, pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0));
    //                    render_pos_set->SetValue(k + 1, pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1));
    //                    render_pos_set->SetValue(k + 2, pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
    //                    newPoints->SetPoint(k / 3, pointset->GetPoint(i)[0] + deform_x * attribute_set->GetValue(j + 0),
    //                                        pointset->GetPoint(i)[1] + deform_y * attribute_set->GetValue(j + 1),
    //                                        pointset->GetPoint(i)[2] + deform_z * attribute_set->GetValue(j + 2));
    //                }
    //            }
    //            render_pos_set->Modified();
    //        }
    //
    //    }
    //    UpdateProgress(1.0f);
    //    this->SetOutput(0, outMesh);
    //    return true;
}


bool iGame::StressDeformationCodeFilter::CalculateIdealDSF() {
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
    //    std::cout << "max_offset : " << U_max << '\n';
    //    std::cout << "max_D : " << D_max << '\n';
    //    std::cout << "res : " << m_K_factor * D_max / U_max << '\n';
    dataObject->GetDeformationData()->SetScaleFactors(m_K_factor * D_max / U_max);
    return true;

}
bool iGame::StressDeformationCodeFilter::SetDeformationAttributeName(std::string _deformation_attribute_name) {
    auto dataObject = this->GetInput(0);
    if(dataObject == nullptr) {
        IGAME_CORE_WARN("[Deformation Filter] Not Setting Input DataObject");
        return false;
    }
    dataObject->GetDeformationData()->SetAttributeName(_deformation_attribute_name);
    return true;
}


