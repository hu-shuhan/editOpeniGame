/**
* @file
* @brief    Related basic data structures for 2×2 matrix.
* @author   Sumzeek
* @date     4/12/2024
* @version  v1.0.0
* @par      Copyright(c): Hangzhou Dianzi University iGame Laboratory
* @par      History:
*	       v1.0.0: Sumzeek, 4/12/2024, first create this file\n
*/

#ifndef IGM_TYPE_MAT2x2_H
#define IGM_TYPE_MAT2x2_H

#include "common.h"
#include "type_vec2.h"

namespace igm
{

/**
* @brief 2×2 matrix class that supports basic matrix operations.
*/
template<typename T>
class mat<2, 2, T> {
public:
    vec<2, T> value[2]; /**< Array to hold the column vectors of the matrix. */

    /**
    * @brief Default constructor. Initializes matrix to identity matrix.
    */
    mat<2, 2, T>() : value{} { setToIdentity(); }

    /**
    * @brief Constructor that initializes matrix with specified values.
    * @param x0 First column, first row value.
    * @param y0 First column, second row value.
    * @param x1 Second column, first row value.
    * @param y1 Second column, second row value.
    */
    mat<2, 2, T>(const T& x0, const T& y0, const T& x1, const T& y1)
        : value{vec<2, T>(x0, y0), vec<2, T>(x1, y1)} {}

    /**
    * @brief Constructor that initializes matrix with given column vectors.
    * @param v0 First column vector.
    * @param v1 Second column vector.
    */
    mat<2, 2, T>(const vec<2, T>& v0, const vec<2, T>& v1) : value{} {
        this->value[0] = v0;
        this->value[1] = v1;
    }

    /**
    * @brief Constructor that initializes matrix as a diagonal matrix.
    * @param diagonal The value to set on the diagonal.
    */
    explicit mat<2, 2, T>(const T& diagonal) : value{} {
        this->value[0] = vec<2, T>(diagonal, 0);
        this->value[1] = vec<2, T>(0, diagonal);
    }

    /**
    * @brief Copy constructor.
    * @param other The matrix to copy from.
    */
    mat<2, 2, T>(const mat<2, 2, T>& other) : value{} {
        this->value[0] = other.value[0];
        this->value[1] = other.value[1];
    }

    /**
    * @brief Stream insertion operator for mat<2, 2, T>.
    * @param os The output stream.
    * @param m The mat<2, 2, T> object to output.
    * @return Reference to the output stream.
    */
    friend std::ostream& operator<<(std::ostream& os, const mat<2, 2, T>& m) {
        os << "[" << m[0][0] << ", " << m[1][0] << ",\n";
        os << " " << m[0][1] << ", " << m[1][1] << "]\n";
        return os;
    }

    /**
    * @brief Assignment operator.
    * @param other The mat<2, 2, T> object to assign from.
    * @return Reference to the assigned mat<2, 2, T> object.
    */
    mat<2, 2, T>& operator=(const mat<2, 2, T>& other) {
        if (this != &other) {
            this->value[0] = other[0];
            this->value[1] = other[1];
        }
        return *this;
    }

    /**
    * @brief Addition operator for mat<2, 2, T>.
    * @param other The mat<2, 2, T> object to add.
    * @return Resultant mat<2, 2, T> object after addition.
    */
    mat<2, 2, T> operator+(const mat<2, 2, T>& other) const {
        mat<2, 2, T> result;
        result[0] = this->value[0] + other[0];
        result[1] = this->value[1] + other[1];
        return result;
    }

    /**
    * @brief Addition assignment operator for mat<2, 2, T>.
    * @param other The mat<2, 2, T> object to add.
    * @return Reference to the updated mat<2, 2, T> object.
    */
    mat<2, 2, T>& operator+=(const mat<2, 2, T>& other) {
        this->value[0] += other[0];
        this->value[1] += other[1];
        return *this;
    }

    /**
    * @brief Subtraction operator for mat<2, 2, T>.
    * @param other The mat<2, 2, T> object to subtract.
    * @return Resultant mat<2, 2, T> object after subtraction.
    */
    mat<2, 2, T> operator-(const mat<2, 2, T>& other) const {
        mat<2, 2, T> result;
        result[0] = this->value[0] - other[0];
        result[1] = this->value[1] - other[1];
        return result;
    }

    /**
    * @brief Subtraction assignment operator for mat<2, 2, T>.
    * @param other The mat<2, 2, T> object to subtract.
    * @return Reference to the updated mat<2, 2, T> object.
    */
    mat<2, 2, T>& operator-=(const mat<2, 2, T>& other) {
        this->value[0] -= other[0];
        this->value[1] -= other[1];
        return *this;
    }

    /**
    * @brief Scalar multiplication operator for mat<2, 2, T>.
    * @param scalar The scalar value to multiply.
    * @return Resultant mat<2, 2, T> object after scalar multiplication.
    */
    mat<2, 2, T> operator*(T scalar) const {
        mat<2, 2, T> result;
        result[0] = this->value[0] * scalar;
        result[1] = this->value[1] * scalar;
        return result;
    }

    /**
    * @brief Scalar multiplication assignment operator for mat<2, 2, T>.
    * @param scalar The scalar value to multiply.
    * @return Reference to the updated mat<2, 2, T> object.
    */
    mat<2, 2, T>& operator*=(const T& scalar) {
        this->value[0] *= scalar;
        this->value[1] *= scalar;
        return *this;
    }

    /**
    * @brief Matrix-vector multiplication operator for mat<2, 2, T> and vec<2, T>.
    *
    * This operator multiplies a 2x2 matrix by a 2-dimensional vector. The resulting
    * vector is computed as follows:
    *
    * result[0] = (m[0][0] * v[0]) + (m[1][0] * v[1])
    * result[1] = (m[0][1] * v[0]) + (m[1][1] * v[1])
    *
    * where m is the matrix and v is the vector.
    *
    * @param v The vec<2, T> object to multiply.
    * @return Resultant vec<2, T> object after matrix-vector multiplication.
    */
    vec<2, T> operator*(const vec<2, T>& v) const {
        return vec<2, T>(value[0][0] * v[0] + value[1][0] * v[1],
                         value[0][1] * v[0] + value[1][1] * v[1]);
    }

    /**
    * @brief Matrix multiplication operator for mat<2, 2, T>.
    * @param other The mat<2, 2, T> object to multiply.
    * @return Resultant mat<2, 2, T> object after matrix multiplication.
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
    * @brief Matrix multiplication assignment operator for mat<2, 2, T>.
    * @param other The mat<2, 2, T> object to multiply.
    * @return Reference to the updated mat<2, 2, T> object.
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
    * @brief Subscript operator for accessing columns of the matrix.
    * @param index The index of the column (0 or 1).
    * @return Reference to the column vector.
    * @throws std::out_of_range if index is out of valid range.
    */
    vec<2, T>& operator[](int index) {
        if (index >= 0 && index < 2) return value[index];
        else
            throw std::out_of_range(
                    "igm Error: mat<2, 2, T> index out of range");
    }

    /**
    * @brief Subscript operator for accessing columns of the matrix (const
    * version).
    * @param index The index of the column (0 or 1).
    * @return Const reference to the column vector.
    * @throws std::out_of_range if index is out of valid range.
    */
    const vec<2, T>& operator[](int index) const {
        if (index >= 0 && index < 2) return value[index];
        else
            throw std::out_of_range(
                    "igm Error: mat<2, 2, T> index out of range");
    }

public:
    /**
    * @brief Get pointer to the underlying data of the matrix.
    * @return Pointer to the first element of the matrix.
    */
    T const* data() const { return &value[0].x; }

    /**
    * @brief Set the matrix to be an identity matrix.
    */
    void setToIdentity();

    /**
    * @brief Compute the determinant of the matrix.
    * @return Determinant value.
    */
    [[nodiscard]] T determinant() const;

    /**
    * @brief Compute the adjoint (adjugate) of the matrix.
    * @return Adjoint matrix.
    * @note This method does not change the data of the matrix.
    */
    [[nodiscard]] mat<2, 2, T> adjoint() const;

    /**
    * @brief Compute the inverse of the matrix.
    * @return Inverse matrix.
    * @note This method does not change the data of the matrix.
    */
    [[nodiscard]] mat<2, 2, T> invert() const;

    /**
    * @brief Compute the transpose of the matrix.
    * @return Transposed matrix.
    * @note This method does not change the data of the matrix.
    */
    [[nodiscard]] mat<2, 2, T> transpose() const;
};

/**
* @brief Retrieves a constant pointer to the data of a mat<2, 2, T> matrix.
* @param m Reference to a mat<2, 2, T> matrix object.
* @return Constant pointer to the data of the matrix `m`.
*/
template<typename T>
T const* value_ptr(const mat<2, 2, T>& m) {
    return m.data();
}

/**
* @brief Calculates the determinant of a mat<2, 2, T>.
* @param m The mat<2, 2, T> object.
* @return Determinant of the matrix.
*/
template<typename T>
T determinant(const mat<2, 2, T>& m);

/**
* @brief Calculates the adjugate (adjoint) of a mat<2, 2, T>.
* @param m The mat<2, 2, T> object.
* @return Adjugate of the matrix.
*/
template<typename T>
mat<2, 2, T> adjoint(mat<2, 2, T>& m);

/**
* @brief Calculates the inverse of a mat<2, 2, T>.
* @param m The mat<2, 2, T> object.
* @return Inverse of the matrix.
*/
template<typename T>
mat<2, 2, T> invert(mat<2, 2, T>& m);

/**
* @brief Calculates the transpose of a mat<2, 2, T>.
* @param m The mat<2, 2, T> object.
* @return Transpose of the matrix.
*/
template<typename T>
mat<2, 2, T> transpose(mat<2, 2, T>& m);
} // namespace igm

#include "type_mat2x2.inl"

#endif // IGM_TYPE_MAT2x2_H