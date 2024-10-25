//
// Created by Sumzeek on 9/9/2024.
//

#include "iGameBasicStyle.h"
#include "iGameInteractor.h"

IGAME_NAMESPACE_BEGIN
void BasicStyle::Initialize(Interactor* a) {
    m_Interactor = a;
    m_Scene = a->GetScene();
    m_Camera = a->GetCamera();
}

void BasicStyle::MousePressEvent(IEvent _event) {
    m_OldPoint2D = _event.pos;
    m_MouseMode = _event.button;
};
void BasicStyle::MouseMoveEvent(IEvent _event) {
    m_NewPoint2D = _event.pos;
    //if (m_OldPoint2D == m_NewPoint2D) { return; }

    switch (m_MouseMode) {
        case LeftButton:
            LeftButtonMouseMove();
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
};
void BasicStyle::LeftButtonMouseMove() { ModelRotation(); }

void BasicStyle::RightButtonMouseMove() { ViewTranslation(); }

void BasicStyle::MiddleButtonMouseMove() {}

void BasicStyle::MouseReleaseEvent(IEvent _event) { m_MouseMode = NoButton; };
void BasicStyle::WheelEvent(IEvent _event) {
    float wheelMoveDirection = 0.0;
    if (_event.delta == 0) {
        std::cout << "The wheel movement given to the interactor is 0"
                  << std::endl;
        return;
    } else if (_event.delta > 0.0) {
        wheelMoveDirection = 1.0f;
    } else {
        wheelMoveDirection = -1.0f;
    }

    auto dist = m_Camera->GetLengthToFocal();
    m_CameraScaleSpeed = dist * 0.1f;

    auto moveSize =
            static_cast<float>(-wheelMoveDirection * m_CameraScaleSpeed);
    auto oldPos = m_Camera->GetCameraPos();
    auto newPos = oldPos + igm::vec3{0.0f, 0.0f, moveSize};
    m_Camera->SetCameraPos(newPos);

    UpdateCameraMoveSpeed(m_Scene->ModelsBoundingSphere());
};

void BasicStyle::RequestSignal(InteractorStyle::Signal signal, void* callData) {
    if (m_Interactor) { m_Interactor->RequestSignal(signal, callData); }
}

void BasicStyle::ModelRotation() {
    igm::vec3 oldPoint3D, newPoint3D;
    MapToSphere(oldPoint3D, newPoint3D);

    igm::vec3 axis = igm::cross(oldPoint3D, newPoint3D); // corss product
    if (axis.length() < 1e-7) {
        axis = igm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        axis.normalize();
    }
    // find the amount of rotation
    igm::vec3 d = oldPoint3D - newPoint3D;
    const double trackballradius = 0.6;
    double t = 0.5 * d.length() / trackballradius;
    if (t < -1.0) {
        t = -1.0;
    } else if (t > 1.0) {
        t = 1.0;
    }

    double phi = 2.0 * asin(t);
    double angle = phi * 180.0 / IGM_PI;

    igm::vec4 center = igm::vec4{m_Scene->ModelsBoundingSphere().xyz(), 1.0f};
    igm::vec3 centerInWorld = (m_Scene->ModelMatrix() * center).xyz();

    igm::mat4 translateToOrigin = igm::translate(igm::mat4{}, -centerInWorld);
    igm::mat4 translateBack = igm::translate(igm::mat4{}, centerInWorld);
    igm::mat4 rotateMatrix = igm::rotate(
            igm::mat4{}, static_cast<float>(igm::radians(angle)), axis);

    igm::mat4 rotate = translateBack * rotateMatrix * translateToOrigin;
    m_Scene->ModelMatrix() = rotate * (m_Scene->ModelMatrix());

    // updated the rotation matrix of the origin
    m_Scene->ModelRotate() = rotateMatrix * (m_Scene->ModelRotate());
}
void BasicStyle::ViewTranslation() {
    if (m_Camera) {
        UpdateCameraMoveSpeed(m_Scene->ModelsBoundingSphere());

        auto offset = m_NewPoint2D - m_OldPoint2D;
        auto moveOffset = igm::vec3{-offset.x * m_CameraMoveSpeed,
                                    offset.y * m_CameraMoveSpeed, 0.0f};
        auto oldPos = m_Camera->GetCameraPos();
        auto oldFocal = m_Camera->GetCameraFocal();
        auto newPos = oldPos + moveOffset;
        auto newFocal = oldFocal + moveOffset;
        m_Camera->SetCameraPos(newPos);
        m_Camera->SetCameraFocal(newFocal);
    }
}
void BasicStyle::MapToSphere(igm::vec3& old_v3D, igm::vec3& new_v3D) {
    auto center = igm::vec3(m_Scene->ModelsBoundingSphere());

    igm::mat4 model = m_Scene->ModelMatrix();
    igm::mat4 view = m_Camera->GetViewMatrix();
    igm::mat4 proj = m_Camera->GetProjectionMatrix();

    auto p = igm::vec4{center, 1.0f};
    auto p_mvp = (proj * view * model * p);
    p_mvp /= p_mvp.w;

    // if the perspective enters the model, rotate around (0,0)
    if (p_mvp.x > 1.0f || p_mvp.x < -1.0f || p_mvp.y > 1.0f ||
        p_mvp.y < -1.0f) {
        p_mvp = igm::vec4{0.0f, 0.0f, 0.0f, 0.0f};
    }

    auto width = m_Camera->GetViewPort().x;
    auto height = m_Camera->GetViewPort().y;

    const double trackballradius = 0.6;
    const double rsqr = trackballradius * trackballradius;

    // calculate old hit sphere point3D
    double oldX = (2.0 * m_OldPoint2D.x - width) / width - p_mvp.x;
    double oldY = -(2.0 * m_OldPoint2D.y - height) / height - p_mvp.y;
    double old_x2y2 = oldX * oldX + oldY * oldY;

    old_v3D[0] = oldX;
    old_v3D[1] = oldY;
    if (old_x2y2 < 0.5 * rsqr) {
        old_v3D[2] = sqrt(rsqr - old_x2y2);
    } else {
        old_v3D[2] = 0.5 * rsqr / sqrt(old_x2y2);
    }

    // calculate new hit sphere point3D
    double newX = (2.0 * m_NewPoint2D.x - width) / width - p_mvp.x;
    double newY = -(2.0 * m_NewPoint2D.y - height) / height - p_mvp.y;
    double new_x2y2 = newX * newX + newY * newY;

    new_v3D[0] = newX;
    new_v3D[1] = newY;
    if (new_x2y2 < 0.5 * rsqr) {
        new_v3D[2] = sqrt(rsqr - new_x2y2);
    } else {
        new_v3D[2] = 0.5 * rsqr / sqrt(new_x2y2);
    }
}
void BasicStyle::UpdateCameraMoveSpeed(const igm::vec4& center) {
    auto viewport = m_Camera->GetViewPort();
    auto viewportF = igm::vec2{static_cast<float>(viewport.x),
                               static_cast<float>(viewport.y)};

    if (m_Camera->GetCameraType() == Camera::ORTHOGRAPHIC) {
        // Step 1: Calculate the world size of one pixel
        float orthoHeight = m_Camera->GetLengthToFocal() * 0.5f;
        float pixelSizeWorld = orthoHeight / viewportF.y;

        // Step 2: Apply the pixel offset to the world coordinates
        igm::vec3 pWorldCoord =
                igm::vec3(center) + igm::vec3(0, pixelSizeWorld, 0);

        m_CameraMoveSpeed = (pWorldCoord - igm::vec3(center)).length();
    } else if (m_Camera->GetCameraType() == Camera::PERSPECTIVE) {
        igm::mat4 model = m_Scene->ModelMatrix();
        igm::mat4 view = m_Camera->GetViewMatrix();
        igm::mat4 proj = m_Camera->GetProjectionMatrix();
        auto mvp = proj * view * model;

        auto centerMvp = mvp * igm::vec4{center.xyz(), 1.0f};
        centerMvp /= centerMvp.w;
        auto bz = centerMvp.z;

        // the center of the bounding-sphere is located behind the near plane
        if (bz > 1.0f || bz < 0.0f) {
            auto cameraFront =
                    (m_Camera->GetCameraFocal() - m_Camera->GetCameraPos())
                            .normalized();
            auto boundingBehind = center.xyz() + cameraFront * center.w;
            auto boundingBehindMvp = mvp * igm::vec4{boundingBehind, 1.0f};
            boundingBehindMvp /= boundingBehindMvp.w;
            bz = boundingBehindMvp.z;

            // the bounding-sphere is behind camera
            if (bz > 1.0f) return;
        }

        // p is the screen coordinate of the center offset one pixel upwards
        auto p = igm::vec3{centerMvp.x, centerMvp.y + 2.0f / viewportF.y,
                           centerMvp.z};
        auto pWorldCoord = mvp.invert() * igm::vec4{p, 1.0f};
        pWorldCoord /= pWorldCoord.w;

        m_CameraMoveSpeed =
                (igm::vec3(pWorldCoord) - igm::vec3(center)).length();

        /*
        Eigen::Matrix4f A;
        Eigen::Vector4f b;
        A << mvp[0][0], mvp[1][0], mvp[2][0], mvp[3][0], mvp[0][1], mvp[1][1],
                mvp[2][1], mvp[3][1], mvp[0][2], mvp[1][2], mvp[2][2], mvp[3][2],
                mvp[0][3], mvp[1][3], mvp[2][3], mvp[3][3];

        igm::vec3 c{0.0f};
        // c is the point located at the center of the screen after mvp transformation
        {
            // | x11 x12 x13 x14 |   |  x  | =  |  0  |
            // | x21 x22 x23 x24 | * |  y  |    |  0  |
            // | x31 x32 x33 x34 |   |  z  |    | w*bz|
            // | x41 x42 x43 x44 |   | 1.0 |    |  w  |
            b << 0.0f, 0.0f, bz, 1.0f;

            Eigen::Vector4f solution = A.colPivHouseholderQr().solve(b);
            //Eigen::Vector4f solution =
            //        A.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);

            float x = solution(0);
            float y = solution(1);
            float z = solution(2);
            float w = solution(3);
            c = igm::vec3{x / w, y / w, z / w};
        }

        igm::vec3 p{0.0f};
        // p is the point located at the screen pixel (0,1) after mvp transformation
        {
            // | x11 x12 x13 x14 |   |  x  | =  |        0       |
            // | x21 x22 x23 x24 | * |  y  |    | w * 2 / height |
            // | x31 x32 x33 x34 |   |  z  |    |      w * bz    |
            // | x41 x42 x43 x44 |   | 1.0 |    |        w       |
            b << 0.0f, 2.0f / viewportF.y, bz, 1.0f;

            Eigen::Vector4f solution = A.colPivHouseholderQr().solve(b);
            //Eigen::Vector4f solution =
            //        A.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);

            float x = solution(0);
            float y = solution(1);
            float z = solution(2);
            float w = solution(3);
            p = igm::vec3{x / w, y / w, z / w};
        }

        m_CameraMoveSpeed = (p - c).length();
        */
    }
}

bool BasicStyle::IsIntersectTriangle(igm::vec3 orig, igm::vec3 end,
                                     igm::vec3 v0, igm::vec3 v1, igm::vec3 v2,
                                     igm::vec3& intersection) {
    float t, u, v;
    igm::vec3 dir = end - orig;
    igm::vec3 E1 = v1 - v0;
    igm::vec3 E2 = v2 - v0;
    // cross product
    igm::vec3 P(dir.y * E2.z - E2.y * dir.z, E2.x * dir.z - E2.z * dir.x,
                dir.x * E2.y - dir.y * E2.x);
    // dot product
    float det = E1.x * P.x + E1.y * P.y + E1.z * P.z;
    igm::vec3 T;
    if (det > 0) {
        T = orig - v0;
    } else {
        T = v0 - orig;
        det = -det;
    }
    if (det < 0) { return false; }

    // Calculate u and make sure u <= 1
    u = T.x * P.x + T.y * P.y + T.z * P.z;
    if (u < 0.0f || u > det) { return false; }

    igm::vec3 Q(T.y * E1.z - E1.y * T.z, E1.x * T.z - E1.z * T.x,
                T.x * E1.y - T.y * E1.x);

    // Calculate v and make sure u + v <= 1
    v = dir.x * Q.x + dir.y * Q.y + dir.z * Q.z;
    if (v < 0.0f || u + v > det) { return false; }

    // Calculate t, scale parameters, ray intersects triangle
    float fInvDet = 1.0f / det;
    u *= fInvDet;
    v *= fInvDet;
    intersection = E1 * u + E2 * v + v0;
    return true;
}


double BasicStyle::DistancePointToPlane(igm::vec3 point, igm::vec3 p1,
                                        igm::vec3 p2, igm::vec3 p3) {
    // Calculate two vectors on the plane
    igm::vec3 v1 = {p2.x - p1.x, p2.y - p1.y, p2.z - p1.z};
    igm::vec3 v2 = {p3.x - p1.x, p3.y - p1.y, p3.z - p1.z};

    // Calculate the normal vector of the plane
    igm::vec3 n = v1.cross(v2);

    // Calculate the d constant in the plane equation
    double d = -n.dot(p1);

    // Calculate the distance from the point to the plane
    double numerator =
            std::abs(n.x * point.x + n.y * point.y + n.z * point.z + d);
    double denominator = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);

    return numerator / denominator;
}

double BasicStyle::DistancePointToLine(igm::vec3 point, igm::vec3 p1,
                                       igm::vec3 p2) {
    vec3ToVector3d v;
    return DistancePointToLine(v(point), v(p1), v(p2));
}

double BasicStyle::DistancePointToLine(Vector3d point, Vector3d p1,
                                       Vector3d p2) {
    return Line::ComputePointToLineDis(p1, (p1 - p2).normalized(), point);
}

igm::vec4 BasicStyle::GetPlane(const igm::vec3& p, const igm::vec3& normal) {
    igm::vec3 n = normal.normalized();
    return igm::vec4(n, -n.dot(p));
}

igm::vec3 BasicStyle::GetNearWorldCoord(const igm::vec2& screenCoord,
                                        const igm::mat4& invertedMvp) {
    igm::vec2 NDC(2.0f * screenCoord.x / m_Interactor->GetWidth() - 1.0f,
                  1.0f - (2.0f * screenCoord.y / m_Interactor->GetHeight()));

    // Clipping coordinate
    igm::vec4 clippingCoord(NDC, 1, 1.0);

    // World coordinate
    igm::vec4 worldCoord = invertedMvp * clippingCoord;
    return igm::vec3(worldCoord / worldCoord.w);
}


igm::vec3 BasicStyle::GetFarWorldCoord(const igm::vec2& screenCoord,
                                       const igm::mat4& invertedMvp) {
    igm::vec2 NDC(2.0f * screenCoord.x / m_Interactor->GetWidth() - 1.0f,
                  1.0f - (2.0f * screenCoord.y / m_Interactor->GetHeight()));

    // Clipping coordinate
    igm::vec4 clippingCoord(NDC, 0.001, 1.0);

    // World coordinate
    igm::vec4 worldCoord = invertedMvp * clippingCoord;
    return igm::vec3(worldCoord / worldCoord.w);
}
IGAME_NAMESPACE_END