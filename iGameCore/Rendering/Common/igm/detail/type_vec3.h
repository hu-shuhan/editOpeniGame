/**
* @file
* @brief    Related basic data structures for three-dimensional vectors.
* @author   Sumzeek
* @date     4/12/2024
* @version  v1.0.0
* @par      Copyright(c): Hangzhou Dianzi University iGame Laboratory
* @par      History:
*	       v1.0.0: Sumzeek, 4/12/2024, first create this file\n
*/

#ifndef IGM_TYPE_VEC3_H
#define IGM_TYPE_VEC3_H

#include "common.h"
#include "type_vec2.h"

namespace igm
{

/**
* A 3D vector class that supports various vector operations.
*/
template<typename T>
class vec<3, T> {
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

    /**
    * Default constructor. Initializes the vector to (0, 0, 0).
    */
    vec() : x(0.0f), y(0.0f), z(0.0f) {}

    /**
    * Copy constructor.
    * @param other The vector to copy from.
    */
    vec(const vec<3, T>& other) : x(other.x), y(other.y), z(other.z) {}
    vec(const vec<4, T>& other) : x(other.x), y(other.y), z(other.z) {}

    /**
    * Constructor that initializes all components to the same value.
    * @param s The value to initialize all components with.
    */
    explicit vec(T s) : x(s), y(s), z(s) {}

    /**
    * Constructor that initializes the vector with specified x, y, and z values.
    * @param x The x component.
    * @param y The y component.
    * @param z The z component.
    */
    vec(T x, T y, T z) : x(x), y(y), z(z) {}

    /**
    * Constructor that initializes the vector with a vec2 and a z value.
    * @param xy The vec2 to initialize the x and y components.
    * @param z The z component.
    */
    vec(const vec<2, T>& xy, T z) : x(xy.x), y(xy.y), z(z) {}

    /**
    * Constructor that initializes the vector with an x value and a vec2.
    * @param x The x component.
    * @param yz The vec2 to initialize the y and z components.
    */
    vec(T x, const vec<2, T>& yz) : x(x), y(yz.x), z(yz.y) {}

    /**
    * Stream insertion operator for easy output of the vector.
    * @param os The output stream.
    * @param v The vector to output.
    * @return The output stream.
    */
    friend std::ostream& operator<<(std::ostream& os, const vec& v) {
        os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
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
        }
        return *this;
    }

    /**
    * Equality operator.
    * @param other The vector to compare with.
    * @return True if the vectors are equal, false otherwise.
    */
    bool operator==(const vec& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    /**
    * Inequality operator.
    * @param other The vector to compare with.
    * @return True if the vectors are not equal, false otherwise.
    */
    bool operator!=(const vec& other) const {
        return x != other.x || y != other.y || z != other.z;
    }

    /**
    * Addition operator.
    * @param other The vector to add.
    * @return A new vector that is the sum of the two vectors.
    */
    vec operator+(const vec& other) const {
        return {x + other.x, y + other.y, z + other.z};
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
        return *this;
    }

    /**
    * Subtraction operator.
    * @param other The vector to subtract.
    * @return A new vector that is the difference of the two vectors.
    */
    vec operator-(const vec& other) const {
        return {x - other.x, y - other.y, z - other.z};
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
        return *this;
    }

    /**
    * Scalar multiplication operator.
    * @param scalar The scalar to multiply by.
    * @return A new vector that is the product of the vector and the scalar.
    */
    vec operator*(const T& scalar) const {
        return {x * scalar, y * scalar, z * scalar};
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
        return *this;
    }

    /**
    * Scalar division operator.
    * @param scalar The scalar to divide by.
    * @return A new vector that is the quotient of the vector and the scalar.
    */
    vec operator/(const T& scalar) const {
        return {x / scalar, y / scalar, z / scalar};
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
        return *this;
    }

    /**
    * Component-wise multiplication operator.
    * @param other The vector to multiply by.
    * @return A new vector that is the component-wise product of the two vectors.
    */
    vec operator*(const vec& other) const {
        return {x * other.x, y * other.y, z * other.z};
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
        return *this;
    }

    /**
    * Negation operator.
    * @return A new vector that is the negation of the original vector.
    */
    vec operator-() const { return {-x, -y, -z}; }

    /**
    * Subscript operator for non-const access.
    * @param index The index of the component (0 for x, 1 for y, 2 for z).
    * @return A reference to the component.
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
    * Subscript operator for const access.
    * @param index The index of the component (0 for x, 1 for y, 2 for z).
    * @return A const reference to the component.
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

public:
    /**
    * Returns a 2-dimensional vector consisting of the x and y components.
    * @return A vec<2, T> containing the x and y components of the original vector.
    */
    vec<2, T> xy() const { return vec<2, T>{x, y}; }

    /**
    * Returns a 2-dimensional vector consisting of the y and x components.
    * @return A vec<2, T> containing the y and x components of the original vector.
    */
    vec<2, T> yx() const { return vec<2, T>{y, x}; }

    /**
    * Returns a 2-dimensional vector consisting of the x and z components.
    * @return A vec<2, T> containing the x and z components of the original vector.
    */
    vec<2, T> xz() const { return vec<2, T>{x, z}; }

    /**
    * Returns a 2-dimensional vector consisting of the z and x components.
    * @return A vec<2, T> containing the z and x components of the original vector.
    */
    vec<2, T> zx() const { return vec<2, T>{z, x}; }

    /**
    * Returns a 2-dimensional vector consisting of the y and z components.
    * @return A vec<2, T> containing the y and z components of the original vector.
    */
    vec<2, T> yz() const { return vec<2, T>{y, z}; }

    /**
    * Returns a 2-dimensional vector consisting of the z and y components.
    * @return A vec<2, T> containing the z and y components of the original vector.
    */
    vec<2, T> zy() const { return vec<2, T>{z, y}; }

public:
    /**
    * Returns a pointer to the underlying data.
    * @return A pointer to the first element of the vector.
    */
    T const* data() const { return &x; }

    /**
    * Computes the length (magnitude) of the vector.
    * @return The length of the vector.
    */
    [[nodiscard]] T length() const { return std::sqrt(x * x + y * y + z * z); }

    /**
    * Normalizes the vector in place.
    * @return A reference to the normalized vector.
    */
    vec normalize() {
        T length = this->length();
        x /= length;
        y /= length;
        z /= length;
        return *this;
    }

    /**
    * Returns a normalized copy of the vector.
    * @return A new vector that is the normalized version of the original vector.
    */
    [[nodiscard]] vec normalized() const {
        T length = this->length();
        return {x / length, y / length, z / length};
    }

    /**
    * Computes the dot product with another vector.
    * @param other The vector to compute the dot product with.
    * @return The dot product.
    */
    [[nodiscard]] T dot(const vec& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    /**
    * Computes the cross product with another vector.
    * @param other The vector to compute the cross product with.
    * @return A new vector that is the cross product of the two vectors.
    */
    [[nodiscard]] vec cross(const vec& other) {
        T newX = y * other.z - z * other.y;
        T newY = z * other.x - x * other.z;
        T newZ = x * other.y - y * other.x;
        return {newX, newY, newZ};
    }
};

/**
* @brief Retrieves a constant pointer to the data of a vec.
* @param v Reference to a vec object.
* @return Constant pointer to the data of the vector `v`.
*/
template<typename T>
T const* value_ptr(const vec<3, T>& v) {
    return v.data();
}

/**
* Returns the component-wise minimum of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return A vector containing the minimum components of the input vectors.
*/
template<typename T>
vec<3, T> min(const vec<3, T>& v1, const vec<3, T>& v2);

/**
* Returns the component-wise maximum of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return A vector containing the maximum components of the input vectors.
*/
template<typename T>
vec<3, T> max(const vec<3, T>& v1, const vec<3, T>& v2);

/**
* Computes the dot product of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return The dot product.
*/
template<typename T>
T dot(const vec<3, T>& v1, const vec<3, T>& v2);

/**
* Computes the cross product of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return A new vector that is the cross product of the two vectors.
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