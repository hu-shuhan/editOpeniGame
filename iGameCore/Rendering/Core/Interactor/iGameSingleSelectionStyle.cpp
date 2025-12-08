#include "iGameSingleSelectionStyle.h"
#include "iGameCtxPresObjData.h"
#include "iGameHistogramPicker.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"
#include "iGameUnstructuredMesh.h"
#include <iGameCellFaceExtracter.h>
#include <iGameSelectionParameter.h>
#include <iGameTimer.h>
#include <map>
#include <queue>
#include <set>

IGAME_NAMESPACE_BEGIN

static int RandomPickNum = 5000;
static int BoxNum = 500;

static double SegmentIntersectsTriangle(const Point& start, const Point& end,
                                        const Point& a, const Point& b,
                                        const Point& c) {
    // 计算方向向量（从start指向end）
    Point dir = {end[0] - start[0], end[1] - start[1], end[2] - start[2]};
    double segmentLength = dir.length();

    // 如果线段长度为0，直接返回-1（没有交点）
    if (segmentLength < 1e-7) { return -1; }

    // 标准化方向向量，使其长度为1
    Point normalizedDir = {(float) (dir[0] / segmentLength),
                           (float) (dir[1] / segmentLength),
                           (float) (dir[2] / segmentLength)};

    Point ab = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    Point ac = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};

    // 使用标准化后的方向向量进行计算
    Point pvec = normalizedDir.cross(ac);
    double det = ab.dot(pvec);

    if (std::abs(det) < 1e-7) { return -1; }

    double invDet = 1.0 / det;
    Point tvec = {start[0] - a[0], start[1] - a[1], start[2] - a[2]};
    double u = tvec.dot(pvec) * invDet;
    if (u < -1e-7 || u > 1 + 1e-7) { return -1; }

    Point qvec = tvec.cross(ab);
    double v = normalizedDir.dot(qvec) * invDet;
    if (v < -1e-7 || u + v > 1 + 1e-7) { return -1; }

    double t = ac.dot(qvec) * invDet;

    // 检查交点是否在线段范围内（从start出发，沿着方向向量的距离）
    if (t < 1e-7) { return -1; }

    if (u > 1e-7 && v > 1e-7 && u + v < 1 - 1e-7) {
        return t; // 返回实际的距离值
    }
    return -1;
}

static double IsLineCrossFace(const Point& startPoint, const Point& endPoint,
                              const std::vector<int>& face,
                              UnstructuredMesh* mesh) {
    if (face.size() <= 2) return -1;
    auto& p0 = mesh->GetPoint(face[0]);
    for (int i = 2; i < face.size(); i++) {
        auto& p1 = mesh->GetPoint(face[i - 1]);
        auto& p2 = mesh->GetPoint(face[i]);
        auto dis = SegmentIntersectsTriangle(startPoint, endPoint, p0, p1, p2);
        if (dis >= 0) return dis;
    }
    return -1;
}

static double IsLineCrossCell(const Point& startPoint, const Point& endPoint,
                              Cell* cell) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 2) return -1;
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

// 静态方法：计算面的法向量，处理退化情况
static Point ComputeFaceNormal(const std::array<Point, 4>& face) {
    // 检查面是否退化（所有点相同）
    bool allPointsSame = true;
    for (size_t i = 1; i < face.size(); ++i) {
        if (!(face[0] == face[i])) {
            allPointsSame = false;
            break;
        }
    }

    if (allPointsSame) {
        // 退化面，返回零向量（会被后续处理识别）
        return Point(0, 0, 0);
    }

    // 尝试不同的边组合来确保非零法向量
    Point v1 = face[1] - face[0];
    Point v2 = face[2] - face[1];
    Point normal = v1.cross(v2);

    // 如果叉积接近零，尝试其他边的组合
    if (normal.squaredLength() < 1e-12) {
        v1 = face[2] - face[0];
        v2 = face[3] - face[0];
        normal = v1.cross(v2);
    }

    // 再次检查，如果仍然接近零，则面可能退化
    if (normal.squaredLength() < 1e-12) { return Point(0, 0, 0); }

    return normal.normalized();
}

// 静态方法：检查点是否在面的同一侧（内部），处理退化情况
static bool IsPointOnSameSide(const Point& p, const std::array<Point, 4>& face,
                              bool& faceDegenerate) {
    Point normal = ComputeFaceNormal(face);

    // 检查法向量是否为零（退化面）
    if (normal.squaredLength() < 1e-12) {
        faceDegenerate = true;
        // 对于退化面，检查点是否与面上的点相同
        // 实际上，如果是包围盒的一个面退化，那么这个面无法提供有效的约束
        // 返回true让其他非退化面来决定
        return true;
    }

    faceDegenerate = false;

    // 找到面上第一个非退化的参考点
    const Point& refPoint = face[0];

    // 计算点到面的有向距离
    double dotProduct = normal[0] * (p[0] - refPoint[0]) +
                        normal[1] * (p[1] - refPoint[1]) +
                        normal[2] * (p[2] - refPoint[2]);

    // 对于立方体，所有内部点应该在面的同一侧
    // 这里假设法向量指向外部，所以内部点的点积应该为负或零（在面上）
    return dotProduct <= 1e-12; // 使用容差
}

// 静态方法：检查点是否在包围盒内，处理全部退化的情况
static bool IsPointInside(const Point& p,
                          const std::array<std::array<Point, 4>, 6>& allFaces) {

    // 首先检查包围盒是否完全退化（所有顶点相同）
    bool allFacesDegenerate = true;
    bool hasValidFace = false;

    // 收集所有顶点（去重）
    std::vector<Point> uniqueVertices;
    for (const auto& face: allFaces) {
        for (const auto& vertex: face) {
            bool found = false;
            for (const auto& uniqueVert: uniqueVertices) {
                if (vertex == uniqueVert) {
                    found = true;
                    break;
                }
            }
            if (!found) { uniqueVertices.push_back(vertex); }
        }
    }

    // 如果所有顶点相同（只有一个唯一顶点）
    if (uniqueVertices.size() == 1) {
        // 包围盒退化到一点，只有该点在其内部
        return (p == uniqueVertices[0]);
    }

    // 正常情况：检查所有面
    int degenerateFaceCount = 0;

    for (const auto& face: allFaces) {
        bool faceDegenerate = false;
        if (!IsPointOnSameSide(p, face, faceDegenerate)) { return false; }
        if (faceDegenerate) { degenerateFaceCount++; }
    }

    // 如果所有面都退化（理论上不应该发生，因为上面已经检查过顶点）
    if (degenerateFaceCount == 6) {
        // 这应该已经被上面的顶点检查捕获，但作为额外保护
        // 检查点是否与任何顶点相同
        for (const auto& face: allFaces) {
            for (const auto& vertex: face) {
                if (p == vertex) { return true; }
            }
        }
        return false;
    }

    return true;
}

SingleSelectionStyle::SingleSelectionStyle() {}
SingleSelectionStyle::~SingleSelectionStyle() {}

void SingleSelectionStyle::MousePressEvent(IEvent _event) {
    SelectionStyle::MousePressEvent(_event);

    if (_event.button != MiddleButton) return;
    if (!SelectionParameter::Instance().GetInSelection()) return;
    switch (GetSelectedType()) {
        case SelectionStyle::SelectPoint:
            if (SelectionParameter::Instance().GetInSelection() &&
                SelectionParameter::Instance().GetHaveBox())
                return;
            this->SelectPoint(_event.pos);
            break;
        case SelectionStyle::SelectCell:
            if (SelectionParameter::Instance().GetInSelection() &&
                SelectionParameter::Instance().GetHaveBox())
                return;
            this->SelectCell(_event.pos);
            break;
        default:
            break;
    }
}

void SingleSelectionStyle::SelectPoint(igm::vec2 pos) {
    if (m_Model == nullptr || !m_Selection) { return; }

    auto [point1, point2] = GetStartPointAndEndPoint(pos);

    std::vector<int> ids;

    auto dataObj = m_Model->GetDataObject();
    auto meshType = dataObj->GetDataObjectType();
    switch (meshType) {
        case IG_SURFACE_MESH: {
            auto mesh = DynamicCast<SurfaceMesh>(dataObj);
            mesh->RequestEditStatus();
            ids = GetPointsInCondition(
                    point1, point2, mesh,
                    SelectionParameter::Instance().GetSelectionRadius(),
                    SelectionParameter::Instance().IsCtMode() ||
                            SelectionParameter::Instance().IsCtBoxMode(),
                    SelectionParameter::Instance().GetSelectVariableIndex(),
                    SelectionParameter::Instance().GetAutoSelectExpdRate(),
                    SelectionParameter::Instance()
                            .GetSelectIgnoreUnSeeAbleCells(),
                    SelectionParameter::Instance()
                            .GetSelectOnlySelectSeeAbleCells());
        } break;
        //case IG_STRUCTURED_MESH:
        case IG_VOLUME_MESH: {
            auto mesh = DynamicCast<VolumeMesh>(dataObj);
            mesh->RequestEditStatus();
            ids = GetPointsInCondition(
                    point1, point2, mesh,
                    SelectionParameter::Instance().GetSelectionRadius(),
                    SelectionParameter::Instance().IsCtMode() ||
                            SelectionParameter::Instance().IsCtBoxMode(),
                    SelectionParameter::Instance().GetSelectVariableIndex(),
                    SelectionParameter::Instance().GetAutoSelectExpdRate(),
                    SelectionParameter::Instance()
                            .GetSelectIgnoreUnSeeAbleCells(),
                    SelectionParameter::Instance()
                            .GetSelectOnlySelectSeeAbleCells());
        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
            ids = GetPointsInCondition(
                    point1, point2, mesh,
                    SelectionParameter::Instance().GetSelectionRadius(),
                    SelectionParameter::Instance().IsCtMode() ||
                            SelectionParameter::Instance().IsCtBoxMode(),
                    SelectionParameter::Instance().GetSelectVariableIndex(),
                    SelectionParameter::Instance().GetAutoSelectExpdRate(),
                    SelectionParameter::Instance()
                            .GetSelectIgnoreUnSeeAbleCells(),
                    SelectionParameter::Instance()
                            .GetSelectOnlySelectSeeAbleCells());
        } break;
        default:
            return;
    }

    //auto mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(
    //        m_Model->GetDataObject());

    //auto ids = GetPointsInCondition(
    //        point1, point2, mesh,
    //        SelectionParameter::Instance().GetSelectionRadius(),
    //        SelectionParameter::Instance().IsCtMode() ||
    //                SelectionParameter::Instance().IsCtBoxMode(),
    //        SelectionParameter::Instance().GetSelectVariableIndex(),
    //        SelectionParameter::Instance().GetAutoSelectExpdRate());

    if (ids.empty()) return;

    if (SelectionParameter::Instance().IsBoxMode()) {
        m_Selection->SelectionCallBackEvent(IG_POINT_BOX, ids);
        return;
    }

    if (SelectionParameter::Instance().GetSelectOrUnSelect()) {
        m_Selection->SelectionCallBackEvent(IG_POINT, ids,
                                            Selection::Operate::Add);
    } else {
        m_Selection->SelectionCallBackEvent(IG_POINT, ids,
                                            Selection::Operate::Remove);
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

void SingleSelectionStyle::SelectCell(igm::vec2 pos) {
    _AT_;
    if (m_Points == nullptr || m_Cells == nullptr) { return; }
    auto [point1, point2] = GetStartPointAndEndPoint(pos);

    std::vector<int> ids;

    auto dataObj = m_Model->GetDataObject();
    auto meshType = dataObj->GetDataObjectType();
    switch (meshType) {
        case IG_SURFACE_MESH: {
            auto mesh = DynamicCast<SurfaceMesh>(dataObj);
            mesh->RequestEditStatus();
            ids = GetCellsInCondition(
                    point1, point2, mesh,
                    SelectionParameter::Instance().GetSelectionRadius(),
                    SelectionParameter::Instance().IsCtMode() ||
                            SelectionParameter::Instance().IsCtBoxMode(),
                    SelectionParameter::Instance().GetSelectVariableIndex(),
                    SelectionParameter::Instance().GetAutoSelectExpdRate(),
                    SelectionParameter::Instance()
                            .GetSelectIgnoreUnSeeAbleCells(),
                    SelectionParameter::Instance()
                            .GetSelectOnlySelectSeeAbleCells());
        } break;
        //case IG_STRUCTURED_MESH:
        case IG_VOLUME_MESH: {
            auto mesh = DynamicCast<VolumeMesh>(dataObj);
            mesh->RequestEditStatus();
            ids = GetCellsInCondition(
                    point1, point2, mesh,
                    SelectionParameter::Instance().GetSelectionRadius(),
                    SelectionParameter::Instance().IsCtMode() ||
                            SelectionParameter::Instance().IsCtBoxMode(),
                    SelectionParameter::Instance().GetSelectVariableIndex(),
                    SelectionParameter::Instance().GetAutoSelectExpdRate(),
                    SelectionParameter::Instance()
                            .GetSelectIgnoreUnSeeAbleCells(),
                    SelectionParameter::Instance()
                            .GetSelectOnlySelectSeeAbleCells());
        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
            ids = GetCellsInCondition(
                    point1, point2, mesh,
                    SelectionParameter::Instance().GetSelectionRadius(),
                    SelectionParameter::Instance().IsCtMode() ||
                            SelectionParameter::Instance().IsCtBoxMode(),
                    SelectionParameter::Instance().GetSelectVariableIndex(),
                    SelectionParameter::Instance().GetAutoSelectExpdRate(),
                    SelectionParameter::Instance()
                            .GetSelectIgnoreUnSeeAbleCells(),
                    SelectionParameter::Instance()
                            .GetSelectOnlySelectSeeAbleCells());
        } break;
        default:
            return;
    }

    //auto mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(
    //        m_Model->GetDataObject());

    //auto ids = GetCellsInCondition(
    //        point1, point2, mesh,
    //        SelectionParameter::Instance().GetSelectionRadius(),
    //        SelectionParameter::Instance().IsCtMode() ||
    //                SelectionParameter::Instance().IsCtBoxMode(),
    //        SelectionParameter::Instance().GetSelectVariableIndex(),
    //        SelectionParameter::Instance().GetAutoSelectExpdRate(),
    //        SelectionParameter::Instance().GetSelectIgnoreUnSeeAbleCells(),
    //        SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());

    if (ids.empty()) return;

    if (SelectionParameter::Instance().IsBoxMode()) {
        m_Selection->SelectionCallBackEvent(IG_CELL_BOX, ids);
        return;
    }

    if (SelectionParameter::Instance().GetSelectOrUnSelect()) {
        m_Selection->SelectionCallBackEvent(IG_CELL, ids,
                                            Selection::Operate::Add);
    } else {
        m_Selection->SelectionCallBackEvent(IG_CELL, ids,
                                            Selection::Operate::Remove);
    }
    return;
}

std::pair<Point, Point>
SingleSelectionStyle::GetStartPointAndEndPoint(igm::vec2 pos) {
    auto mvp = m_Interactor->GetMVP();
    auto mvp_invert = mvp.invert();

    // 3D World coordinate
    igm::vec3 point1_ = GetNearWorldCoord(pos, mvp_invert);
    igm::vec3 point2_ = GetFarWorldCoord(pos, mvp_invert);

    Point point1(point1_.x, point1_.y, point1_.z);
    Point point2(point2_.x, point2_.y, point2_.z);
    return {point1, point2};
}

std::vector<int> SingleSelectionStyle::GetPointsInCondition(
        const Point& startPoint, const Point& endPoint, UnstructuredMesh* mesh,
        double radius, bool useAutoSelect, int variableIndex,
        double autoSelectExpdRate, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    if (!useAutoSelect || variableIndex < 0) {
        return GetPointsInRadiusMode(startPoint, endPoint, mesh, radius,
                                     selectIgnoreUnSeeAbleCells,
                                     onlySelectSeeAbleCells);
    } else {
        return GetPointsInCtMode(startPoint, endPoint, mesh, radius,
                                 variableIndex, autoSelectExpdRate,
                                 selectIgnoreUnSeeAbleCells,
                                 onlySelectSeeAbleCells);
    }
}

std::vector<int> SingleSelectionStyle::GetPointsInCondition(
        const Point& startPoint, const Point& endPoint, VolumeMesh* mesh,
        double radius, bool useAutoSelect, int variableIndex,
        double autoSelectExpdRate, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    if (!useAutoSelect || variableIndex < 0) {
        return GetPointsInRadiusMode(startPoint, endPoint, mesh, radius,
                                     selectIgnoreUnSeeAbleCells,
                                     onlySelectSeeAbleCells);
    } else {
        return GetPointsInCtMode(startPoint, endPoint, mesh, radius,
                                 variableIndex, autoSelectExpdRate,
                                 selectIgnoreUnSeeAbleCells,
                                 onlySelectSeeAbleCells);
    }
}

std::vector<int> SingleSelectionStyle::GetPointsInCondition(
        const Point& startPoint, const Point& endPoint, SurfaceMesh* mesh,
        double radius, bool useAutoSelect, int variableIndex,
        double autoSelectExpdRate, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    if (!useAutoSelect || variableIndex < 0) {
        return GetPointsInRadiusMode(startPoint, endPoint, mesh, radius,
                                     selectIgnoreUnSeeAbleCells,
                                     onlySelectSeeAbleCells);
    } else {
        return GetPointsInCtMode(startPoint, endPoint, mesh, radius,
                                 variableIndex, autoSelectExpdRate,
                                 selectIgnoreUnSeeAbleCells,
                                 onlySelectSeeAbleCells);
    }
}

template<class T>
static inline void SortVector(std::vector<T>& v) {
    std::sort(v.begin(), v.end());
}

static void BuildSeeAbleFaceForMesh_AddCell(
        int cellId, Cell* cell,
        std::map<std::vector<int>, std::vector<int>>& result) {
    if (cell == nullptr) return;
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 1) return;
        std::vector<int> facePointIds;
        for (int i = 0; i < pointSize; i++) {
            auto pointId = cell->GetPointId(i);
            facePointIds.push_back(pointId);
        }
        SortVector(facePointIds);
        result[facePointIds].push_back(cellId);
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            BuildSeeAbleFaceForMesh_AddCell(cellId, face, result);
        }
    }
}

static std::vector<int> BuildSeeAbleFaceForMesh(UnstructuredMesh* mesh) {
    std::map<std::vector<int>, std::vector<int>> tempRe;
    int cellNum = mesh->GetNumberOfCells();
    for (int cellId = 0; cellId < cellNum; cellId++) {
        auto cell = mesh->GetCell(cellId);
        BuildSeeAbleFaceForMesh_AddCell(cellId, cell, tempRe);
    }
    std::set<int> reSet;
    for (auto& face_cell: tempRe) {
        if (face_cell.second.size() != 1) continue;
        reSet.insert(face_cell.second.front());
    }
    return std::vector<int>(reSet.begin(), reSet.end());
}

std::vector<int> SingleSelectionStyle::GetCellsInCondition(
        const Point& startPoint, const Point& endPoint, UnstructuredMesh* mesh,
        double radius, bool useAutoSelect, int variableIndex,
        double autoSelectExpdRate, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    if (!useAutoSelect || variableIndex < 0) {
        return GetCellsInRadiusMode(startPoint, endPoint, mesh, radius,
                                    selectIgnoreUnSeeAbleCells,
                                    onlySelectSeeAbleCells);
    } else {
        return GetCellsInCtMode(startPoint, endPoint, mesh, radius,
                                variableIndex, autoSelectExpdRate,
                                selectIgnoreUnSeeAbleCells,
                                onlySelectSeeAbleCells);
    }
}

std::vector<int> SingleSelectionStyle::GetCellsInCondition(
        const Point& startPoint, const Point& endPoint, VolumeMesh* mesh,
        double radius, bool useAutoSelect, int variableIndex,
        double autoSelectExpdRate, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    if (!useAutoSelect || variableIndex < 0) {
        return GetCellsInRadiusMode(startPoint, endPoint, mesh, radius,
                                    selectIgnoreUnSeeAbleCells,
                                    onlySelectSeeAbleCells);
    } else {
        return GetCellsInCtMode(startPoint, endPoint, mesh, radius,
                                variableIndex, autoSelectExpdRate,
                                selectIgnoreUnSeeAbleCells,
                                onlySelectSeeAbleCells);
    }
}

std::vector<int> SingleSelectionStyle::GetCellsInCondition(
        const Point& startPoint, const Point& endPoint, SurfaceMesh* mesh,
        double radius, bool useAutoSelect, int variableIndex,
        double autoSelectExpdRate, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    if (!useAutoSelect || variableIndex < 0) {
        return GetCellsInRadiusMode(startPoint, endPoint, mesh, radius,
                                    selectIgnoreUnSeeAbleCells,
                                    onlySelectSeeAbleCells);
    } else {
        return GetCellsInCtMode(startPoint, endPoint, mesh, radius,
                                variableIndex, autoSelectExpdRate,
                                selectIgnoreUnSeeAbleCells,
                                onlySelectSeeAbleCells);
    }
}

std::vector<int>
SingleSelectionStyle::GetPointsInRadiusMode(const Point& startPoint,
                                            const Point& endPoint,
                                            PointSet* mesh, double radius) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    SmartPointer<PointPicker> picker = PointPicker::New();
    picker->SetDataObject(mesh);
    Point p;
    auto id = picker->PickClosetPointOnLine(startPoint, (endPoint - startPoint),
                                            p);
    if (id == -1) return re;
    if (radius <= 0) {
        re.push_back(id);
        return re;
    }
    auto points = mesh->GetPoints();
    auto& thisPoint = points->GetPoint(id);
    /*################################# CORE START #################################*/
    for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++) {
        auto& point = points->GetPoint(pointId);
        if ((thisPoint - point).length() <= radius) { re.push_back(pointId); }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInRadiusMode(
        const Point& startPoint, const Point& endPoint, UnstructuredMesh* mesh,
        double radius, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstPoint(startPoint, endPoint, mesh,
                            selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || (!onlySelectSeeAbleCells &&
                        SelectionParameter::Instance().IsBoxMode())) {
        re.push_back(id);
        return re;
    }
    auto points = mesh->GetPoints();
    auto& thisPoint = points->GetPoint(id);
    /*################################# CORE START #################################*/
    for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++) {
        auto& point = points->GetPoint(pointId);
        if ((thisPoint - point).length() <= radius) { re.push_back(pointId); }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInRadiusMode(
        const Point& startPoint, const Point& endPoint, VolumeMesh* mesh,
        double radius, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstPoint(startPoint, endPoint, mesh,
                            selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || (!onlySelectSeeAbleCells &&
                        SelectionParameter::Instance().IsBoxMode())) {
        re.push_back(id);
        return re;
    }
    auto points = mesh->GetPoints();
    auto& thisPoint = points->GetPoint(id);
    /*################################# CORE START #################################*/
    for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++) {
        auto& point = points->GetPoint(pointId);
        if ((thisPoint - point).length() <= radius) { re.push_back(pointId); }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInRadiusMode(
        const Point& startPoint, const Point& endPoint, SurfaceMesh* mesh,
        double radius, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstPoint(startPoint, endPoint, mesh,
                            selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || (!onlySelectSeeAbleCells &&
                        SelectionParameter::Instance().IsBoxMode())) {
        re.push_back(id);
        return re;
    }
    auto points = mesh->GetPoints();
    auto& thisPoint = points->GetPoint(id);
    /*################################# CORE START #################################*/
    for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++) {
        auto& point = points->GetPoint(pointId);
        if ((thisPoint - point).length() <= radius) { re.push_back(pointId); }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetCellsInRadiusMode(
        const Point& startPoint, const Point& endPoint, UnstructuredMesh* mesh,
        double radius, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstCell(startPoint, endPoint, mesh,
                           selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || (!onlySelectSeeAbleCells &&
                        SelectionParameter::Instance().IsBoxMode())) {
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
    auto _NormalSelectFunc = [&](int cellIndex) {
        igIndex thatCell[IGAME_CELL_MAX_SIZE]{};
        int thatCellSize = cells->GetCellIds(cellIndex, thatCell);
        Point thatCellCentralPoint =
                GetCentralOfCell(thatCellSize, thatCell, points);
        if ((thisCellCentralPoint - thatCellCentralPoint).length() <= radius) {
            re.push_back(cellIndex);
        }
    };

    if (onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellIndex: seeAbleFaces) { _NormalSelectFunc(cellIndex); }
    } else {
        for (int cellIndex = 0; cellIndex < cells->GetNumberOfCells();
             cellIndex++) {
            _NormalSelectFunc(cellIndex);
        }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetCellsInRadiusMode(
        const Point& startPoint, const Point& endPoint, VolumeMesh* mesh,
        double radius, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstCell(startPoint, endPoint, mesh,
                           selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || (!onlySelectSeeAbleCells &&
                        SelectionParameter::Instance().IsBoxMode())) {
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
    auto _NormalSelectFunc = [&](int cellIndex) {
        igIndex thatCell[IGAME_CELL_MAX_SIZE]{};
        int thatCellSize = cells->GetCellIds(cellIndex, thatCell);
        Point thatCellCentralPoint =
                GetCentralOfCell(thatCellSize, thatCell, points);
        if ((thisCellCentralPoint - thatCellCentralPoint).length() <= radius) {
            re.push_back(cellIndex);
        }
    };

    if (onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellIndex: seeAbleFaces) { _NormalSelectFunc(cellIndex); }
    } else {
        for (int cellIndex = 0; cellIndex < cells->GetNumberOfCells();
             cellIndex++) {
            _NormalSelectFunc(cellIndex);
        }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetCellsInRadiusMode(
        const Point& startPoint, const Point& endPoint, SurfaceMesh* mesh,
        double radius, bool selectIgnoreUnSeeAbleCells,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstCell(startPoint, endPoint, mesh,
                           selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || (!onlySelectSeeAbleCells &&
                        SelectionParameter::Instance().IsBoxMode())) {
        re.push_back(id);
        return re;
    }

    auto cells = mesh->GetFaces();
    auto points = mesh->GetPoints();
    igIndex thisCell[IGAME_CELL_MAX_SIZE]{};
    int thisCellSize = cells->GetCellIds(id, thisCell);
    iGame::Point thisCellCentralPoint =
            GetCentralOfCell(thisCellSize, thisCell, points);
    /*################################# CORE START #################################*/
    auto _NormalSelectFunc = [&](int cellIndex) {
        igIndex thatCell[IGAME_CELL_MAX_SIZE]{};
        int thatCellSize = cells->GetCellIds(cellIndex, thatCell);
        Point thatCellCentralPoint =
                GetCentralOfCell(thatCellSize, thatCell, points);
        if ((thisCellCentralPoint - thatCellCentralPoint).length() <= radius) {
            re.push_back(cellIndex);
        }
    };

    if (onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellIndex: seeAbleFaces) { _NormalSelectFunc(cellIndex); }
    } else {
        for (int cellIndex = 0; cellIndex < cells->GetNumberOfCells();
             cellIndex++) {
            _NormalSelectFunc(cellIndex);
        }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInCtMode(
        const Point& startPoint, const Point& endPoint, UnstructuredMesh* mesh,
        double radius, int variableIndex, double autoSelectExpdRate,
        bool selectIgnoreUnSeeAbleCells, bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstPoint(startPoint, endPoint, mesh,
                            selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || variableIndex < 0 || autoSelectExpdRate <= 0.0) {
        re.push_back(id);
        return re;
    }
    auto points = mesh->GetPoints();
    auto& thisPoint = points->GetPoint(id);
    /*################################# CORE START #################################*/
    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    std::vector<std::pair<int, int>> variableIndexs =
            CtxPresObjData_Main::GenerateVariableIndex(attrs, IG_POINT);
    if (variableIndex >= variableIndexs.size()) return re;
    std::pair<std::vector<double>, std::vector<double>> variableMinMaxData =
            CtxPresObjData_Main::GenerateMinMaxData(attrs, IG_POINT);

    auto hisPicker = HistogramPicker(
            attrs, variableIndexs[variableIndex], points->GetNumberOfPoints(),
            BoxNum, RandomPickNum, variableMinMaxData.first[variableIndex],
            variableMinMaxData.second[variableIndex]);
    double thisPointData = CtxPresObjData_Main::GenerateObjData(
            id, attrs, variableIndexs[variableIndex]);
    auto [minRange, maxRange] = hisPicker.CalculateMinMaxValueToPick(
            thisPointData, autoSelectExpdRate);
    for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++) {
        auto& point = points->GetPoint(pointId);
        double pointData = CtxPresObjData_Main::GenerateObjData(
                pointId, attrs, variableIndexs[variableIndex]);
        if (((thisPoint - point).length() <= radius) &&
            (minRange <= pointData && pointData <= maxRange)) {
            re.push_back(pointId);
        }
    }
    re = GetFiltedPointsOfUsingAutoValueRange(id, re, mesh);
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInCtMode(
        const Point& startPoint, const Point& endPoint, VolumeMesh* mesh,
        double radius, int variableIndex, double autoSelectExpdRate,
        bool selectIgnoreUnSeeAbleCells, bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstPoint(startPoint, endPoint, mesh,
                            selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || variableIndex < 0 || autoSelectExpdRate <= 0.0) {
        re.push_back(id);
        return re;
    }
    auto points = mesh->GetPoints();
    auto& thisPoint = points->GetPoint(id);
    /*################################# CORE START #################################*/
    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    std::vector<std::pair<int, int>> variableIndexs =
            CtxPresObjData_Main::GenerateVariableIndex(attrs, IG_POINT);
    if (variableIndex >= variableIndexs.size()) return re;
    std::pair<std::vector<double>, std::vector<double>> variableMinMaxData =
            CtxPresObjData_Main::GenerateMinMaxData(attrs, IG_POINT);

    auto hisPicker = HistogramPicker(
            attrs, variableIndexs[variableIndex], points->GetNumberOfPoints(),
            BoxNum, RandomPickNum, variableMinMaxData.first[variableIndex],
            variableMinMaxData.second[variableIndex]);
    double thisPointData = CtxPresObjData_Main::GenerateObjData(
            id, attrs, variableIndexs[variableIndex]);
    auto [minRange, maxRange] = hisPicker.CalculateMinMaxValueToPick(
            thisPointData, autoSelectExpdRate);
    for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++) {
        auto& point = points->GetPoint(pointId);
        double pointData = CtxPresObjData_Main::GenerateObjData(
                pointId, attrs, variableIndexs[variableIndex]);
        if (((thisPoint - point).length() <= radius) &&
            (minRange <= pointData && pointData <= maxRange)) {
            re.push_back(pointId);
        }
    }
    re = GetFiltedPointsOfUsingAutoValueRange(id, re, mesh);
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInCtMode(
        const Point& startPoint, const Point& endPoint, SurfaceMesh* mesh,
        double radius, int variableIndex, double autoSelectExpdRate,
        bool selectIgnoreUnSeeAbleCells, bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstPoint(startPoint, endPoint, mesh,
                            selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || variableIndex < 0 || autoSelectExpdRate <= 0.0) {
        re.push_back(id);
        return re;
    }
    auto points = mesh->GetPoints();
    auto& thisPoint = points->GetPoint(id);
    /*################################# CORE START #################################*/
    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    std::vector<std::pair<int, int>> variableIndexs =
            CtxPresObjData_Main::GenerateVariableIndex(attrs, IG_POINT);
    if (variableIndex >= variableIndexs.size()) return re;
    std::pair<std::vector<double>, std::vector<double>> variableMinMaxData =
            CtxPresObjData_Main::GenerateMinMaxData(attrs, IG_POINT);

    auto hisPicker = HistogramPicker(
            attrs, variableIndexs[variableIndex], points->GetNumberOfPoints(),
            BoxNum, RandomPickNum, variableMinMaxData.first[variableIndex],
            variableMinMaxData.second[variableIndex]);
    double thisPointData = CtxPresObjData_Main::GenerateObjData(
            id, attrs, variableIndexs[variableIndex]);
    auto [minRange, maxRange] = hisPicker.CalculateMinMaxValueToPick(
            thisPointData, autoSelectExpdRate);
    for (int pointId = 0; pointId < points->GetNumberOfPoints(); pointId++) {
        auto& point = points->GetPoint(pointId);
        double pointData = CtxPresObjData_Main::GenerateObjData(
                pointId, attrs, variableIndexs[variableIndex]);
        if (((thisPoint - point).length() <= radius) &&
            (minRange <= pointData && pointData <= maxRange)) {
            re.push_back(pointId);
        }
    }
    re = GetFiltedPointsOfUsingAutoValueRange(id, re, mesh);
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetCellsInCtMode(
        const Point& startPoint, const Point& endPoint, UnstructuredMesh* mesh,
        double radius, int variableIndex, double autoSelectExpdRate,
        bool selectIgnoreUnSeeAbleCells, bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstCell(startPoint, endPoint, mesh,
                           selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || variableIndex < 0 || autoSelectExpdRate <= 0.0) {
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
    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    std::vector<std::pair<int, int>> variableIndexs =
            CtxPresObjData_Main::GenerateVariableIndex(attrs, IG_CELL);
    if (variableIndex >= variableIndexs.size()) return re;
    std::pair<std::vector<double>, std::vector<double>> variableMinMaxData =
            CtxPresObjData_Main::GenerateMinMaxData(attrs, IG_CELL);

    auto hisPicker = HistogramPicker(
            attrs, variableIndexs[variableIndex], cells->GetNumberOfCells(),
            BoxNum, RandomPickNum, variableMinMaxData.first[variableIndex],
            variableMinMaxData.second[variableIndex]);
    double thisCellData = CtxPresObjData_Main::GenerateObjData(
            id, attrs, variableIndexs[variableIndex]);
    // auto [minRange, maxRange] = hisPicker.CalculateMinMaxValueToPick(
    //         thisCellData, autoSelectExpdRate);
    auto range = hisPicker.CalculateMinMaxValueToPick(thisCellData,
                                                      autoSelectExpdRate);
    double minRange = range.first;
    double maxRange = range.second;

    auto _AutoSelectFunc = [&](int cellIndex) {
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
        }
    };

    if (onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellIndex: seeAbleFaces) { _AutoSelectFunc(cellIndex); }
    } else {
        for (int cellIndex = 0; cellIndex < cells->GetNumberOfCells();
             cellIndex++) {
            _AutoSelectFunc(cellIndex);
        }
    }
    re = GetFiltedCellsOfUsingAutoValueRange(id, re, mesh);
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetCellsInCtMode(
        const Point& startPoint, const Point& endPoint, VolumeMesh* mesh,
        double radius, int variableIndex, double autoSelectExpdRate,
        bool selectIgnoreUnSeeAbleCells, bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstCell(startPoint, endPoint, mesh,
                           selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || variableIndex < 0 || autoSelectExpdRate <= 0.0) {
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
    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    std::vector<std::pair<int, int>> variableIndexs =
            CtxPresObjData_Main::GenerateVariableIndex(attrs, IG_CELL);
    if (variableIndex >= variableIndexs.size()) return re;
    std::pair<std::vector<double>, std::vector<double>> variableMinMaxData =
            CtxPresObjData_Main::GenerateMinMaxData(attrs, IG_CELL);

    auto hisPicker = HistogramPicker(
            attrs, variableIndexs[variableIndex], cells->GetNumberOfCells(),
            BoxNum, RandomPickNum, variableMinMaxData.first[variableIndex],
            variableMinMaxData.second[variableIndex]);
    double thisCellData = CtxPresObjData_Main::GenerateObjData(
            id, attrs, variableIndexs[variableIndex]);
    // auto [minRange, maxRange] = hisPicker.CalculateMinMaxValueToPick(
    //         thisCellData, autoSelectExpdRate);
    auto range = hisPicker.CalculateMinMaxValueToPick(thisCellData,
                                                      autoSelectExpdRate);
    double minRange = range.first;
    double maxRange = range.second;

    auto _AutoSelectFunc = [&](int cellIndex) {
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
        }
    };

    if (onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellIndex: seeAbleFaces) { _AutoSelectFunc(cellIndex); }
    } else {
        for (int cellIndex = 0; cellIndex < cells->GetNumberOfCells();
             cellIndex++) {
            _AutoSelectFunc(cellIndex);
        }
    }
    re = GetFiltedCellsOfUsingAutoValueRange(id, re, mesh);
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetCellsInCtMode(
        const Point& startPoint, const Point& endPoint, SurfaceMesh* mesh,
        double radius, int variableIndex, double autoSelectExpdRate,
        bool selectIgnoreUnSeeAbleCells, bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    auto id = GetFirstCell(startPoint, endPoint, mesh,
                           selectIgnoreUnSeeAbleCells, onlySelectSeeAbleCells);
    if (id == -1) return re;
    if (radius <= 0 || variableIndex < 0 || autoSelectExpdRate <= 0.0) {
        re.push_back(id);
        return re;
    }

    auto cells = mesh->GetFaces();
    auto points = mesh->GetPoints();
    igIndex thisCell[IGAME_CELL_MAX_SIZE]{};
    int thisCellSize = cells->GetCellIds(id, thisCell);
    iGame::Point thisCellCentralPoint =
            GetCentralOfCell(thisCellSize, thisCell, points);
    /*################################# CORE START #################################*/
    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    std::vector<std::pair<int, int>> variableIndexs =
            CtxPresObjData_Main::GenerateVariableIndex(attrs, IG_CELL);
    if (variableIndex >= variableIndexs.size()) return re;
    std::pair<std::vector<double>, std::vector<double>> variableMinMaxData =
            CtxPresObjData_Main::GenerateMinMaxData(attrs, IG_CELL);

    auto hisPicker = HistogramPicker(
            attrs, variableIndexs[variableIndex], cells->GetNumberOfCells(),
            BoxNum, RandomPickNum, variableMinMaxData.first[variableIndex],
            variableMinMaxData.second[variableIndex]);
    double thisCellData = CtxPresObjData_Main::GenerateObjData(
            id, attrs, variableIndexs[variableIndex]);
    // auto [minRange, maxRange] = hisPicker.CalculateMinMaxValueToPick(
    //         thisCellData, autoSelectExpdRate);
    auto range = hisPicker.CalculateMinMaxValueToPick(thisCellData,
                                                      autoSelectExpdRate);
    double minRange = range.first;
    double maxRange = range.second;

    auto _AutoSelectFunc = [&](int cellIndex) {
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
        }
    };

    if (onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellIndex: seeAbleFaces) { _AutoSelectFunc(cellIndex); }
    } else {
        for (int cellIndex = 0; cellIndex < cells->GetNumberOfCells();
             cellIndex++) {
            _AutoSelectFunc(cellIndex);
        }
    }
    re = GetFiltedCellsOfUsingAutoValueRange(id, re, mesh);
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInBox(
        const std::array<std::array<Point, 4>, 6>& allFaces, PointSet* mesh) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    /*################################# CORE START #################################*/
    for (int pointId = 0; pointId < mesh->GetNumberOfPoints(); pointId++) {
        auto& point = mesh->GetPoint(pointId);
        if (IsPointInside(point, allFaces)) { re.push_back(pointId); }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInBox(
        const std::array<std::array<Point, 4>, 6>& allFaces,
        UnstructuredMesh* mesh, bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    /*################################# CORE START #################################*/
    for (int pointId = 0; pointId < mesh->GetNumberOfPoints(); pointId++) {
        auto& point = mesh->GetPoint(pointId);
        if (IsPointInside(point, allFaces)) { re.push_back(pointId); }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInBox(
        const std::array<std::array<Point, 4>, 6>& allFaces, VolumeMesh* mesh,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    /*################################# CORE START #################################*/
    for (int pointId = 0; pointId < mesh->GetNumberOfPoints(); pointId++) {
        auto& point = mesh->GetPoint(pointId);
        if (!IsPointInside(point, allFaces)) continue;
        if (!onlySelectSeeAbleCells || mesh->IsBoundaryPoint(pointId)) {
            re.push_back(pointId);
        }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetPointsInBox(
        const std::array<std::array<Point, 4>, 6>& allFaces, SurfaceMesh* mesh,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    /*################################# CORE START #################################*/
    for (int pointId = 0; pointId < mesh->GetNumberOfPoints(); pointId++) {
        auto& point = mesh->GetPoint(pointId);
        if (IsPointInside(point, allFaces)) { re.push_back(pointId); }
    }
    return re;
    /*################################# CORE END #################################*/
}

static bool IsCellInsie(Cell* cell,
                        const std::array<std::array<Point, 4>, 6>& allFaces) {
    if (cell == nullptr) return false;
    int pointSize = cell->GetNumberOfPoints();
    for (int pointI = 0; pointI < pointSize; pointI++) {
        auto& point = cell->GetPoint(pointI);
        auto result = IsPointInside(point, allFaces);
        if (!result) return false;
    }
    return true;
}

std::vector<int> SingleSelectionStyle::GetCellsInBox(
        const std::array<std::array<Point, 4>, 6>& allFaces,
        UnstructuredMesh* mesh, bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    /*################################# CORE START #################################*/
    auto _NormalSelectFunc = [&](int cellIndex) {
        Cell* cell = mesh->GetCell(cellIndex);
        if (IsCellInsie(cell, allFaces)) { re.push_back(cellIndex); }
    };

    if (onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellIndex: seeAbleFaces) { _NormalSelectFunc(cellIndex); }
    } else {
        for (int cellIndex = 0; cellIndex < mesh->GetNumberOfCells();
             cellIndex++) {
            _NormalSelectFunc(cellIndex);
        }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetCellsInBox(
        const std::array<std::array<Point, 4>, 6>& allFaces, VolumeMesh* mesh,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    /*################################# CORE START #################################*/
    auto _NormalSelectFunc = [&](int cellIndex) {
        Cell* cell = mesh->GetCell(cellIndex);
        if (IsCellInsie(cell, allFaces)) { re.push_back(cellIndex); }
    };

    if (onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellIndex: seeAbleFaces) { _NormalSelectFunc(cellIndex); }
    } else {
        for (int cellIndex = 0; cellIndex < mesh->GetNumberOfVolumes();
             cellIndex++) {
            _NormalSelectFunc(cellIndex);
        }
    }
    return re;
    /*################################# CORE END #################################*/
}

std::vector<int> SingleSelectionStyle::GetCellsInBox(
        const std::array<std::array<Point, 4>, 6>& allFaces, SurfaceMesh* mesh,
        bool onlySelectSeeAbleCells) {
    std::vector<int> re;
    if (mesh == nullptr) return re;
    /*################################# CORE START #################################*/
    auto _NormalSelectFunc = [&](int cellIndex) {
        Cell* cell = mesh->GetFace(cellIndex);
        if (IsCellInsie(cell, allFaces)) { re.push_back(cellIndex); }
    };

    if (onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellIndex: seeAbleFaces) { _NormalSelectFunc(cellIndex); }
    } else {
        for (int cellIndex = 0; cellIndex < mesh->GetNumberOfFaces();
             cellIndex++) {
            _NormalSelectFunc(cellIndex);
        }
    }
    return re;
    /*################################# CORE END #################################*/
}

static double pointToLineDistance(const Point& lineP1, const Point& lineP2,
    const Point& point) {
    Point lineDir = lineP2 - lineP1;
    Point pointDir = point - lineP1;
    Point theCross = pointDir.cross(lineDir);
    double area = theCross.length();
    double lineLength = lineDir.length();
    if (lineLength < 1e-10) { return pointDir.length(); }
    return area / lineLength;
}

static void _GetClosestPointOnNearestCell(const Point& startPoint,
                                          const Point& endPoint, Cell* cell,
                                          double& faceDis, double& pointDis,
                                          igIndex& pointId) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        auto iFaceDis = IsLineCrossCell(startPoint, endPoint, cell);
        if (iFaceDis < 0 || faceDis < iFaceDis) return;
        int pointNum = cell->GetNumberOfPoints();
        for (int pointIndex = 0; pointIndex < pointNum; pointIndex++) {
            auto& p = cell->GetPoint(pointIndex);
            auto iPointDis = pointToLineDistance(startPoint, endPoint, p);
            if (iPointDis < pointDis || iFaceDis < faceDis) {
                faceDis = iFaceDis;
                pointDis = iPointDis;
                pointId = cell->GetPointId(pointIndex);
            }
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            _GetClosestPointOnNearestCell(startPoint, endPoint, face, faceDis,
                                          pointDis, pointId);
        }
    }
}

//return point id
static igIndex GetClosestPointOnNearestCell(const Point& startPoint,
                                            const Point& endPoint, Cell* cell) {
    igIndex reId{-1};
    double faceDis = std::numeric_limits<double>::max();
    double pointDis = std::numeric_limits<double>::max();
    _GetClosestPointOnNearestCell(startPoint, endPoint, cell, faceDis, pointDis,
                                  reId);
    return reId;
}

int SingleSelectionStyle::GetFirstPoint(const Point& startPoint,
                                        const Point& endPoint,
                                        UnstructuredMesh* mesh,
                                        bool selectIgnoreUnSeeAbleCells,
                                        bool onlySelectSeeAbleCells) {
    int cellId =
            GetFirstCell(startPoint, endPoint, mesh, selectIgnoreUnSeeAbleCells,
                         onlySelectSeeAbleCells);
    if (cellId == -1) return -1;
    auto cell = mesh->GetCell(cellId);
    int cellPointNum = cell->GetNumberOfPoints();

    return GetClosestPointOnNearestCell(startPoint, endPoint, cell);
}

int SingleSelectionStyle::GetFirstPoint(const Point& startPoint,
                                        const Point& endPoint, VolumeMesh* mesh,
                                        bool selectIgnoreUnSeeAbleCells,
                                        bool onlySelectSeeAbleCells) {
    int cellId =
            GetFirstCell(startPoint, endPoint, mesh, selectIgnoreUnSeeAbleCells,
                         onlySelectSeeAbleCells);
    if (cellId == -1) return -1;
    auto cell = mesh->GetVolume(cellId);

    return GetClosestPointOnNearestCell(startPoint, endPoint, cell);
}

int SingleSelectionStyle::GetFirstPoint(const Point& startPoint,
                                        const Point& endPoint,
                                        SurfaceMesh* mesh,
                                        bool selectIgnoreUnSeeAbleCells,
                                        bool onlySelectSeeAbleCells) {
    int cellId =
            GetFirstCell(startPoint, endPoint, mesh, selectIgnoreUnSeeAbleCells,
                         onlySelectSeeAbleCells);
    if (cellId == -1) return -1;
    auto cell = mesh->GetFace(cellId);

    return GetClosestPointOnNearestCell(startPoint, endPoint, cell);
}

int SingleSelectionStyle::GetFirstCell(const Point& startPoint,
                                       const Point& endPoint,
                                       UnstructuredMesh* mesh,
                                       bool selectIgnoreUnSeeAbleCells,
                                       bool onlySelectSeeAbleCells) {
    double minDis = -1;
    int id = -1;
    if (mesh == nullptr) return id;
    if (selectIgnoreUnSeeAbleCells || onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellId: seeAbleFaces) {
            Cell* cell = mesh->GetCell(cellId);
            auto dis = IsLineCrossCell(startPoint, endPoint, cell);
            if (dis < 0) continue;
            if (minDis == -1 || dis < minDis) {
                minDis = dis;
                id = cellId;
            }
        }
    } else {
        for (int cellId = 0; cellId < mesh->GetNumberOfCells(); cellId++) {
            Cell* cell = mesh->GetCell(cellId);
            auto dis = IsLineCrossCell(startPoint, endPoint, cell);
            if (dis < 0) continue;
            if (minDis == -1 || dis < minDis) {
                minDis = dis;
                id = cellId;
            }
        }
    }
    return id;
}

int SingleSelectionStyle::GetFirstCell(const Point& startPoint,
                                       const Point& endPoint, VolumeMesh* mesh,
                                       bool selectIgnoreUnSeeAbleCells,
                                       bool onlySelectSeeAbleCells) {
    double minDis = -1;
    int id = -1;
    if (mesh == nullptr) return id;
    if (selectIgnoreUnSeeAbleCells || onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellId: seeAbleFaces) {
            Cell* cell = mesh->GetCell(cellId);
            auto dis = IsLineCrossCell(startPoint, endPoint, cell);
            if (dis < 0) continue;
            if (minDis == -1 || dis < minDis) {
                minDis = dis;
                id = cellId;
            }
        }
    } else {
        for (int cellId = 0; cellId < mesh->GetNumberOfVolumes(); cellId++) {
            Cell* cell = mesh->GetVolume(cellId);
            auto dis = IsLineCrossCell(startPoint, endPoint, cell);
            if (dis < 0) continue;
            if (minDis == -1 || dis < minDis) {
                minDis = dis;
                id = cellId;
            }
        }
    }
    return id;
}

int SingleSelectionStyle::GetFirstCell(const Point& startPoint,
                                       const Point& endPoint, SurfaceMesh* mesh,
                                       bool selectIgnoreUnSeeAbleCells,
                                       bool onlySelectSeeAbleCells) {
    double minDis = -1;
    int id = -1;
    if (mesh == nullptr) return id;
    if (selectIgnoreUnSeeAbleCells || onlySelectSeeAbleCells) {
        auto& seeAbleFaces = mesh->GetSelection()->GetSeeAbleCells(mesh);
        for (auto& cellId: seeAbleFaces) {
            Cell* cell = mesh->GetFace(cellId);
            auto dis = IsLineCrossCell(startPoint, endPoint, cell);
            if (dis < 0) continue;
            if (minDis == -1 || dis < minDis) {
                minDis = dis;
                id = cellId;
            }
        }
    } else {
        for (int cellId = 0; cellId < mesh->GetNumberOfFaces(); cellId++) {
            Cell* cell = mesh->GetFace(cellId);
            auto dis = IsLineCrossCell(startPoint, endPoint, cell);
            if (dis < 0) continue;
            if (minDis == -1 || dis < minDis) {
                minDis = dis;
                id = cellId;
            }
        }
    }
    return id;
}

using PointId = int;
using CellId = int;

static void FindEdgesOfCell(Cell* cell,
                            std::set<std::pair<PointId, PointId>>& edges) {
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
            edges.insert(std::minmax(pointIdA, pointIdB));
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            FindEdgesOfCell(face, edges);
        }
    }
}

static bool IfCellHavePoint(Cell* cell, const std::set<int>& pointIds) {
    int pointNum = cell->GetNumberOfPoints();
    for (int i = 0; i < pointNum; i++) {
        auto pointId = cell->GetPointId(i);
        if (pointIds.count(pointId) != 0) return true;
    }
    return false;
}

static bool IfCellHavePoint(Cell* cell, int _pointId) {
    int pointNum = cell->GetNumberOfPoints();
    for (int i = 0; i < pointNum; i++) {
        auto pointId = cell->GetPointId(i);
        if (pointId == _pointId) return true;
    }
    return false;
}

std::vector<int> SingleSelectionStyle::GetFiltedPointsOfUsingAutoValueRange(
        int keyPointId, const std::vector<int>& pointIds,
        UnstructuredMesh* mesh) {
    std::set<int> pointIds_Set(pointIds.begin(), pointIds.end());
    std::vector<int> cellIds;
    int cellNum = mesh->GetNumberOfCells();
    int keyCellId = -1;
    for (int cellId = 0; cellId < cellNum; cellId++) {
        auto cell = mesh->GetCell(cellId);
        if (!IfCellHavePoint(cell, pointIds_Set)) continue;
        if (keyCellId == -1 && IfCellHavePoint(cell, keyPointId))
            keyCellId = cellId;
        cellIds.push_back(cellId);
    }
    cellIds = GetFiltedCellsOfUsingAutoValueRange(keyCellId, cellIds, mesh);
    std::set<int> pointIdInCells;
    for (auto& cellId: cellIds) {
        auto cell = mesh->GetCell(cellId);
        int pointNum = cell->GetNumberOfPoints();
        for (int i = 0; i < pointNum; i++) {
            auto pointId = cell->GetPointId(i);
            if (pointIds_Set.count(pointId) == 0) continue;
            pointIdInCells.insert(pointId);
        }
    }
    return std::vector<int>(pointIdInCells.begin(), pointIdInCells.end());
}

std::vector<int> SingleSelectionStyle::GetFiltedPointsOfUsingAutoValueRange(
        int keyPointId, const std::vector<int>& pointIds, VolumeMesh* mesh) {
    std::set<int> pointIds_Set(pointIds.begin(), pointIds.end());
    std::vector<int> cellIds;
    int cellNum = mesh->GetNumberOfVolumes();
    int keyCellId = -1;
    for (int cellId = 0; cellId < cellNum; cellId++) {
        auto cell = mesh->GetVolume(cellId);
        if (!IfCellHavePoint(cell, pointIds_Set)) continue;
        if (keyCellId == -1 && IfCellHavePoint(cell, keyPointId))
            keyCellId = cellId;
        cellIds.push_back(cellId);
    }
    cellIds = GetFiltedCellsOfUsingAutoValueRange(keyCellId, cellIds, mesh);
    std::set<int> pointIdInCells;
    for (auto& cellId: cellIds) {
        auto cell = mesh->GetVolume(cellId);
        int pointNum = cell->GetNumberOfPoints();
        for (int i = 0; i < pointNum; i++) {
            auto pointId = cell->GetPointId(i);
            if (pointIds_Set.count(pointId) == 0) continue;
            pointIdInCells.insert(pointId);
        }
    }
    return std::vector<int>(pointIdInCells.begin(), pointIdInCells.end());
}

std::vector<int> SingleSelectionStyle::GetFiltedPointsOfUsingAutoValueRange(
        int keyPointId, const std::vector<int>& pointIds, SurfaceMesh* mesh) {
    std::set<int> pointIds_Set(pointIds.begin(), pointIds.end());
    std::vector<int> cellIds;
    int cellNum = mesh->GetNumberOfFaces();
    int keyCellId = -1;
    for (int cellId = 0; cellId < cellNum; cellId++) {
        auto cell = mesh->GetFace(cellId);
        if (!IfCellHavePoint(cell, pointIds_Set)) continue;
        if (keyCellId == -1 && IfCellHavePoint(cell, keyPointId))
            keyCellId = cellId;
        cellIds.push_back(cellId);
    }
    cellIds = GetFiltedCellsOfUsingAutoValueRange(keyCellId, cellIds, mesh);
    std::set<int> pointIdInCells;
    for (auto& cellId: cellIds) {
        auto cell = mesh->GetFace(cellId);
        int pointNum = cell->GetNumberOfPoints();
        for (int i = 0; i < pointNum; i++) {
            auto pointId = cell->GetPointId(i);
            if (pointIds_Set.count(pointId) == 0) continue;
            pointIdInCells.insert(pointId);
        }
    }
    return std::vector<int>(pointIdInCells.begin(), pointIdInCells.end());
}

std::vector<int> SingleSelectionStyle::GetFiltedCellsOfUsingAutoValueRange(
        int keyCellId, const std::vector<int>& cellIds,
        UnstructuredMesh* mesh) {
    if (mesh == nullptr || cellIds.empty()) return {};
    std::map<std::pair<PointId, PointId>, std::set<CellId>> map_Edge_Cells;
    std::map<CellId, std::set<std::pair<PointId, PointId>>> map_Cell_Edges;
    //gather data
    for (auto& cellId: cellIds) {
        auto cell = mesh->GetCell(cellId);
        FindEdgesOfCell(cell, map_Cell_Edges[cellId]);
        for (auto& edge: map_Cell_Edges[cellId]) {
            map_Edge_Cells[edge].insert(cellId);
        }
    }
    //find results
    std::set<CellId> set_ChoosedCellIds;
    std::queue<CellId> queue_CurrentCellIds;
    set_ChoosedCellIds.insert(keyCellId);
    queue_CurrentCellIds.push(keyCellId);
    while (!queue_CurrentCellIds.empty()) {
        auto currentCellId = queue_CurrentCellIds.front();
        queue_CurrentCellIds.pop();
        auto& edges = map_Cell_Edges[currentCellId];
        for (auto& edge: edges) {
            auto& cellIdsOfEdge = map_Edge_Cells[edge];
            for (auto& cellId: cellIdsOfEdge) {
                if (set_ChoosedCellIds.count(cellId) != 0) continue;
                set_ChoosedCellIds.insert(cellId);
                queue_CurrentCellIds.push(cellId);
            }
        }
    }
    return std::vector<int>(set_ChoosedCellIds.begin(),
                            set_ChoosedCellIds.end());
}

std::vector<int> SingleSelectionStyle::GetFiltedCellsOfUsingAutoValueRange(
        int keyCellId, const std::vector<int>& cellIds, VolumeMesh* mesh) {
    if (mesh == nullptr || cellIds.empty()) return {};
    std::map<std::pair<PointId, PointId>, std::set<CellId>> map_Edge_Cells;
    std::map<CellId, std::set<std::pair<PointId, PointId>>> map_Cell_Edges;
    //gather data
    for (auto& cellId: cellIds) {
        auto cell = mesh->GetCell(cellId);
        FindEdgesOfCell(cell, map_Cell_Edges[cellId]);
        for (auto& edge: map_Cell_Edges[cellId]) {
            map_Edge_Cells[edge].insert(cellId);
        }
    }
    //find results
    std::set<CellId> set_ChoosedCellIds;
    std::queue<CellId> queue_CurrentCellIds;
    set_ChoosedCellIds.insert(keyCellId);
    queue_CurrentCellIds.push(keyCellId);
    while (!queue_CurrentCellIds.empty()) {
        auto currentCellId = queue_CurrentCellIds.front();
        queue_CurrentCellIds.pop();
        auto& edges = map_Cell_Edges[currentCellId];
        for (auto& edge: edges) {
            auto& cellIdsOfEdge = map_Edge_Cells[edge];
            for (auto& cellId: cellIdsOfEdge) {
                if (set_ChoosedCellIds.count(cellId) != 0) continue;
                set_ChoosedCellIds.insert(cellId);
                queue_CurrentCellIds.push(cellId);
            }
        }
    }
    return std::vector<int>(set_ChoosedCellIds.begin(),
                            set_ChoosedCellIds.end());
}

std::vector<int> SingleSelectionStyle::GetFiltedCellsOfUsingAutoValueRange(
        int keyCellId, const std::vector<int>& cellIds, SurfaceMesh* mesh) {
    if (mesh == nullptr || cellIds.empty()) return {};
    std::map<std::pair<PointId, PointId>, std::set<CellId>> map_Edge_Cells;
    std::map<CellId, std::set<std::pair<PointId, PointId>>> map_Cell_Edges;
    //gather data
    for (auto& cellId: cellIds) {
        auto cell = mesh->GetFace(cellId);
        FindEdgesOfCell(cell, map_Cell_Edges[cellId]);
        for (auto& edge: map_Cell_Edges[cellId]) {
            map_Edge_Cells[edge].insert(cellId);
        }
    }
    //find results
    std::set<CellId> set_ChoosedCellIds;
    std::queue<CellId> queue_CurrentCellIds;
    set_ChoosedCellIds.insert(keyCellId);
    queue_CurrentCellIds.push(keyCellId);
    while (!queue_CurrentCellIds.empty()) {
        auto currentCellId = queue_CurrentCellIds.front();
        queue_CurrentCellIds.pop();
        auto& edges = map_Cell_Edges[currentCellId];
        for (auto& edge: edges) {
            auto& cellIdsOfEdge = map_Edge_Cells[edge];
            for (auto& cellId: cellIdsOfEdge) {
                if (set_ChoosedCellIds.count(cellId) != 0) continue;
                set_ChoosedCellIds.insert(cellId);
                queue_CurrentCellIds.push(cellId);
            }
        }
    }
    return std::vector<int>(set_ChoosedCellIds.begin(),
                            set_ChoosedCellIds.end());
}

static void GetPointsOfOneCell(Cell* cell, std::set<int>& reSet) {
    if (cell == nullptr) return;
    int pointNum = cell->GetNumberOfPoints();
    for (int i = 0; i < pointNum; i++) {
        auto pointId = cell->GetPointId(i);
        reSet.insert(pointId);
    }
}

std::vector<int>
SingleSelectionStyle::GetPointsOfCells(const std::vector<int>& cellIds,
                                       UnstructuredMesh* mesh) {
    if (cellIds.empty() || mesh == nullptr) return {};
    std::set<int> reSet;
    for (auto& cellId: cellIds) {
        auto cell = mesh->GetCell(cellId);
        GetPointsOfOneCell(cell, reSet);
    }
    return std::vector<int>(reSet.begin(), reSet.end());
}

std::vector<int>
SingleSelectionStyle::GetPointsOfCells(const std::vector<int>& cellIds,
                                       VolumeMesh* mesh) {
    if (cellIds.empty() || mesh == nullptr) return {};
    std::set<int> reSet;
    for (auto& cellId: cellIds) {
        auto cell = mesh->GetVolume(cellId);
        GetPointsOfOneCell(cell, reSet);
    }
    return std::vector<int>(reSet.begin(), reSet.end());
}

std::vector<int>
SingleSelectionStyle::GetPointsOfCells(const std::vector<int>& cellIds,
                                       SurfaceMesh* mesh) {
    if (cellIds.empty() || mesh == nullptr) return {};
    std::set<int> reSet;
    for (auto& cellId: cellIds) {
        auto cell = mesh->GetFace(cellId);
        GetPointsOfOneCell(cell, reSet);
    }
    return std::vector<int>(reSet.begin(), reSet.end());
}

IGAME_NAMESPACE_END
