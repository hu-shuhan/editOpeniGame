#include "iGameSingleSelectionStyle.h"
#include "iGameCtxPresObjData.h"
#include "iGameHistogramPicker.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"
#include "iGameUnstructuredMesh.h"
#include <iGameCellFaceExtracter.h>
#include <iGameSelectionParameter.h>

IGAME_NAMESPACE_BEGIN

static int RandomPickNum = 5000;
static int BoxNum = 500;

static double SegmentIntersectsTriangle(const Point& start, const Point& end,
                                        const Point& a, const Point& b,
                                        const Point& c) {
    Point dir = {end[0] - start[0], end[1] - start[1], end[2] - start[2]};
    Point ab = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    Point ac = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    Point pvec = dir.cross(ac);

    double det = ab.dot(pvec);
    if (std::abs(det) < 1e-7) { return -1; }

    double invDet = 1.0 / det;
    Point tvec = {start[0] - a[0], start[1] - a[1], start[2] - a[2]};
    double u = tvec.dot(pvec) * invDet;
    if (u < -1e-7 || u > 1 + 1e-7) { return -1; }

    Point qvec = tvec.cross(ab);
    double v = dir.dot(qvec) * invDet;
    if (v < -1e-7 || u + v > 1 + 1e-7) { return -1; }

    double t = ac.dot(qvec) * invDet;
    if (t < 1e-7 || t > 1 - 1e-7) { return -1; }

    if (u > 1e-7 && v > 1e-7 && u + v < 1 - 1e-7) { return t * dir.length(); }
    return -1;
}

static double IsLineCrossCell(const Point& startPoint, const Point& endPoint,
                              Cell* cell) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 2) return false;
        auto& p0 = cell->GetPoint(0);
        for (int i = 2; i < pointSize; i++) {
            auto& p1 = cell->GetPoint(i - 1);
            auto& p2 = cell->GetPoint(i);
            auto dis =
                    SegmentIntersectsTriangle(startPoint, endPoint, p0, p1, p2);
            if (dis >= 0) return dis;
        }
        return -1;
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            auto dis = IsLineCrossCell(startPoint, endPoint, face);
            if (dis >= 0) return dis;
        }
        return -1;
    }
}

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

void SingleSelectionStyle::SelectPoint(igm::vec2 pos) {
    if (m_Model == nullptr || !m_Selection) { return; }
    auto mvp = m_Interactor->GetMVP();
    auto mvp_invert = mvp.invert();

    // 3D World coordinate
    igm::vec3 point1_ = GetNearWorldCoord(pos, mvp_invert);
    igm::vec3 point2_ = GetFarWorldCoord(pos, mvp_invert);

    Point point1(point1_.x, point1_.y, point1_.z);
    Point point2(point2_.x, point2_.y, point2_.z);
    auto mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(
            m_Model->GetDataObject());

    auto ids = GetPointsInCondition(
            point1, point2, mesh,
            SelectionParameter::Instance().GetSelectionRadius(),
            (SelectionParameter::Instance().GetSelectVariableIndex() >= 0),
            SelectionParameter::Instance().GetSelectVariableIndex(),
            (SelectionParameter::Instance().GetSelectVariableRange() < 0),
            SelectionParameter::Instance().GetSelectVariableRange());
    if (ids.empty()) return;
    if (SelectionParameter::Instance().GetSelectOrUnSelect()) {
        auto events = Selection::GeneratePointEvents(
                ids, Selection::Event::Operate::Add, mesh,
                m_Model->GetPainter3D().get());
        m_Selection->SelectionCallBackEvent(events);
    } else {
        auto events = Selection::GeneratePointEvents(
                ids, Selection::Event::Operate::Remove, mesh,
                m_Model->GetPainter3D().get());
        m_Selection->SelectionCallBackEvent(events);
    }
    return;
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

void SingleSelectionStyle::SelectFace(igm::vec2 pos) {
    if (m_Points == nullptr || m_Cells == nullptr) { return; }
    auto mvp = m_Interactor->GetMVP();
    auto mvp_invert = mvp.invert();

    // 3D World coordinate
    igm::vec3 point1_ = GetNearWorldCoord(pos, mvp_invert);
    igm::vec3 point2_ = GetFarWorldCoord(pos, mvp_invert);

    Point point1(point1_.x, point1_.y, point1_.z);
    Point point2(point2_.x, point2_.y, point2_.z);
    auto mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(
            m_Model->GetDataObject());
    auto ids = GetCellsInCondition(
            point1, point2, mesh,
            SelectionParameter::Instance().GetSelectionRadius(),
            (SelectionParameter::Instance().GetSelectVariableIndex() >= 0),
            SelectionParameter::Instance().GetSelectVariableIndex(),
            (SelectionParameter::Instance().GetSelectVariableRange() < 0),
            SelectionParameter::Instance().GetSelectVariableRange());
    if (ids.empty()) return;
    if (SelectionParameter::Instance().GetSelectOrUnSelect()) {
        //auto events = Selection::GenerateEvents(
        //        ids, IG_CELL, Selection::Event::Operate::Add, mesh,
        //        m_Model->GetPainter3D().get());
        auto events = Selection::GenerateCellEvents(
                ids, Selection::Event::Operate::Add, mesh);
        m_Selection->SelectionCallBackEvent(events, true);
    } else {
        auto events = Selection::GenerateCellEvents(
                ids, Selection::Event::Operate::Remove, mesh);
        m_Selection->SelectionCallBackEvent(events, true);
    }
    return;
}

std::vector<int> SingleSelectionStyle::GetPointsInCondition(
        const Point& startPoint, const Point& endPoint, UnstructuredMesh* mesh,
        double radius, bool useVariableCondition, int variableIndex,
        bool useAutoValueRange, double valueRange) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    SmartPointer<PointPicker> picker = PointPicker::New();
    picker->SetDataObject(mesh);
    Point p;
    auto id = picker->PickClosetPointOnLine(startPoint, (endPoint - startPoint),
                                            p);
    if (id == -1) return re;
    if (radius == 0) {
        re.push_back(id);
        return re;
    }
    auto points = mesh->GetPoints();
    auto& thisPoint = points->GetPoint(id);
    /*################################# CORE START #################################*/
    if (useVariableCondition == false || variableIndex < 0 ||
        (useAutoValueRange == false && valueRange >= 1.0)) {
        for (int pointId = 0; pointId < points->GetNumberOfPoints();
             pointId++) {
            auto& point = points->GetPoint(pointId);
            if ((thisPoint - point).length() <= radius) {
                re.push_back(pointId);
            }
        }
        return re;
    }

    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    std::vector<std::pair<int, int>> variableIndexs =
            CtxPresObjData_Main::GenerateVariableIndex(attrs, IG_POINT);
    if (variableIndex >= variableIndexs.size()) return re;
    std::pair<std::vector<double>, std::vector<double>> variableMinMaxData =
            CtxPresObjData_Main::GenerateMinMaxData(attrs, IG_POINT);

    if (valueRange >= 0 && useAutoValueRange == false) {
        double dataRange = (variableMinMaxData.second[variableIndex] -
                            variableMinMaxData.first[variableIndex]) *
                           valueRange;
        double thisPointData = CtxPresObjData_Main::GenerateObjData(
                id, attrs, variableIndexs[variableIndex]);
        for (int pointId = 0; pointId < points->GetNumberOfPoints();
             pointId++) {
            auto& point = points->GetPoint(pointId);
            double pointData = CtxPresObjData_Main::GenerateObjData(
                    pointId, attrs, variableIndexs[variableIndex]);
            if (((thisPoint - point).length() <= radius) &&
                (std::abs(thisPointData - pointData) <= dataRange)) {
                re.push_back(pointId);
            }
        }
    } else if (valueRange < 0 || useAutoValueRange == true) {
        auto hisPicker = HistogramPicker(
                attrs, variableIndexs[variableIndex],
                points->GetNumberOfPoints(), BoxNum, RandomPickNum,
                variableMinMaxData.first[variableIndex],
                variableMinMaxData.second[variableIndex]);
        double thisPointData = CtxPresObjData_Main::GenerateObjData(
                id, attrs, variableIndexs[variableIndex]);
        auto [minRange, maxRange] =
                hisPicker.CalculateMinMaxValueToPick(thisPointData);
        for (int pointId = 0; pointId < points->GetNumberOfPoints();
             pointId++) {
            auto& point = points->GetPoint(pointId);
            double pointData = CtxPresObjData_Main::GenerateObjData(
                    pointId, attrs, variableIndexs[variableIndex]);
            if (((thisPoint - point).length() <= radius) &&
                (minRange <= pointData && pointData <= maxRange)) {
                re.push_back(pointId);
            }
        }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetCellsInCondition(
        const Point& startPoint, const Point& endPoint, UnstructuredMesh* mesh,
        double radius, bool useVariableCondition, int variableIndex,
        bool useAutoValueRange, double valueRange) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    double minDis = -1;
    int id = -1;
    for (int cellId = 0; cellId < mesh->GetNumberOfCells(); cellId++) {
        Cell* cell = mesh->GetCell(cellId);
        auto dis = IsLineCrossCell(startPoint, endPoint, cell);
        if (dis < 0) continue;
        if (minDis == -1 || dis < minDis) {
            minDis = dis;
            id = cellId;
        }
    }
    if (id == -1) return re;
    if (radius == 0) {
        re.push_back(id);
        return re;
    }

    auto cells = mesh->GetCells();
    auto points = mesh->GetPoints();
    igIndex thisCell[IGAME_CELL_MAX_SIZE]{};
    int thisCellSize = cells->GetCellIds(id, thisCell);
    iGame::Point thisCellCentralPoint =
            GetCentralOfCell(thisCellSize, thisCell, points);
    /*################################# CORE START #################################*/
    if (useVariableCondition == false || variableIndex < 0 ||
        (useAutoValueRange == false && valueRange >= 1.0)) {
        //Calculate the center point of each surface and compare the radii.
        for (int cellIndex = 0; cellIndex < cells->GetNumberOfCells();
             cellIndex++) {
            igIndex thatCell[IGAME_CELL_MAX_SIZE]{};
            int thatCellSize = cells->GetCellIds(cellIndex, thatCell);
            Point thatCellCentralPoint =
                    GetCentralOfCell(thatCellSize, thatCell, points);
            if ((thisCellCentralPoint - thatCellCentralPoint).length() <=
                radius) {
                re.push_back(cellIndex);
                //selectedCellCenters.push_back(thatCellCentralPoint);
            }
        }
        return re;
    }

    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    std::vector<std::pair<int, int>> variableIndexs =
            CtxPresObjData_Main::GenerateVariableIndex(attrs, IG_CELL);
    if (variableIndex >= variableIndexs.size()) return re;
    std::pair<std::vector<double>, std::vector<double>> variableMinMaxData =
            CtxPresObjData_Main::GenerateMinMaxData(attrs, IG_CELL);

    if (valueRange >= 0 && useAutoValueRange == false) {
        double dataRange = (variableMinMaxData.second[variableIndex] -
                            variableMinMaxData.first[variableIndex]) *
                           valueRange;
        double thisCellData = CtxPresObjData_Main::GenerateObjData(
                id, attrs, variableIndexs[variableIndex]);
        for (int cellIndex = 0; cellIndex < cells->GetNumberOfCells();
             cellIndex++) {
            igIndex thatCell[IGAME_CELL_MAX_SIZE]{};
            int thatCellSize = cells->GetCellIds(cellIndex, thatCell);
            Point thatCellCentralPoint =
                    GetCentralOfCell(thatCellSize, thatCell, points);
            double cellData = CtxPresObjData_Main::GenerateObjData(
                    cellIndex, attrs, variableIndexs[variableIndex]);
            if (((thisCellCentralPoint - thatCellCentralPoint).length() <=
                 radius) &&
                (std::abs(thisCellData - cellData) <= dataRange)) {
                re.push_back(cellIndex);
                //selectedCellCenters.push_back(thatCellCentralPoint);
            }
        }
    } else if (valueRange < 0 || useAutoValueRange == true) {
        auto hisPicker = HistogramPicker(
                attrs, variableIndexs[variableIndex], cells->GetNumberOfCells(),
                BoxNum, RandomPickNum, variableMinMaxData.first[variableIndex],
                variableMinMaxData.second[variableIndex]);
        double thisCellData = CtxPresObjData_Main::GenerateObjData(
                id, attrs, variableIndexs[variableIndex]);
        auto [minRange, maxRange] =
                hisPicker.CalculateMinMaxValueToPick(thisCellData);
        for (int cellIndex = 0; cellIndex < cells->GetNumberOfCells();
             cellIndex++) {
            igIndex thatCell[IGAME_CELL_MAX_SIZE]{};
            int thatCellSize = cells->GetCellIds(cellIndex, thatCell);
            Point thatCellCentralPoint =
                    GetCentralOfCell(thatCellSize, thatCell, points);
            double cellData = CtxPresObjData_Main::GenerateObjData(
                    cellIndex, attrs, variableIndexs[variableIndex]);
            if (((thisCellCentralPoint - thatCellCentralPoint).length() <=
                 radius) &&
                (minRange <= cellData && cellData <= maxRange)) {
                re.push_back(cellIndex);
                //selectedCellCenters.push_back(thatCellCentralPoint);
            }
        }
    }
    return re;
    /*################################# CORE END #################################*/
}

IGAME_NAMESPACE_END
