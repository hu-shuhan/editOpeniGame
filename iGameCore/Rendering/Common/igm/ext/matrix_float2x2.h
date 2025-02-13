/**
 * @file
 * @brief    iGame-Matrix库单浮点型二阶矩阵头文件
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_MATRIX_FLOAT2x2_H
#define IGM_MATRIX_FLOAT2x2_H

#include "../detail/type_mat2x2.h"

namespace igm
{
// Define a typedef for a 2x2 matrix specialized for float type.
typedef mat<2, 2, float> mat2;
typedef mat<2, 2, float> mat2x2;
typedef mat<2, 2, float> fmat2;
typedef mat<2, 2, float> fmat2x2;
} // namespace igm

#endif // IGM_MATRIX_FLOAT2x2_H
