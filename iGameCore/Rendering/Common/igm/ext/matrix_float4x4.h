/**
 * @file
 * @brief    iGame-Matrix库单浮点型四阶矩阵头文件
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_MATRIX_FLOAT4x4_H
#define IGM_MATRIX_FLOAT4x4_H

#include "../detail/type_mat4x4.h"

namespace igm
{
// Define a typedef for a 4x4 matrix specialized for float type.
typedef mat<4, 4, float> mat4;
typedef mat<4, 4, float> mat4x4;
typedef mat<4, 4, float> fmat4;
typedef mat<4, 4, float> fmat4x4;
} // namespace igm

#endif // IGM_MATRIX_FLOAT4x4_H
