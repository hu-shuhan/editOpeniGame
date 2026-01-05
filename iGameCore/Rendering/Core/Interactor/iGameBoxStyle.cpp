#include "iGameBoxStyle.h"
#include "iGameInteractor.h"
#include "iGameLine.h"
#include "iGameScene.h"
#include "iGameSelectionParameter.h"
IGAME_NAMESPACE_BEGIN
static double SegmentIntersectsTriangle(const Point& start, const Point& dir,
                                        const Point& a, const Point& b,
                                        const Point& c,
                                        Point& intersectionPoint) {
    // 计算方向向量（从start指向end）
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

    if (u >= -1e-7 && v >= -1e-7 && u + v <= 1 + 1e-7) {
        // 计算交点坐标
        intersectionPoint = start + normalizedDir * t;
        return t; // 返回实际的距离值
    }
    return -1;
}

void BoxStyle::MousePressEvent(IEvent event) {
    BasicStyle::MousePressEvent(event);
    if (!SelectionParameter::Instance().GetHaveBox()) return;
    m_SelectedDirection = -1;
    m_SelectedItem = -1;

    if (m_DynamicBox == nullptr) return;
    if (event.button != MouseButton::MiddleButton) return;

    m_MVP = m_Interactor->GetMVP();
    m_InvertedMVP = m_MVP.invert();

    auto& pos = event.pos;
    m_OldPoint2D = event.pos;

    igm::vec3 nearPoint = GetNearWorldCoord(pos, m_InvertedMVP);
    igm::vec3 farPoint = GetFarWorldCoord(pos, m_InvertedMVP);
    igm::vec3 rayDir = (farPoint - nearPoint).normalized();

    Point lineStartPoint = Point(nearPoint.x, nearPoint.y, nearPoint.z);
    Point lineDir = Point(rayDir.x, rayDir.y, rayDir.z);

    m_MaxDis = m_DynamicBox->GetLength().length() * 0.02;

    //Select Box Ope Point
    auto& opePoints = m_DynamicBox->GetOpePoints();
    if (m_SelectedDirection == -1) {
        float minPointDist = std::numeric_limits<float>::max();
        for (int i = 0; i < 6; i++) {
            float dist = Line::ComputePointToLineDis(lineStartPoint, lineDir,
                                                     opePoints[i]);
            if (dist > m_MaxDis) continue;
            if (m_SelectedDirection == -1 || dist < minPointDist) {
                m_SelectedDirection = i;
                minPointDist = dist;
                m_SelectedItem = IG_POINT;
            }
        }
    }

    //Select Mid Point
    auto& midPoint = m_DynamicBox->GetMidPoint();
    if (m_SelectedDirection == -1) {
        float dist =
                Line::ComputePointToLineDis(lineStartPoint, lineDir, midPoint);
        if (dist <= m_MaxDis) {
            m_SelectedDirection = -2;
            m_SelectedItem = IG_MID_POINT;
        }
    }

    //Select Box Face
    Point intersectionPoint;
    if (m_SelectedDirection == -1) {
        auto boxFaces = m_DynamicBox->GetAllFaces();
        float minFaceDist = std::numeric_limits<float>::max();
        for (int i = 0; i < 6; i++) {
            auto& boxFace = boxFaces[i];
            auto& p0 = boxFace[0];
            for (int pIndex = 1; pIndex < 3; pIndex++) {
                int pI1 = pIndex;
                int pI2 = (pIndex + 1) % 4;
                auto& p1 = boxFace[pI1];
                auto& p2 = boxFace[pI2];
                Point tempP;
                auto dist = SegmentIntersectsTriangle(lineStartPoint, lineDir,
                                                      p0, p1, p2, tempP);
                if (dist == -1) continue;
                if (m_SelectedDirection == -1 || dist < minFaceDist) {
                    m_SelectedDirection = i;
                    minFaceDist = dist;
                    m_SelectedItem = IG_CELL;
                    intersectionPoint = tempP;
                    break;
                }
            }
        }
    }

    if (m_SelectedDirection == -1) {
        SetNeedReSet();
        return;
    }

    igm::vec4 p;
    if (m_SelectedItem == IG_MID_POINT) {
        p = igm::vec4{midPoint[0], midPoint[1], midPoint[2], 1.f};
    } else if (m_SelectedItem == IG_POINT) {
        p = igm::vec4{opePoints[m_SelectedDirection][0],
                      opePoints[m_SelectedDirection][1],
                      opePoints[m_SelectedDirection][2], 1.f};
    } else if (m_SelectedItem == IG_CELL) {
        p = igm::vec4{intersectionPoint[0], intersectionPoint[1],
                      intersectionPoint[2], 1.f};
    }
    //m_PrePosition = opePoints[m_SelectedDirection];
    m_PrePosition = Point(p.x, p.y, p.z);
    p = m_MVP * p;
    m_SelectedNDCZ = p.z / p.w;
}

void BoxStyle::MouseMoveEvent(IEvent event) {
    if (!SelectionParameter::Instance().GetHaveBox()) return;
    if (m_SelectedDirection == -1) return;
    if (m_DynamicBox == nullptr) return;
    if (m_SelectedItem != IG_POINT && m_SelectedItem != IG_CELL &&
        m_SelectedItem != IG_MID_POINT)
        return;
    if (m_MouseMode != MouseButton::MiddleButton) return;

    igm::vec2 pos = event.pos;
    m_NewPoint2D = event.pos;

    igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                  1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

    igm::vec4 Point_NDC{NDC, m_SelectedNDCZ, 1.f};
    igm::vec4 newPoint_WorldCoord = m_InvertedMVP * Point_NDC;
    newPoint_WorldCoord /= newPoint_WorldCoord.w;

    Point nowPosition = Point(newPoint_WorldCoord.x, newPoint_WorldCoord.y,
                              newPoint_WorldCoord.z);
    auto dir = nowPosition - m_PrePosition;

    if (m_SelectedItem == IG_MID_POINT) {
        m_DynamicBox->MovePosition(nowPosition);
    } else if (m_SelectedItem == IG_POINT) {
        m_DynamicBox->MoveOpePoint((DynamicBox::OpeInt) m_SelectedDirection,
                                   dir);
    } else if (m_SelectedItem == IG_CELL) {
        m_DynamicBox->RotateBox(m_PrePosition, nowPosition);
        //################# TEST #################
        //m_DynamicBox->OldP = m_PrePosition;
        //m_DynamicBox->NewP = nowPosition;
        //m_DynamicBox->RotateBox(m_OldPoint2D, m_NewPoint2D,
        //                        igm::vec3{m_Scene->GetRotationBoundingSphere()},
        //                        m_Scene->GetModelMatrix(),
        //                        m_Scene->GetCamera());
    }
    m_PrePosition = nowPosition;
    PointMoveCallBack();
    SetChooedStation(false);
    ClearDraw();
    ToDraw();
    m_OldPoint2D = m_NewPoint2D;
}

void BoxStyle::InitBox(const Point& p1, const Point& p2) {
    m_DynamicBox = DynamicBox::New(p1, p2);
    ToDraw();
}

void BoxStyle::DeleteBox() { m_DynamicBox = nullptr; }

void BoxStyle::SetChooedStation(bool choosedStation) {
    m_ChoosedStation = choosedStation;
}

void BoxStyle::ToDraw() {
    if (m_DynamicBox == nullptr) return;
    auto painter = m_Scene->GetPainter3D();

    painter->SetPen(3);
    //if (m_ChoosedStation) {
        painter->SetPen(Color::White);
    //} else {
    //    painter->SetPen(Color::Red);
    //}
    for (int i = 0; i < 6; i++) {
        int opeLineHandle = painter->DrawLine(m_DynamicBox->GetMidPoint(),
                                              m_DynamicBox->GetOpePoints()[i]);
        m_DrawHandles.push_back(opeLineHandle);
    }
    auto edgeLines = m_DynamicBox->GetAllEdges();
    for (auto& edgeLine: edgeLines) {
        int edgeHandle = painter->DrawLine(edgeLine.first, edgeLine.second);
        m_DrawHandles.push_back(edgeHandle);
    }

    painter->SetPen(16);
    painter->SetPen(Color::Blue);
    int midHandle = painter->DrawPoint(m_DynamicBox->GetMidPoint());
    m_DrawHandles.push_back(midHandle);

    painter->SetPen(7);
    painter->SetPen(Color::Red);
    for (int i = 0; i < 6; i++) {
        int opeHandle = painter->DrawPoint(m_DynamicBox->GetOpePoints()[i]);
        m_DrawHandles.push_back(opeHandle);
    }

    //################# TEST #################
    //{
    //    painter->SetPen(12);
    //    painter->SetPen(Color::Yellow);
    //    int opeHandle = painter->DrawPoint(m_DynamicBox->OldP);
    //    m_DrawHandles.push_back(opeHandle);
    //}
    //{
    //    painter->SetPen(12);
    //    painter->SetPen(Color::Green);
    //    int opeHandle = painter->DrawPoint(m_DynamicBox->NewP);
    //    m_DrawHandles.push_back(opeHandle);
    //}
}

void BoxStyle::ClearDraw() {
    auto painter = m_Scene->GetPainter3D();
    if (!painter) return;
    for (auto& drawHandle: m_DrawHandles) { painter->Delete(drawHandle); }
    m_DrawHandles.clear();
}

DynamicBox::Pointer BoxStyle::GetBox() { return m_DynamicBox; }

void BoxStyle::_SetPointMoveCallBack(const std::string& name,
                                     std::function<void()> callBack) {
    m_PointMoveCallBacks[name] = callBack;
}

void BoxStyle::RemovePointMoveCallBack(const std::string& name) {
    m_PointMoveCallBacks.erase(name);
}

void BoxStyle::SetUpdateWidgetFunc(std::function<void()> func) {
    m_UpdateWidgetFunc = std::make_shared<std::function<void()>>(func);
}

void BoxStyle::RemoveUpdateWidgetFunc() { m_UpdateWidgetFunc = nullptr; }

void BoxStyle::PointMoveCallBack() {
    for (auto& pmcb: m_PointMoveCallBacks) { pmcb.second(); }
}

void BoxStyle::SetNeedReSet() {
    SelectionParameter::Instance().SetHaveBox(false);
    ClearDraw();
    if (m_UpdateWidgetFunc) (*m_UpdateWidgetFunc)();
}

BoxStyle::~BoxStyle() { ClearDraw(); }

IGAME_NAMESPACE_END