/**
 * @class    mat<4, 4, T>
 * @brief    mat<4, 4, T>类包含四阶矩阵的数据结构和函数定义。
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_TYPE_MAT4x4_H
#define IGM_TYPE_MAT4x4_H

#include "common.h"
#include "type_vec4.h"

namespace igm
{

template<typename T>
class mat<4, 4, T> {
public:
    /**
     * @brief 默认构造函数，将矩阵初始化为单位矩阵。
     */
    mat<4, 4, T>() : value{} { setToIdentity(); }

    /**
     * @brief 使用指定值初始化矩阵的构造函数。
     * @param x0, y0, z0, w0 第一个列向量的值。
     * @param x1, y1, z1, w1 第二个列向量的值。
     * @param x2, y2, z2, w2 第三个列向量的值。
     * @param x3, y3, z3, w3 第四个列向量的值。
     */
    mat<4, 4, T>(const T& x0, const T& y0, const T& z0, const T& w0,
                 const T& x1, const T& y1, const T& z1, const T& w1,
                 const T& x2, const T& y2, const T& z2, const T& w2,
                 const T& x3, const T& y3, const T& z3, const T& w3)
        : value{vec<4, T>(x0, y0, z0, w0), vec<4, T>(x1, y1, z1, w1),
                vec<4, T>(x2, y2, z2, w2), vec<4, T>(x3, y3, z3, w3)} {}

    /**
     * @brief 使用指定向量初始化矩阵的构造函数。
     * @param v0 第一个列向量。
     * @param v1 第二个列向量。
     * @param v2 第三个列向量。
     * @param v3 第四个列向量。
     */
    mat<4, 4, T>(const vec<4, T>& v0, const vec<4, T>& v1, const vec<4, T>& v2,
                 const vec<4, T>& v3)
        : value{v0, v1, v2, v3} {}

    /**
     * @brief 构造函数，将矩阵初始化为对角矩阵。
     * @param diagonal 设置对角线上的值。
     */
    explicit mat<4, 4, T>(const T& diagonal) : value{} {
        this->value[0] = vec<4, T>(diagonal, 0, 0, 0);
        this->value[1] = vec<4, T>(0, diagonal, 0, 0);
        this->value[2] = vec<4, T>(0, 0, diagonal, 0);
        this->value[3] = vec<4, T>(0, 0, 0, diagonal);
    }

    /**
     * @brief 拷贝构造函数。
     * @param other 要拷贝的矩阵。
     */
    mat<4, 4, T>(const mat<4, 4, T>& other)
        : value{other.value[0], other.value[1], other.value[2],
                other.value[3]} {}

    /**
     * @brief 三阶矩阵流插入运算符。
     * @param os 输出流。
     * @param m 要输出的三阶矩阵对象。
     * @return 输出流的引用。
     */
    friend std::ostream& operator<<(std::ostream& os, const mat<4, 4, T>& m) {
        os << "[";
        os << m[0][0] << ", " << m[1][0] << ", " << m[2][0] << ", " << m[3][0]
           << "\n";
        os << " " << m[0][1] << ", " << m[1][1] << ", " << m[2][1] << ", "
           << m[3][1] << "\n";
        os << " " << m[0][2] << ", " << m[1][2] << ", " << m[2][2] << ", "
           << m[3][2] << "\n";
        os << " " << m[0][3] << ", " << m[1][3] << ", " << m[2][3] << ", "
           << m[3][3] << "]\n";
        return os;
    }

    /**
     * @brief 赋值运算符。
     * @param other 要赋值的四阶矩阵对象。
     * @return 经过赋值后，本对象的引用。
     */
    mat<4, 4, T>& operator=(const mat<4, 4, T>& other) {
        if (this != &other) {
            this->value[0] = other[0];
            this->value[1] = other[1];
            this->value[2] = other[2];
            this->value[3] = other[3];
        }
        return *this;
    }

    /**
     * @brief  四阶矩阵的加法运算符。
     * @param  other  要加的四阶矩阵对象。
     * @return  一个新的四阶矩阵，其为两个四阶矩阵的加法结果。
     */
    mat<4, 4, T> operator+(const mat<4, 4, T>& other) const {
        mat<4, 4, T> result;
        result[0] = this->value[0] + other[0];
        result[1] = this->value[1] + other[1];
        result[2] = this->value[2] + other[2];
        result[3] = this->value[3] + other[3];
        return result;
    }

    /**
     * @brief  四阶矩阵的加法赋值运算符。
     * @param  other  要加上的四阶矩阵对象。
     * @return  经过加法后的本对象引用。
     */
    mat<4, 4, T>& operator+=(const mat<4, 4, T>& other) {
        this->value[0] += other[0];
        this->value[1] += other[1];
        this->value[2] += other[2];
        this->value[3] += other[3];
        return *this;
    }

    /**
     * @brief  四阶矩阵的减法运算符。
     * @param  other  要减去的四阶矩阵对象。
     * @return  一个新的四阶矩阵，其为两个四阶矩阵的减法结果。
     */
    mat<4, 4, T> operator-(const mat<4, 4, T>& other) const {
        mat<4, 4, T> result;
        result[0] = this->value[0] - other[0];
        result[1] = this->value[1] - other[1];
        result[2] = this->value[2] - other[2];
        result[3] = this->value[3] - other[3];
        return result;
    }

    /**
     * @brief  四阶矩阵的减法赋值运算符。
     * @param  other  要减去的四阶矩阵对象。
     * @return  经过减法后的本对象引用。
     */
    mat<4, 4, T>& operator-=(const mat<4, 4, T>& other) {
        this->value[0] -= other[0];
        this->value[1] -= other[1];
        this->value[2] -= other[2];
        this->value[3] -= other[3];
        return *this;
    }

    /**
     * @brief  四阶矩阵的常数乘法运算符。
     * @param  scalar  要乘的常数值。
     * @return  一个新的四阶矩阵，其为四阶矩阵乘以常数的结果。
     */
    mat<4, 4, T> operator*(T scalar) const {
        mat<4, 4, T> result;
        result[0] = this->value[0] * scalar;
        result[1] = this->value[1] * scalar;
        result[2] = this->value[2] * scalar;
        result[3] = this->value[3] * scalar;
        return result;
    }

    /**
     * @brief  四阶矩阵的常数乘法赋值运算符。
     * @param  scalar  要乘的常数值。
     * @return  经过常数乘法后的本对象引用。
     */
    mat<4, 4, T>& operator*=(const T& scalar) {
        this->value[0] *= scalar;
        this->value[1] *= scalar;
        this->value[2] *= scalar;
        this->value[3] *= scalar;
        return *this;
    }

    /**
     * @brief 四阶矩阵和四维向量的矩阵-向量乘法运算符。
     *
     * 该操作符将一个四阶的矩阵与一个四维向量相乘。结果向量的计算方式如下：
     *
     * result[0] = (m[0][0] * v[0]) + (m[1][0] * v[1]) + (m[2][0] * v[2]) + (m[3][0] * v[3])
     * result[1] = (m[0][1] * v[0]) + (m[1][1] * v[1]) + (m[2][1] * v[2]) + (m[3][1] * v[3])
     * result[2] = (m[0][2] * v[0]) + (m[1][2] * v[1]) + (m[2][2] * v[2]) + (m[3][2] * v[3])
     * result[3] = (m[0][3] * v[0]) + (m[1][3] * v[1]) + (m[2][3] * v[2]) + (m[3][3] * v[3])
     *
     * 其中，m 是矩阵，v 是向量。
     *
     * @param v 要相乘的三维向量对象。
     * @return  经过乘法运算后的新三维向量。
     */
    vec<4, T> operator*(const vec<4, T>& v) const {
        return vec<4, T>(value[0][0] * v[0] + value[1][0] * v[1] +
                                 value[2][0] * v[2] + value[3][0] * v[3],
                         value[0][1] * v[0] + value[1][1] * v[1] +
                                 value[2][1] * v[2] + value[3][1] * v[3],
                         value[0][2] * v[0] + value[1][2] * v[1] +
                                 value[2][2] * v[2] + value[3][2] * v[3],
                         value[0][3] * v[0] + value[1][3] * v[1] +
                                 value[2][3] * v[2] + value[3][3] * v[3]);
    }

    /**
     * @brief 四阶矩阵和四阶矩阵的矩阵乘法运算符。
     * @param other 要相乘的四阶矩阵对象。
     * @return 经过乘法运算后的新的四阶矩阵。
     */
    mat<4, 4, T> operator*(const mat<4, 4, T>& other) const {
        mat<4, 4, T> result;
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                result.value[c][r] = value[0][r] * other.value[c][0] +
                                     value[1][r] * other.value[c][1] +
                                     value[2][r] * other.value[c][2] +
                                     value[3][r] * other.value[c][3];
            }
        }
        return result;
    }

    /**
     * @brief 四阶矩阵和四阶矩阵的矩阵乘法赋值运算符。
     * @param other 要相乘的四阶矩阵对象。
     * @return 经过矩阵乘法后的本对象引用。
     */
    mat<4, 4, T>& operator*=(const mat<4, 4, T>& other) {
        mat<4, 4, T> result;
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                result.value[c][r] = value[0][r] * other.value[c][0] +
                                     value[1][r] * other.value[c][1] +
                                     value[2][r] * other.value[c][2] +
                                     value[3][r] * other.value[c][3];
            }
        }
        *this = result;
        return *this;
    }

    /**
     * @brief 下标运算符，用于访问矩阵的列向量。
     * @param index 列的索引（0-3）。
     * @return 返回列向量的引用。
     */
    vec<4, T>& operator[](int index) {
        assert(index >= 0 && index < 4);
        return value[index];
    }

    /**
     * @brief 下标运算符，用于访问矩阵的列向量（常数版本）。
     * @param index 列的索引（0-2）。
     * @return 返回列向量的常量引用。
     */
    const vec<4, T>& operator[](int index) const {
        assert(index >= 0 && index < 4);
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
     * @return 一个新的四阶矩阵，其为调用对象的伴随矩阵。
     */
    [[nodiscard]] mat<4, 4, T> adjoint() const;

    /**
     * @brief 计算矩阵的逆矩阵。
     * @return 一个新的四阶矩阵，其为调用对象的逆矩阵。
     */
    [[nodiscard]] mat<4, 4, T> invert() const;

    /**
     * @brief 计算矩阵的转置矩阵。
     * @return 一个新的四阶矩阵，其为调用对象的转置矩阵。
     */
    [[nodiscard]] mat<4, 4, T> transpose() const;

protected:
    vec<4, T> value[4]; /**< 用于保存矩阵的列向量的数组。 */
};

/**
 * @brief 获取四阶矩阵矩阵数据的常量指针。
 * @param m 四阶矩阵对象。
 * @return 矩阵 m 底层数据的常量指针。
 */
template<typename T>
T const* value_ptr(const mat<4, 4, T>& m) {
    return m.data();
}

/**
 * @brief 计算四阶矩阵的行列式。
 * @param m 四阶矩阵对象。
 * @return 矩阵 m 的行列式值。
 */
template<typename T>
T determinant(const mat<4, 4, T>& m);

/**
 * @brief 把一个四阶矩阵设置为其伴随矩阵。
 * @param m 四阶矩阵对象。
 * @return 矩阵 m 的伴随矩阵。
 */
template<typename T>
mat<4, 4, T> adjoint(mat<4, 4, T>& m);

/**
 * @brief 把一个四阶矩阵设置为其逆矩阵
 * @param m 四阶矩阵对象。
 * @return 矩阵 m 的逆矩阵。
 */
template<typename T>
mat<4, 4, T> invert(mat<4, 4, T>& m);

/**
 * @brief 把一个四阶矩阵设置为其转置矩阵
 * @param m 四阶矩阵对象。
 * @return 矩阵 m 的转置矩阵。
 */
template<typename T>
mat<4, 4, T> transpose(mat<4, 4, T>& m);

} // namespace igm

#include "type_mat4x4.inl"

#endif // IGM_TYPE_MAT4x4_H
