#include "iGameSingleSelectionStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"

IGAME_NAMESPACE_BEGIN

void SingleSelectionStyle::MousePressEvent(IEvent _event) {
    SelectionStyle::MousePressEvent(_event);

    if (_event.button != LeftButton) return;
    switch (GetSelectedType()) {
        case SelectionStyle::SelectPoint:
            this->SelectPoint(_event.pos);
            break;
        case SelectionStyle::SelectCell:
            this->SelectFace(_event.pos);
            break;
        default:
            break;
    }
}

void SingleSelectionStyle::SelectPoint(igm::vec2 pos) {
    if (m_Model == nullptr) { return; }
    auto mvp = m_Interactor->GetMVP();
    auto mvp_invert = mvp.invert();

    // 3D World coordinate
    igm::vec3 point1 = GetNearWorldCoord(pos, mvp_invert);
    igm::vec3 point2 = GetFarWorldCoord(pos, mvp_invert);

    igm::vec3 dir = (point1 - point2).normalized();

    igIndex id = -1;
    Point p;
    auto obj = m_Model->GetDataObject();
    PointPicker::Pointer picker = PointPicker::New();
    picker->SetDataObject(obj);
    id = picker->PickClosetPointOnLine(Vector3d(point1.x, point1.y, point1.z),
                                       Vector3d(dir.x, dir.y, dir.z), p);

    //m_Model->GetPointPainter()->Clear();
    if (id != -1) {
        //std::cout << "pick point id: " << id << std::endl;
        auto painter = m_Model->GetPainter();
        painter->SetPen(10);
        painter->SetPen(Color::Red);
        //painter->DrawPoint(m_Points->GetPoint(id));
        painter->DrawPoint(p);

        if (m_Selection && !obj->HasSubDataObject()) {
            Selection::Event e;
            e.type = Selection::Event::PickPoint;
            e.pickId = id;
            e.pos = m_Points->GetPoint(id);
            m_Selection->FilterEvent(e);
        }
    }
}

void SingleSelectionStyle::SelectFace(igm::vec2 pos) {
    if (m_Points == nullptr || m_Cells == nullptr) { return; }

    auto mvp = m_Interactor->GetMVP();
    auto mvp_invert = mvp.invert();

    // 3D World coordinate
    igm::vec3 point1 = GetNearWorldCoord(pos, mvp_invert);
    igm::vec3 point2 = GetFarWorldCoord(pos, mvp_invert);

    igm::vec3 dir = (point1 - point2).normalized();

    Vector3Tovec3 t;
    double zMax = -std::numeric_limits<double>::max();
    igm::vec3 intersect;
    igIndex id = -1;
    for (int i = 0; i < m_Cells->GetNumberOfCells(); i++) {
        igIndex face[16]{};
        int size = m_Cells->GetCellIds(i, face);

        bool flag = false;
        auto& p0 = m_Points->GetPoint(face[0]);
        for (int j = 2; j < size; j++) {
            auto& p1 = m_Points->GetPoint(face[j - 1]);
            auto& p2 = m_Points->GetPoint(face[j]);

            if (IsIntersectTriangle(point1, point2, t(p0), t(p1), t(p2),
                                    intersect)) {
                flag = true;
                break;
            }
        }

        if (flag) {
            igm::vec3 center(0);
            for (int j = 0; j < size; j++) {
                auto& p = m_Points->GetPoint(face[j]);
                center += t(p);
            }
            center /= size;
            igm::vec4 fDepth = mvp * igm::vec4(center, 1.0f);
            fDepth /= fDepth.w;

            if (zMax < fDepth.z && fDepth.z > 0 && fDepth.z < 1.0) {
                id = i;
                zMax = fDepth.z;
            }
        }
    }

    //m_Model->GetFacePainter()->Clear();
    if (id != -1) {
        std::cout << "pick face id: " << id << std::endl;
        igIndex face[16]{};
        int size = m_Cells->GetCellIds(id, face);
        auto& p0 = m_Points->GetPoint(face[0]);

        auto painter = m_Model->GetPainter();
        painter->SetPen(3);
        painter->SetPen(Color::Black);
        painter->SetBrush(Color::Red);

        for (int j = 2; j < size; j++) {
            auto& p1 = m_Points->GetPoint(face[j - 1]);
            auto& p2 = m_Points->GetPoint(face[j]);
            painter->DrawTriangle(p0, p1, p2);
        }
        if (m_Selection) {
            Selection::Event e;
            e.type = Selection::Event::PickFace;
            e.pickId = id;
            e.pos = Vector3f(intersect.x, intersect.y, intersect.z);
            m_Selection->FilterEvent(e);
        }
    }
}

IGAME_NAMESPACE_END
