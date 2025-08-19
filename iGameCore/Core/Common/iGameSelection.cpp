#include <iGameSelection.h>
#include <iGameModel.h>
#include <set>
#include <utility>
#include <algorithm>
IGAME_NAMESPACE_BEGIN

static iGame::Point GetCentralOfCell(int cellPointSize, int cellPoints[], Points* points) {
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

static void DrawPoint(Painter3D::Pointer painter, const Point& point, std::vector<IGuint>& drawHandles) {
    auto drawHandle = painter->DrawPoint(point);
    drawHandles.push_back(drawHandle);
}

//static void DrawCell(Painter3D::Pointer painter, Cell* cell, std::vector<IGuint>& drawHandles) {
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

static void DrawCell(Painter3D::Pointer painter, int cellPointSize, int cellPoints[], Points* points,
                     std::vector<IGuint>& drawHandles) {
    if (cellPointSize <= 0) return;
    std::vector<std::pair<int, int>> needDrawLines;
    auto cellFaceNum = cellPoints[0];
    for (int faceI = 0, cellPointsI = 1; faceI < cellFaceNum; faceI++) {
        auto facePointNum = cellPoints[cellPointsI];
        cellPointsI++;
        if (facePointNum <= 0) return;
        for (int pointI = 0; pointI < facePointNum; pointI++) {
            auto pointI_A = pointI;
            auto pointI_B = (pointI + 1) % facePointNum;
            auto pointIdA = cellPoints[cellPointsI + pointI_A];
            auto pointIdB = cellPoints[cellPointsI + pointI_B];
            needDrawLines.push_back(std::minmax(pointIdA, pointIdB));
        }
        cellPointsI += facePointNum;
    }
    std::sort(needDrawLines.begin(), needDrawLines.end());
    auto last = std::unique(needDrawLines.begin(), needDrawLines.end());
    needDrawLines.erase(last, needDrawLines.end());
    //Draw
    for (auto& pointId: needDrawLines) {
        auto& pointA = points->GetPoint(pointId.first);
        auto& pointB = points->GetPoint(pointId.second);
        auto drawHandle = painter->DrawLine(pointA, pointB);
        drawHandles.push_back(drawHandle);
    }

    //if (cellPointSize >= 8) return;//Temp return; We should find a new method to draw the big cell
    //for (int i = 0; i < cellPointSize - 1; i++) {
    //    auto& p0 = points->GetPoint(cellPoints[i]);
    //    for (int j = i + 1; j < cellPointSize; j++) {
    //        auto& p1 = points->GetPoint(cellPoints[j]);
    //        auto drawHandle = painter->DrawLine(p0, p1);
    //        drawHandles.push_back(drawHandle);
    //    }
    //}
}

std::vector<Selection::Event> Selection::GenerateEvents(const std::vector<igIndex>& ids, IGenum type,
                                                        Event::Operate ope, Points* points, CellArray* cellArrays,
                                                        Painter3D* painter) {
    switch (type) {
        case IG_POINT: {
            painter->SetPen(10);
            painter->SetPen(Color::Red);
            std::vector<Selection::Event> events;
            for (auto& pointId: ids) {
                Selection::Event e;
                e.type = Selection::Event::PickPoint;
                e.pickId = pointId;
                auto& point = points->GetPoint(pointId);
                if (ope == Selection::Event::Operate::Add) {
                    e.operate = Selection::Event::Operate::Add;
                    DrawPoint(painter, point, e.drawHandles);
                } else
                    e.operate = Selection::Event::Operate::Remove;
                e.pos = point;
                events.push_back(e);
            }
            return events;
        } break;
        case IG_CELL: {
            painter->SetPen(3);
            painter->SetPen(0.9f, 0.145f, 0.863f);
            std::vector<Selection::Event> events;
            for (int i = 0; i < ids.size(); i++) {
                auto& cellId = ids[i];
                igIndex thatCell[IGAME_CELL_MAX_SIZE]{};
                int thatCellSize{};
                if (cellArrays != nullptr) { thatCellSize = cellArrays->GetCellIds(cellId, thatCell); }
                Selection::Event e;
                e.type = Selection::Event::PickFace;
                e.pickId = cellId;
                if (ope == Selection::Event::Operate::Add) {
                    e.operate = Selection::Event::Operate::Add;
                    DrawCell(painter, thatCellSize, thatCell, points, e.drawHandles);
                } else
                    e.operate = Selection::Event::Operate::Remove;
                if (points != nullptr) {
                    auto cellCenter = GetCentralOfCell(thatCellSize, thatCell, points);
                    e.pos = cellCenter;
                }
                //e.pos = Vector3f(intersect.x, intersect.y, intersect.z);
                events.push_back(e);
            }
            return events;
        } break;
        default:
            return {};
            break;
    }
}

void Selection::SelectionCallBackEvent(const std::vector<Event>& _events) {
    if (_events.empty()) return;
    for (auto& _event: _events) { AddItem(_event); }
    for (auto& callBackFunc: m_CallBackFunctor) { callBackFunc.second(_events); }
}

void Selection::SelectionCallBackEvent(const Event& event) {
    AddItem(event);
    for (auto& callBackFunc: m_CallBackFunctor) { callBackFunc.second({event}); }
}

void Selection::Reset() {
    if (m_Model != nullptr) {
        auto painter = m_Model->GetPainter3D();
        for (auto& selectedItem: m_SelectedItems) {
            for (auto& _event: selectedItem.second) {
                for (auto& drawHandle: _event.second.drawHandles) { painter->Delete(drawHandle); }
            }
        }
    }
    m_SelectedItems.clear();
    for (auto& callBackFunc: m_ClearSelectionCallBackFunctor) { callBackFunc.second(); }
}

void Selection::AddItem(const Event& event) {
    if (m_SelectedItems[event.type].count(event.pickId) != 0 && m_Model != nullptr) {
        auto painter = m_Model->GetPainter3D();
        auto& handles = m_SelectedItems[event.type][event.pickId].drawHandles;
        for (auto& handle: handles) { painter->Delete(handle); }
    }
    if (event.operate == Event::Operate::Remove) {
        m_SelectedItems[event.type].erase(event.pickId);
        return;
    }
    m_SelectedItems[event.type][event.pickId] = event;
}

IGAME_NAMESPACE_END

