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
void PointSet::RequestPointStatus() {
    if (m_PointDeleteMarker == nullptr) { m_PointDeleteMarker = DeleteMarker::New(); }
    m_PointDeleteMarker->Initialize(this->GetNumberOfPoints());
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
    if (m_Points->GetMTime() > m_Positions->GetMTime()) {
        m_Positions = m_Points->ConvertToArray();
        m_Positions->Modified();
    }

    // convert scalar data
    if (m_AttributeIndex == -1) {
        m_UseColor = false;
        m_ColorWithCell = false;
    } else {
        m_UseColor = true;

        auto& attr = this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
        if (attr.type == IG_RGB) {
            this->m_ColorMapper->SetVectorModeToRGBColors();
        } else {
            this->m_ColorMapper->SetVectorModeToComponent();
        }
        if (!attr.isDeleted && attr.attachmentType == IG_POINT) {
            if (m_AttributeHelper->GetMTime() > m_Colors->GetMTime() ||
                m_ColorMapper->GetMTime() > m_Colors->GetMTime()) {
                m_ColorWithCell = false;
                this->SetAttributeWithPointData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
            }
        }
    }
}

//void PointSet::ViewCloudPicture(Scene* scene, int index, int demension) {
//    auto& attr = this->GetAttributeSet()->GetAttribute(index);
//    if (!attr.isDeleted && attr.attachmentType == IG_POINT) {
//        this->SetAttributeWithPointData(attr.pointer, attr.dataRange,
//                                        demension);
//    }
//    scene->Update();
//}

void PointSet::SetAttributeWithPointData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange, igIndex dimension) {
    if (m_ColorMapper->GetMTime() <= this->GetMTime()) {
        double minimal_val = attrRange->GetValue(2 + dimension * 2 + 0);
        double maximal_val = attrRange->GetValue(2 + dimension * 2 + 1);
        if (minimal_val < maximal_val) {
            m_ColorMapper->SetRange(minimal_val, maximal_val);
        } else {
            m_ColorMapper->InitRange(attr, dimension);
        }
    }
    m_Colors = m_ColorMapper->MapScalars(attr, dimension);
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
IGAME_NAMESPACE_END
