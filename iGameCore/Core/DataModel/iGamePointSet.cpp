#include "iGamePointSet.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN
void PointSet::SetPoints(Points::Pointer points) {
    if (m_Points != points) {
        m_Points = points;
        this->Modified();
    }
}
Points::Pointer PointSet::GetPoints() { return m_Points; }

IGsize PointSet::GetNumberOfPoints() {
    return m_Points ? m_Points->GetNumberOfPoints() : 0;
}

const Point& PointSet::GetPoint(const IGsize ptId) const {
    return m_Points->GetPoint(ptId);
}

void PointSet::SetPoint(const IGsize ptId, const Point& p) {
    m_Points->SetPoint(ptId, p);
}

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

bool PointSet::IsPointDeleted(const IGsize ptId) {
    return m_PointDeleteMarker->IsDeleted(ptId);
}

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
    IGsize res = 0;
    if (m_Points) res += m_Points->GetRealMemorySize();
    if (m_PointDeleteMarker) res += m_PointDeleteMarker->GetRealMemorySize();
    if (m_Attributes) res += m_Attributes->GetRealMemorySize();
    return res + sizeof(m_InEditStatus);
}
void PointSet::RequestPointStatus() {
    if (m_PointDeleteMarker == nullptr) {
        m_PointDeleteMarker = DeleteMarker::New();
    }
    m_PointDeleteMarker->Initialize(this->GetNumberOfPoints());
}

void PointSet::ComputeBoundingBox() {
    // std::cout << m_BoundingHelper->GetMTime() << " " << m_Points->GetMTime() <<
    // std::endl;
    if (m_Bounding.isNull() ||
        m_BoundingHelper->GetMTime() < m_Points->GetMTime()) {
        m_Bounding.reset();
        for (int i = 0; i < GetNumberOfPoints(); i++) {
            m_Bounding.add(GetPoint(i));
        }
        m_BoundingHelper->Modified();
    }
}

void PointSet::ConvertToDrawableData() {
    m_Positions = m_Points->ConvertToArray();
    m_Positions->Modified();
}

void PointSet::ViewCloudPicture(Scene* scene, int index, int demension) {
    auto& attr = this->GetAttributeSet()->GetAttribute(index);
    if (!attr.isDeleted && attr.attachmentType == IG_POINT) {
        this->SetAttributeWithPointData(attr.pointer, attr.dataRange,
                                        demension);
    }
    scene->Update();
}

void PointSet::SetAttributeWithPointData(ArrayObject::Pointer attr,
                                         std::pair<float, float>& range,
                                         igIndex dimension) {
    if (m_ViewAttribute != attr || m_ViewDemension != dimension ||
        m_ColorMapper->GetMTime() > this->GetMTime()) {
        if (attr == nullptr) {
            m_UseColor = false;
            m_ViewAttribute = nullptr;
            m_ViewDemension = -1;
            return;
        }
        m_ViewAttribute = attr;
        m_ViewDemension = dimension;

        m_UseColor = true;

        if (range.first != range.second) {
            m_ColorMapper->SetRange(range.first, range.second);
        } else if (dimension == -1) {
            m_ColorMapper->InitRange(attr);
        } else {
            m_ColorMapper->InitRange(attr, dimension);
        }
        range.first = m_ColorMapper->GetRange()[0];
        range.second = m_ColorMapper->GetRange()[1];
        m_Colors = m_ColorMapper->MapScalars(attr, dimension);
        m_Colors->Modified();
        if (m_Colors == nullptr) { return; }
    }
}
IGAME_NAMESPACE_END