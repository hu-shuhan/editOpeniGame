#include "iGameDataObject.h"
#include "iGameSurfaceMesh.h"

IGAME_NAMESPACE_BEGIN
DataObject::Pointer DataObject::CreateDataObject(IGenum type) {
    switch (type) {
        case IG_DATA_OBJECT:
            return DataObject::New();
        case IG_POINT_SET:
            return PointSet::New();
        case IG_SURFACE_MESH:
            return SurfaceMesh::New();
        default:
            return nullptr;
    }
}

DataObject::Pointer DataObject::GetSubDataObject(DataObjectId id) {
    if (m_SubDataObjectsHelper == nullptr) { return nullptr; }
    return m_SubDataObjectsHelper->GetSubDataObject(id);
}

DataObjectId DataObject::AddSubDataObject(DataObject::Pointer obj) {
    if (m_SubDataObjectsHelper == nullptr) {
        m_SubDataObjectsHelper = SubDataObjectsHelper::New();
    }
    obj->SetParent(this);
    obj->SetColorMapper(this->GetColorMapper());


    if (obj->IsDrawable()) {
        auto drawObject = DynamicCast<DrawObject>(obj);
        drawObject->ConvertToDrawableData();
    }

    return m_SubDataObjectsHelper->AddSubDataObject(obj);
}

void DataObject::RemoveSubDataObject(DataObjectId id) {
    if (m_SubDataObjectsHelper == nullptr) { return; }

    return m_SubDataObjectsHelper->RemoveSubDataObject(id);
}

void DataObject::ClearSubDataObject() {
    if (m_SubDataObjectsHelper == nullptr) { return; }

    return m_SubDataObjectsHelper->ClearSubDataObject();
}

bool DataObject::HasSubDataObject() noexcept {
    if (m_SubDataObjectsHelper == nullptr) { return false; }

    return m_SubDataObjectsHelper->HasSubDataObject();
}

int DataObject::GetNumberOfSubDataObjects() noexcept {
    if (m_SubDataObjectsHelper == nullptr) { return 0; }

    return m_SubDataObjectsHelper->GetNumberOfSubDataObjects();
}

DataObject::SubIterator DataObject::SubDataObjectIteratorBegin() {
    return m_SubDataObjectsHelper->Begin();
}

DataObject::SubConstIterator DataObject::SubDataObjectIteratorBegin() const {
    return m_SubDataObjectsHelper->Begin();
}

DataObject::SubIterator DataObject::SubDataObjectIteratorEnd() {
    return m_SubDataObjectsHelper->End();
}

DataObject* DataObject::FindParent() {
    if (m_Parent != nullptr) { return m_Parent->FindParent(); }
    return this;
}

DataObject::SubConstIterator DataObject::SubDataObjectIteratorEnd() const {
    return m_SubDataObjectsHelper->End();
}

DataObjectId DataObject::GetIncrementDataObjectId() {
    static DataObjectId globalDataObjectId = 0;
    return globalDataObjectId++;
}
void DataObject::SetParent(DataObject* parent) {
    if (m_Parent != parent) { m_Parent = parent; }
}

//void DataObject::Draw(Scene* scene) {
//    ProcessSubDataObjects(&DataObject::Draw, scene);
//}
//void DataObject::DrawPhase1(Scene* scene) {
//    ProcessSubDataObjects(&DataObject::DrawPhase1, scene);
//}
//void DataObject::DrawPhase2(Scene* scene) {
//    ProcessSubDataObjects(&DataObject::DrawPhase2, scene);
//}
//void DataObject::TestOcclusionResults(Scene* scene) {
//    ProcessSubDataObjects(&DataObject::TestOcclusionResults, scene);
//}

//void DataObject::ViewCloudPicture(Scene* scene, int index, int dimension) {
//    m_AttributeIndex = index;
//    m_AttributeDimension = dimension;
//    ProcessSubDataObjects(&DataObject::ViewCloudPicture, scene, index,
//                          dimension);
//}

//void DataObject::ViewCloudPictureOfModel(Scene* scene, int index,
//                                         int demension) {
//    auto* parent = FindParent();
//    if (parent != this) {
//        parent->ViewCloudPicture(scene, index, demension);
//    } else {
//        this->ViewCloudPicture(scene, index, demension);
//    }
//}

int DataObject::GetAttributeIndex() { return this->m_AttributeIndex; }

int DataObject::GetAttributeDimension() { return this->m_AttributeDimension; }



StreamingData::Pointer DataObject::GetTimeFrames() {
    if (m_TimeFrames == nullptr) m_TimeFrames = StreamingData::New();
    return m_TimeFrames;
}

DeformationData::Pointer DataObject::GetDeformationData() {
    if (nullptr == m_DeformationData) {
        m_DeformationData = DeformationData::New();
    }
    return m_DeformationData;
}

bool DataObject::UpdateSubDataObjectDataRange() {
    /* Update SubDataObject's DataRange to Global DataRange and ReConvert Drawable data. */
    if(m_SubDataObjectsHelper == nullptr) return false;
    auto attributes = this->GetAttributeSet()->GetAllAttributes();
    for(auto it = SubDataObjectIteratorBegin(); it != SubDataObjectIteratorEnd(); ++ it){
        if(!it->second->IsDrawable()) continue;
        const auto& obj = DynamicCast<DrawObject>(it->second);
        const auto& display_obj = obj->GetDisplayObject();

        for(int i = 0; i < attributes->GetNumberOfElements(); i ++){
            auto& par = attributes->GetElement(i);
            if(par.dataRange == nullptr || par.dataRange->GetMTime() < par.pointer->GetMTime()){
                par.UpdateAllDataRange();
            }
            obj->GetAttributeSet()->GetAttribute(i).dataRange = par.GetDataRange();

            /* Process Display mesh's DataRange. */
            if(display_obj != nullptr) display_obj->GetAttributeSet()->GetAttribute(i).dataRange = par.GetDataRange();
        }
        obj->ConvertToDrawableData();
    }
    return true;
}

bool DataObject::ReCollectSubDataObjectDataRange() {
    /* Update SubDataObject's DataRange to Global DataRange and ReConvert Drawable data. */
    if(m_SubDataObjectsHelper == nullptr) return false;
    auto attributes = this->GetAttributeSet()->GetAllAttributes();
    for(IGsize k = 0; k < attributes->GetNumberOfElements(); k ++){
        double dataRange_max[64]{DBL_MIN}, dataRange_min[64] {DBL_MAX};
        auto par_attr = attributes->GetElement(k);
        int dim = par_attr.pointer->GetDimension();
        for(auto it = SubDataObjectIteratorBegin(); it != SubDataObjectIteratorEnd(); ++ it){
                if(!it->second->IsDrawable()) continue;
                const auto& obj = DynamicCast<DrawObject>(it->second);
                const auto& display_obj = obj->GetDisplayObject();
                auto subAttribute = obj->GetAttributeSet()->GetAttribute(k);
                subAttribute.UpdateAllDataRange();
                const auto& ScalarDataRange = subAttribute.GetDataRange();
                for(int j = 0; j < subAttribute.pointer->GetDimension() + 1; j ++){
                    dataRange_min[j] = std::min(dataRange_min[j], ScalarDataRange->GetValue(2 * j + 0));
                    dataRange_max[j] = std::max(dataRange_max[j], ScalarDataRange->GetValue(2 * j + 1));
                }
        }
        DoubleArray::Pointer parent_dataRange = DoubleArray::New();
        parent_dataRange->SetDimension(2);
        parent_dataRange->Resize(dim + 1);
        for(int j = 0; j < dim + 1; j ++){
            parent_dataRange->SetElement(j, {dataRange_min[j], dataRange_max[j]});
        }
        par_attr.SetDataRange(parent_dataRange);
    }

    return true;
}

void DataObject::SetAttributeSet(AttributeSet::Pointer p) {
        m_Attributes = p;
        m_AttributeHelper->Modified();
}

void DataObject::UpdateAnimation(int keyframe_idx) {
    if(this->GetTimeFrames() == nullptr || this->GetTimeFrames()->GetTimeNum() <= keyframe_idx) return;
    auto timeFrameType = this->GetTimeFrames()->GetTargetFrameType(keyframe_idx);
    auto timeFrameData = this->GetTimeFrames()->GetTargetTimeFrameData(keyframe_idx);
    if(timeFrameType == StreamingType::MultiSubFiles){
        this->ClearSubDataObject();
        for(auto& subObj : timeFrameData){
            auto subDataObj = DynamicCast<iGame::DataObject>(subObj);
            if(subDataObj){
                this->AddSubDataObject(subDataObj);
            }
        }
    } else if(timeFrameType == StreamingType::SingleFieldAttributes){
        auto attributeSet = DynamicCast<iGame::AttributeSet>(timeFrameData[0]);
        if(attributeSet){
            this->SetAttributeSet(attributeSet);
            DynamicCast<iGame::PointSet>(this)->GetPoints()->Modified();
            if(this->IsDrawable()) DynamicCast<iGame::DrawObject>(this)->ConvertToDrawableData();
        }
    }
    this->ReCollectSubDataObjectDataRange();
    this->UpdateSubDataObjectDataRange();
}

IGAME_NAMESPACE_END