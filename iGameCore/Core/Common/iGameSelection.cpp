#include <algorithm>
#include <iGameModel.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <set>
#include <utility>
#include <iGameBoxStyle.h>
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

static void SumCellPoints(Cell* cell, Point& point, int& pointNum) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        for (int i = 0; i < pointSize; i++) {
            point += cell->GetPoint(i);
            pointNum++;
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            SumCellPoints(face, point, pointNum);
        }
    }
}

static Point GetCentralOfCell(Cell* cell) {
    Point p{};
    p.setZero();
    int pointNum{};
    SumCellPoints(cell, p, pointNum);
    if (pointNum == 0) return p;
    return p / pointNum;
}

static void DrawPoint(Painter3D* painter, const Point& point, std::vector<IGuint>& drawHandles) {
    if (painter == nullptr) return;
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

static void DrawCell_OffSet(Painter3D* painter, int cellPointSize, int cellPoints[], Points* points,
                            std::vector<IGuint>& drawHandles) {
    if (painter == nullptr) return;
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

static void DrawCell(Painter3D* painter, int cellPointSize, int cellPoints[], Points* points,
                     std::vector<IGuint>& drawHandles) {
    if (painter == nullptr) return;
    for (int i = 0; i < cellPointSize - 1; i++) {
        auto& p0 = points->GetPoint(cellPoints[i]);
        for (int j = i + 1; j < cellPointSize; j++) {
            auto& p1 = points->GetPoint(cellPoints[j]);
            auto drawHandle = painter->DrawLine(p0, p1);
            drawHandles.push_back(drawHandle);
        }
    }
}

static void DrawEdges(Painter3D* painter, const std::set<std::pair<int, int>>& edges, UnstructuredMesh* mesh,
                      std::vector<IGuint>& drawHandles) {
    if (painter == nullptr || edges.empty() || mesh == nullptr) return;
    painter->SetPen(3);
    painter->SetPen(0.9f, 0.145f, 0.863f);
    for (auto& edge: edges) {
        auto& p1 = mesh->GetPoint(edge.first);
        auto& p2 = mesh->GetPoint(edge.second);
        auto handle = painter->DrawLine(p1, p2);
        drawHandles.push_back(handle);
    }
}

static void DrawEdges(Painter3D* painter, const std::set<std::pair<int, int>>& edges, UnstructuredMesh* mesh) {
    if (painter == nullptr || edges.empty() || mesh == nullptr) return;
    painter->Clear();
    painter->SetPen(3);
    painter->SetPen(0.9f, 0.145f, 0.863f);
    for (auto& edge: edges) {
        auto& p1 = mesh->GetPoint(edge.first);
        auto& p2 = mesh->GetPoint(edge.second);
        auto handle = painter->DrawLine(p1, p2);
    }
}

static void DrawBoundingBoxs(Painter3D* painter, const std::vector<std::pair<Point, Point>>& boxs) {
    if (painter == nullptr || boxs.empty()) return;
    painter->Clear();
    painter->SetPen(5);
    painter->SetPen(0.9f, 0.9f, 0.9f);
    painter->SetBrush(iGame::Brush::Style::NoBrush);
    for (auto& box_: boxs) { painter->DrawCube(box_.first, box_.second); }
}

static void DrawOneBoundingBox(Painter3D* painter, const Point& p1, const Point& p2) {
    if (painter == nullptr) return;
    painter->Clear();
    painter->SetPen(5);
    painter->SetPen(0.9f, 0.9f, 0.9f);
    painter->SetBrush(iGame::Brush::Style::NoBrush);
    painter->DrawCube(p1, p2);
}

static void DrawOneBoundingBox(Painter3D* painter, const std::pair<Point, Point>& box) {
    DrawOneBoundingBox(painter, box.first, box.second);
}

static void CollectCellLines(Cell* cell, std::vector<std::pair<int, int>>& lines) {
    if (cell == nullptr) return;
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 1) return;
        for (int pointI = 0; pointI < pointSize; pointI++) {
            auto pointIA = pointI;
            auto pointIB = (pointI + 1) % pointSize;
            auto pointIdA = cell->GetPointId(pointIA);
            auto pointIdB = cell->GetPointId(pointIB);
            lines.push_back(std::minmax(pointIdA, pointIdB));
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            CollectCellLines(face, lines);
        }
    }
}

static void DrawCell(UnstructuredMesh* mesh, Painter3D* painter, Cell* cell, std::vector<IGuint>& drawHandles) {
    std::vector<std::pair<int, int>> needDrawLines;
    CollectCellLines(cell, needDrawLines);
    std::sort(needDrawLines.begin(), needDrawLines.end());
    auto last = std::unique(needDrawLines.begin(), needDrawLines.end());
    needDrawLines.erase(last, needDrawLines.end());
    //Draw
    for (auto& idPair: needDrawLines) {
        auto& pointA = mesh->GetPoint(idPair.first);
        auto& pointB = mesh->GetPoint(idPair.second);
        auto drawHandle = painter->DrawLine(pointA, pointB);
        drawHandles.push_back(drawHandle);
    }
}

//std::vector<Selection::Event> Selection::GenerateEvents(const std::vector<igIndex>& ids, IGenum type,
//                                                        Event::Operate ope, UnstructuredMesh* mesh,
//                                                        Painter3D* painter) {
//    if (mesh == nullptr) return {};
//    switch (type) {
//        case IG_POINT: {
//            if (painter != nullptr) {
//                painter->SetPen(10);
//                painter->SetPen(Color::Red);
//            }
//            std::vector<Selection::Event> events;
//            for (auto& pointId: ids) {
//                Selection::Event e;
//                e.type = Selection::Event::PickPoint;
//                e.pickId = pointId;
//                auto& point = mesh->GetPoint(pointId);
//                if (ope == Selection::Event::Operate::Add) {
//                    e.operate = Selection::Event::Operate::Add;
//                    if (painter != nullptr) DrawPoint(painter, point, e.drawHandles);
//                } else
//                    e.operate = Selection::Event::Operate::Remove;
//                e.pos = point;
//                events.push_back(e);
//            }
//            return events;
//        } break;
//        case IG_CELL: {
//            if (painter != nullptr) {
//                painter->SetPen(3);
//                painter->SetPen(0.9f, 0.145f, 0.863f);
//            }
//            std::vector<Selection::Event> events;
//            for (int i = 0; i < ids.size(); i++) {
//                auto& cellId = ids[i];
//                Cell* cell = mesh->GetCell(cellId);
//                Selection::Event e;
//                e.type = Selection::Event::PickFace;
//                e.pickId = cellId;
//                if (ope == Selection::Event::Operate::Add) {
//                    e.operate = Selection::Event::Operate::Add;
//                    if (painter != nullptr) { DrawCell(mesh, painter, cell, e.drawHandles); }
//                } else
//                    e.operate = Selection::Event::Operate::Remove;
//                e.pos.setZero();
//                e.pos = GetCentralOfCell(cell);
//                //e.pos = Vector3f(intersect.x, intersect.y, intersect.z);
//                events.push_back(e);
//            }
//            return events;
//        } break;
//        default:
//            return {};
//            break;
//    }
//}
//
//std::vector<Selection::Event> Selection::GeneratePointEvents(const std::vector<igIndex>& ids, Event::Operate ope,
//                                                             UnstructuredMesh* mesh, Painter3D* painter) {
//    if (mesh == nullptr) return {};
//    if (painter != nullptr) {
//        painter->SetPen(10);
//        painter->SetPen(Color::Red);
//    }
//    std::vector<Selection::Event> events;
//    for (auto& pointId: ids) {
//        Selection::Event e;
//        e.type = Selection::Event::PickPoint;
//        e.pickId = pointId;
//        auto& point = mesh->GetPoint(pointId);
//        if (ope == Selection::Event::Operate::Add) {
//            e.operate = Selection::Event::Operate::Add;
//            if (painter != nullptr) DrawPoint(painter, point, e.drawHandles);
//        } else
//            e.operate = Selection::Event::Operate::Remove;
//        e.pos = point;
//        events.push_back(e);
//    }
//    return events;
//}
//
//std::vector<Selection::Event> Selection::GenerateCellEvents(const std::vector<igIndex>& ids, Event::Operate ope,
//                                                            UnstructuredMesh* mesh, Painter3D* painter) {
//    if (mesh == nullptr) return {};
//    if (painter != nullptr) {
//        painter->SetPen(3);
//        painter->SetPen(0.9f, 0.145f, 0.863f);
//    }
//    std::vector<Selection::Event> events;
//    for (int i = 0; i < ids.size(); i++) {
//        auto& cellId = ids[i];
//        Cell* cell = mesh->GetCell(cellId);
//        Selection::Event e;
//        e.type = Selection::Event::PickFace;
//        e.pickId = cellId;
//        if (ope == Selection::Event::Operate::Add) {
//            e.operate = Selection::Event::Operate::Add;
//            if (painter != nullptr) { DrawCell(mesh, painter, cell, e.drawHandles); }
//        } else
//            e.operate = Selection::Event::Operate::Remove;
//        e.pos.setZero();
//        e.pos = GetCentralOfCell(cell);
//        //e.pos = Vector3f(intersect.x, intersect.y, intersect.z);
//        events.push_back(e);
//    }
//    return events;
//}
//
//void Selection::SelectionCallBackEvent(const std::vector<Event>& _events, bool letCellDrawWithExtracter) {
//    if (_events.empty()) return;
//    if (letCellDrawWithExtracter&&m_Model!=nullptr) {
//        auto mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
//        bool shouldDraw{};
//        for (auto& _event: _events) {
//            if (_event.type != Event::Type::PickFace) continue;
//            shouldDraw = true;
//            auto& id = _event.pickId;
//            auto cell = mesh->GetCell(id);
//            if (_event.operate == Event::Operate::Add) m_CellFaceExtracter.AddCell(id, cell);
//            else if (_event.operate == Event::Operate::Remove)
//                m_CellFaceExtracter.RemoveCell(id, cell);
//        }
//        if (shouldDraw) {
//            auto edges = m_CellFaceExtracter.GetExtractPointIdPairs();
//            auto painter = m_Model->GetPainter3D(Painter3D::Usage::Selection);
//            std::vector<IGuint> handles;
//            DrawEdges(painter, edges, mesh, handles);
//            SetOtherDrawHandles(handles);
//        }
//    }
//    for (auto& _event: _events) { AddItem(_event); }
//    for (auto& callBackFunc: m_CallBackFunctor_old) { callBackFunc.second(_events); }
//}
//
//void Selection::SelectionCallBackEvent(const Event& event, bool letCellDrawWithExtracter) {
//    if (letCellDrawWithExtracter && m_Model != nullptr) {
//        auto mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
//        auto& _event = event;
//        if (_event.type == Event::Type::PickFace) {
//            auto& id = _event.pickId;
//            auto cell = mesh->GetCell(id);
//            if (_event.operate == Event::Operate::Add) m_CellFaceExtracter.AddCell(id, cell);
//            else if (_event.operate == Event::Operate::Remove)
//                m_CellFaceExtracter.RemoveCell(id, cell);
//            auto edges = m_CellFaceExtracter.GetExtractPointIdPairs();
//            auto painter = m_Model->GetPainter3D(Painter3D::Usage::Selection);
//            std::vector<IGuint> handles;
//            DrawEdges(painter, edges, mesh, handles);
//            SetOtherDrawHandles(handles);
//        }
//    }
//    AddItem(event);
//    for (auto& callBackFunc: m_CallBackFunctor_old) { callBackFunc.second({event}); }
//}

void Selection::SelectionCallBackEvent(IGenum itemType, const std::vector<igIndex>& ids, Operate ope) {
    switch (itemType) {
        case IG_POINT: {
            for (auto& id: ids) { AddItem(itemType, id, ope); }
            for (auto& func: m_CallBackFunctor) { func.second(itemType, ids, ope); }
            DrawPoints();
        } break;
        case IG_CELL: {
            for (auto& id: ids) { AddItem(itemType, id, ope); }
            for (auto& func: m_CallBackFunctor) { func.second(itemType, ids, ope); }
            DrawCellEdges();
        } break;
        case IG_POINT_BOX: {
            auto pMinMax = m_CellFaceExtracter.GetPointsBoundingBox(ids, _GetMesh());
            for (auto& func: m_BoxSelectInitCallBackFunctor) { func.second(itemType, pMinMax.first, pMinMax.second); }
            SetBoxStyle(pMinMax);
        } break;
        case IG_CELL_BOX: {
            auto pMinMax = m_CellFaceExtracter.GetCellsBoundingBox(ids, _GetMesh());
            for (auto& func: m_BoxSelectInitCallBackFunctor) { func.second(itemType, pMinMax.first, pMinMax.second); }
            SetBoxStyle(pMinMax);
        } break;
        case IG_CHANGE: {
            for (auto& func : m_CallBackFunctor) { func.second(itemType, ids, ope); }
        } break;
        default:
            break;
    }
}

void Selection::SelectionCallBackEvent(IGenum itemType, const igIndex& id, Operate ope) {
    switch (itemType) {
        case IG_POINT: {
            AddItem(itemType, id, ope);
            for (auto& func: m_CallBackFunctor) { func.second(itemType, {id}, ope); }
            DrawPoints();
        } break;
        case IG_CELL: {
            AddItem(itemType, id, ope);
            for (auto& func: m_CallBackFunctor) { func.second(itemType, {id}, ope); }
            DrawCellEdges();
        } break;
        case IG_POINT_BOX: {
            auto pMinMax = m_CellFaceExtracter.GetPointsBoundingBox({id}, _GetMesh());
            for (auto& func: m_BoxSelectInitCallBackFunctor) { func.second(itemType, pMinMax.first, pMinMax.second); }
            SetBoxStyle(pMinMax);
        } break;
        case IG_CELL_BOX: {
            auto pMinMax = m_CellFaceExtracter.GetCellsBoundingBox({id}, _GetMesh());
            for (auto& func: m_BoxSelectInitCallBackFunctor) { func.second(itemType, pMinMax.first, pMinMax.second); }
            SetBoxStyle(pMinMax);
        } break;
        case IG_CHANGE: {
            for (auto& func: m_CallBackFunctor) { func.second(itemType, {id}, ope); }
        } break;
        default:
            break;
    }
}

const std::set<igIndex>& Selection::GetSelectedItems(IGenum itemType) { return m_SelectedItems[itemType]; }

const std::set<igIndex>& Selection::GetSelectedCells() { return m_SelectedItems[IG_CELL]; }

const std::set<igIndex>& Selection::GetSelectedPoints() { return m_SelectedItems[IG_POINT]; }

bool Selection::IsSelectedItem(IGenum itemType, igIndex itemId) { return m_SelectedItems[itemType].count(itemId) != 0; }

void Selection::Reset() {
    if (m_Model != nullptr) {
        m_Model->GetPainter3D(Painter3D::Usage::SelectedPoint)->Clear();
        m_Model->GetPainter3D(Painter3D::Usage::SelectedCell)->Clear();
        m_Model->GetPainter3D(Painter3D::Usage::SelectionBox)->Clear();
    }
    m_SelectedItems.clear();
    for (auto& callBackFunc: m_ClearSelectionCallBackFunctor) { callBackFunc.second(); }
}

void Selection::ClearSelections() { Reset(); }

const std::vector<int>& Selection::GetSeeAbleCells(UnstructuredMesh* mesh) {
    if (m_SeeAbleCells.empty()) { m_SeeAbleCells = m_CellFaceExtracter.GetSurfaceCellIds(mesh); }
    return m_SeeAbleCells;
}

void Selection::SetSelectItemVisable(bool visable) {
    if (m_Model == nullptr) return;
    m_Model->GetPainter3D(Painter3D::Usage::SelectedPoint)->SetTotallyHide(!visable);
    m_Model->GetPainter3D(Painter3D::Usage::SelectedCell)->SetTotallyHide(!visable);
}

void Selection::SetSelectBoxVisable(bool visable) {
    if (m_Model == nullptr) return;
    m_Model->GetPainter3D(Painter3D::Usage::SelectionBox)->SetTotallyHide(!visable);
}

void Selection::SetBoxStyle(const std::pair<Point, Point>& p) {
    if (m_Model == nullptr) return;
    auto scene = m_Model->GetScene();
    if (scene == nullptr) return;
    auto interactor = scene->GetInteractor();
    if (interactor == nullptr) return;
    auto boxStyle = BoxStyle::New();
    boxStyle->Initialize(interactor);
    boxStyle->InitBox(p.first, p.second);
    interactor->_SetSpecialInteractor("SelectBox", boxStyle);
}

void Selection::AddItem(IGenum itemType, const igIndex& itemId, Operate ope) {
    if (ope == Operate::Remove) {
        m_SelectedItems[itemType].erase(itemId);
    } else if (ope == Operate::Add) {
        m_SelectedItems[itemType].insert(itemId);
    }
}

void Selection::DrawPoints() {
    if (m_Model == nullptr) return;
    auto mesh = _GetMesh();
    if (mesh == nullptr) return;
    auto painter = m_Model->GetPainter3D(Painter3D::Usage::SelectedPoint);
    if (painter == nullptr) return;
    painter->Clear();
    painter->SetPen(10);
    painter->SetPen(Color::Red);
    auto& pointIds = GetSelectedPoints();
    for (auto& pId: pointIds) {
        auto& point = mesh->GetPoint(pId);
        painter->DrawPoint(point);
    }
}

void Selection::DrawCellEdges() {
    if (m_Model == nullptr) return;
    auto mesh = _GetMesh();
    if (mesh == nullptr) return;
    auto painter = m_Model->GetPainter3D(Painter3D::Usage::SelectedCell);
    if (painter == nullptr) return;
    auto edges = m_CellFaceExtracter.GetExtractPointIdPairs(m_SelectedItems[IG_CELL], mesh);
    DrawEdges(painter, edges, mesh);
}

void Selection::DrawBoundingBox(const std::pair<Point, Point>& p) {
    if (m_Model == nullptr) return;
    auto mesh = _GetMesh();
    if (mesh == nullptr) return;
    auto painter = m_Model->GetPainter3D(Painter3D::Usage::SelectionBox);
    if (painter == nullptr) return;
    DrawOneBoundingBox(painter, p);
}

void Selection::DrawCellBoundingBoxs() {
    if (m_Model == nullptr) return;
    auto mesh = _GetMesh();
    if (mesh == nullptr) return;
    auto painter = m_Model->GetPainter3D(Painter3D::Usage::SelectionBox);
    if (painter == nullptr) return;
    auto boxs = m_CellFaceExtracter.GetExtractBoundingBoxs(m_SelectedItems[IG_CELL], mesh);
    DrawBoundingBoxs(painter, boxs);
}

UnstructuredMesh* Selection::_GetMesh() {
    if (m_Model == nullptr) return nullptr;
    if (m_Model->GetDataObject()->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
        return DynamicCast<UnstructuredMesh>(m_Model->GetDataObject());
    }
    if (m_DataObjectPointerMesh == nullptr) {
        m_DataObjectPointerMesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
    }
    return DynamicCast<UnstructuredMesh>(m_DataObjectPointerMesh);
}

IGAME_NAMESPACE_END
