/**
 * @class    vec<3, T>
 * @brief    vec<3, T>类包含三维向量的数据结构和函数定义。
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#ifndef IGM_TYPE_VEC3_H
#define IGM_TYPE_VEC3_H

#include "common.h"
#include "type_vec2.h"

namespace igm
{

template<typename T>
class vec<3, T> {
public:
    /**
     * @brief 默认构造函数。将向量初始化为 (0, 0, 0)。
     */
    vec() : x(0.0f), y(0.0f), z(0.0f) {}

    /**
     * @brief 构造函数，将所有分量初始化为相同的值。
     * @param s 用于初始化所有分量的值。
     */
    explicit vec(T s) : x(s), y(s), z(s) {}

    /**
     * @brief 构造函数，使用指定的 x、y 和 z 值初始化向量。
     * @param x x 分量。
     * @param y y 分量。
     * @param z z 分量。
     */
    vec(T x, T y, T z) : x(x), y(y), z(z) {}

    /**
     * @brief 构造函数，使用二维向量和 z 值初始化向量。
     * @param xy 用于初始化 x 和 y 分量的二维向量。
     * @param z z 分量。
     */
    vec(const vec<2, T>& xy, T z) : x(xy.x), y(xy.y), z(z) {}

    /**
     * @brief 构造函数，使用 x 值和二维向量初始化向量。
     * @param x x 分量。
     * @param yz 用于初始化 y 和 z 分量的二维向量。
     */
    vec(T x, const vec<2, T>& yz) : x(x), y(yz.x), z(yz.y) {}

    /**
     * @brief 拷贝构造函数，从另一个三维向量复制数据。
     * @param other 要复制的三维向量。
     */
    vec(const vec<3, T>& other) : x(other.x), y(other.y), z(other.z) {}

    /**
     * @brief 构造函数，从另一个四维向量复制数据。
     * @param other 要复制的四维向量，只复制向量的x，y，z分量。
     */
    explicit vec(const vec<4, T>& other) : x(other.x), y(other.y), z(other.z) {}

    /**
     * @brief 流插入操作符，用于轻松输出向量。
     * @param os 输出流。
     * @param v 要输出的向量。
     * @return 输出流。
     */
    friend std::ostream& operator<<(std::ostream& os, const vec& v) {
        os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
        return os;
    }

    /**
     * @brief 赋值运算符，将一个三维向量赋值给当前向量。
     * @param other 要赋值的三维向量。
     * @return 经过赋值后，本对象的引用。
     */
    vec& operator=(const vec& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
            z = other.z;
        }
        return *this;
    }

    /**
     * @brief 等于运算符，用于比较两个三维向量是否相等。
     * @param other 要比较的向量。
     * @return 如果向量相等，则返回 true，否则返回 false。
     */
    bool operator==(const vec& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    /**
     *  @brief 不等于运算符，用于比较两个三维向量是否不相等。
     *  @param other 要比较的向量。
     *  @return 如果向量不相等，则返回 true，否则返回 false。
     */
    bool operator!=(const vec& other) const {
        return x != other.x || y != other.y || z != other.z;
    }

    /**
     *  @brief 加法运算符，用于将两个向量相加。
     *  @param other 要加的向量。
     *  @return 返回一个新的向量，表示两个向量的和。
     */
    vec operator+(const vec& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    /**
     *  @brief 加法赋值运算符，将另一个向量加到当前向量上，并更新当前向量。
     *  @param other 需要加的向量。
     *  @return 经过加法赋值后，本对象的引用。
     */
    vec& operator+=(const vec& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    /**
     *  @brief 减法运算符，从当前向量中减去另一个向量，并返回结果。
     *  @param other 需要减去的向量。
     *  @return 一个新向量，表示两个向量的差。
     */
    vec operator-(const vec& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    /**
     *  @brief 减法赋值运算符，从当前向量中减去另一个向量，并更新当前向量。
     *  @param other 需要减去的向量。
     *  @return 经过减法赋值后，本对象的引用。
     */
    vec& operator-=(const vec& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    /**
     *  @brief 标量乘法运算符，将本向量与标量相乘，返回一个新的向量。
     *  @param scalar 需要乘以的标量。
     *  @return 一个新的向量，表示本向量与标量的乘积。
     */
    vec operator*(const T& scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }

    /**
     *  @brief 标量乘法赋值运算符，将本向量与标量相乘，并更新当前向量。
     *  @param scalar 需要乘以的标量。
     *  @return 经过乘法赋值后，本对象的引用。
     */
    vec& operator*=(const T& scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    /**
     * @brief 标量除法运算符，将本向量除以标量并返回新向量。
     * @param scalar 需要除以的标量。
     * @return 一个新的向量，它是向量与标量的商。
     */
    vec operator/(const T& scalar) const {
        return {x / scalar, y / scalar, z / scalar};
    }

    /**
     * @brief 标量除法赋值运算符，将本向量除以标量并更新当前向量。
     * @param scalar 需要除以的标量。
     * @return 经过除法赋值后，本对象的引用。
     */
    vec& operator/=(const T& scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    /**
     * @brief 分量乘法运算符，返回两个向量的分量乘积。
     * @param other 另一个要乘的向量。
     * @return 一个新的向量，它是两个向量各分量的乘积。
     */
    vec operator*(const vec& other) const {
        return {x * other.x, y * other.y, z * other.z};
    }

    /**
     * @brief 分量乘法赋值运算符，更新当前向量为两个向量的分量乘积。
     * @param other 另一个要乘的向量。
     * @return 经过乘法赋值后，本对象的引用。
     */
    vec& operator*=(const vec& other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    /**
     * @brief 取反运算符，返回原始向量的相反向量。
     * @return 一个新的向量，表示原始向量的取反。
     */
    vec operator-() const { return {-x, -y, -z}; }

    /**
     * @brief 下标运算符，用于非 const 访问。
     * @param index 分量的索引（0 表示 x，1 表示 y，2 表示 z）。
     * @return 分量的引用。
     */
    T& operator[](int index) {
        assert(index >= 0 && index < 3);
        switch (index) {
            default:
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
        }
    }

    /**
     * @brief 下标运算符，用于 const 访问。
     * @param index 分量的索引（0 表示 x，1 表示 y，2 表示 z）。
     * @return 分量的常量引用。
     */
    const T& operator[](int index) const {
        assert(index >= 0 && index < 3);
        switch (index) {
            default:
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
        }
    }

    /**
     * @brief 返回一个包含x和y分量的二维向量。
     * @return 一个新的二维向量，包含原始向量的x和y分量。
     */
    vec<2, T> xy() const { return vec<2, T>{x, y}; }

    /**
     * @brief 返回一个包含y和x分量的二维向量。
     * @return 一个新的二维向量，包含原始向量的y和x分量。
     */
    vec<2, T> yx() const { return vec<2, T>{y, x}; }

    /**
     * @brief 返回一个包含x和z分量的二维向量。
     * @return 一个新的二维向量，包含原始向量的x和z分量。
     */
    vec<2, T> xz() const { return vec<2, T>{x, z}; }

    /**
     * @brief 返回一个包含z和x分量的二维向量。
     * @return 一个新的二维向量，包含原始向量的z和x分量。
     */
    vec<2, T> zx() const { return vec<2, T>{z, x}; }

    /**
     * @brief 返回一个包含y和z分量的二维向量。
     * @return 一个新的二维向量，包含原始向量的y和z分量。
     */
    vec<2, T> yz() const { return vec<2, T>{y, z}; }

    /**
     * @brief 返回一个包含z和y分量的二维向量。
     * @return 一个新的二维向量，包含原始向量的z和y分量。
     */
    vec<2, T> zy() const { return vec<2, T>{z, y}; }

    /**
     * @brief 返回指向底层数据的指针。
     * @return 指向向量第一个元素的指针。
     */
    T const* data() const { return &x; }

    /**
     * @brief 计算向量的长度（大小）。
     * @return 向量的长度。
     */
    [[nodiscard]] T length() const { return std::sqrt(x * x + y * y + z * z); }

    /**
     * @brief 对本向量进行归一化处理。
     * @return 归一化后的本向量的引用。
     */
    vec normalize() {
        T length = this->length();
        x /= length;
        y /= length;
        z /= length;
        return *this;
    }

    /**
     * @brief 返回本向量的归一化副本。
     * @return 一个新的向量，表示原始向量的归一化版本。
     */
    [[nodiscard]] vec normalized() const {
        T length = this->length();
        return {x / length, y / length, z / length};
    }

    /**
     * @brief 计算与另一个向量的点积。
     * @param other 要与之计算点积的向量。
     * @return 点积的值。
     */
    [[nodiscard]] T dot(const vec& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    /**
     * @brief 计算与另一个向量的叉积。
     * @param other 要计算叉积的向量。
     * @return 两个向量的叉积结果。
     */
    [[nodiscard]] vec cross(const vec& other) {
        T newX = y * other.z - z * other.y;
        T newY = z * other.x - x * other.z;
        T newZ = x * other.y - y * other.x;
        return {newX, newY, newZ};
    }

public:
    // 联合体 (Union) 允许使用不同的成员名称访问相同的内存位置。
    union {
        T x, r, s; // 表示向量的第一个分量。
    };
    union {
        T y, g, t; // 表示向量的第二个分量。
    };
    union {
        T z, b, p; // 表示向量的第三个分量。
    };
};

/**
 * @brief 获取三维向量的数据的常量指针。
 * @param v 三维向量对象。
 * @return 向量 v 底层数据的常量指针。
 */
template<typename T>
T const* value_ptr(const vec<3, T>& v) {
    return v.data();
}

/**
 * @brief 返回两个向量按分量比较后的最小值。
 * @param v1 第一个向量。
 * @param v2 第二个向量。
 * @return 一个包含输入向量按分量比较后的最小值的新向量。
 */
template<typename T>
vec<3, T> min(const vec<3, T>& v1, const vec<3, T>& v2);

/**
 * @brief 返回两个向量按分量比较后的最大值。
 * @param v1 第一个向量。
 * @param v2 第二个向量。
 * @return 一个包含输入向量按分量比较后的最大值的新向量。
 */
template<typename T>
vec<3, T> max(const vec<3, T>& v1, const vec<3, T>& v2);

/**
 * @brief 计算两个向量的点积。
 * @param v1 第一个向量。
 * @param v2 第二个向量。
 * @return 点积结果。
 */
template<typename T>
T dot(const vec<3, T>& v1, const vec<3, T>& v2);

/**
 * @brief 计算两个向量的叉积。
 * @param v1 第一个向量。
 * @param v2 第二个向量。
 * @return 一个新向量，表示两个向量的叉积结果。
 */
template<typename T>
vec<3, T> cross(const vec<3, T>& v1, const vec<3, T>& v2);

} // namespace igm

#include "type_vec3.inl"

//namespace std
//{
//
///**
//* Hash function specialization for the vec class, allowing it to be used in
//* unordered containers.
//*/
//template<typename T>
//struct hash<igm::vec<3, T>> {
//    /**
//  * Computes the hash value for a vec.
//  * @param v The vector to hash.
//  * @return The hash value.
//  */
//    size_t operator()(const igm::vec<3, T>& v) const {
//        size_t h1 = std::hash<T>()(v.x);
//        size_t h2 = std::hash<T>()(v.y);
//        size_t h3 = std::hash<T>()(v.z);
//        return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
//    }
//};
//
//} // namespace std

#endif // IGM_TYPE_VEC3_H