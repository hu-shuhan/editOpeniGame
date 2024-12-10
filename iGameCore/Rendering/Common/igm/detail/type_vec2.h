/**
* @file
* @brief    Related basic data structures for two-dimensional vectors.
* @author   Sumzeek
* @date     4/12/2024
* @version  v1.0.0
* @par      Copyright(c): Hangzhou Dianzi University iGame Laboratory
* @par      History:
*	       v1.0.0: Sumzeek, 4/12/2024, first create this file\n
*/

#ifndef IGM_TYPE_VEC2_H
#define IGM_TYPE_VEC2_H

#include "common.h"

namespace igm
{

/**
* A 2D vector class that supports various vector operations.
*/
template<typename T>
class vec<2, T> {
public:
    // Union allows the same memory location to be accessed using different member
    // names.
    union {
        T x, r, s; // Represents the first component of the vector.
    };
    union {
        T y, g, t; // Represents the second component of the vector.
    };

    /**
    * Default constructor. Initializes the vector to (0, 0).
    */
    vec<2, T>() : x(0.0f), y(0.0f) {}

    /**
    * Constructor that initializes both components to the same value.
    * @param s The value to initialize both components with.
    */
    explicit vec(T s) : x(s), y(s) {}

    /**
    * Constructor that initializes the vector with specified x and y values.
    * @param x The x component.
    * @param y The y component.
    */
    vec(T x, T y) : x(x), y(y) {}

    /**
    * Copy constructor.
    * @param other The vector to copy from.
    */
    vec(const vec<2, T>& other) : x(other.x), y(other.y) {}
    vec(const vec<3, T>& other) : x(other.x), y(other.y) {}
    vec(const vec<4, T>& other) : x(other.x), y(other.y) {}

    /**
    * Stream insertion operator for easy output of the vector.
    * @param os The output stream.
    * @param v The vector to output.
    * @return The output stream.
    */
    friend std::ostream& operator<<(std::ostream& os, const vec<2, T>& v) {
        os << "(" << v[0] << ", " << v[1] << ")";
        return os;
    }

    /**
    * Assignment operator.
    * @param other The vector to assign from.
    * @return The assigned vector.
    */
    vec<2, T>& operator=(const vec<2, T>& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
        }
        return *this;
    }

    /**
    * Equality operator.
    * @param other The vector to compare with.
    * @return True if the vectors are equal, false otherwise.
    */
    bool operator==(const vec<2, T>& other) const {
        return x == other.x && y == other.y;
    }

    /**
    * Inequality operator.
    * @param other The vector to compare with.
    * @return True if the vectors are not equal, false otherwise.
    */
    bool operator!=(const vec<2, T>& other) const {
        return x != other.x || y != other.y;
    }

    /**
    * Addition operator.
    * @param other The vector to add.
    * @return A new vector that is the sum of the two vectors.
    */
    vec<2, T> operator+(const vec<2, T>& other) const {
        return {x + other.x, y + other.y};
    }

    /**
    * Addition assignment operator.
    * @param other The vector to add.
    * @return The updated vector.
    */
    vec<2, T>& operator+=(const vec<2, T>& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    /**
    * Subtraction operator.
    * @param other The vector to subtract.
    * @return A new vector that is the difference of the two vectors.
    */
    vec<2, T> operator-(const vec<2, T>& other) const {
        return {x - other.x, y - other.y};
    }

    /**
    * Subtraction assignment operator.
    * @param other The vector to subtract.
    * @return The updated vector.
    */
    vec<2, T>& operator-=(const vec<2, T>& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    /**
    * Scalar multiplication operator.
    * @param scalar The scalar to multiply by.
    * @return A new vector that is the product of the vector and the scalar.
    */
    vec<2, T> operator*(const T& scalar) const {
        return {x * scalar, y * scalar};
    }

    /**
    * Scalar multiplication assignment operator.
    * @param scalar The scalar to multiply by.
    * @return The updated vector.
    */
    vec<2, T>& operator*=(const T& scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    /**
    * Scalar division operator.
    * @param scalar The scalar to divide by.
    * @return A new vector that is the quotient of the vector and the scalar.
    */
    vec<2, T> operator/(const T& scalar) const {
        return {x / scalar, y / scalar};
    }

    /**
    * Scalar division assignment operator.
    * @param scalar The scalar to divide by.
    * @return The updated vector.
    */
    vec<2, T>& operator/=(const T& scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    /**
    * Component-wise multiplication operator.
    * @param other The vector to multiply by.
    * @return A new vector that is the component-wise product of the two vectors.
    */
    vec<2, T> operator*(const vec<2, T>& other) const {
        return {x * other.x, y * other.y};
    }

    /**
    * Component-wise multiplication assignment operator.
    * @param other The vector to multiply by.
    * @return The updated vector.
    */
    vec<2, T>& operator*=(const vec<2, T>& other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    /**
    * Negation operator.
    * @return A new vector that is the negation of the original vector.
    */
    vec<2, T> operator-() const { return {-x, -y}; }

    /**
    * Subscript operator for non-const access.
    * @param index The index of the component (0 for x, 1 for y).
    * @return A reference to the component.
    */
    T& operator[](int index) {
        assert(index >= 0 && index < 2);
        switch (index) {
            default:
            case 0:
                return x;
            case 1:
                return y;
        }
    }

    /**
    * Subscript operator for const access.
    * @param index The index of the component (0 for x, 1 for y).
    * @return A const reference to the component.
    */
    const T& operator[](int index) const {
        assert(index >= 0 && index < 2);
        switch (index) {
            default:
            case 0:
                return x;
            case 1:
                return y;
        }
    }

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
    [[nodiscard]] T length() const { return std::sqrt(x * x + y * y); }

    /**
    * Normalizes the vector in place.
    * @return A reference to the normalized vector.
    */
    vec<2, T> normalize() {
        T length = this->length();
        x /= length;
        y /= length;
        return *this;
    }

    /**
    * Returns a normalized copy of the vector.
    * @return A new vector that is the normalized version of the original vector.
    */
    [[nodiscard]] vec<2, T> normalized() const {
        T length = this->length();
        return {x / length, y / length};
    }

    /**
    * Computes the dot product with another vector.
    * @param other The vector to compute the dot product with.
    * @return The dot product.
    */
    [[nodiscard]] T dot(const vec& other) const {
        return x * other.x + y * other.y;
    }
};

/**
* @brief Retrieves a constant pointer to the data of a vec2.
* @param v Reference to a vec2 object.
* @return Constant pointer to the data of the vector `v`.
*/
template<typename T>
T const* value_ptr(const vec<2, T>& v) {
    return v.data();
}

/**
* Returns the component-wise minimum of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return A vector containing the minimum components of the input vectors.
*/
template<typename T>
vec<2, T> min(const vec<2, T>& v1, const vec<2, T>& v2);

/**
* Returns the component-wise maximum of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return A vector containing the maximum components of the input vectors.
*/
template<typename T>
vec<2, T> max(const vec<2, T>& v1, const vec<2, T>& v2);

/**
* Computes the dot product of two vectors.
* @param v1 The first vector.
* @param v2 The second vector.
* @return The dot product.
*/
template<typename T>
T dot(const vec<2, T>& v1, const vec<2, T>& v2);

} // namespace igm

#include "type_vec2.inl"

//namespace std
//{
//
///**
//* Hash function specialization for the vec2 class, allowing it to be used in
//* unordered containers.
//*/
//template<typename T>
//struct hash<igm::vec<2, T>> {
//    /**
//  * Computes the hash value for a vec2.
//  * @param v The vector to hash.
//  * @return The hash value.
//  */
//    size_t operator()(const igm::vec<2, T>& v) const {
//        size_t h1 = std::hash<T>()(v.x);
//        size_t h2 = std::hash<T>()(v.y);
//        return h1 ^ (h2 << 1);
//    }
//};
//
//} // namespace std

#endif // IGM_TYPE_VEC2_H