#include "iGameGetClosestCellsInLine.h"
#include <iGameCell.h>
#include <climits>
#include <numeric>
IGAME_NAMESPACE_BEGIN
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
            SumCellPoints(face, point);
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

static double SegmentIntersectsTriangle(const Point& start, const Point& end, const Point& a, const Point& b,
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

static double IsLineCrossCell(const Point& startPoint, const Point& endPoint, Cell* cell) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 2) return false;
        auto& p0 = cell->GetPoint(0);
        for (int i = 2; i < pointSize; i++) {
            auto& p1 = cell->GetPoint(i - 1);
            auto& p2 = cell->GetPoint(i);
            auto dis = SegmentIntersectsTriangle(startPoint, endPoint, p0, p1, p2);
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

static double IsLineCrossCell(const Point& startPoint, const Point& endPoint, int cellId,
                            UnstructuredMesh::Pointer mesh) {
    auto cell = mesh->GetCell(cellId);
    return IsLineCrossCell(startPoint, endPoint, cell);
}

static bool IsCellHavePointInRadius(Cell* cell, const Point& p, double radius) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        for (int i = 0; i < pointSize; i++) {
            auto& point = cell->GetPoint(i);
            if ((point - p).length() <= radius) return true;
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            if (IsCellHavePointInRadius(face, p, radius)) return true;
        }
    }
    return false;
}

bool iGameGetClosestCellsInLine::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_Radius < 0) return false;
    RUN();
    return true;
}

const std::vector<int>& iGameGetClosestCellsInLine::GetResult() { return m_Ids; }

void iGameGetClosestCellsInLine::RUN() {
    m_Ids.clear();
    double minDis = -1;
    int id = -1;
    for (int cellId = 0; cellId < m_Mesh->GetNumberOfCells(); cellId++) {
        Cell* cell = m_Mesh->GetCell(cellId);
        auto dis = IsLineCrossCell(m_StartPoint, m_EndPoint, cell);
        if (dis < 0) continue;
        if (minDis == -1 || dis < minDis) {
            minDis = dis;
            id = cellId;
        }
    }
    if (id == -1) return;
    if (m_Radius == 0) {
        m_Ids.push_back(id);
        return;
    }
    auto pCenter = GetCentralOfCell(m_Mesh->GetCell(id));
    for (int cellId = 0; cellId < m_Mesh->GetNumberOfCells(); cellId++) {
        Cell* cell = m_Mesh->GetCell(cellId);
        if (IsCellHavePointInRadius(cell, pCenter, m_Radius)) { m_Ids.push_back(cellId); }
    }
}

iGameGetClosestCellsInLine::iGameGetClosestCellsInLine(const Point& startPoint, const endPoint, double radius) {
    m_StartPoint = startPoint;
    m_EndPoint = endPoint;
    m_Radius = radius;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(0);
}
IGAME_NAMESPACE_END