#include "iGameDataChangeData.h"
#include <algorithm>
#include <cmath>
#include <climits>
#include <cfloat>
using namespace std;
IGAME_NAMESPACE_BEGIN

const float EPSILON = 1e-6f;

static void hsbToRgb(float h, float s, float bVal, float& r, float& g, float& b) {
    float c = bVal * s;
    float x = c * (1.0f - std::fabs(std::fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = bVal - c;

    float r1, g1, b1;

    if (h >= 0.0f - EPSILON && h < 60.0f + EPSILON) {
        r1 = c + m;
        g1 = x + m;
        b1 = 0.0f + m;
    } else if (h >= 60.0f - EPSILON && h < 120.0f + EPSILON) {
        r1 = x + m;
        g1 = c + m;
        b1 = 0.0f + m;
    } else if (h >= 120.0f - EPSILON && h < 180.0f + EPSILON) {
        r1 = 0.0f + m;
        g1 = c + m;
        b1 = x + m;
    } else if (h >= 180.0f - EPSILON && h < 240.0f + EPSILON) {
        r1 = 0.0f + m;
        g1 = x + m;
        b1 = c + m;
    } else if (h >= 240.0f - EPSILON && h < 300.0f + EPSILON) {
        r1 = x + m;
        g1 = 0.0f + m;
        b1 = c + m;
    } else {
        r1 = c + m;
        g1 = 0.0f + m;
        b1 = x + m;
    }

    r = r1;
    g = g1;
    b = b1;
}

static iGame::Point GetCentralOfCell(int cellPointSize, int cellPoints[], Points::Pointer points) {
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


static bool SegmentIntersectsTriangle(const Point& start, const Point& end, const Point& a, const Point& b, const Point& c) {
    Point dir = {end[0] - start[0], end[1] - start[1], end[2] - start[2]};
    Point ab = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    Point ac = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    Point pvec = dir.cross(ac);

    double det = ab.dot(pvec);
    if (std::abs(det) < 1e-7) {
        return false;
    }

    double invDet = 1.0 / det;
    Point tvec = {start[0] - a[0], start[1] - a[1], start[2] - a[2]};
    double u = tvec.dot(pvec) * invDet;
    if (u < -1e-7 || u > 1 + 1e-7) { return false; }

    Point qvec = tvec.cross(ab);
    double v = dir.dot(qvec) * invDet;
    if (v < -1e-7 || u + v > 1 + 1e-7) { return false; }

    double t = ac.dot(qvec) * invDet;
    if (t < 1e-7 || t > 1 - 1e-7) {
        return false;
    }
    return (u > 1e-7) && (v > 1e-7) && (u + v < 1 - 1e-7);
}

static bool IsLineCrossCell(const Point& startPoint, const Point& endPoint, Cell* cell) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 2) return false;
        auto& p0 = cell->GetPoint(0);
        for (int i = 2; i < pointSize; i++) {
            auto& p1 = cell->GetPoint(i - 1);
            auto& p2 = cell->GetPoint(i);
            if (SegmentIntersectsTriangle(startPoint, endPoint, p0, p1, p2)) return true;
        }
        return false;
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            if (IsLineCrossCell(startPoint, endPoint, face)) return true;
        }
        return false;
    }
}

static bool IsLineCrossCell(const Point& startPoint, const Point& endPoint, int cellId,
                            UnstructuredMesh::Pointer mesh) {
    auto cell = mesh->GetCell(cellId);
    return IsLineCrossCell(startPoint, endPoint, cell);
}

static std::map<int, int> _GenerateCellIndexInLine(const Point& startPoint, const Point& endPoint,
                                                   Points::Pointer points, CellArray::Pointer cells,
                                                   UnstructuredMesh::Pointer mesh) {
    std::map<int, int> re;
    for (int cellId = 0; cellId < cells->GetNumberOfCells(); cellId++) {
        Cell* cell = mesh->GetCell(cellId);
        if (IsLineCrossCell(startPoint, endPoint, cell)) re[cellId] = re.size();
    }
    return re;
}

static std::map<int, int> _GeneratePointIndexInLine(const Point& startPoint, const Point& endPoint,
                                                    Points::Pointer points, CellArray::Pointer cells,
                                                    UnstructuredMesh::Pointer mesh) {
    auto cellIds = _GenerateCellIndexInLine(startPoint, endPoint, points, cells, mesh);
    std::map<int, int> re;
    for (auto& cellId_: cellIds) {
        igIndex cell[IGAME_CELL_MAX_SIZE]{};
        auto& cellId = cellId_.first;
        int size = cells->GetCellIds(cellId, cell);
        for (int i = 0; i < size; i++) {
            if (re.count(cell[i]) != 0) continue;
            re[cell[i]] = re.size();
        }
    }
    return re;
}

void DataChangeData::SetObjDistance(const std::vector<double>& objDistance) { m_ObjDistance = objDistance; }
const std::vector<double>& DataChangeData::GetObjDistance() { return m_ObjDistance; }
void DataChangeData::SetChoosedObjIds(const std::set<int>& choosedObjIds) { m_ChoosedObjIds = choosedObjIds; }
void DataChangeData::AddChoosedObjId(int objId) { m_ChoosedObjIds.insert(objId); }
void DataChangeData::RemoveChoosedObjId(int objId) { m_ChoosedObjIds.erase(objId); }
void DataChangeData::ClearChoosedObjIds() { m_ChoosedObjIds.clear(); }
const std::set<int>& DataChangeData::GetChoosedObjIds() { return m_ChoosedObjIds; }
void DataChangeData::SetObjDrawSort(const std::vector<int>& objDrawSort) { m_ObjDrawSort = objDrawSort; }
const std::vector<int>& DataChangeData::GetObjDrawSort() { return m_ObjDrawSort; }
//void DataChangeData::SetVariableHue(const std::vector<int>& variableHue) { m_VariableHue = variableHue; }
//const std::vector<int>& DataChangeData::GetVariableHue() { return m_VariableHue; }
void DataChangeData::SetVariableHS(const std::vector<std::pair<int, int>>& variableHS) { m_VariableHS = variableHS; }
const std::vector<std::pair<int, int>>& DataChangeData::GetVariableHS() { return m_VariableHS; }
void DataChangeData::SetVariableColor(const std::vector<std::tuple<int, int, int>>& variableColor) {
    m_VariableColor = variableColor;
}
const std::vector<std::tuple<int, int, int>>& DataChangeData::GetVariableColor() { return m_VariableColor; }
void DataChangeData::SetChoosedVariableColor(const std::vector<std::tuple<int, int, int>>& variableColor) {
    m_ChoosedVariableColor = variableColor;
}
const std::vector<std::tuple<int, int, int>>& DataChangeData::GetChoosedVariableColor() {
    return m_ChoosedVariableColor;
}
void DataChangeData::SetMaxDistance(double maxDistance) { m_MaxDistance = maxDistance; }
double DataChangeData::GetMaxDistance() const { return m_MaxDistance; }
void DataChangeData::SetMinDistance(double minDistance) { m_MinDistance = minDistance; }
double DataChangeData::GetMinDistance() const { return m_MinDistance; }
void DataChangeData::SetMaxValue(double value) { m_MaxValue = value; }
double DataChangeData::GetMaxValue() const { return m_MaxValue; }
void DataChangeData::SetMinValue(double value) { m_MinValue = value; }
double DataChangeData::GetMinValue() const { return m_MinValue; }
void DataChangeData::SetObjIndexs(const std::map<int, int>& objIndexs) { m_ObjIndexs = objIndexs; }
const std::map<int, int>& DataChangeData::GetObjIndexs() { return m_ObjIndexs; }

std::map<int, int> DataChangeData::GenerateObjIndex(const Point& startPoint, const Point& endPoint,
                                                    Points::Pointer points,
                                               CellArray::Pointer cells, UnstructuredMesh::Pointer mesh,
                                               IGenum dataType) {
    if (dataType == IG_POINT) return _GeneratePointIndexInLine(startPoint, endPoint, points, cells, mesh);
    return _GenerateCellIndexInLine(startPoint, endPoint, points, cells, mesh);
}

std::vector<double> DataChangeData::GenerateObjDistance(const Point& startPoint, const std::map<int, int>& objIndexs,
                                                        Points::Pointer points) {
    std::vector<double> re(objIndexs.size());
    for (auto& objId_: objIndexs) {
        int objId = objId_.first;
        int reIndex = objId_.second;
        re[reIndex] = GenerateObjDistance(startPoint, objId, points);
    }
    return re;
}

double DataChangeData::GenerateObjDistance(const Point& startPoint, int objId, Points::Pointer points) {
    auto& point = points->GetPoint(objId);
    return (startPoint - point).length();
}

std::vector<double> DataChangeData::GenerateObjDistance(const Point& startPoint, const std::map<int, int>& objIndexs,
                                                        CellArray::Pointer cells, Points::Pointer points) {
    std::vector<double> re(objIndexs.size());
    for (auto& objId_: objIndexs) {
        int objId = objId_.first;
        int reIndex = objId_.second;
        re[reIndex] = GenerateObjDistance(startPoint, objId, cells, points);
    }
    return re;
}

double DataChangeData::GenerateObjDistance(const Point& startPoint, int objId, CellArray::Pointer cells,
                                           Points::Pointer points) {
    igIndex thisCell[IGAME_CELL_MAX_SIZE]{};
    int thisCellSize = cells->GetCellIds(objId, thisCell);
    iGame::Point thisCellCentralPoint = GetCentralOfCell(thisCellSize, thisCell, points);
    return (startPoint - thisCellCentralPoint).length();
}

std::vector<int> DataChangeData::GenerateObjDrawSort(const std::vector<double>& objDistance,
                                                     const std::map<int, int>& objIndexs) {
    std::vector<int> re;
    for (auto& objId_: objIndexs) {
        auto& objId = objId_.first;
        re.push_back(objId);
    }
    std::sort(re.begin(), re.end(), [&](int objIdA, int objIdB) {
        auto objIndexA = objIndexs.at(objIdA);
        auto objIndexB = objIndexs.at(objIdB);
        return objDistance[objIndexA] < objDistance[objIndexB];
    });
    return re;
}

double DataChangeData::GenerateObjMaxDistance(const std::vector<int>& objDrawSort,
                                              const std::vector<double>& objDistance,
                                              const std::map<int, int>& objIndexs) {
    if (objDrawSort.empty()) return {};
    auto objId = objDrawSort.back();
    auto objIndex = objIndexs.at(objId);
    return objDistance[objIndex];
}

double DataChangeData::GenerateObjMinDistance(const std::vector<int>& objDrawSort,
                                              const std::vector<double>& objDistance,
                                              const std::map<int, int>& objIndexs) {
    if (objDrawSort.empty()) return {};
    auto objId = objDrawSort.front();
    auto objIndex = objIndexs.at(objId);
    return objDistance[objIndex];
}

std::pair<double, double> DataChangeData::GenerateObjMinMaxValue(const std::vector<std::vector<double>>& objDatas,
                                                                 const std::vector<bool>& variableShow) {
    double minValue = std::numeric_limits<double>::max();
    double maxValue = -std::numeric_limits<double>::max();
    for (auto& objData: objDatas) {
        for (int variableIndex = 0; variableIndex < variableShow.size(); variableIndex++) {
            if (!variableShow[variableIndex]) continue;
            minValue = min(minValue, objData[variableIndex]);
            maxValue = max(maxValue, objData[variableIndex]);
        }
    }
    return {minValue, maxValue};
}

std::vector<int> DataChangeData::GenerateVariableHue(int variableNum) {
    std::vector<int> result;
    if (variableNum <= 0) { return result; }
    result.reserve(variableNum);
    for (int i = 0; i < variableNum; ++i) {
        double hue = (360.0 * i) / static_cast<double>(variableNum);
        int hue_int = static_cast<int>(std::round(hue)) % 360;
        result.push_back(hue_int);
    }
    return result;
}

std::vector<std::pair<int, int>> DataChangeData::GenerateHS(int variableNum, int minH, int maxH, int minS, int maxS) {
    if (variableNum <= 0) { return {}; }

    double H_range;
    if (maxH >= minH) {
        H_range = maxH - minH;
    } else {
        H_range = (360.0 - minH) + maxH;
    }
    double S_range = maxS - minS;

    if (H_range <= 0 && S_range <= 0) {
        if (variableNum == 1) {
            return {{minH, minS}};
        } else {
            return {};
        }
    }

    int best_r = 1;
    int best_c = variableNum;
    double best_d_min = 0.0;

    for (int r = 1; r <= variableNum; r++) {
        int c = (variableNum + r - 1) / r;
        if (c == 0) c = 1;

        double dx = (c > 0) ? H_range / c : 0;
        double dy = (r > 0) ? S_range / r : 0;
        double d_min = (dx > 0 && dy > 0) ? std::min(dx, dy) : std::max(dx, dy);

        if (d_min > best_d_min || (d_min == best_d_min && (r * c < best_r * best_c))) {
            best_d_min = d_min;
            best_r = r;
            best_c = c;
        }
    }

    std::vector<std::pair<double, double>> grid_points;
    for (int j = 0; j < best_r; j++) {
        double s_val = minS + (j + 0.5) * (S_range / best_r);
        for (int i = 0; i < best_c; i++) {
            double h_val = minH + (i + 0.5) * (H_range / best_c);

            h_val = std::fmod(h_val, 360.0);
            if (h_val < 0) { h_val += 360.0; }

            grid_points.emplace_back(h_val, s_val);
        }
    }

    std::vector<std::pair<int, int>> result;
    int total_points = best_r * best_c;

    if (variableNum == 1) {
        auto& p = grid_points[0];
        result.emplace_back(static_cast<int>(std::round(p.first)), static_cast<int>(std::round(p.second)));
    } else {
        for (int i = 0; i < variableNum; i++) {
            double index = static_cast<double>(i) * (total_points - 1) / (variableNum - 1);
            int idx = static_cast<int>(std::round(index));
            idx = std::min(idx, total_points - 1);

            auto& p = grid_points[idx];
            int hue = static_cast<int>(std::round(p.first));
            int sat = static_cast<int>(std::round(p.second));

            hue = (hue % 360 + 360) % 360;
            result.emplace_back(hue, sat);
        }
    }

    return result;
}

std::vector<std::tuple<int, int, int>> DataChangeData::GenerateVariableColor(const std::vector<int>& variableHue,
                                                                             int saturation, int light) {
    double s = (double) saturation / 255.0;
    double L = (double) light / 255.0;
    float r{}, g{}, b{};
    std::vector<std::tuple<int, int, int>> re(variableHue.size());
    for (int i = 0; i < variableHue.size(); i++) {
        hsbToRgb(variableHue[i], s, L, r, g, b);
        int ir = r * 255;
        int ig = g * 255;
        int ib = b * 255;
        re[i] = {ir, ig, ib};
    }
    return re;
}

std::vector<std::tuple<int, int, int>>
DataChangeData::GenerateVariableColor(const std::vector<std::pair<int, int>>& variableHS, int light) {
    double L = (double) light / 255.0;
    float r{}, g{}, b{};
    std::vector<std::tuple<int, int, int>> re(variableHS.size());
    for (int i = 0; i < variableHS.size(); i++) {
        float h = variableHS[i].first;
        float s = variableHS[i].second / static_cast<float>(255);
        hsbToRgb(h, s, L, r, g, b);
        int ir = r * 255;
        int ig = g * 255;
        int ib = b * 255;
        re[i] = {ir, ig, ib};
    }
    return re;
}

std::vector<std::vector<double>>
DataChangeData::GenerateObjectDatas(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                                    const std::map<int, int>& objIndexs) {
    std::vector<std::vector<double>> re(objIndexs.size());
    for (auto& objId_: objIndexs) {
        int objId=objId_.first;
        int reIndex = objId_.second;
        re[reIndex] = GenerateObjectData(attrs, dataType, objId);
    }
    return re;
}

std::set<int> DataChangeData::GenerateChoosedObjIds(
        const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems, IGenum dataType,
        const std::map<int, int>& objIndexs) {
    std::set<int> re;
    switch (dataType) {
        case IG_POINT: {
            if (selectedItems.count(Selection::Event::Type::PickPoint) == 0) break;
            auto& selectedPoints = selectedItems.at(Selection::Event::Type::PickPoint);
            for (auto& point: selectedPoints) {
                auto pointId = point.first;
                if (objIndexs.count(pointId) == 0) continue;
                re.insert(pointId);
            }
            break;
        }
        case IG_CELL: {
            if (selectedItems.count(Selection::Event::Type::PickFace) == 0) break;
            auto& selectedCells = selectedItems.at(Selection::Event::Type::PickFace);
            for (auto& cell: selectedCells) {
                auto cellId = cell.first;
                if (objIndexs.count(cellId) == 0) continue;
                re.insert(cellId);
            }
            break;
        }
        default:
            break;
    }
    return re;
}

double DataChangeData::GenerateMinValueInChoosedVariable(const std::vector<double>& minValues,
                                                         const std::vector<bool>& variableShow) {
    double re = std::numeric_limits<double>::max();
    for (int i = 0; i < variableShow.size(); i++) {
        if (!variableShow[i]) continue;
        re = min(re, minValues[i]);
    }
    return re;
}

double DataChangeData::GenerateMaxValueInChoosedVariable(const std::vector<double>& maxValues,
                                                         const std::vector<bool>& variableShow) {
    double re = -std::numeric_limits<double>::max();
    for (int i = 0; i < variableShow.size(); i++) {
        if (!variableShow[i]) continue;
        re = max(re, maxValues[i]);
    }
    return re;
}
IGAME_NAMESPACE_END