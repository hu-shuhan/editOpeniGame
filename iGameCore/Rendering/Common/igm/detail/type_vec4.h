/**
* @file
* @brief    Related basic data structures for four-dimensional vectors.
* @author   Sumzeek
* @date     4/12/2024
* @version  v1.0.0
* @par      Copyright(c): Hangzhou Dianzi University iGame Laboratory
* @par      History:
*	       v1.0.0: Sumzeek, 4/12/2024, first create this file\n
*/

#ifndef IGM_TYPE_VEC4_H
#define IGM_TYPE_VEC4_H

#include "common.h"
#include "type_vec2.h"
#include "type_vec3.h"

namespace igm
{

/**
* A 4D vector class that supports various vector operations.
*/
template<typename T>
class vec<4, T> {
public:
    // Union allows the same memory location to be accessed using different member
    // names.
    union {
        T x, r, s; // Represents the first component of the vector.
    };
    union {
        T y, g, t; // Represents the second component of the vector.
    };
    union {
        T z, b, p; // Represents the third component of the vector.
    };
    union {
        T w, a, q; // Represents the fourth component of the vector.
    };

    /**
    * Default constructor. Initializes the vector to (0, 0, 0, 0).
    */
    vec() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}

    /**
    * Copy constructor.
    * @param other The vector to copy from.
    */
    vec(const vec& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}

    /**
    * Constructor that initializes all components to the same value.
    * @param s The value to initialize all components with.
    */
    explicit vec(T s) : x(s), y(s), z(s), w(s) {}

    /**
    * Constructor that initializes the vector with specified x, y, z, and w
    * values.
    * @param x The x component.
    * @param y The y component.
    * @param z The z component.
    * @param w The w component.
    */
    vec(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    /**
    * Constructor that initializes the vector with a vec2 and z, w values.
    * @param xy The vec2 to initialize the x and y components.
    * @param z The z component.
    * @param w The w component.
    */
    vec(const vec<2, T>& xy, T z, T w) : x(xy.x), y(xy.y), z(z), w(w) {}

    /**
    * Constructor that initializes the vector with x value, a vec2, and w value.
    * @param x The x component.
    * @param yz The vec2 to initialize the y and z components.
    * @param w The w component.
    */
    vec(T x, const vec<2, T>& yz, T w) : x(x), y(yz.x), z(yz.y), w(w) {}

    /**
    * Constructor that initializes the vector with x, y values, and a vec2.
    * @param x The x component.
    * @param y The y component.
    * @param zw The vec2 to initialize the z and w components.
    */
    vec(T x, T y, const vec<2, T>& zw) : x(x), y(y), z(zw.x), w(zw.y) {}

    /**
    * Constructor that initializes the vector with a vec3 and w value.
    * @param xyz The vec3 to initialize the x, y, and z components.
    * @param w The w component.
    */
    vec(const vec<3, T>& xyz, T w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    /**
    * Constructor that initializes the vector with x value and a vec3.
    * @param x The x component.
    * @param yzw The vec3 to initialize the y, z, and w components.
    */
    vec(T x, const vec<3, T>& yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z) {}

    /**
    * Stream insertion operator for easy output of the vector.
    * @param os The output stream.
    * @param v The vector to output.
    * @return The output stream.
    */
    friend std::ostream& operator<<(std::ostream& os, const vec& v) {
        os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3]
           << ")";
        return os;
    }

    /**
    * Assignment operator.
    * @param other The vector to assign from.
    * @return The assigned vector.
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
    * Equality operator.
    * @param other The vector to compare with.
    * @return True if the vectors are equal, false otherwise.
    */
    bool operator==(const vec& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    /**
    * Inequality operator.
    * @param other The vector to compare with.
    * @return True if the vectors are not equal, false otherwise.
    */
    bool operator!=(const vec& other) const {
        return x != other.x || y != other.y || z != other.z || w != other.w;
    }

    /**
    * Addition operator.
    * @param other The vector to add.
    * @return A new vector that is the sum of the two vectors.
    */
    vec operator+(const vec& other) const {
        return {x + other.x, y + other.y, z + other.z, w + other.w};
    }

    /**
  * Addition assignment operator.
  * @param other The vector to add.
  * @return The updated vector.
  */
    vec& operator+=(const vec& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    /**
    * Subtraction operator.
    * @param other The vector to subtract.
    * @return A new vector that is the difference of the two vectors.
    */
    vec operator-(const vec& other) const {
        return {x - other.x, y - other.y, z - other.z, w - other.w};
    }

    /**
    * Subtraction assignment operator.
    * @param other The vector to subtract.
    * @return The updated vector.
    */
    vec& operator-=(const vec& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    /**
    * Scalar multiplication operator.
    * @param scalar The scalar to multiply by.
    * @return A new vector that is the product of the vector and the scalar.
    */
    vec operator*(const T& scalar) const {
        return {x * scalar, y * scalar, z * scalar, w * scalar};
    }

    /**
    * Scalar multiplication assignment operator.
    * @param scalar The scalar to multiply by.
    * @return The updated vector.
    */
    vec& operator*=(const T& scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    /**
    * Scalar division operator.
    * @param scalar The scalar to divide by.
    * @return A new vector that is the quotient of the vector and the scalar.
    */
    vec operator/(const T& scalar) const {
        return {x / scalar, y / scalar, z / scalar, w / scalar};
    }

    /**
    * Scalar division assignment operator.
    * @param scalar The scalar to divide by.
    * @return The updated vector.
    */
    vec& operator/=(const T& scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    /**
    * Component-wise multiplication operator.
    * @param other The vector to multiply by.
    * @return A new vector that is the component-wise product of the two vectors.
    */
    vec operator*(const vec& other) const {
        return {x * other.x, y * other.y, z * other.z, w * other.w};
    }

    /**
    * Component-wise multiplication assignment operator.
    * @param other The vector to multiply by.
    * @return The updated vector.
    */
    vec& operator*=(const vec& other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }

    /**
    * Negation operator.
    * @return A new vector that is the negation of the original vector.
    */
    vec operator-() const { return {-x, -y, -z, -w}; }

    /**
    * Subscript operator for non-const access.
    * @param index The index of the component (0 for x, 1 for y, 2 for z, 3 for
    * w).
    * @return A reference to the component.
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
    * Subscript operator for const access.
    * @param index The index of the component (0 for x, 1 for y, 2 for z, 3 for
    * w).
    * @return A const reference to the component.
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

public:
    /**
    * Returns a 2-dimensional vector consisting of the x and y components.
    * @return A vec<2, T> containing the x and y components of the original vector.
    */
    vec<2, T> xy() const { return vec<2, T>{x, y}; }

    /**
    * Returns a 2-dimensional vector consisting of the x and z components.
    * @return A vec<2, T> containing the x and z components of the original vector.
    */
    vec<2, T> xz() const { return vec<2, T>{x, z}; }

    /**
    * Returns a 2-dimensional vector consisting of the x and w components.
    * @return A vec<2, T> containing the x and w components of the original vector.
    */
    vec<2, T> xw() const { return vec<2, T>{x, w}; }

    /**
    * Returns a 2-dimensional vector consisting of the y and x components.
    * @return A vec<2, T> containing the y and x components of the original vector.
    */
    vec<2, T> yx() const { return vec<2, T>{y, x}; }

    /**
    * Returns a 2-dimensional vector consisting of the y and z components.
    * @return A vec<2, T> containing the y and z components of the original vector.
    */
    vec<2, T> yz() const { return vec<2, T>{y, z}; }

    /**
    * Returns a 2-dimensional vector consisting of the y and w components.
    * @return A vec<2, T> containing the y and w components of the original vector.
    */
    vec<2, T> yw() const { return vec<2, T>{y, w}; }

    /**
    * Returns a 2-dimensional vector consisting of the z and x components.
    * @return A vec<2, T> containing the z and x components of the original vector.
    */
    vec<2, T> zx() const { return vec<2, T>{z, x}; }

    /**
    * Returns a 2-dimensional vector consisting of the z and y components.
    * @return A vec<2, T> containing the z and y components of the original vector.
    */
    vec<2, T> zy() const { return vec<2, T>{z, y}; }

    /**
    * Returns a 2-dimensional vector consisting of the z and w components.
    * @return A vec<2, T> containing the z and w components of the original vector.
    */
    vec<2, T> zw() const { return vec<2, T>{z, w}; }

    /**
    * Returns a 2-dimensional vector consisting of the w and x components.
    * @return A vec<2, T> containing the w and x components of the original vector.
    */
    vec<2, T> wx() const { return vec<2, T>{w, x}; }

    /**
    * Returns a 2-dimensional vector consisting of the w and y components.
    * @return A vec<2, T> containing the w and y components of the original vector.
    */
    vec<2, T> wy() const { return vec<2, T>{w, y}; }

    /**
    * Returns a 2-dimensional vector consisting of the w and z components.
    * @return A vec<2, T> containing the w and z components of the original vector.
    */
    vec<2, T> wz() const { return vec<2, T>{w, z}; }

    /**
    * Returns a 3-dimensional vector consisting of the x, y, and z components.
    * @return A vec<3, T> containing the x, y, and z components of the original vector.
    */
    vec<3, T> xyz() const { return vec<3, T>{x, y, z}; }

    /**
    * Returns a 3-dimensional vector consisting of the x, y, and w components.
    * @return A vec<3, T> containing the x, y, and w components of the original vector.
    */
    vec<3, T> xyw() const { return vec<3, T>{x, y, w}; }

    /**
    * Returns a 3-dimensional vector consisting of the x, z, and y components.
    * @return A vec<3, T> containing the x, z, and y components of the original vector.
    */
    vec<3, T> xzy() const { return vec<3, T>{x, z, y}; }

    /**
    * Returns a 3-dimensional vector consisting of the x, z, and w components.
    * @return A vec<3, T> containing the x, z, and w components of the original vector.
    */
    vec<3, T> xzw() const { return vec<3, T>{x, z, w}; }

    /**
    * Returns a 3-dimensional vector consisting of the x, w, and y components.
    * @return A vec<3, T> containing the x, w, and y components of the original vector.
    */
    vec<3, T> xwy() const { return vec<3, T>{x, w, y}; }

    /**
    * Returns a 3-dimensional vector consisting of the x, w, and z components.
    * @return A vec<3, T> containing the x, w, and z components of the original vector.
    */
    vec<3, T> xwz() const { return vec<3, T>{x, w, z}; }

    /**
    * Returns a 3-dimensional vector consisting of the y, x, and z components.
    * @return A vec<3, T> containing the y, x, and z components of the original vector.
    */
    vec<3, T> yxz() const { return vec<3, T>{y, x, z}; }

    /**
    * Returns a 3-dimensional vector consisting of the y, x, and w components.
    * @return A vec<3, T> containing the y, x, and w components of the original vector.
    */
    vec<3, T> yxw() const { return vec<3, T>{y, x, w}; }

    /**
    * Returns a 3-dimensional vector consisting of the y, z, and x components.
    * @return A vec<3, T> containing the y, z, and x components of the original vector.
    */
    vec<3, T> yzx() const { return vec<3, T>{y, z, x}; }

    /**
    * Returns a 3-dimensional vector consisting of the y, z, and w components.
    * @return A vec<3, T> containing the y, z, and w components of the original vector.
    */
    vec<3, T> yzw() const { return vec<3, T>{y, z, w}; }

    /**
    * Returns a 3-dimensional vector consisting of the y, w, and x components.
    * @return A vec<3, T> containing the y, w, and x components of the original vector.
    */
    vec<3, T> ywx() const { return vec<3, T>{y, w, x}; }

    /**
    * Returns a 3-dimensional vector consisting of the y, w, and z components.
    * @return A vec<3, T> containing the y, w, and z components of the original vector.
    */
    vec<3, T> ywz() const { return vec<3, T>{y, w, z}; }

    /**
    * Returns a 3-dimensional vector consisting of the z, x, and y components.
    * @return A vec<3, T> containing the z, x, and y components of the original vector.
    */
    vec<3, T> zxy() const { return vec<3, T>{z, x, y}; }

    /**
    * Returns a 3-dimensional vector consisting of the z, x, and w components.
    * @return A vec<3, T> containing the z, x, and w components of the original vector.
    */
    vec<3, T> zxw() const { return vec<3, T>{z, x, w}; }

    /**
    * Returns a 3-dimensional vector consisting of the z, y, and x components.
    * @return A vec<3, T> containing the z, y, and x components of the original vector.
    */
    vec<3, T> zyx() const { return vec<3, T>{z, y, x}; }

    /**
    * Returns a 3-dimensional vector consisting of the z, y, and w components.
    * @return A vec<3, T> containing the z, y, and w components of the original vector.
    */
    vec<3, T> zyw() const { return vec<3, T>{z, y, w}; }

    /**
    * Returns a 3-dimensional vector consisting of the z, w, and x components.
    * @return A vec<3, T> containing the z, w, and x components of the original vector.
    */
    vec<3, T> zwx() const { return vec<3, T>{z, w, x}; }

    /**
    * Returns a 3-dimensional vector consisting of the z, w, and y components.
    * @return A vec<3, T> containing the z, w, and y components of the original vector.
    */
    vec<3, T> zwy() const { return vec<3, T>{z, w, y}; }

    /**
    * Returns a 3-dimensional vector consisting of the w, x, and y components.
    * @return A vec<3, T> containing the w, x, and y components of the original vector.
    */
    vec<3, T> wxy() const { return vec<3, T>{w, x, y}; }

    /**
    * Returns a 3-dimensional vector consisting of the w, x, and z components.
    * @return A vec<3, T> containing the w, x, and z components of the original vector.
    */
    vec<3, T> wxz() const { return vec<3, T>{w, x, z}; }

    /**
    * Returns a 3-dimensional vector consisting of the w, y, and x components.
    * @return A vec<3, T> containing the w, y, and x components of the original vector.
    */
    vec<3, T> wyx() const { return vec<3, T>{w, y, x}; }

    /**
    * Returns a 3-dimensional vector consisting of the w, y, and z components.
    * @return A vec<3, T> containing the w, y, and z components of the original vector.
    */
    vec<3, T> wyz() const { return vec<3, T>{w, y, z}; }

    /**
    * Returns a 3-dimensional vector consisting of the w, z, and x components.
    * @return A vec<3, T> containing the w, z, and x components of the original vector.
    */
    vec<3, T> wzx() const { return vec<3, T>{w, z, x}; }

    /**
    * Returns a 3-dimensional vector consisting of the w, z, and y components.
    * @return A vec<3, T> containing the w, z, and y components of the original vector.
    */
    vec<3, T> wzy() const { return vec<3, T>{w, z, y}; }

public:
    /**
    * Returns a pointer to the data array of the vector.
    * @return A pointer to the first component.
    */
    T const* data() const { return &x; }

    /**
    * Computes the length (magnitude) of the vector.
    * @return The length of the vector.
    */
    [[nodiscard]] T length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }

    /**
    * Normalizes the vector (makes it unit length).
    * @return The normalized vector.
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
    * Returns a normalized copy of the vector.
    * @return A new vector that is the normalized version of the original vector.
    */
    [[nodiscard]] vec normalized() const {
        T length = this->length();
        return {x / length, y / length, z / length, w / length};
    }

    /**
    * Computes the dot product with another vector.
    * @param other The vector to compute the dot product with.
    * @return The dot product.
    */
    [[nodiscard]] T dot(const vec& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }
};

/**
* @brief Retrieves a constant pointer to the data of a vec.
* @param v Reference to a vec object.
* @return Constant pointer to the data of the vector `v`.
*/
template<typename T>
T const* value_ptr(const vec<4, T>& v) {
    return v.data();
}

/**
* Returns the component-wise minimum of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return A vector containing the minimum components of the input vectors.
*/
template<typename T>
vec<4, T> min(const vec<4, T>& v1, const vec<4, T>& v2);

/**
* Returns the component-wise maximum of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return A vector containing the maximum components of the input vectors.
*/
template<typename T>
vec<4, T> max(const vec<4, T>& v1, const vec<4, T>& v2);

/**
* Computes the dot product of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return The dot product.
*/
template<typename T>
T dot(const vec<4, T>& v1, const vec<4, T>& v2);

} // namespace igm

#include "type_vec4.inl"

#endif // IGM_TYPE_VEC4_H