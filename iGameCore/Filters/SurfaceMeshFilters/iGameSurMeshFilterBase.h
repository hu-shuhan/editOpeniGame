#pragma once
#include "iGameFilter.h"

IGAME_NAMESPACE_BEGIN
inline double ComputeTriCotA(const Vector3d& a, const Vector3d& b, const Vector3d& c) {
    auto ba = a - b;
    auto ca = a - c;
    double ll = ba.cross(ca).length();
    if (ll == 0.0) return std::numeric_limits<double>::max();
    return ba.dot(ca) / ll;
}
inline double ComputeTriArea(const Vector3d& a, const Vector3d& b, const Vector3d& c) {
    auto d10 = b - a;
    auto d20 = c - a;

    return CrossProduct(d10, d20).norm() / 2;
}
inline Vector3d ConputeTriNormal(const Vector3d& a, const Vector3d& b, const Vector3d& c) {
    auto d10 = b - a;
    auto d20 = c - a;

    return CrossProduct(d10, d20);
}
inline double ConputeTriQuality(const Vector3d& a, const Vector3d& b, const Vector3d& c) {
    auto d10 = b - a;
    auto d20 = c - a;
    auto d12 = b - c;

    Vector3f normal = CrossProduct(d10, d20);

    double x = normal.norm();
    if (x == 0) return 0;
    double y = std::max(d10.squaredNorm(), std::max(d20.squaredNorm(), d12.squaredNorm()));
    if (y == 0) return 0;
    return x / y;
}
inline bool IsInTriangle(const Point& p, const Point& a, const Point& b, const Point& c) {

    Vector3d ab = b - a;
    Vector3d bc = c - b;
    Vector3d ca = a - c;

    Vector3d ap = p - a;
    Vector3d bp = p - b;
    Vector3d cp = p - c;

    Vector3d cross1 = ab.cross(ap); // Cross product of (b-a) and (p-a)
    Vector3d cross2 = bc.cross(bp); // Cross product of (c-b) and (p-b)
    Vector3d cross3 = ca.cross(cp); // Cross product of (a-c) and (p-c)

    bool sameSign1 = cross1.dot(cross2) > 0;
    bool sameSign2 = cross2.dot(cross3) > 0;
    bool sameSign3 = cross3.dot(cross1) > 0;

    return sameSign1 && sameSign2 && sameSign3;
}
inline double GetCosTheta(const Vector3d& n1, const Vector3d& n2) {
    double dotProduct = n1.dot(n2);
    double lengths = n1.length() * n2.length();
    if (lengths == 0) return 0.0;
    double cosTheta = dotProduct / lengths;
    cosTheta = std::max(-1.0, std::min(1.0, cosTheta)); // 限制cosTheta在[-1, 1]范围内
    return std::acos(cosTheta);                         // 返回夹角，单位为弧度
}
IGAME_NAMESPACE_END