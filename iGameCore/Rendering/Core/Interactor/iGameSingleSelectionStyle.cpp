#include "iGameSingleSelectionStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

SingleSelectionStyle::SingleSelectionStyle() {}
SingleSelectionStyle::~SingleSelectionStyle() {}

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

//static void DrawPoint(Painter3D::Pointer painter, const Point& point,
//                      std::vector<IGuint>& drawHandles) {
//    auto drawHandle = painter->DrawPoint(point);
//    drawHandles.push_back(drawHandle);
//}

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
    SmartPointer<PointPicker> picker = PointPicker::New();
    picker->SetDataObject(obj);
    id = picker->PickClosetPointOnLine(Vector3d(point1.x, point1.y, point1.z),
                                       Vector3d(dir.x, dir.y, dir.z), p);

    //m_Model->GetPointPainter()->Clear();
    if (id != -1) {
        /*
        auto painter = m_Model->GetPainter3D();
        painter->SetPen(10);
        painter->SetPen(Color::Red);
        painter->DrawPoint(p);
        */

        if (m_Selection
            //&& !obj->HasSubDataObject()
            ) {
            std::vector<int> selectedPointIds;
            auto& thisPoint = m_Points->GetPoint(id);
            
            for (int pointId = 0; pointId < m_Points->GetNumberOfPoints();
                 pointId++) {
                auto& point = m_Points->GetPoint(pointId);
                if ((thisPoint - point).length() <= m_SelectRadius) {
                    selectedPointIds.push_back(pointId);
                }
            }

            if (m_Select_OR_UnSelect) {
                auto events = Selection::GenerateEvents(
                        selectedPointIds, IG_POINT,
                        Selection::Event::Operate::Add, m_Points.get(),
                        m_Cells.get(), m_Model->GetPainter3D().get());
                m_Selection->SelectionCallBackEvent(events);
            } else {
                auto events = Selection::GenerateEvents(
                        selectedPointIds, IG_POINT,
                        Selection::Event::Operate::Remove, m_Points.get(),
                        m_Cells.get(), m_Model->GetPainter3D().get());
                m_Selection->SelectionCallBackEvent(events);
            }
            //auto painter = m_Model->GetPainter3D();
            //painter->SetPen(10);
            //painter->SetPen(Color::Red);
            //std::vector<Selection::Event> events;
            //for (auto& pointId: selectedPointIds) {
            //    Selection::Event e;
            //    e.type = Selection::Event::PickPoint;
            //    e.pickId = pointId;
            //    auto& point = m_Points->GetPoint(pointId);
            //    if (m_Select_OR_UnSelect) {
            //        e.operate = Selection::Event::Operate::Add;
            //        DrawPoint(painter, point, e.drawHandles);
            //    }
            //    else
            //        e.operate = Selection::Event::Operate::Remove;
            //    e.pos = point;
            //    events.push_back(e);
            //}
            //m_Selection->SelectionCallBackEvent(events);
        }
    }
}

static iGame::Point GetCentralOfCell(int cellPointSize, int cellPoints[],
                                     Points::Pointer points) {
    Point p;
    p.setZero();
    for (int i = 0; i < cellPointSize; i++) {
        int pointIndex = cellPoints[i];
        auto& point = points->GetPoint(pointIndex);
        p += point;
    }
    p /= cellPointSize;
    return p;
}

//static void DrawCell(Painter3D::Pointer painter, Cell* cell,
//                     std::vector<IGuint>& drawHandles) {
//    if (cell == nullptr) return;
//    auto faceNum = cell->GetNumberOfFaces();
//    if (faceNum == 0) {
//        int pointSize = cell->GetNumberOfPoints();
//        if (pointSize <= 2) return;
//        auto& p0 = cell->GetPoint(0);
//        for (int i = 1; i < pointSize - 1; i++) {
//            auto& p1 = cell->GetPoint(i);
//            auto& p2 = cell->GetPoint(i + 1);
//            auto drawHandle = painter->DrawTriangle(p0, p1, p2);
//            drawHandles.push_back(drawHandle);
//        }
//    } else {
//        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
//            auto face = cell->GetFace(faceIndex);
//            DrawCell(painter, face, drawHandles);
//        }
//    }
//}

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
        igIndex face[IGAME_CELL_MAX_SIZE]{};
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
        if (m_Selection) {
            std::vector<int> selectedCellIds;
            //std::vector<Point> selectedCellCenters;

            igIndex thisCell[IGAME_CELL_MAX_SIZE]{};
            int thisCellSize = m_Cells->GetCellIds(id, thisCell);

            //Obtain the average value of all points of the cell
            //and obtain the cell within the radius accordingly.
            iGame::Point thisCellCentralPoint =
                    GetCentralOfCell(thisCellSize, thisCell, m_Points);

            auto mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(
                    m_Model->GetDataObject());

            //Calculate the center point of each surface and compare the radii.
            for (int cellIndex = 0; cellIndex < m_Cells->GetNumberOfCells();
                 cellIndex++) {
                igIndex thatCell[IGAME_CELL_MAX_SIZE]{};
                int thatCellSize = m_Cells->GetCellIds(cellIndex,thatCell);
                Point thatCellCentralPoint =
                        GetCentralOfCell(thatCellSize, thatCell, m_Points);
                if ((thisCellCentralPoint - thatCellCentralPoint).length() <=
                    m_SelectRadius) {
                    selectedCellIds.push_back(cellIndex);
                    //selectedCellCenters.push_back(thatCellCentralPoint);
                }
            }

            if (m_Select_OR_UnSelect) {
                auto events = Selection::GenerateEvents(
                        selectedCellIds, IG_CELL,
                        Selection::Event::Operate::Add, m_Points.get(),
                        m_Cells.get(), m_Model->GetPainter3D().get());
                m_Selection->SelectionCallBackEvent(events);
            } else {
                auto events = Selection::GenerateEvents(
                        selectedCellIds, IG_CELL,
                        Selection::Event::Operate::Remove, m_Points.get(),
                        m_Cells.get(), m_Model->GetPainter3D().get());
                m_Selection->SelectionCallBackEvent(events);
            }
            //auto painter = m_Model->GetPainter3D();
            //painter->SetPen(3);
            //painter->SetPen(Color::Black);
            //painter->SetBrush(Color::Red);
            //std::vector<Selection::Event> events;
            //for (int i = 0; i < selectedCellIds.size(); i++)
            //{
            //    auto& cellId = selectedCellIds[i];
            //    auto& cellCenter = selectedCellCenters[i];
            //    Selection::Event e;
            //    e.type = Selection::Event::PickFace;
            //    e.pickId = cellId;
            //    if (m_Select_OR_UnSelect) {
            //        e.operate = Selection::Event::Operate::Add;
            //        DrawCell(painter, mesh->GetCell(cellId), e.drawHandles);
            //    }
            //    else
            //        e.operate = Selection::Event::Operate::Remove;
            //    e.pos = cellCenter;
            //    //e.pos = Vector3f(intersect.x, intersect.y, intersect.z);
            //    events.push_back(e);
            //}
            //m_Selection->SelectionCallBackEvent(events);
        }
    }
}

IGAME_NAMESPACE_END
