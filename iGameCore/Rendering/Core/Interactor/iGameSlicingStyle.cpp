#include "iGameSlicingStyle.h"
#include "iGameInteractor.h"

IGAME_NAMESPACE_BEGIN
void SlicingStyle::Initialize(Interactor* a) {
    BasicStyle::Initialize(a);
    m_Model = a->GetModel();
    m_DataObject = m_Model->GetDataObject();
    m_Painter = m_Model->GetPainter();

    auto& bbox = m_DataObject->GetBoundingBox();
    Vector3d p1 = bbox.min;
    Vector3d p7 = bbox.max;

    if (boxHandle != 0) { m_Painter->Delete(boxHandle); }
    m_Painter->SetPen(1);
    m_Painter->SetPen(Color::White);
    m_Painter->SetBrush(Color::None);
    boxHandle = m_Painter->DrawCube(p1, p7);

    center = bbox.center();
    head = rear = headBound = rearBound = center;
    head[0] += bbox.diag() / 2;
    rear[0] -= bbox.diag() / 2;
    len = bbox.diag();
    headBound[0] += (bbox.max - bbox.min)[0] / 2;
    rearBound[0] -= (bbox.max - bbox.min)[0] / 2;
    normal = (head - rear).normalized();
    top = center;
    top[2] += (bbox.max - bbox.min)[2] / 2;
    left = center;
    left[1] += (bbox.max - bbox.min)[1] / 2;

    m_Painter->SetPen(4);
    m_Painter->SetPen(Color::Red);
    lineHandle = m_Painter->DrawLine(head, rear);

    m_Painter->SetPen(16);
    centerHandle = m_Painter->DrawPoint(center);
    m_Painter->SetPen(Color::Green);
    headHandle = m_Painter->DrawPoint(head);
    m_Painter->SetPen(Color::Blue);
    rearHandle = m_Painter->DrawPoint(rear);

    ComputeSlicingPlane(plane);
    DrawSlicingPlane(plane);

}
void SlicingStyle::MousePressEvent(IEvent _event) {
    BasicStyle::MousePressEvent(_event);

    mvp = m_Interactor->GetMVP();
    invMVP = mvp.invert();
    Vector3Tovec3 v;

    auto& pos = _event.pos;
    igm::vec3 point1 = GetNearWorldCoord(pos, invMVP);
    igm::vec3 point2 = GetFarWorldCoord(pos, invMVP);

    igm::vec3 dir = (point1 - point2).normalized();

    double d1 = DistancePointToLine(v(center), point1, point2);
    double d2 = DistancePointToLine(v(head), point1, point2);
    double d3 = DistancePointToLine(v(rear), point1, point2);

    if (d1 < 0.1) {
        selectId = 0;
    } else if (d2 < 0.1) {
        selectId = 1;
    } else if (d3 < 0.1) {
        selectId = 2;
    } else {
        selectId = -1;
    }



    //std::cout << p1 << " " << p2 << " " << p3 << " " << p4 << std::endl;

    //std::cout << pos << std::endl;
}
void SlicingStyle::MouseMoveEvent(IEvent _event) {
    igm::vec2 pos = _event.pos;
    m_NewPoint2D = _event.pos;

    switch (m_MouseMode) {
    case LeftButton:
        if (selectId == 0) {
            igm::vec2 pos1 = igm::vec2(pos.x, 0);
            igm::vec2 pos2 = igm::vec2(pos.x, m_Interactor->GetHeight());

            igm::vec3 p1 = GetNearWorldCoord(pos1, invMVP);
            igm::vec3 p2 = GetFarWorldCoord(pos1, invMVP);
            igm::vec3 p3 = GetNearWorldCoord(pos2, invMVP);
            igm::vec3 intersection;

            LinePlaneIntersection(v(head), v(rear), p1, p2, p3,
                                    intersection);
            Vector3d newCenter = V(intersection);
            if (!m_DataObject->GetBoundingBox().isIn(newCenter)) { return; }

            center = newCenter;
            m_Painter->SetPen(16);
            m_Painter->SetPen(Color::Red);
            m_Painter->Delete(centerHandle);
            centerHandle = m_Painter->DrawPoint(center);

            ComputeSlicingPlane(plane);
            DrawSlicingPlane(plane);
        } else if (selectId == 1) {
            igm::vec3 p1 = GetNearWorldCoord(pos, invMVP);
            igm::vec3 p2 = GetFarWorldCoord(pos, invMVP);
            igm::vec3 intersection;

            LinePlaneIntersection(p1, p2, v(center), v(head), v(top),
                                    intersection);

            double headLen = (head - center).length();
            Vector3d newHead = V(intersection);
            Vector3d dir = (newHead - center).normalized();
            //std::cout << len << std::endl;
            head = center + headLen * dir;
            rear = center - (len - headLen) * dir;
            normal = (head - rear).normalized();

            m_Painter->SetPen(16);
            m_Painter->Delete(headHandle);
            m_Painter->SetPen(Color::Green);
            headHandle = m_Painter->DrawPoint(head);
            m_Painter->Delete(rearHandle);
            m_Painter->SetPen(Color::Blue);
            rearHandle = m_Painter->DrawPoint(rear);

            m_Painter->SetPen(4);
            m_Painter->SetPen(Color::Red);
            m_Painter->Delete(lineHandle);
            lineHandle = m_Painter->DrawLine(head, rear);

            ComputeSlicingPlane(plane);
            DrawSlicingPlane(plane);
        } else if (selectId == 2) {
            igm::vec3 p1 = GetNearWorldCoord(pos, invMVP);
            igm::vec3 p2 = GetFarWorldCoord(pos, invMVP);
            igm::vec3 intersection;

            LinePlaneIntersection(p1, p2, v(center), v(head), v(left),
                                    intersection);

            double rearLen = (rear - center).length();
            Vector3d newRear = V(intersection);
            Vector3d dir = (newRear - center).normalized();
            //std::cout << len << std::endl;
            rear = center + rearLen * dir;
            head = center - (len - rearLen) * dir;
            normal = (head - rear).normalized();

            m_Painter->SetPen(16);
            m_Painter->Delete(headHandle);
            m_Painter->SetPen(Color::Green);
            headHandle = m_Painter->DrawPoint(head);
            m_Painter->Delete(rearHandle);
            m_Painter->SetPen(Color::Blue);
            rearHandle = m_Painter->DrawPoint(rear);

            m_Painter->SetPen(4);
            m_Painter->SetPen(Color::Red);
            m_Painter->Delete(lineHandle);
            lineHandle = m_Painter->DrawLine(head, rear);

            ComputeSlicingPlane(plane);
            DrawSlicingPlane(plane);
        }
        break;
    case RightButton:
        RightButtonMouseMove();
        break;
    case MiddleButton:
        MiddleButtonMouseMove();
        break;
    default:
        break;
    }

    m_OldPoint2D = m_NewPoint2D;
}

void SlicingStyle::MouseReleaseEvent(IEvent _event) {
    BasicStyle::MouseReleaseEvent(_event);
    selectId = -1;
}

void SlicingStyle::RightButtonMouseMove() { BasicStyle::ModelRotation(); }

void SlicingStyle::MiddleButtonMouseMove() { BasicStyle::ViewTranslation(); }



//inline void reorderPoints(std::vector<Point>& points) {
//    std::vector<Point> sortedPoints = points;
//    Point centroid = {0, 0, 0};
//    for (const auto& point: points) {
//        centroid += point;
//    }
//    centroid /= points.size();
//
//    // °´¼«½ÇÅÅÐò
//    std::sort(sortedPoints.begin(), sortedPoints.end(),
//              [&centroid](const Point& a, const Point& b) {
//                  return calculateAngle(a, centroid) <
//                         calculateAngle(b, centroid);
//              });
//
//    return sortedPoints;
//}

SlicingStyle::~SlicingStyle() {
    for (int i = 0; i < 10; i++) {
        if (planeHandle[i] != 0) { m_Painter->Delete(planeHandle[i]); }
    }
    if (boxHandle != 0) { m_Painter->Delete(boxHandle); }
    if (headHandle != 0) { m_Painter->Delete(headHandle); }
    if (rearHandle != 0) { m_Painter->Delete(rearHandle); }
    if (centerHandle != 0) { m_Painter->Delete(centerHandle); }
    if (lineHandle != 0) { m_Painter->Delete(lineHandle); }
}

//inline bool calculateAngle(const Vector3d& a, const Vector3d& b) {
//
//}

void SlicingStyle::ComputeSlicingPlane(std::vector<Vector3d>& plane) {

    int count = 0;
    auto& bbox = m_DataObject->GetBoundingBox();
    Vector3d p1 = bbox.min;
    Vector3d p7 = bbox.max;
    Vector3d p2 = Vector3d(p7[0], p1[1], p1[2]);
    Vector3d p3 = Vector3d(p7[0], p7[1], p1[2]);
    Vector3d p4 = Vector3d(p1[0], p7[1], p1[2]);
    Vector3d p5 = Vector3d(p1[0], p1[1], p7[2]);
    Vector3d p6 = Vector3d(p7[0], p1[1], p7[2]);
    Vector3d p8 = Vector3d(p1[0], p7[1], p7[2]);

    plane.clear();
    Vector3d intersection;
    bool ok = false;
    if (LinePlaneIntersection2(p1, p2, center, normal, intersection)) {
        plane.push_back(intersection);
    }
    if (LinePlaneIntersection2(p2, p3, center, normal, intersection)) {
        plane.push_back(intersection);
    }
    if (LinePlaneIntersection2(p3, p4, center, normal, intersection)) {
        plane.push_back(intersection);
    }
    if (LinePlaneIntersection2(p4, p1, center, normal, intersection)) {
        plane.push_back(intersection);
    }

    if (LinePlaneIntersection2(p1, p5, center, normal, intersection)) {
        plane.push_back(intersection);
    }
    if (LinePlaneIntersection2(p2, p6, center, normal, intersection)) {
        plane.push_back(intersection);
    }
    if (LinePlaneIntersection2(p3, p7, center, normal, intersection)) {
        plane.push_back(intersection);
    }
    if (LinePlaneIntersection2(p4, p8, center, normal, intersection)) {
        plane.push_back(intersection);
    }

    if (LinePlaneIntersection2(p5, p6, center, normal, intersection)) {
        plane.push_back(intersection);
    }
    if (LinePlaneIntersection2(p6, p7, center, normal, intersection)) {
        plane.push_back(intersection);
    }
    if (LinePlaneIntersection2(p7, p8, center, normal, intersection)) {
        plane.push_back(intersection);
    }
    if (LinePlaneIntersection2(p8, p5, center, normal, intersection)) {
        plane.push_back(intersection);
    }


    if (plane.size() == 4) {
        for (int i = 1; i < 4; i++) {
            if (i != 2 &&((plane[0] - plane[i]).normalized() -
                (plane[0] - center).normalized())
                .squaredLength() < 0.01) {
                std::swap(plane[i], plane[2]);
                break;
            }
        }
        //for (int i = 1; i < 4; i++) {
        //    Vector3d intersection;
        //    if (i != 2 && IsIntersect(plane[0], plane[2], plane[1], plane[3],
        //                              intersection)) {
        //        std::swap(plane[i], plane[2]);
        //        break;
        //    }
        //}

    }

    //if (plane.size() > 3) {
    //    Vector3d centroid = {0, 0, 0};
    //    for (const auto& point: plane) {
    //        centroid += point;
    //    }
    //    centroid /= plane.size();

    //    std::sort(plane.begin(), plane.end(),
    //              [&centroid](const Vector3d& a, const Vector3d& b) {
    //                    return calculateAngle(a, centroid) <
    //                            calculateAngle(b, centroid);
    //                });
    //}

}
void SlicingStyle::DrawSlicingPlane(std::vector<Vector3d>& plane) {
    for (int i = 0; i < 10; i++) { 
        if (planeHandle[i] != 0) {
            m_Painter->Delete(planeHandle[i]);
        }
    }

    m_Painter->SetPen(4);
    m_Painter->SetPen(Color::White);
    //std::cout << plane.size() << std::endl;
    for (int i = 0; i < plane.size(); i++) {
        planeHandle[i] =
                m_Painter->DrawLine(plane[i], plane[(i + 1) % plane.size()]);
    }

    for (int i = plane.size(); i < 10; i++) { 
        planeHandle[i] = 0;
    }
}
IGAME_NAMESPACE_END