/**
 * @file
 * @brief    iGame-Matrix库无符号整型四维向量头文件
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_VECTOR_UINT4_H
#define IGM_VECTOR_UINT4_H

#include "../detail/type_vec4.h"

namespace igm
{
// Define a typedef for a 4-dimensional vector specialized for unsigned int type.
typedef vec<4, unsigned int> uvec4;
} // namespace igm

#endif // IGM_VECTOR_UINT4_H
