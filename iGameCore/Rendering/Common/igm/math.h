/**
 * @file
 * @brief    iGame-Matrix库数学函数头文件
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */


#ifndef IGM_MATH_H
#define IGM_MATH_H

#include "mat2x2.h"
#include "mat3x3.h"
#include "mat4x4.h"
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"

namespace igm
{

/**
 * @brief 计算点到直线的距离
 *
 * @param linePoint 直线上的一点
 * @param dir 直线的方向向量
 * @param point 需要计算的点
 * @return double 返回点到直线的距离
 */
inline double computePointToLineDis(const igm::vec3& linePoint,
                                    const igm::vec3& dir,
                                    const igm::vec3& point) {
    return std::fabs(igm::cross(dir, point - linePoint).length() /
                     dir.length());
}

} // namespace igm


#endif // IGM_IGM_H