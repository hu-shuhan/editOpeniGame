#include "iGameMultiSelectionStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"

IGAME_NAMESPACE_BEGIN

MultiSelectionStyle::MultiSelectionStyle() { m_InvertedMVP = igm::mat4{}; }

MultiSelectionStyle::~MultiSelectionStyle() {}

void MultiSelectionStyle::MousePressEvent(IEvent event) {
    SelectionStyle::MousePressEvent(event);
    m_InvertedMVP = m_Interactor->GetMVP().invert();
}

void MultiSelectionStyle::LeftButtonMouseMove() {
    float x1 = m_OldPoint2D.x;
    float y1 = m_OldPoint2D.y;
    float x2 = m_NewPoint2D.x;
    float y2 = m_NewPoint2D.y;
    if (x2 < x1) { std::swap(x1, x2); }
    if (y2 < y1) { std::swap(y1, y2); }

    igm::vec3 leftTopPoint =
            GetNearWorldCoord(igm::vec2(x1, y1), m_InvertedMVP);
    igm::vec3 leftButtomPoint =
            GetNearWorldCoord(igm::vec2(x1, y2), m_InvertedMVP);
    igm::vec3 rightTopPoint =
            GetNearWorldCoord(igm::vec2(x2, y1), m_InvertedMVP);
    igm::vec3 rightButtomPoint =
            GetNearWorldCoord(igm::vec2(x2, y2), m_InvertedMVP);

    igm::vec3 leftTopInterPoint =
            GetFarWorldCoord(igm::vec2(x1, y1), m_InvertedMVP);
    igm::vec3 rightButtomInterPoint =
            GetFarWorldCoord(igm::vec2(x2, y2), m_InvertedMVP);

    igm::vec3 leftNormal = (leftButtomPoint - leftTopPoint)
                                   .cross(leftTopInterPoint - leftTopPoint);
    igm::vec4 leftPlane = GetPlane(leftTopPoint, leftNormal);
    igm::vec3 topNormal = (leftTopInterPoint - leftTopPoint)
                                  .cross(rightTopPoint - leftTopPoint);
    igm::vec4 topPlane = GetPlane(leftTopPoint, topNormal);
    igm::vec3 rightNormal =
            (rightTopPoint - rightButtomPoint)
                    .cross(rightButtomInterPoint - rightButtomPoint);
    igm::vec4 rightPlane = GetPlane(rightButtomPoint, rightNormal);
    igm::vec3 bottomNormal = (rightButtomInterPoint - rightButtomPoint)
                                     .cross(leftButtomPoint - rightButtomPoint);
    igm::vec4 butttomPlane = GetPlane(rightButtomPoint, bottomNormal);
    igm::vec3 nearNormal = (rightButtomPoint - rightTopPoint)
                                   .cross(leftTopPoint - rightTopPoint);
    igm::vec4 nearPlane = GetPlane(rightTopPoint, nearNormal);
    igm::vec4 farPlane = GetPlane(leftTopInterPoint, -nearNormal);

    std::vector<igm::vec4> planes;
    planes.push_back(leftPlane);
    planes.push_back(topPlane);
    planes.push_back(rightPlane);
    planes.push_back(butttomPlane);
    planes.push_back(nearPlane);
    planes.push_back(farPlane);

    switch (GetSelectedType()) {
        case SelectionStyle::SelectPoint:
            this->SelectPoints(planes);
            break;
        case SelectionStyle::SelectCell:
            this->SelectFaces(planes);
            break;
        default:
            break;
    }
}

void MultiSelectionStyle::SelectPoints(const std::vector<igm::vec4>& planes) {
    //m_Model->GetPointPainter()->Clear();

    auto painter = m_Model->GetPainter3D();
    painter->SetPen(10);
    painter->SetPen(Color::Red);

    for (int i = 0; i < m_Points->GetNumberOfPoints(); i++) {
        auto& p = m_Points->GetPoint(i);

        if (IsPointInFrustum(igm::vec3(p[0], p[1], p[2]), planes)) {
            painter->DrawPoint(p);
        }
    }
}

void MultiSelectionStyle::SelectFaces(const std::vector<igm::vec4>& planes) {
    std::vector<bool> pvis(m_Points->GetNumberOfPoints(), false);
    //m_Model->GetFacePainter()->Clear();
    for (int i = 0; i < m_Points->GetNumberOfPoints(); i++) {
        auto& p = m_Points->GetPoint(i);
        if (IsPointInFrustum(igm::vec3(p[0], p[1], p[2]), planes)) {
            pvis[i] = true;
        }
    }

    igIndex face[16]{};
    for (int i = 0; i < m_Cells->GetNumberOfCells(); i++) {
        int size = m_Cells->GetCellIds(i, face);
        bool flag = false;
        for (int j = 0; j < size; j++) {
            if (pvis[face[j]]) {
                flag = true;
                break;
            }
        }
        if (flag) {
            auto painter = m_Model->GetPainter3D();
            painter->SetPen(3);
            painter->SetPen(Color::Green);
            painter->SetBrush(Color::Red);

            for (int j = 2; j < size; j++) {
                painter->DrawTriangle(m_Points->GetPoint(face[0]),
                                      m_Points->GetPoint(face[j - 1]),
                                      m_Points->GetPoint(face[j]));

                //std::cout << "--------------\n";
                //std::cout << m_Mesh->GetPoint(face[0]) << std::endl;
                //std::cout << m_Mesh->GetPoint(face[j - 1]) << std::endl;
                //std::cout << m_Mesh->GetPoint(face[j]) << std::endl;
            }
        }
    }
}

bool MultiSelectionStyle::IsPointInFrustum(
        const igm::vec3& p, const std::vector<igm::vec4>& planes) {
    if (planes.size() < 6) return false;
    for (int i = 0; i < 6; i++) {
        auto& plane = planes[i];
        float dist = p.x * plane.x + p.y * plane.y + p.z * plane.z + plane.w;
        if (dist < 0) { return false; }
    }

    return true;
}

IGAME_NAMESPACE_END
