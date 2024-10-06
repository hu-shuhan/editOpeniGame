//
// Created by Sumzeek on 4/12/2024.
//

#include "clip_space.h"

namespace igm
{
template<typename T>
mat<4, 4, T> translate(mat<4, 4, T> const& m, vec<3, T> const& v) {
    mat<4, 4, T> result(m);
    result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
    return result;
}

template<typename T>
mat<4, 4, T> rotate(mat<4, 4, T> const& m, T angle, vec<3, T> const& v) {
    T const a = angle;
    T const c = cos(a);
    T const s = sin(a);

    vec<3, T> axis(v.normalized());
    vec<3, T> temp(axis * (1.0f - c));

    mat<4, 4, T> Rotate;
    Rotate[0][0] = c + temp[0] * axis[0];
    Rotate[0][1] = temp[0] * axis[1] + s * axis[2];
    Rotate[0][2] = temp[0] * axis[2] - s * axis[1];

    Rotate[1][0] = temp[1] * axis[0] - s * axis[2];
    Rotate[1][1] = c + temp[1] * axis[1];
    Rotate[1][2] = temp[1] * axis[2] + s * axis[0];

    Rotate[2][0] = temp[2] * axis[0] + s * axis[1];
    Rotate[2][1] = temp[2] * axis[1] - s * axis[0];
    Rotate[2][2] = c + temp[2] * axis[2];

    mat<4, 4, T> Result;
    Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
    Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
    Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
    Result[3] = m[3];
    return Result;
}

//template<typename T>
//mat<4, 4, T> lookAt(const vec<3, T>& eye, const vec<3, T>& center,
//                    const vec<3, T>& up) {
//    return lookAtRH(eye, center, up);
//}

template<typename T>
mat<4, 4, T> lookAtRH(const vec<3, T>& eye, const vec<3, T>& center,
                      const vec<3, T>& up) {
    vec<3, T> const f((center - eye).normalized());
    vec<3, T> const s((cross(f, up)).normalized());
    vec<3, T> const u(cross(s, f));

    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = s.x;
    Result[1][0] = s.y;
    Result[2][0] = s.z;
    Result[0][1] = u.x;
    Result[1][1] = u.y;
    Result[2][1] = u.z;
    Result[0][2] = -f.x;
    Result[1][2] = -f.y;
    Result[2][2] = -f.z;
    Result[3][0] = -dot(s, eye);
    Result[3][1] = -dot(u, eye);
    Result[3][2] = dot(f, eye);
    return Result;
}

template<typename T>
mat<4, 4, T> lookAtLH(const vec<3, T>& eye, const vec<3, T>& center,
                      const vec<3, T>& up) {
    vec<3, T> const f((center - eye).normalized());
    vec<3, T> const s((cross(f, up)).normalized());
    vec<3, T> const u(cross(f, s));

    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = s.x;
    Result[1][0] = s.y;
    Result[2][0] = s.z;
    Result[0][1] = u.x;
    Result[1][1] = u.y;
    Result[2][1] = u.z;
    Result[0][2] = -f.x;
    Result[1][2] = -f.y;
    Result[2][2] = -f.z;
    Result[3][0] = -dot(s, eye);
    Result[3][1] = -dot(u, eye);
    Result[3][2] = dot(f, eye);
    return Result;
}

//template<typename T>
//mat<4, 4, T> perspective(T fovy, T aspect, T zNear) {
//    return perspectiveRH_ZO(fovy, aspect, zNear);
//}
//
//template<typename T>
//mat<4, 4, T> perspective(T fovy, T aspect, T zNear, T zFar) {
//    return perspectiveRH_ZO(fovy, aspect, zNear, zFar);
//}
//
//template<typename T>
//mat<4, 4, T> perspectiveReversedZ(T fovy, T aspect, T zNear) {
//    return perspectiveRH_OZ(fovy, aspect, zNear);
//}
//
//template<typename T>
//mat<4, 4, T> perspectiveReversedZ(T fovy, T aspect, T zNear, T zFar) {
//    return perspectiveRH_OZ(fovy, aspect, zNear, zFar);
//}

template<typename T>
// depth range: 0.0(near plane) -> 1.0(far plane)
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear, T zFar) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = zFar / (zNear - zFar);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = -(zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: 0.0(near plane) -> 1.0(far plane)
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = -static_cast<T>(1);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = -zNear;
    return Result;
}

template<typename T>
// depth range: -1.0(near plane) -> 1.0(far plane)
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear, T zFar) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = -(zFar + zNear) / (zFar - zNear);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: -1.0(near plane) -> 1.0(far plane)
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = -static_cast<T>(1);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = -(static_cast<T>(2) * zNear);
    return Result;
}

template<typename T>
// depth range: 1.0(near plane) -> 0.0(far plane)
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear, T zFar) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = std::tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = zNear / (zFar - zNear);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = (zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: 1.0(near plane) -> 0.0(far plane)
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = std::tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = zNear;
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear, T zFar) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = zFar / (zFar - zNear);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = -(zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = static_cast<T>(1);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = -zNear;
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear, T zFar) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = (zFar + zNear) / (zFar - zNear);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = static_cast<T>(1);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = -(static_cast<T>(2) * zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear, T zFar) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = -(zNear) / (zFar - zNear);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = (zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear) {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = zNear;
    return Result;
}

//template<typename T>
//mat<4, 4, T> ortho(T left, T right, T bottom, T top, T zNear, T zFar) {
//    return orthoRH_NO(left, right, bottom, top, zNear, zFar);
//}

template<typename T>
// depth range: 0.0(near plane) -> 1.0(far plane)
mat<4, 4, T> orthoRH_ZO(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = -static_cast<T>(1) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -zNear / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: 0.0(near plane) -> 1.0(far plane)
mat<4, 4, T> orthoRH_ZO(T left, T right, T bottom, T top, T zNear) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(0);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    return Result;
}

template<typename T>
// depth range: -1.0(near plane) -> 1.0(far plane)
mat<4, 4, T> orthoRH_NO(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = -static_cast<T>(2) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -(zFar + zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: -1.0(near plane) -> 1.0(far plane)
mat<4, 4, T> orthoRH_NO(T left, T right, T bottom, T top, T zNear) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(0);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -static_cast<T>(1);
    return Result;
}

template<typename T>
// depth range: 1.0(near plane) -> 0.0(far plane)
mat<4, 4, T> orthoRH_OZ(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(1) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = zFar / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: 1.0(near plane) -> 0.0(far plane)
mat<4, 4, T> orthoRH_OZ(T left, T right, T bottom, T top, T zNear) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(0);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = static_cast<T>(1);
    return Result;
}

template<typename T>
mat<4, 4, T> orthoLH_ZO(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(1) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -zNear / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> orthoLH_ZO(T left, T right, T bottom, T top, T zNear) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(0);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    return Result;
}

template<typename T>
mat<4, 4, T> orthoLH_NO(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(2) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -(zFar + zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> orthoLH_NO(T left, T right, T bottom, T top, T zNear) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(0);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -static_cast<T>(1);
    return Result;
}

template<typename T>
mat<4, 4, T> orthoLH_OZ(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = -static_cast<T>(1) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = zFar / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> orthoLH_OZ(T left, T right, T bottom, T top, T zNear) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(0);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = static_cast<T>(1);
    return Result;
}

} // namespace igm