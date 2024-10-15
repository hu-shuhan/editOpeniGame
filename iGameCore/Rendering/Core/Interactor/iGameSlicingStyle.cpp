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
    head = rear = center;
    head[0] += bbox.diag() / 2;
    rear[0] -= bbox.diag() / 2;
    len = bbox.diag();
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
    Invoke();
}

void SlicingStyle::Invoke() { 
    slicingPlane.point[0] = center[0];
    slicingPlane.point[1] = center[1];
    slicingPlane.point[2] = center[2];
    slicingPlane.normal[0] = normal[0];
    slicingPlane.normal[1] = normal[1];
    slicingPlane.normal[2] = normal[2];

    this->RequestSignal(InteractorStyle::Slicing, &slicingPlane);
}

void SlicingStyle::MousePressEvent(IEvent _event) {
    BasicStyle::MousePressEvent(_event);

    mvp = m_Interactor->GetMVP();
    invMVP = mvp.invert();
    Vector3Tovec3 v;

    auto& pos = _event.pos;
    igm::vec3 point1 = GetNearWorldCoord(pos, invMVP);
    igm::vec3 point2 = GetFarWorldCoord(pos, invMVP);
    Vector3d intersection;

    if (DistancePointToLine(v(center), point1, point2) < 0.1) {
        selectId = 0;
    } else if (DistancePointToLine(v(head), point1, point2) < 0.1) {
        selectId = 1;
    } else if (DistancePointToLine(v(rear), point1, point2) < 0.1) {
        selectId = 2;
    } else if (IsIntersect(V(point1), V(point2), head, center, intersection)) {
        selectId = 3;
    } else {
        selectId = -1;
    }
}
void SlicingStyle::MouseMoveEvent(IEvent _event) {
    igm::vec2 pos = _event.pos;
    m_NewPoint2D = _event.pos;

    switch (m_MouseMode) {
    case LeftButton:
        if (selectId == 0) { // 拖动中点 Drag center pointer
            igm::vec2 pos1 = igm::vec2(pos.x, 0);
            igm::vec2 pos2 = igm::vec2(pos.x, m_Interactor->GetHeight());

            // p1,p2,p3 组成视锥平面 form the cone plane
            igm::vec3 p1 = GetNearWorldCoord(pos1, invMVP);
            igm::vec3 p2 = GetFarWorldCoord(pos1, invMVP);
            igm::vec3 p3 = GetNearWorldCoord(pos2, invMVP);
            igm::vec3 intersection;

            // 计算直线与与视锥平面的交点 Calculate the intersection of the line with the cone plane
            LinePlaneIntersection(v(head), v(rear), p1, p2, p3,
                                    intersection);
            Vector3d newCenter = V(intersection);
            if (!m_DataObject->GetBoundingBox().isIn(newCenter)) { return; }

            Vector3d dir = newCenter - center;
            top = top + dir;
            left = left + dir;
            center = newCenter;
            m_Painter->SetPen(16);
            m_Painter->SetPen(Color::Red);
            m_Painter->Delete(centerHandle);
            centerHandle = m_Painter->DrawPoint(center);

            ComputeSlicingPlane(plane);
            DrawSlicingPlane(plane);
            Invoke();
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
            Invoke();
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
            Invoke();
        } else if (selectId == 3) {
            
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

    // 计算切平面与boundingbox的交点 Calculate the intersection of the tangent plane and the boundingbox
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

    Vector3d centroid = {0, 0, 0};
    for (const auto& point: plane) {
        centroid += point;
    }
    centroid /= plane.size();
    Vector3d axis = (0.75 * plane[0] + 0.25 * plane[1]) - centroid;
    double length = axis.length();
    Vector3d n = head - center;

    // 计算与axis向量的夹角，带正负 Calculate the Angle with the axis vector, plus or minus
    auto calculateAngle = [&](const Vector3d& p) -> double {
        Vector3d axis2 = p - centroid;
        return std::acos(axis.dot(axis2) / length / axis2.length()) *
               (axis.cross(axis2) * n > 0 ? 1 : -1);
    };

    std::sort(plane.begin(), plane.end(),
              [&](const Vector3d& a, const Vector3d& b) {
                  return calculateAngle(a) < calculateAngle(b);
              });

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

bool SlicingStyle::LinePlaneIntersection2(const Vector3d& A, const Vector3d& B,
                            const Vector3d& P, const Vector3d& N,
                            Vector3d& intersection) {
    // 直线的方向向量 The direction vector of the line
    Vector3d u = B - A;

    double D = -N.dot(P);

    double denominator = N.dot(u);
    if (denominator == 0) {
        return false; // 直线与平面平行 The line is parallel to the plane
    }

    double t = -(N.dot(A) + D) / denominator;

    if (t < 0 || t > 1) {
        return false; // 交点不在直线段上 The intersection is not on the line segment
    }

    // 计算交点 Calculated intersection
    intersection = A + t * u;
    return true;
}

bool SlicingStyle::IsIntersect(const Vector3d& p1, const Vector3d& p2,
                               const Vector3d& p3,
                 const Vector3d& p4, Vector3d& intersection) {
    Vector3d d1 = p2 - p1;
    Vector3d d2 = p4 - p3;
    Vector3d r = p1 - p3;

    double d = d1.dot(d2.cross(d2));

    if (std::abs(d) < 1e-10) {
        // 直线平行或重合 The lines are parallel or overlap
        return false;
    }

    double t = (r.cross(d2).dot(d2)) / d;
    double u = (r.cross(d1).dot(d1)) / d;

    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        intersection = p1 + t * d1; // 计算交点 Calculated intersection
        return true;
    }

    return false;
}

IGAME_NAMESPACE_END