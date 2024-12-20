/**
 * @file
 * @brief    iGame-Matrix库矩阵裁减函数头文件
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_CLIP_H
#define IGM_CLIP_H

#include "../detail/type_mat4x4.h"
#include "../detail/type_vec3.h"

namespace igm
{

/**
 * @brief 对给定的四阶矩阵执行平移操作。
 * @param m 需要平移的原始矩阵。
 * @param v 平移向量。
 * @return 一个新的四阶矩阵，表示原始矩阵经过平移向量变换后的结果。
 */
template<typename T>
mat<4, 4, T> translate(mat<4, 4, T> const& m, vec<3, T> const& v);

/**
 * @brief 对给定的四阶矩阵执行旋转操作。
 * @param m 需要旋转的原始四阶矩阵。
 * @param angle 旋转角度（单位：弧度）。
 * @param v 旋转轴向量。
 * @return 一个新的四阶矩阵，表示原始四阶矩阵围绕给定的轴向量旋转指定角度后的结果。
 */
template<typename T>
mat<4, 4, T> rotate(mat<4, 4, T> const& m, T angle, vec<3, T> const& v);

/**
 * @brief 为右手坐标系创建视图矩阵。
 * @param eye 相机的位置。
 * @param center 观察的目标点。
 * @param up 向上的方向向量。
 * @return 一个用于右手坐标系的视图矩阵。
 */
template<typename T>
mat<4, 4, T> lookAtRH(const vec<3, T>& eye, const vec<3, T>& center,
                      const vec<3, T>& up);

/**
 * @brief 为左手坐标系创建视图矩阵。
 * @param eye 相机的位置。
 * @param center 观察的目标点。
 * @param up 向上的方向向量。
 * @return 一个用于左手坐标系的视图矩阵。
 */
template<typename T>
mat<4, 4, T> lookAtLH(const vec<3, T>& eye, const vec<3, T>& center,
                      const vec<3, T>& up);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从0到1。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从0到1，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从-1到1。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从-1到1，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从1到0。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从1到0，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围为从0到1。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围为从0到1，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围从-1到1。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围从-1到1，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围从1到0。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围从1到0，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear);

/**
 * @brief 为右手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的0到1。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
template<typename T>
mat<4, 4, T> orthoRH_ZO(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的-1到1。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
template<typename T>
mat<4, 4, T> orthoRH_NO(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的1到0。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
template<typename T>
mat<4, 4, T> orthoRH_OZ(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的0到1。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
template<typename T>
mat<4, 4, T> orthoLH_ZO(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的-1到1。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
template<typename T>
mat<4, 4, T> orthoLH_NO(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的1到0。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
template<typename T>
mat<4, 4, T> orthoLH_OZ(T left, T right, T bottom, T top, T zNear, T zFar);

} // namespace igm

#include "clip_space.inl"

#endif // IGM_CLIP_H
