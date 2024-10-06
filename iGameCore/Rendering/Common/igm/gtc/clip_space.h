/**
 * @file
 * @brief    Contains cutting-related usage of iGame Matrix.
 * @author   Sumzeek
 * @date     4/12/2024
 * @version  v1.0.0
 * @par      Copyright(c): Hangzhou Dianzi University iGame Laboratory
 * @par      History:
 *	       v1.0.0: Sumzeek, 4/12/2024, first create this file\n
 */

#ifndef OPENIGAME_IGM_CLIP_H
#define OPENIGAME_IGM_CLIP_H

#include "../detail/type_mat4x4.h"
#include "../detail/type_vec3.h"

namespace igm
{
/**
 * Performs a translation operation on the given 4×4 matrix.
 * @param m The original matrix to be translated.
 * @param v The translation vector.
 * @return A new matrix that represents the original matrix translated by the
 * vector.
 */
template<typename T>
mat<4, 4, T> translate(mat<4, 4, T> const& m, vec<3, T> const& v);

/**
 * Performs a rotation operation on the given 4×4 matrix.
 * @param m The original matrix to be rotated.
 * @param angle The angle of rotation in radians.
 * @param v The axis vector around which the rotation occurs.
 * @return A new matrix that represents the original matrix rotated by the given
 * angle around the axis vector.
 */
template<typename T>
mat<4, 4, T> rotate(mat<4, 4, T> const& m, T angle, vec<3, T> const& v);

/**
 * Creates a view matrix for a right-handed coordinate system.
 * @param eye The position of the camera.
 * @param center The point to look at.
 * @param up The up direction vector.
 * @return A view matrix for a right-handed coordinate system.
 */
template<typename T>
mat<4, 4, T> lookAtRH(const vec<3, T>& eye, const vec<3, T>& center,
                      const vec<3, T>& up);

/**
 * Creates a view matrix for a left-handed coordinate system.
 * @param eye The position of the camera.
 * @param center The point to look at.
 * @param up The up direction vector.
 * @return A view matrix for a left-handed coordinate system.
 */
template<typename T>
mat<4, 4, T> lookAtLH(const vec<3, T>& eye, const vec<3, T>& center,
                      const vec<3, T>& up);

///**
// * Creates a view matrix. The handedness of the coordinate system is
// * implementation-defined.
// * @param eye The position of the camera.
// * @param center The point to look at.
// * @param up The up direction vector.
// * @return A view matrix.
// * @note Default right-handed coordinate system
// */
//template<typename T>
//mat<4, 4, T> lookAt(const vec<3, T>& eye, const vec<3, T>& center,
//                    const vec<3, T>& up);

/**
 * Creates a perspective projection matrix for a right-handed coordinate system
 * with a zero to one depth range.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @param zFar The far clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear, T zFar);

/**
 * Creates a perspective projection matrix for a right-handed coordinate system
 * with a zero to one depth range and default infinite far plane.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear);

/**
 * Creates a perspective projection matrix for a right-handed coordinate system
 * with a near to one depth range.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @param zFar The far clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear, T zFar);

/**
 * Creates a perspective projection matrix for a right-handed coordinate system
 * with a near to one depth range and default infinite far plane.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear);

/**
 * Creates a perspective projection matrix for a right-handed coordinate system
 * with a one to zero depth range.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @param zFar The far clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear, T zFar);

/**
 * Creates a perspective projection matrix for a right-handed coordinate system
 * with a one to zero depth range and default infinite far plane.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear);

/**
 * Creates a perspective projection matrix for a left-handed coordinate system
 * with a zero to one depth range.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @param zFar The far clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear, T zFar);

/**
 * Creates a perspective projection matrix for a left-handed coordinate system
 * with a zero to one depth range and default infinite far plane.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear);

/**
 * Creates a perspective projection matrix for a left-handed coordinate system
 * with a near to one depth range.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @param zFar The far clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear, T zFar);

/**
 * Creates a perspective projection matrix for a left-handed coordinate system
 * with a near to one depth range and default infinite far plane.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear);

/**
 * Creates a perspective projection matrix for a left-handed coordinate system
 * with a one to zero depth range.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @param zFar The far clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear, T zFar);

/**
 * Creates a perspective projection matrix for a left-handed coordinate system
 * with a one to zero depth range and default infinite far plane.
 * @param fovy Field of view in the y direction, in radians.
 * @param aspect Aspect ratio, defined as width divided by height.
 * @param zNear The near clipping plane.
 * @return A perspective projection matrix.
 */
template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear);

/**
 * Creates an orthogonal projection matrix for a right-handed coordinate system
 * with a normalized device coordinate depth range of -1 to 1.
 * @param left The coordinate for the left vertical clipping plane.
 * @param right The coordinate for the right vertical clipping plane.
 * @param bottom The coordinate for the bottom horizontal clipping plane.
 * @param top The coordinate for the top horizontal clipping plane.
 * @param zNear The near depth clipping plane.
 * @param zFar The far depth clipping plane.
 * @return An orthogonal projection matrix.
 */
template<typename T>
mat<4, 4, T> orthoRH_NO(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * Computes an orthogonal matrix for given parameters. The handedness of the
 * coordinate system and depth range are implementation-defined.
 * @param left The coordinate for the left vertical clipping plane.
 * @param right The coordinate for the right vertical clipping plane.
 * @param bottom The coordinate for the bottom horizontal clipping plane.
 * @param top The coordinate for the top horizontal clipping plane.
 * @param zNear The near depth clipping plane.
 * @param zFar The far depth clipping plane.
 * @return An orthogonal projection matrix.
 */
template<typename T>
mat<4, 4, T> ortho(T left, T right, T bottom, T top, T zNear, T zFar);
} // namespace igm

#include "clip_space.inl"

#endif // OPENIGAME_IGM_CLIP_H
