/**
 * @file
 * @brief    iGame-Matrix库常用函数
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_COMMON_H
#define IGM_COMMON_H

#include <cassert>
#include <cmath> // 必需的头文件，包含 std::sqrt
#include <iostream>

/** iGame Matrix 使用的 Pi 常量 */
#define IGM_PI 3.14159265358979323846

namespace igm
{

// 模板类定义，用于表示向量（尺寸为 N）
template<int N, typename T>
class vec {};

// 模板类定义，用于表示矩阵（尺寸为 M×N）
template<int M, int N, typename T>
class mat {};

/**
 * 将角度从度数转换为弧度。
 * @param degrees 输入的角度（单位：度）。
 * @return 返回转换后的角度（单位：弧度）。
 */
template<typename T>
double radians(T degrees);

/**
 * 计算一个 2×2 矩阵的行列式。
 * @note 矩阵采用列主序（column-major）排列。
 * @param a 第1行第1列的元素。
 * @param b 第2行第1列的元素。
 * @param c 第1行第2列的元素。
 * @param d 第2行第2列的元素。
 * @return 返回 2×2 矩阵的行列式。
 */
template<typename T>
double determinant2x2(T a, T b, T c, T d);

/**
 * 计算一个 3×3 矩阵的行列式。
 * @note 矩阵采用列主序（column-major）排列。
 * @param a1 第1行第1列的元素。
 * @param a2 第2行第1列的元素。
 * @param a3 第3行第1列的元素。
 * @param b1 第1行第2列的元素。
 * @param b2 第2行第2列的元素。
 * @param b3 第3行第2列的元素。
 * @param c1 第1行第3列的元素。
 * @param c2 第2行第3列的元素。
 * @param c3 第3行第3列的元素。
 * @return 返回 3×3 矩阵的行列式。
 */
template<typename T>
double determinant3x3(T a1, T a2, T a3, T b1, T b2, T b3, T c1, T c2, T c3);

} // namespace igm

#include "common.inl"

#endif // IGM_COMMON_H
