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
            // 补充按名字同步：子对象/抽壳网格的属性顺序或数量可能与父容器不一致，
            // 按下标会漏掉（例如抽壳的 Stress-Sig），导致其转换沿用陈旧范围而全红。
            if (subAttrSet && par.pointer) {
                const int si = subAttrSet->GetAttributeIndex(par.pointer->GetName());
                if (si >= 0) { subAttrSet->GetAttribute(si).dataRange = par.GetDataRange(); }
            }
            if (dispAttrSet && par.pointer) {
                const int di = dispAttrSet->GetAttributeIndex(par.pointer->GetName());
                if (di >= 0) { dispAttrSet->GetAttribute(di).dataRange = par.GetDataRange(); }
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
        // 锁定属性：父容器保持固定范围，不被子对象按当帧数据聚合覆盖
        if (par_attr.rangeLocked) continue;
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

bool DataObject::FixAttributeRange(const std::string& attrName, double minv, double maxv, int dimension) {
    auto writeRange = [&](AttributeSet::Pointer attrs) {
        if (!attrs) return;
        const int idx = attrs->GetAttributeIndex(attrName);
        if (idx < 0) return;
        auto& attr = attrs->GetAttribute(idx);
        if (!attr.pointer) return;
        auto range = attr.GetDataRange();
        if (!range) return;
        int e = dimension + 1;   // dataRange 元素 0 为模长，元素 i+1 为第 i 个分量
        if (e < 0 || e >= range->GetNumberOfElements()) { e = 0; }
        range->SetElement(e, {minv, maxv});
        range->Modified();
        attr.rangeLocked = true;
    };
    writeRange(GetAttributeSet());
    auto frames = PeekTimeFrames();
    if (frames) {
        const size_t n = frames->GetTimeNum();
        for (size_t j = 0; j < n; ++j) {
            auto frameData = frames->GetTargetTimeFrameData(j);
            for (auto& o : frameData) {
                if (auto d = DynamicCast<DataObject>(o)) { writeRange(d->GetAttributeSet()); }
                else if (auto a = DynamicCast<AttributeSet>(o)) { writeRange(a); }
            }
        }
    }
    // 立即锁定当前挂载的帧对象（可能与 GetTargetTimeFrameData 返回的是不同实例）
    ReapplyRangeLocks();
    return true;
}

bool DataObject::UnfixAttributeRange(const std::string& attrName) {
    // 父容器只清标志（占位 dataRange 无真实数据，不能置空）
    if (auto attrs = GetAttributeSet()) {
        const int idx = attrs->GetAttributeIndex(attrName);
        if (idx >= 0) { attrs->GetAttribute(idx).rangeLocked = false; }
    }
    auto clearFrame = [&](AttributeSet::Pointer attrs) {
        if (!attrs) return;
        const int idx = attrs->GetAttributeIndex(attrName);
        if (idx < 0) return;
        auto& attr = attrs->GetAttribute(idx);
        attr.rangeLocked = false;
        attr.SetDataRange(nullptr);   // 置空：下次聚合按当帧数据重算
    };
    auto frames = PeekTimeFrames();
    if (frames) {
        const size_t n = frames->GetTimeNum();
        for (size_t j = 0; j < n; ++j) {
            auto frameData = frames->GetTargetTimeFrameData(j);
            for (auto& o : frameData) {
                if (auto d = DynamicCast<DataObject>(o)) { clearFrame(d->GetAttributeSet()); }
                else if (auto a = DynamicCast<AttributeSet>(o)) { clearFrame(a); }
            }
        }
    }
    return true;
}

bool DataObject::IsAttributeRangeLocked(const std::string& attrName) {
    auto attrs = GetAttributeSet();
    if (!attrs) return false;
    const int idx = attrs->GetAttributeIndex(attrName);
    if (idx < 0) return false;
    return attrs->GetAttribute(idx).rangeLocked;
}

void DataObject::ReapplyRangeLocks() {
    auto attrs = GetAttributeSet();
    if (!attrs || !HasSubDataObject()) return;
    const int n = static_cast<int>(attrs->GetNumberOfAttributes());
    for (int i = 0; i < n; ++i) {
        auto& par = attrs->GetAttribute(i);
        if (!par.rangeLocked || !par.pointer || !par.dataRange) continue;
        const int elems = par.dataRange->GetNumberOfElements();
        for (auto it = SubDataObjectIteratorBegin(); it != SubDataObjectIteratorEnd(); ++it) {
            auto sub = DynamicCast<DataObject>(it->second);
            if (!sub) continue;
            auto subAttrs = sub->GetAttributeSet();
            const int si = subAttrs ? subAttrs->GetAttributeIndex(par.pointer->GetName()) : -1;
            if (si < 0) continue;
            auto& sa = subAttrs->GetAttribute(si);
            auto sr = sa.GetDataRange();
            if (!sr) continue;
            const int se = std::min(elems, static_cast<int>(sr->GetNumberOfElements()));
            for (int e = 0; e < se; ++e) {
                sr->SetElement(e, {par.dataRange->GetValue(2 * e),
                                   par.dataRange->GetValue(2 * e + 1)});
            }
            sr->Modified();
            sa.rangeLocked = true;
        }
    }
}

bool DataObject::ScanAttributeRange(AttributeSet::Pointer attrs, const std::string& attrName,
                                    int dimension, double& mn, double& mx) {
    if (!attrs) return false;
    const int idx = attrs->GetAttributeIndex(attrName);
    if (idx < 0) return false;
    auto& attr = attrs->GetAttribute(idx);
    auto arr = attr.pointer;
    if (!arr) return false;
    const IGsize n = arr->GetNumberOfElements();
    if (n == 0) return false;
    const int arrDim = arr->GetDimension();
    int comp = dimension;
    if (comp >= arrDim) { comp = arrDim - 1; }
    double lo = DBL_MAX, hi = DBL_MIN;
    for (IGsize i = 0; i < n; ++i) {
        double v = arr->GetElementValue(i, comp);   // comp=-1 取模长
        lo = std::min(lo, v);
        hi = std::max(hi, v);
    }
    if (lo > hi) return false;
    mn = lo;
    mx = hi;
    return true;
}

bool DataObject::ComputeGlobalRange(const std::string& attrName, int dimension, double& mn, double& mx) {
    double lo = DBL_MAX, hi = DBL_MIN;
    bool found = false;
    auto scan = [&](AttributeSet::Pointer attrs) {
        double smn, smx;
        if (ScanAttributeRange(attrs, attrName, dimension, smn, smx)) {
            lo = std::min(lo, smn);
            hi = std::max(hi, smx);
            found = true;
        }
    };
    scan(GetAttributeSet());
    if (HasSubDataObject()) {
        for (auto it = SubDataObjectIteratorBegin(); it != SubDataObjectIteratorEnd(); ++it) {
            auto sub = DynamicCast<DataObject>(it->second);
            if (sub) { scan(sub->GetAttributeSet()); }
        }
    }
    if (auto frames = PeekTimeFrames()) {
        const size_t n = frames->GetTimeNum();
        for (size_t j = 0; j < n; ++j) {
            auto frameData = frames->GetTargetTimeFrameData(j);
            for (auto& o : frameData) {
                if (auto d = DynamicCast<DataObject>(o)) { scan(d->GetAttributeSet()); }
                else if (auto a = DynamicCast<AttributeSet>(o)) { scan(a); }
            }
        }
    }
    if (!found) return false;
    mn = lo;
    mx = hi;
    return true;
}

bool DataObject::ExpandRangeLocksForCurrentFrame() {
    auto attrs = GetAttributeSet();
    if (!attrs || !HasSubDataObject()) return true;
    const int n = static_cast<int>(attrs->GetNumberOfAttributes());
    for (int i = 0; i < n; ++i) {
        auto& par = attrs->GetAttribute(i);
        if (par.rangeMode != AttributeSet::RangeMode::ExpandOnly || !par.pointer) continue;
        const int dim = par.rangeLockedDimension;
        // 扫描当前挂载帧的真实范围（锁定后 dataRange 是运行值，必须从数据算）
        double mn = DBL_MAX, mx = DBL_MIN;
        bool found = false;
        for (auto it = SubDataObjectIteratorBegin(); it != SubDataObjectIteratorEnd(); ++it) {
            auto sub = DynamicCast<DataObject>(it->second);
            if (!sub) continue;
            double smn, smx;
            if (ScanAttributeRange(sub->GetAttributeSet(), par.pointer->GetName(), dim, smn, smx)) {
                mn = std::min(mn, smn);
                mx = std::max(mx, smx);
                found = true;
            }
        }
        if (!found) continue;
        // 单调扩张
        if (!par.runningRangeValid) {
            par.runningMin = mn;
            par.runningMax = mx;
            par.runningRangeValid = true;
        } else {
            par.runningMin = std::min(par.runningMin, mn);
            par.runningMax = std::max(par.runningMax, mx);
        }
        // 写回父容器与挂载子对象（保持锁定）
        auto pr = par.GetDataRange();
        if (!pr) continue;
        const int elems = pr->GetNumberOfElements();
        int e = dim + 1;
        if (e < 0 || e >= elems) { e = 0; }
        pr->SetElement(e, {par.runningMin, par.runningMax});
        pr->Modified();
        for (auto it = SubDataObjectIteratorBegin(); it != SubDataObjectIteratorEnd(); ++it) {
            auto sub = DynamicCast<DataObject>(it->second);
            if (!sub) continue;
            auto subAttrs = sub->GetAttributeSet();
            const int si = subAttrs ? subAttrs->GetAttributeIndex(par.pointer->GetName()) : -1;
            if (si < 0) continue;
            auto& sa = subAttrs->GetAttribute(si);
            auto sr = sa.GetDataRange();
            if (!sr) continue;
            if (e < static_cast<int>(sr->GetNumberOfElements())) {
                sr->SetElement(e, {par.runningMin, par.runningMax});
                sr->Modified();
            }
            sa.rangeLocked = true;
        }
    }
    return true;
}

bool DataObject::SetAttributeRangeMode(const std::string& attrName, AttributeSet::RangeMode mode, int dimension) {
    auto attrs = GetAttributeSet();
    if (!attrs) return false;
    const int idx = attrs->GetAttributeIndex(attrName);
    if (idx < 0) return false;
    auto& attr = attrs->GetAttribute(idx);

    if (mode == AttributeSet::RangeMode::PerFrame) {
        attr.rangeMode = AttributeSet::RangeMode::PerFrame;
        attr.rangeLocked = false;
        attr.runningRangeValid = false;
        UnfixAttributeRange(attrName);
        return true;
    }
    if (mode == AttributeSet::RangeMode::FixedGlobal) {
        double gmin, gmax;
        if (!ComputeGlobalRange(attrName, dimension, gmin, gmax)) { return false; }
        if (gmax <= gmin) { gmin = std::min(gmin, -1.0); gmax = std::max(gmax, 1.0); }
        FixAttributeRange(attrName, gmin, gmax, dimension);
        attr.rangeMode = AttributeSet::RangeMode::FixedGlobal;
        attr.rangeLockedDimension = dimension;
        attr.runningRangeValid = false;
        return true;
    }
    // ExpandOnly：初始化运行范围（用当前挂载帧）并锁定
    attr.rangeMode = AttributeSet::RangeMode::ExpandOnly;
    attr.rangeLockedDimension = dimension;
    attr.rangeLocked = true;
    attr.runningRangeValid = false;
    ExpandRangeLocksForCurrentFrame();
    if (!attr.runningRangeValid) {   // 无可用帧时兜底
        attr.runningMin = -1.0;
        attr.runningMax = 1.0;
        attr.runningRangeValid = true;
        FixAttributeRange(attrName, -1.0, 1.0, dimension);
    }
    return true;
}

AttributeSet::RangeMode DataObject::GetAttributeRangeMode(const std::string& attrName) {
    auto attrs = GetAttributeSet();
    if (!attrs) return AttributeSet::RangeMode::PerFrame;
    const int idx = attrs->GetAttributeIndex(attrName);
    if (idx < 0) return AttributeSet::RangeMode::PerFrame;
    return attrs->GetAttribute(idx).rangeMode;
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
    // 帧挂载后先重放范围锁定，再聚合值域：
    // 否则缓存重读的新帧（未带锁）会被 ReCollect 重新聚合成当帧范围
    this->ReapplyRangeLocks();
    // 只扩不缩模式：用当前帧真实数据单调扩张运行范围并写回
    this->ExpandRangeLocksForCurrentFrame();
    this->ReCollectSubDataObjectDataRange();
    this->UpdateSubDataObjectDataRange();
}

IGAME_NAMESPACE_END
