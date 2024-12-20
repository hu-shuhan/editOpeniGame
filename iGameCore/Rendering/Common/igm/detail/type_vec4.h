/**
* @class    vec<4, T>
* @brief    vec<4, T>类包含四维向量的数据结构和函数定义。
* @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
*/

#ifndef IGM_TYPE_VEC4_H
#define IGM_TYPE_VEC4_H

#include "common.h"
#include "type_vec2.h"
#include "type_vec3.h"

namespace igm
{

template<typename T>
class vec<4, T> {
public:
    /**
     * @brief 默认构造函数。将向量初始化为 (0, 0, 0, 0)。
     */
    vec() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}

    /**
     * @brief 构造函数，将所有分量初始化为相同的值。
     * @param s 用于初始化所有分量的值。
     */
    explicit vec(T s) : x(s), y(s), z(s), w(s) {}

    /**
     * @brief 构造函数，用于初始化向量的 x、y、z 和 w 分量。
     * @param x x 分量。
     * @param y y 分量。
     * @param z z 分量。
     * @param w w 分量。
     */
    vec(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    /**
     * @brief 构造函数，通过一个二维向量和 z、w 值初始化向量。
     * @param xy 用于初始化 x 和 y 分量的二维向量。
     * @param z z 分量。
     * @param w w 分量。
     */
    vec(const vec<2, T>& xy, T z, T w) : x(xy.x), y(xy.y), z(z), w(w) {}

    /**
     * @brief 构造函数，通过 x 值、一个二维向量和 w 值初始化向量。
     * @param x x 分量。
     * @param yz 用于初始化 y 和 z 分量的二维向量。
     * @param w w 分量。
     */
    vec(T x, const vec<2, T>& yz, T w) : x(x), y(yz.x), z(yz.y), w(w) {}

    /**
     * @brief 构造函数，通过 x、y 值和一个二维向量初始化向量。
     * @param x x 分量。
     * @param y y 分量。
     * @param zw 用于初始化 z 和 w 分量的二维向量。
     */
    vec(T x, T y, const vec<2, T>& zw) : x(x), y(y), z(zw.x), w(zw.y) {}

    /**
     * @brief 构造函数，用于通过一个三维向量和 w 值初始化向量。
     * @param xyz 用于初始化 x, y 和 z 分量的三维向量。
     * @param w w 分量。
     */
    vec(const vec<3, T>& xyz, T w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    /**
     * @brief 构造函数，用于通过 x 值和一个三维向量初始化向量。
     * @param x x 分量。
     * @param yzw 用于初始化 y, z 和 w 分量的三维向量。
     */
    vec(T x, const vec<3, T>& yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z) {}

    /**
     * @brief 拷贝构造函数，从另一个四维向量复制数据。
     * @param other 要复制的四维向量。
     */
    vec(const vec& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}

    /**
     * @brief 流插入运算符，用于方便地输出向量。
     * @param os 输出流。
     * @param v 要输出的向量。
     * @return 输出流。
     */
    friend std::ostream& operator<<(std::ostream& os, const vec& v) {
        os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3]
           << ")";
        return os;
    }

    /**
     * @brief 赋值运算符，将一个四维向量赋值给当前向量。
     * @param other 要赋值的四维向量。
     * @return 经过赋值后，本对象的引用。
     */
    vec& operator=(const vec& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
            z = other.z;
            w = other.w;
        }
        return *this;
    }

    /**
     * @brief 等于运算符，用于比较两个四维向量是否相等。
     * @param other 要比较的向量。
     * @return 如果向量相等，则返回 true，否则返回 false。
     */
    bool operator==(const vec& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    /**
     *  @brief 不等于运算符，用于比较两个四维向量是否不相等。
     *  @param other 要比较的向量。
     *  @return 如果向量不相等，则返回 true，否则返回 false。
     */
    bool operator!=(const vec& other) const {
        return x != other.x || y != other.y || z != other.z || w != other.w;
    }

    /**
     *  @brief 加法运算符，用于将两个向量相加。
     *  @param other 要加的向量。
     *  @return 返回一个新的向量，表示两个向量的和。
     */
    vec operator+(const vec& other) const {
        return {x + other.x, y + other.y, z + other.z, w + other.w};
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
        w += other.w;
        return *this;
    }

    /**
     *  @brief 减法运算符，从当前向量中减去另一个向量，并返回结果。
     *  @param other 需要减去的向量。
     *  @return 一个新向量，表示两个向量的差。
     */
    vec operator-(const vec& other) const {
        return {x - other.x, y - other.y, z - other.z, w - other.w};
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
        w -= other.w;
        return *this;
    }

    /**
     *  @brief 标量乘法运算符，将本向量与标量相乘，返回一个新的向量。
     *  @param scalar 需要乘以的标量。
     *  @return 一个新的向量，表示本向量与标量的乘积。
     */
    vec operator*(const T& scalar) const {
        return {x * scalar, y * scalar, z * scalar, w * scalar};
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
        w *= scalar;
        return *this;
    }

    /**
     * @brief 标量除法运算符，将本向量除以标量并返回新向量。
     * @param scalar 需要除以的标量。
     * @return 一个新的向量，它是向量与标量的商。
     */
    vec operator/(const T& scalar) const {
        return {x / scalar, y / scalar, z / scalar, w / scalar};
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
        w /= scalar;
        return *this;
    }

    /**
     * @brief 分量乘法运算符，返回两个向量的分量乘积。
     * @param other 另一个要乘的向量。
     * @return 一个新的向量，它是两个向量各分量的乘积。
     */
    vec operator*(const vec& other) const {
        return {x * other.x, y * other.y, z * other.z, w * other.w};
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
        w *= other.w;
        return *this;
    }

    /**
     * @brief 取反运算符，返回原始向量的相反向量。
     * @return 一个新的向量，表示原始向量的取反。
     */
    vec operator-() const { return {-x, -y, -z, -w}; }

    /**
     * @brief 下标运算符，用于非 const 访问。
     * @param index 分量的索引（0 表示 x，1 表示 y，2 表示 z，3 表示 w）。
     * @return 分量的引用。
     */
    T& operator[](int index) {
        assert(index >= 0 && index < 4);
        switch (index) {
            default:
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            case 3:
                return w;
        }
    }

    /**
     * @brief 下标运算符，用于 const 访问。
     * @param index 分量的索引（0 表示 x，1 表示 y，2 表示 z，3 表示 w）。
     * @return 分量的常量引用。
     */
    const T& operator[](int index) const {
        assert(index >= 0 && index < 4);
        switch (index) {
            default:
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            case 3:
                return w;
        }
    }

    /**
     * @brief 返回由 x 和 y 分量组成的二维向量。
     * @return 一个新的二维向量，包含原始向量的 x 和 y 分量。
     */
    vec<2, T> xy() const { return vec<2, T>{x, y}; }

    /**
     * @brief 返回由 x 和 z 分量组成的二维量。
     * @return 一个新的二维向量，包含原始向量的 x 和 z 分量。
     */
    vec<2, T> xz() const { return vec<2, T>{x, z}; }

    /**
     * @brief 返回由 x 和 w 分量组成的二维向量。
     * @return 一个新的二维向量，包含原始向量的 x 和 w 分量。
     */
    vec<2, T> xw() const { return vec<2, T>{x, w}; }

    /**
     * @brief 返回由 y 和 x 分量组成的三维向量。
     * @return 一个新的二维向量，包含原始向量的 y 和 x 分量。
     */
    vec<2, T> yx() const { return vec<2, T>{y, x}; }

    /**
     * @brief 返回由 y 和 z 分量组成的三维向量。
     * @return 一个新的二维向量，包含原始向量的 y 和 z 分量。
     */
    vec<2, T> yz() const { return vec<2, T>{y, z}; }

    /**
     * @brief 返回由 y 和 w 分量组成的三维向量。
     * @return 一个新的二维向量，包含原始向量的 y 和 w 分量。
     */
    vec<2, T> yw() const { return vec<2, T>{y, w}; }

    /**
     * @brief 返回由 z 和 x 分量组成的二维向量。
     * @return 一个新的二维向量，包含原始向量的 z 和 x 分量。
     */
    vec<2, T> zx() const { return vec<2, T>{z, x}; }

    /**
     * @brief 返回由 z 和 y 分量组成的二维向量。
     * @return 一个新的二维向量，包含原始向量的 z 和 y 分量。
     */
    vec<2, T> zy() const { return vec<2, T>{z, y}; }

    /**
     * @brief 返回由 z 和 w 分量组成的二维向量。
     * @return 一个新的二维向量，包含原始向量的 z 和 w 分量。
     */
    vec<2, T> zw() const { return vec<2, T>{z, w}; }

    /**
     * @brief 返回由 w 和 x 分量组成的二维向量。
     * @return 一个新的二维向量，包含原始向量的 w 和 x 分量。
     */
    vec<2, T> wx() const { return vec<2, T>{w, x}; }

    /**
     * @brief 返回由 w 和 y 分量组成的二维向量。
     * @return 一个新的二维向量，包含原始向量的 w 和 y 分量。
     */
    vec<2, T> wy() const { return vec<2, T>{w, y}; }

    /**
     * @brief 返回由 w 和 z 分量组成的二维向量。
     * @return 一个新的二维向量，包含原始向量的 w 和 z 分量。
     */
    vec<2, T> wz() const { return vec<2, T>{w, z}; }

    /**
     * @brief 返回由 x、y 和 z 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 x、y 和 z 分量。
     */
    vec<3, T> xyz() const { return vec<3, T>{x, y, z}; }

    /**
     * @brief 返回由 x、y 和 w 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 x、y 和 w 分量。
     */
    vec<3, T> xyw() const { return vec<3, T>{x, y, w}; }

    /**
     * @brief 返回由 x、z 和 y 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 x、z 和 y 分量。
     */
    vec<3, T> xzy() const { return vec<3, T>{x, z, y}; }

    /**
     * @brief 返回由 x、z 和 w 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 x、z 和 w 分量。
     */
    vec<3, T> xzw() const { return vec<3, T>{x, z, w}; }

    /**
     * @brief 返回由 x、w 和 y 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 x、w 和 y 分量。
     */
    vec<3, T> xwy() const { return vec<3, T>{x, w, y}; }

    /**
     * @brief 返回由 x、w 和 z 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 x、w 和 z 分量。
     */
    vec<3, T> xwz() const { return vec<3, T>{x, w, z}; }

    /**
     * @brief 返回由 y、x 和 z 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 y、x 和 z 分量。
     */
    vec<3, T> yxz() const { return vec<3, T>{y, x, z}; }

    /**
     * @brief 返回由 y、x 和 w 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 y、x 和 w 分量。
     */
    vec<3, T> yxw() const { return vec<3, T>{y, x, w}; }

    /**
     * @brief 返回由 y、z 和 x 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 y、z 和 x 分量。
     */
    vec<3, T> yzx() const { return vec<3, T>{y, z, x}; }

    /**
     * @brief 返回由 y、z 和 w 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 y、z 和 w 分量。
     */
    vec<3, T> yzw() const { return vec<3, T>{y, z, w}; }

    /**
     * @brief 返回由 y、w 和 x 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 y、w 和 x 分量。
     */
    vec<3, T> ywx() const { return vec<3, T>{y, w, x}; }

    /**
     * @brief 返回由 y、w 和 z 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 y、w 和 z 分量。
     */
    vec<3, T> ywz() const { return vec<3, T>{y, w, z}; }

    /**
     * @brief 返回由 z、x 和 y 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 z、x 和 y 分量。
     */
    vec<3, T> zxy() const { return vec<3, T>{z, x, y}; }

    /**
     * @brief 返回由 z、x 和 w 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 z、x 和 w 分量。
     */
    vec<3, T> zxw() const { return vec<3, T>{z, x, w}; }

    /**
     * @brief 返回由 z、y 和 x 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 z、y 和 x 分量。
     */
    vec<3, T> zyx() const { return vec<3, T>{z, y, x}; }

    /**
     * @brief 返回由 z、y 和 w 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 z、y 和 w 分量。
     */
    vec<3, T> zyw() const { return vec<3, T>{z, y, w}; }

    /**
     * @brief 返回由 z、w 和 x 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 z、w 和 x 分量。
     */
    vec<3, T> zwx() const { return vec<3, T>{z, w, x}; }

    /**
     * @brief 返回由 z、w 和 y 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 z、w 和 y 分量。
     */
    vec<3, T> zwy() const { return vec<3, T>{z, w, y}; }

    /**
     * @brief 返回由 w、x 和 y 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 w、x 和 y 分量。
     */
    vec<3, T> wxy() const { return vec<3, T>{w, x, y}; }

    /**
     * @brief 返回由 w、x 和 z 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 w、x 和 z 分量。
     */
    vec<3, T> wxz() const { return vec<3, T>{w, x, z}; }

    /**
     * @brief 返回由 w、y 和 x 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 w、y 和 x 分量。
     */
    vec<3, T> wyx() const { return vec<3, T>{w, y, x}; }

    /**
     * @brief 返回由 w、y 和 z 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 w、y 和 z 分量。
     */
    vec<3, T> wyz() const { return vec<3, T>{w, y, z}; }

    /**
     * @brief 返回由 w、z 和 x 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 w、z 和 x 分量。
     */
    vec<3, T> wzx() const { return vec<3, T>{w, z, x}; }

    /**
     * @brief 返回由 w、z 和 y 分量组成的三维向量。
     * @return 一个新的三维向量，包含原始向量的 w、z 和 y 分量。
     */
    vec<3, T> wzy() const { return vec<3, T>{w, z, y}; }

    /**
     * @brief 返回指向底层数据的指针。
     * @return 指向向量第一个元素的指针。
     */
    T const* data() const { return &x; }

    /**
     * @brief 计算向量的长度（大小）。
     * @return 向量的长度。
     */
    [[nodiscard]] T length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }

    /**
     * @brief 对本向量进行归一化处理。
     * @return 归一化后的本向量的引用。
     */
    vec normalize() {
        T length = this->length();
        x /= length;
        y /= length;
        z /= length;
        w /= length;
        return {this->x, this->y, this->z, this->w};
    }

    /**
     * @brief 返回本向量的归一化副本。
     * @return 一个新的向量，表示原始向量的归一化版本。
     */
    [[nodiscard]] vec normalized() const {
        T length = this->length();
        return {x / length, y / length, z / length, w / length};
    }

    /**
     * @brief 计算与另一个向量的点积。
     * @param other 要计算点积的向量。
     * @return 点积的结果。
     */
    [[nodiscard]] T dot(const vec& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
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
    union {
        T w, a, q; // 表示向量的第四个分量。
    };
};

/**
 * @brief 获取四维向量的数据的常量指针。
 * @param v 四维向量对象。
 * @return 向量 v 底层数据的常量指针。
 */
template<typename T>
T const* value_ptr(const vec<4, T>& v) {
    return v.data();
}

/**
 * @brief 返回两个向量按分量比较后的最小值。
 * @param v1 第一个向量。
 * @param v2 第二个向量。
 * @return 一个包含输入向量按分量比较后的最小值的新向量。
 */
template<typename T>
vec<4, T> min(const vec<4, T>& v1, const vec<4, T>& v2);

/**
 * @brief 返回两个向量按分量比较后的最大值。
 * @param v1 第一个向量。
 * @param v2 第二个向量。
 * @return 一个包含输入向量按分量比较后的最大值的新向量。
 */
template<typename T>
vec<4, T> max(const vec<4, T>& v1, const vec<4, T>& v2);

/**
 * @brief 计算两个向量的点积。
 * @param v1 第一个向量。
 * @param v2 第二个向量。
 * @return 点积的值。
 */
template<typename T>
T dot(const vec<4, T>& v1, const vec<4, T>& v2);

} // namespace igm

#include "type_vec4.inl"

#endif // IGM_TYPE_VEC4_H