/**
 * @file
 * @brief    iGame-Matrix库单浮点型四维向量头文件
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_VECTOR_FLOAT4_H
#define IGM_VECTOR_FLOAT4_H

#include "../detail/type_vec4.h"

namespace igm
{
// Define a typedef for a 4-dimensional vector specialized for float type.
typedef vec<4, float> vec4;
typedef vec<4, float> fvec4;
} // namespace igm

#endif // IGM_VECTOR_FLOAT4_H
