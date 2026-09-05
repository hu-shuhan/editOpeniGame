#include "iGamePointSet.h"
#include "iGameModel.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN
void PointSet::SetPoints(Points::Pointer points) {
    if (m_Points != points) {
        m_Points = points;
        m_Points->Modified();
        this->Modified();
    }
}
Points::Pointer PointSet::GetPoints() { return m_Points; }

IGsize PointSet::GetNumberOfPoints() { return m_Points ? m_Points->GetNumberOfPoints() : 0; }

const Point& PointSet::GetPoint(const IGsize ptId) const { return m_Points->GetPoint(ptId); }

void PointSet::SetPoint(const IGsize ptId, const Point& p) { m_Points->SetPoint(ptId, p); }

IGsize PointSet::AddPoint(const Point& p) {
    if (!InEditStatus()) { RequestEditStatus(); }
    IGsize id = m_Points->AddPoint(p);
    m_PointDeleteMarker->AddTag();
    return id;
}

void PointSet::RequestEditStatus() {
    if (InEditStatus()) { return; }
    RequestPointStatus();
    MakeEditStatusOn();
}

void PointSet::DeletePoint(const IGsize ptId) {
    if (!InEditStatus()) { RequestEditStatus(); }
    m_PointDeleteMarker->MarkDeleted(ptId);
}

bool PointSet::IsPointDeleted(const IGsize ptId) { return m_PointDeleteMarker->IsDeleted(ptId); }

void PointSet::GarbageCollection() {
    IGsize i, mapId = 0;
    for (i = 0; i < GetNumberOfPoints(); i++) {
        if (IsPointDeleted(i)) continue;
        if (i != mapId) { m_Points->SetPoint(mapId, m_Points->GetPoint(i)); }
        mapId++;
    }
    m_Points->Resize(mapId);

    m_PointDeleteMarker = nullptr;
    Modified();
    MakeEditStatusOff();
}

bool PointSet::InEditStatus() { return m_InEditStatus; }
void PointSet::MakeEditStatusOn() { m_InEditStatus = true; }
void PointSet::MakeEditStatusOff() { m_InEditStatus = false; }

PointSet::PointSet() {
    m_Points = Points::New();
    m_ViewStyle = IG_POINT;
}
IGsize PointSet::GetRealMemorySize() {
    IGsize res = this->DrawObject::GetRealMemorySize();
    if (m_Points) res += m_Points->GetRealMemorySize();
    if (m_PointDeleteMarker) res += m_PointDeleteMarker->GetRealMemorySize();
    return res + sizeof(m_InEditStatus);
}
void PointSet::RequestPointStatus(const std::function<void(double)>& onProgress) {
    if (onProgress) onProgress(0.0);
    if (m_PointDeleteMarker == nullptr) {
        m_PointDeleteMarker = DeleteMarker::New();
        if (onProgress) onProgress(1.0);
    }
    m_PointDeleteMarker->Initialize(this->GetNumberOfPoints());
    if (onProgress) onProgress(1.0);
}

void PointSet::ComputeBoundingBox() {
    // std::cout << m_BoundingHelper->GetMTime() << " " << m_Points->GetMTime() <<
    // std::endl;
    if (m_Bounding.isNull() || m_BoundingHelper->GetMTime() < m_Points->GetMTime()) {
        m_Bounding.reset();
        for (int i = 0; i < GetNumberOfPoints(); i++) { m_Bounding.add(GetPoint(i)); }
        m_BoundingHelper->Modified();
    }
}

void PointSet::ConvertToDrawableData() {
    bool needReConvertGeometry = m_ReConvertToDrawableData;
    needReConvertGeometry |= m_Points->GetMTime() > m_ReConvertHelper->GetMTime();
    needReConvertGeometry |= m_Clipper->GetMTime() > m_ReConvertHelper->GetMTime();

    bool needReConvertScalar = needReConvertGeometry;
    needReConvertScalar |= m_AttributeHelper->GetMTime() > m_ReConvertHelper->GetMTime();

    // convert point data
    if (needReConvertGeometry) {
        m_Positions = m_Points->ConvertToArray();
        m_Positions->Modified();
    }

    // convert scalar data
    bool updateColorMapper = m_ColorMapper->GetMTime() > m_ReConvertHelper->GetMTime();
    if (needReConvertScalar || m_AttributeChanged || updateColorMapper) {
        m_AttributeChanged = false;
        if (m_AttributeIndex != -1) {
            auto& attr = this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
            if (attr.type == IG_RGB) {
                this->m_ColorMapper->SetVectorModeToRGBColors();
            } else {
                this->m_ColorMapper->SetVectorModeToComponent();
            }

            if (!attr.isDeleted && attr.attachmentType == IG_POINT) {
                m_ColorWithCell = false;
                this->SetAttributeWithPointData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
            }
        }
    }

    m_ReConvertToDrawableData = false;
    m_ReConvertHelper->Modified();
}

void PointSet::SetAttributeWithPointData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange, igIndex dimension) {
    /* 当pointMapper 外部更新（调整颜色映射的 Range）， 则不用调整ColorMap的范围*/
    if (!m_ColorMapper->GetStable()) {
        int minIdx = 2 + dimension * 2 + 0;
        int maxIdx = 2 + dimension * 2 + 1;
        double minimal_val = attrRange->GetValue(minIdx);
        double maximal_val = attrRange->GetValue(maxIdx);
        
        if (minimal_val < maximal_val) {
            m_ColorMapper->SetRange(minimal_val, maximal_val);
        } else {
            m_ColorMapper->InitRange(attr, dimension);
        }
    }
    m_Colors = m_ColorMapper->MapScalars(attr, dimension, 4);
    m_Colors->Modified();
    if (m_Colors == nullptr) { return; }
}

FlatArray<igIndex>::Pointer PointSet::GetPointMap() { return m_PointMap; }

void PointSet::SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange, igIndex dimension) {}

SmartPointer<Selection> PointSet::GetSelection(Model* model) {
    if (m_Selection == nullptr) {
        m_Selection = Selection::New();
        m_Selection->SetModel(model);
    }
    return m_Selection.get();
}

void PointSet::SetSelection(SmartPointer<Selection> selection) { m_Selection = selection; }

IGAME_NAMESPACE_END
