#include "iGameDataObject.h"
#include "iGameDataObject.h"
#include "iGameDataObject.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN
DataObject::Pointer DataObject::CreateDataObject(IGenum type) {
    switch (type) {
        case IG_DATA_OBJECT:
            return DataObject::New();
        case IG_POINT_SET:
            return PointSet::New();
        case IG_SURFACE_MESH:
            return SurfaceMesh::New();
        case IG_VOLUME_MESH:
            return VolumeMesh::New();
        case IG_STRUCTURED_MESH:
            return StructuredMesh::New();
        case IG_UNSTRUCTURED_MESH:
            return UnstructuredMesh::New();
        default:
            return nullptr;
    }
}

DataObject::Pointer DataObject::GetSubDataObject(DataObjectId id) {
    if (m_SubDataObjectsHelper == nullptr) { return nullptr; }
    return m_SubDataObjectsHelper->GetSubDataObject(id);
}

DataObjectId DataObject::AddSubDataObject(DataObject::Pointer obj) {
    if (m_SubDataObjectsHelper == nullptr) { m_SubDataObjectsHelper = SubDataObjectsHelper::New(); }
    obj->SetParent(this);
    obj->SetColorMapper(this->GetColorMapper());

    if (obj->IsDrawable()) {
        auto drawObject = DynamicCast<DrawObject>(obj);
        drawObject->ConvertToDrawableData();
    }

    DataObjectId id = m_SubDataObjectsHelper->AddSubDataObject(obj);
    this->ReCollectSubDataObjectDataRange();
    this->UpdateSubDataObjectDataRange();
    return id;
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

DataObject::SubIterator DataObject::SubDataObjectIteratorBegin() { return m_SubDataObjectsHelper->Begin(); }

DataObject::SubConstIterator DataObject::SubDataObjectIteratorBegin() const { return m_SubDataObjectsHelper->Begin(); }

DataObject::SubIterator DataObject::SubDataObjectIteratorEnd() { return m_SubDataObjectsHelper->End(); }

DataObject* DataObject::FindParent() {
    if (m_Parent != nullptr) { return m_Parent->FindParent(); }
    return this;
}

IGsize DataObject::GetRealMemorySize() {
    IGsize res = 0;
    if (m_Attributes) res += m_Attributes->GetRealMemorySize();
    if (this->HasSubDataObject()) {
        for (auto it = m_SubDataObjectsHelper->Begin(); it != m_SubDataObjectsHelper->End(); ++it) {
            res += it->second->GetRealMemorySize();
        }
    }
    return res;
}

DataObject::SubConstIterator DataObject::SubDataObjectIteratorEnd() const { return m_SubDataObjectsHelper->End(); }

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
void DataObject::SetAttributeIndex(int index) { this->m_AttributeIndex = index; }

int DataObject::GetAttributeDimension() { return this->m_AttributeDimension; }


StreamingData::Pointer DataObject::GetTimeFrames() {
    if (m_TimeFrames == nullptr) m_TimeFrames = StreamingData::New();
    return m_TimeFrames;
}

DeformationData::Pointer DataObject::GetDeformationData() {
    if (nullptr == m_DeformationData) { m_DeformationData = DeformationData::New(); }
    return m_DeformationData;
}

bool DataObject::UpdateSubDataObjectDataRange() {
    /* Update SubDataObject's DataRange to Global DataRange and ReConvert Drawable data. */
    if (m_SubDataObjectsHelper == nullptr) return false;
    auto attributes = this->GetAttributeSet()->GetAllAttributes();
    for (auto it = SubDataObjectIteratorBegin(); it != SubDataObjectIteratorEnd(); ++it) {
        if (!it->second->IsDrawable()) continue;
        const auto& obj = DynamicCast<DrawObject>(it->second);
        if (!obj) continue;
        const auto& display_obj = obj->GetRenderableObject();

        auto subAttrSet = obj->GetAttributeSet();
        auto dispAttrSet = display_obj != nullptr ? display_obj->GetAttributeSet() : nullptr;
        const size_t subCount = subAttrSet ? subAttrSet->GetNumberOfAttributes() : 0;
        const size_t dispCount = dispAttrSet ? dispAttrSet->GetNumberOfAttributes() : 0;

        for (int i = 0; i < attributes->GetNumberOfElements(); i++) {
            auto& par = attributes->GetElement(i);
            if (!par.pointer) continue;
            if (par.dataRange == nullptr || par.dataRange->GetMTime() < par.pointer->GetMTime()) {
                par.UpdateAllDataRange();
            }
            // GetAttribute 不做边界检查且此处是写操作，越界会破坏堆内存，

            if (static_cast<size_t>(i) < subCount) {
                subAttrSet->GetAttribute(i).dataRange = par.GetDataRange();
            }

            /* Process Display mesh's DataRange. */
            if (static_cast<size_t>(i) < dispCount) {
                dispAttrSet->GetAttribute(i).dataRange = par.GetDataRange();
            }
        }
        obj->ConvertToDrawableData();
    }
    return true;
}

bool DataObject::ReCollectSubDataObjectDataRange() {
    /* Collect SubDataObject's DataRange to Update Father's DataObject's DataRange. */
    if (m_SubDataObjectsHelper == nullptr) return false;
    auto attributes = this->GetAttributeSet()->GetAllAttributes();
    for (IGsize k = 0; k < attributes->GetNumberOfElements(); k++) {
        double dataRange_max[64]{DBL_MIN}, dataRange_min[64]{DBL_MAX};
        auto par_attr = attributes->GetElement(k);
        if (!par_attr.pointer) continue;
        int dim = par_attr.pointer->GetDimension();
        bool anyCollected = false;
        for (auto it = SubDataObjectIteratorBegin(); it != SubDataObjectIteratorEnd(); ++it) {
            if (!it->second->IsDrawable()) continue;
            const auto& obj = DynamicCast<DrawObject>(it->second);
            if (!obj) continue;
            auto subAttrSet = obj->GetAttributeSet();

            if (!subAttrSet || static_cast<size_t>(k) >= subAttrSet->GetNumberOfAttributes()) continue;
            auto& subAttribute = subAttrSet->GetAttribute(k);
            if (!subAttribute.pointer) continue;
            subAttribute.UpdateAllDataRange();
            const auto& ScalarDataRange = subAttribute.GetDataRange();
            if (!ScalarDataRange) continue;
            const int subDim = subAttribute.pointer->GetDimension();
            for (int j = 0; j < subDim + 1 && j < 64; j++) {
                dataRange_min[j] = std::min(dataRange_min[j], ScalarDataRange->GetValue(2 * j + 0));
                dataRange_max[j] = std::max(dataRange_max[j], ScalarDataRange->GetValue(2 * j + 1));
            }
            anyCollected = true;
        }
        if (!anyCollected) continue; // 没有任何子对象提供该属性，保留父容器原值域
        auto  parent_dataRange = par_attr.GetDataRange();
        if (!parent_dataRange) continue;
        parent_dataRange->SetDimension(2);
        parent_dataRange->Resize(dim + 1);
        for (int j = 0; j < dim + 1; j++) { parent_dataRange->SetElement(j, {dataRange_min[j], dataRange_max[j]}); }
    }

    return true;
}

void DataObject::SetBlockMapping(IntArray::Pointer p) {
    if (m_BlockMappingAttrIndex >= 0) {
        m_Attributes->DeleteAttribute(m_BlockMappingAttrIndex);
        m_BlockMappingAttrIndex = -1;
    }
    if (p == nullptr) return;
    m_BlockMappingAttrIndex = static_cast<int>(
        m_Attributes->AddAttribute(IG_BLOCK_MAPPING, IG_CELL, p));
}

IntArray* DataObject::GetBlockMapping() {
    if (m_BlockMappingAttrIndex < 0) return nullptr;
    return DynamicCast<IntArray>(m_Attributes->GetAttribute(m_BlockMappingAttrIndex).pointer);
}

bool DataObject::HasBlockMapping() const {
    return m_BlockMappingAttrIndex >= 0;
}

void DataObject::SetAttributeSet(AttributeSet::Pointer p) {
    if (p != m_Attributes) {
        m_Attributes = p;
        m_Attributes->m_DataObject = this;
        m_AttributeHelper->Modified();
    }
}

const BoundingBox& DataObject::GetBoundingBox() {
    ComputeBoundingBox();
    if (this->HasSubDataObject()) {
        for (auto it = SubDataObjectIteratorBegin(); it != SubDataObjectIteratorEnd(); ++it)
            m_Bounding.add(it->second->GetBoundingBox());
    }
    return m_Bounding;
}

void DataObject::UpdateAnimation(int keyframe_idx) {
    if (this->GetTimeFrames() == nullptr || this->GetTimeFrames()->GetTimeNum() <= keyframe_idx) return;
    auto timeFrameType = this->GetTimeFrames()->GetTargetFrameType(keyframe_idx);
    auto timeFrameData = this->GetTimeFrames()->GetTargetTimeFrameData(keyframe_idx);
    if (timeFrameType == StreamingType::MultiSubFiles) {
        this->ClearSubDataObject();

        for (auto& subObj: timeFrameData) {
            auto subDataObj = DynamicCast<iGame::DrawObject>(subObj);
            if (subDataObj) {
//                subDataObj->SetShellRenderingOption(false);
                this->AddSubDataObject(subDataObj);
            }
        }
        if (this->IsDrawable()) DynamicCast<iGame::DrawObject>(this)->ConvertToDrawableData();
    } else if (timeFrameType == StreamingType::SingleFieldAttributes) {
        auto attributeSet = DynamicCast<iGame::AttributeSet>(timeFrameData[0]);
        if (attributeSet) {
            this->SetAttributeSet(attributeSet);
//            this->GetAttributeSet()->GetAllAttributes()
            this->Modified();
//            if (this->IsDrawable()) {
//                DynamicCast<iGame::DrawObject>(this)->ConvertToDrawableData();
//            }
        }
    }
    this->ReCollectSubDataObjectDataRange();
    this->UpdateSubDataObjectDataRange();
}

IGAME_NAMESPACE_END