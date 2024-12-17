/**
 * @file
 * @brief    Public functions used by iGame Matrix module.
 * @author   Sumzeek
 * @date     4/12/2024
 * @version  v1.0.0
 * @par      Copyright(c): Hangzhou Dianzi University iGame Laboratory
 * @par      History:
 *	       v1.0.0: Sumzeek, 4/12/2024, first create this file\n
 */

#ifndef IGM_COMMON_H
#define IGM_COMMON_H

#include <cassert>
#include <cmath> // Necessary for std::sqrt
#include <iostream>

/** Pi value used by iGame Matrix */
#define IGM_PI 3.14159265358979323846

namespace igm
{

template<int N, typename T>
class vec {};

template<int N, int M, typename T>
class mat {};

/**
 * Converts an angle in degrees to radians.
 * @param degrees The angle in degrees.
 * @return The angle in radians.
 */
template<typename T>
double radians(T degrees);

/**
 * Calculates the determinant of a 2×2 matrix.
 * @note The matrix is in column-major order.
 * @param a The element at the first row and first column.
 * @param b The element at the second row and first column.
 * @param c The element at the first row and second column.
 * @param d The element at the second row and second column.
 * @return The determinant of the 2×2 matrix.
 */
template<typename T>
double determinant2x2(T a, T b, T c, T d);

/**
 * Calculates the determinant of a 3×3 matrix.
 * @note The matrix is in column-major order.
 * @param a1 The element at the first row and first column.
 * @param a2 The element at the second row and first column.
 * @param a3 The element at the third row and first column.
 * @param b1 The element at the first row and second column.
 * @param b2 The element at the second row and second column.
 * @param b3 The element at the third row and second column.
 * @param c1 The element at the first row and third column.
 * @param c2 The element at the second row and third column.
 * @param c3 The element at the third row and third column.
 * @return The determinant of the 3×3 matrix.
 */
template<typename T>
double determinant3x3(T a1, T a2, T a3, T b1, T b2, T b3, T c1, T c2, T c3);

} // namespace igm

#include "common.inl"

#endif // IGM_COMMON_H