/**
 * @class    mat<2, 2, T>
 * @brief    mat<2, 2, T>类包含二阶矩阵的数据结构和函数定义。
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_TYPE_MAT2x2_H
#define IGM_TYPE_MAT2x2_H

#include "common.h"
#include "type_vec2.h"

namespace igm
{

template<typename T>
class mat<2, 2, T> {
public:
    /**
     * @brief 默认构造函数，将矩阵初始化为单位矩阵。
     */
    mat() : value{} { setToIdentity(); }

    /**
     * @brief  构造函数，用指定的值初始化二阶矩阵。
     * @param  x0  第一列，第一行的值。
     * @param  y0  第一列，第二行的值。
     * @param  x1  第二列，第一行的值。
     * @param  y1  第二列，第二行的值。
     */
    mat(const T& x0, const T& y0, const T& x1, const T& y1)
        : value{vec<2, T>(x0, y0), vec<2, T>(x1, y1)} {}

    /**
     * @brief  构造函数，用给定的二维列向量初始化二阶矩阵。
     * @param  v0  第一个列向量。
     * @param  v1  第二个列向量。
     */
    mat(const vec<2, T>& v0, const vec<2, T>& v1) : value{} {
        this->value[0] = v0;
        this->value[1] = v1;
    }

    /**
     * @brief  构造函数，将矩阵初始化为对角矩阵。
     * @param  diagonal  设置在对角线上的值。
     */
    explicit mat(const T& diagonal) : value{} {
        this->value[0] = vec<2, T>(diagonal, 0);
        this->value[1] = vec<2, T>(0, diagonal);
    }

    /**
     * @brief  拷贝构造函数。
     * @param  other  要拷贝的矩阵。
     */
    mat(const mat<2, 2, T>& other) : value{} {
        this->value[0] = other.value[0];
        this->value[1] = other.value[1];
    }

    /**
     * @brief  二阶矩阵的流插入运算符。
     * @param  os 输出流。
     * @param  m 要输出的二阶矩阵对象。
     * @return 输出流的引用。
     */
    friend std::ostream& operator<<(std::ostream& os, const mat<2, 2, T>& m) {
        os << "[" << m[0][0] << ", " << m[1][0] << ",\n";
        os << " " << m[0][1] << ", " << m[1][1] << "]\n";
        return os;
    }

    /**
     * @brief  赋值运算符。
     * @param  other  要赋值的二阶矩阵对象。
     * @return  经过赋值后，本对象的引用。
     */
    mat<2, 2, T>& operator=(const mat<2, 2, T>& other) {
        if (this != &other) {
            this->value[0] = other[0];
            this->value[1] = other[1];
        }
        return *this;
    }

    /**
     * @brief  二阶矩阵的加法运算符。
     * @param  other  要加的二阶矩阵对象。
     * @return  一个新的二阶矩阵，其为两个二阶矩阵的加法结果。
     */
    mat<2, 2, T> operator+(const mat<2, 2, T>& other) const {
        mat<2, 2, T> result;
        result[0] = this->value[0] + other[0];
        result[1] = this->value[1] + other[1];
        return result;
    }

    /**
     * @brief  二阶矩阵的加法赋值运算符。
     * @param  other  要加上的二阶矩阵对象。
     * @return  经过加法后的本对象引用。
     */
    mat<2, 2, T>& operator+=(const mat<2, 2, T>& other) {
        this->value[0] += other[0];
        this->value[1] += other[1];
        return *this;
    }

    /**
     * @brief  二阶矩阵的减法运算符。
     * @param  other  要减去的二阶矩阵对象。
     * @return  一个新的二阶矩阵，其为两个二阶矩阵的减法结果。
     */
    mat<2, 2, T> operator-(const mat<2, 2, T>& other) const {
        mat<2, 2, T> result;
        result[0] = this->value[0] - other[0];
        result[1] = this->value[1] - other[1];
        return result;
    }

    /**
     * @brief  二阶矩阵的减法赋值运算符。
     * @param  other  要减去的二阶矩阵对象。
     * @return  经过减法后的本对象引用。
     */
    mat<2, 2, T>& operator-=(const mat<2, 2, T>& other) {
        this->value[0] -= other[0];
        this->value[1] -= other[1];
        return *this;
    }

    /**
     * @brief  二阶矩阵的常数乘法运算符。
     * @param  scalar  要乘的常数值。
     * @return  一个新的二阶矩阵，其为二阶矩阵乘以常数的结果。
     */
    mat<2, 2, T> operator*(T scalar) const {
        mat<2, 2, T> result;
        result[0] = this->value[0] * scalar;
        result[1] = this->value[1] * scalar;
        return result;
    }

    /**
     * @brief  二阶矩阵的常数乘法赋值运算符。
     * @param  scalar  要乘的常数值。
     * @return  经过常数乘法后的本对象引用。
     */
    mat<2, 2, T>& operator*=(const T& scalar) {
        this->value[0] *= scalar;
        this->value[1] *= scalar;
        return *this;
    }

    /**
     * @brief  二阶矩阵和二维向量的矩阵-向量乘法运算符。
     *
     * 该运算符将一个二阶矩阵与一个二维向量相乘。结果向量的计算方式如下：
     *
     * result[0] = (m[0][0] * v[0]) + (m[1][0] * v[1])
     * result[1] = (m[0][1] * v[0]) + (m[1][1] * v[1])
     *
     * 其中 m 是矩阵，v 是向量。
     *
     * @param v 要相乘的二维向量对象。
     * @return  经过乘法运算后的新二维向量。
     */
    vec<2, T> operator*(const vec<2, T>& v) const {
        return vec<2, T>(value[0][0] * v[0] + value[1][0] * v[1],
                         value[0][1] * v[0] + value[1][1] * v[1]);
    }

    /**
     * @brief 二阶矩阵和二阶矩阵的矩阵乘法运算符。
     * @param other 要相乘的二阶矩阵对象。
     * @return 经过乘法运算后的新的二阶矩阵。
     */
    mat<2, 2, T> operator*(const mat<2, 2, T>& other) const {
        mat<2, 2, T> result;
        for (int c = 0; c < 2; ++c) {
            for (int r = 0; r < 2; ++r) {
                result.value[c][r] = value[0][r] * other.value[c][0] +
                                     value[1][r] * other.value[c][1];
            }
        }
        return result;
    }

    /**
     * @brief 二阶矩阵和二阶矩阵的矩阵乘法赋值运算符。
     * @param other 要相乘的二阶矩阵对象。
     * @return 经过矩阵乘法后的本对象引用。
     */
    mat<2, 2, T>& operator*=(const mat<2, 2, T>& other) {
        mat<2, 2, T> result;
        for (int c = 0; c < 2; ++c) {
            for (int r = 0; r < 2; ++r) {
                result.value[c][r] = value[0][r] * other.value[c][0] +
                                     value[1][r] * other.value[c][1];
            }
        }
        *this = result;
        return *this;
    }

    /**
     * @brief 下标运算符，用于访问矩阵的列向量。
     * @param index 列的索引（0 或 1）。
     * @return 返回列向量的引用。
     */
    vec<2, T>& operator[](int index) {
        assert(index >= 0 && index < 2);
        return value[index];
    }

    /**
     * @brief 下标运算符，用于访问矩阵的列向量（常数版本）。
     * @param index 列的索引（0 或 1）。
     * @return 返回列向量的常量引用。
     */
    const vec<2, T>& operator[](int index) const {
        assert(index >= 0 && index < 2);
        return value[index];
    }

    /**
     * @brief 获取指向矩阵底层数据的指针。
     * @return 指向矩阵第一个列向量元素的指针。
     */
    T const* data() const { return &value[0].x; }

    /**
     * @brief 将矩阵设置为单位矩阵。
     */
    void setToIdentity();

    /**
     * @brief 计算矩阵的行列式。
     * @return 行列式值。
     */
    [[nodiscard]] T determinant() const;

    /**
     * @brief 计算矩阵的伴随矩阵（伴随矩阵）。
     * @return 一个新的二阶矩阵，其为调用对象的伴随矩阵。
     */
    [[nodiscard]] mat<2, 2, T> adjoint() const;

    /**
     * @brief 计算矩阵的逆矩阵。
     * @return 一个新的二阶矩阵，其为调用对象的逆矩阵。
     */
    [[nodiscard]] mat<2, 2, T> invert() const;

    /**
     * @brief 计算矩阵的转置矩阵。
     * @return 一个新的二阶矩阵，其为调用对象的转置矩阵。
     */
    [[nodiscard]] mat<2, 2, T> transpose() const;

protected:
    vec<2, T> value[2]; /**< 用于保存矩阵的列向量的数组。 */
};

/**
 * @brief 获取二阶矩阵矩阵数据的常量指针。
 * @param m 二阶矩阵对象。
 * @return 矩阵 m 底层数据的常量指针。
 */
template<typename T>
T const* value_ptr(const mat<2, 2, T>& m) {
    return m.data();
}

/**
 * @brief 计算二阶矩阵的行列式。
 * @param m 二阶矩阵对象。
 * @return 矩阵 m 的行列式值。
 */
template<typename T>
T determinant(const mat<2, 2, T>& m);

/**
 * @brief 把一个二阶矩阵设置为其伴随矩阵。
 * @param m 二阶矩阵对象。
 * @return 矩阵 m 的伴随矩阵。
 */
template<typename T>
mat<2, 2, T> adjoint(mat<2, 2, T>& m);

/**
 * @brief 把一个二阶矩阵设置为其逆矩阵
 * @param m 二阶矩阵对象。
 * @return 矩阵 m 的逆矩阵。
 */
template<typename T>
mat<2, 2, T> invert(mat<2, 2, T>& m);

/**
 * @brief 把一个二阶矩阵设置为其转置矩阵
 * @param m 二阶矩阵对象。
 * @return 矩阵 m 的转置矩阵。
 */
template<typename T>
mat<2, 2, T> transpose(mat<2, 2, T>& m);

} // namespace igm

#include "type_mat2x2.inl"

#endif // IGM_TYPE_MAT2x2_H