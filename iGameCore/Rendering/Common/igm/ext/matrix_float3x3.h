/**
 * @file
 * @brief    iGame-Matrix库单浮点型三阶矩阵头文件
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_MATRIX_FLOAT3x3_H
#define IGM_MATRIX_FLOAT3x3_H

#include "../detail/type_mat3x3.h"

namespace igm
{
// Define a typedef for a 3x3 matrix specialized for float type.
typedef mat<3, 3, float> mat3;
typedef mat<3, 3, float> mat3x3;
typedef mat<3, 3, float> fmat3;
typedef mat<3, 3, float> fmat3x3;
} // namespace igm

#endif // IGM_MATRIX_FLOAT3x3_H
