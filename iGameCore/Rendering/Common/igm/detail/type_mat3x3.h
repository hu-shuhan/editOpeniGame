/**
* @file
* @brief    Related basic data structures for 3×3 matrix.
* @author   Sumzeek
* @date     4/12/2024
* @version  v1.0.0
* @par      Copyright(c): Hangzhou Dianzi University iGame Laboratory
* @par      History:
*	       v1.0.0: Sumzeek, 4/12/2024, first create this file\n
*/

#ifndef IGM_TYPE_MAT3x3_H
#define IGM_TYPE_MAT3x3_H

#include "common.h"
#include "type_vec3.h"

namespace igm
{

/**
* @brief 3×3 matrix class that supports basic matrix operations.
*/
template<typename T>
class mat<3, 3, T> {
public:
    vec<3, T> value[3]; /**< Array to hold the column vectors of the matrix. */

    /**
    * @brief Default constructor. Initializes matrix to identity matrix.
    */
    mat<3, 3, T>() : value{} { setToIdentity(); }

    /**
    * @brief Constructor that initializes matrix with specified values.
    * @param x0 First column, first row value.
    * @param y0 First column, second row value.
    * @param z0 First column, third row value.
    * @param x1 Second column, first row value.
    * @param y1 Second column, second row value.
    * @param z1 Second column, third row value.
    * @param x2 Third column, first row value.
    * @param y2 Third column, second row value.
    * @param z2 Third column, third row value.
    */
    mat<3, 3, T>(const T& x0, const T& y0, const T& z0, const T& x1,
                 const T& y1, const T& z1, const T& x2, const T& y2,
                 const T& z2)
        : value{vec<3, T>(x0, y0, z0), vec<3, T>(x1, y1, z1),
                vec<3, T>(x2, y2, z2)} {}

    /**
    * @brief Constructor that initializes matrix with given column vectors.
    * @param v0 First column vector.
    * @param v1 Second column vector.
    * @param v2 Third column vector.
    */
    mat<3, 3, T>(const vec<3, T>& v0, const vec<3, T>& v1, const vec<3, T>& v2)
        : value{} {
        this->value[0] = v0;
        this->value[1] = v1;
        this->value[2] = v2;
    }

    /**
    * @brief Constructor that initializes matrix as a diagonal matrix.
    * @param diagonal The value to set on the diagonal.
    */
    explicit mat<3, 3, T>(const T& diagonal) : value{} {
        this->value[0] = vec<3, T>(diagonal, 0, 0);
        this->value[1] = vec<3, T>(0, diagonal, 0);
        this->value[2] = vec<3, T>(0, 0, diagonal);
    }

    /**
    * @brief Copy constructor.
    * @param other The matrix to copy from.
    */
    mat<3, 3, T>(const mat<3, 3, T>& other) : value{} {
        this->value[0] = other.value[0];
        this->value[1] = other.value[1];
        this->value[2] = other.value[2];
    }

    /**
    * @brief Stream insertion operator for mat<3, 3, T>.
    * @param os The output stream.
    * @param m The mat<3, 3, T> object to output.
    * @return Reference to the output stream.
    */
    friend std::ostream& operator<<(std::ostream& os, const mat<3, 3, T>& m) {
        os << "[" << m[0][0] << ", " << m[1][0] << ", " << m[2][0] << ", "
           << "\n";
        os << " " << m[0][1] << ", " << m[1][1] << ", " << m[2][1] << ", "
           << "\n";
        os << " " << m[0][2] << ", " << m[1][2] << ", " << m[2][2] << ", "
           << "]\n";
        return os;
    }

    /**
    * @brief Assignment operator.
    * @param other The mat<3, 3, T> object to assign from.
    * @return Reference to the assigned mat<3, 3, T> object.
    */
    mat<3, 3, T>& operator=(const mat<3, 3, T>& other) {
        if (this != &other) {
            this->value[0] = other[0];
            this->value[1] = other[1];
            this->value[2] = other[2];
        }
        return *this;
    }

    /**
    * @brief Addition operator for mat<3, 3, T>.
    * @param other The mat<3, 3, T> object to add.
    * @return Resultant mat<3, 3, T> object after addition.
    */
    mat<3, 3, T> operator+(const mat<3, 3, T>& other) const {
        mat<3, 3, T> result;
        result[0] = this->value[0] + other[0];
        result[1] = this->value[1] + other[1];
        result[2] = this->value[2] + other[2];
        return result;
    }

    /**
    * @brief Addition assignment operator for mat<3, 3, T>.
    * @param other The mat<3, 3, T> object to add.
    * @return Reference to the updated mat<3, 3, T> object.
    */
    mat<3, 3, T>& operator+=(const mat<3, 3, T>& other) {
        this->value[0] += other[0];
        this->value[1] += other[1];
        this->value[2] += other[2];
        return *this;
    }

    /**
    * @brief Subtraction operator for mat<3, 3, T>.
    * @param other The mat<3, 3, T> object to subtract.
    * @return Resultant mat<3, 3, T> object after subtraction.
    */
    mat<3, 3, T> operator-(const mat<3, 3, T>& other) const {
        mat<3, 3, T> result;
        result[0] = this->value[0] - other[0];
        result[1] = this->value[1] - other[1];
        result[2] = this->value[2] - other[2];
        return result;
    }

    /**
    * @brief Subtraction assignment operator for mat<3, 3, T>.
    * @param other The mat<3, 3, T> object to subtract.
    * @return Reference to the updated mat<3, 3, T> object.
    */
    mat<3, 3, T>& operator-=(const mat<3, 3, T>& other) {
        this->value[0] -= other[0];
        this->value[1] -= other[1];
        this->value[2] -= other[2];
        return *this;
    }

    /**
    * @brief Scalar multiplication operator for mat<3, 3, T>.
    * @param scalar The scalar value to multiply.
    * @return Resultant mat<3, 3, T> object after scalar multiplication.
    */
    mat<3, 3, T> operator*(T scalar) const {
        mat<3, 3, T> result;
        result[0] = this->value[0] * scalar;
        result[1] = this->value[0] * scalar;
        result[2] = this->value[0] * scalar;
        return result;
    }

    /**
    * @brief Scalar multiplication assignment operator for mat<3, 3, T>.
    * @param scalar The scalar value to multiply.
    * @return Reference to the updated mat<3, 3, T> object.
    */
    mat<3, 3, T>& operator*=(const T& scalar) {
        this->value[0] *= scalar;
        this->value[1] *= scalar;
        this->value[2] *= scalar;
        return *this;
    }

    /**
    * @brief Matrix-vector multiplication operator for mat<3, 3, T> and vec<3, T>.
    *
    * This operator multiplies a 3x3 matrix by a 3-dimensional vector. The resulting
    * vector is computed as follows:
    *
    * result[0] = (m[0][0] * v[0]) + (m[1][0] * v[1]) + (m[2][0] * v[2])
    * result[1] = (m[0][1] * v[0]) + (m[1][1] * v[1]) + (m[2][1] * v[2])
    * result[2] = (m[0][2] * v[0]) + (m[1][2] * v[1]) + (m[2][2] * v[2])
    *
    * where m is the matrix and v is the vector.
    *
    * @param v The vec<3, T> object to multiply.
    * @return Resultant vec<3, T> object after matrix-vector multiplication.
    */
    vec<3, T> operator*(const vec<3, T>& v) const {
        return vec<3, T>(
                value[0][0] * v[0] + value[1][0] * v[1] + value[2][0] * v[2],
                value[0][1] * v[0] + value[1][1] * v[1] + value[2][1] * v[2],
                value[0][2] * v[0] + value[1][2] * v[1] + value[2][2] * v[2]);
    }


    /**
    * @brief Matrix multiplication operator for mat<3, 3, T>.
    * @param other The mat<3, 3, T> object to multiply.
    * @return Resultant mat<3, 3, T> object after matrix multiplication.
    */
    mat<3, 3, T> operator*(const mat<3, 3, T>& other) const {
        mat<3, 3, T> result;
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) {
                result.value[c][r] = value[0][r] * other.value[c][0] +
                                     value[1][r] * other.value[c][1] +
                                     value[2][r] * other.value[c][2];
            }
        }
        return result;
    }

    /**
    * @brief Matrix multiplication assignment operator for mat<3, 3, T>.
    * @param other The mat<3, 3, T> object to multiply.
    * @return Reference to the updated mat<3, 3, T> object.
    */
    mat<3, 3, T>& operator*=(const mat<3, 3, T>& other) {
        mat<3, 3, T> result;
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) {
                result.value[c][r] = value[0][r] * other.value[c][0] +
                                     value[1][r] * other.value[c][1] +
                                     value[2][r] * other.value[c][2];
            }
        }
        *this = result;
        return *this;
    }

    /**
    * @brief Subscript operator to access columns of the mat<3, 3, T>.
    * @param index The column index (0-2).
    * @return Reference to the column vector at the specified index.
    * @throw std::out_of_range if index is out of range.
    */
    vec<3, T>& operator[](int index) {
        if (index >= 0 && index < 3) return value[index];
        else
            throw std::out_of_range(
                    "igm Error: mat<3, 3, T> index out of range");
    }

    /**
    * @brief Const subscript operator to access columns of the mat<3, 3, T>.
    * @param index The column index (0-2).
    * @return Const reference to the column vector at the specified index.
    * @throw std::out_of_range if index is out of range.
    */
    const vec<3, T>& operator[](int index) const {
        if (index >= 0 && index < 3) return value[index];
        else
            throw std::out_of_range(
                    "igm Error: mat<3, 3, T> index out of range");
    }

    /**
    * @brief Accessor function to retrieve the underlying data array.
    * @return Pointer to the data array storing column vectors.
    */
    T const* data() const { return &value[0].x; }

    /**
    * @brief Sets the matrix to an identity matrix.
    */
    void setToIdentity();

    /**
    * @brief Calculates the determinant of the matrix.
    * @return Determinant of the matrix.
    */
    [[nodiscard]] T determinant() const;

    /**
    * @brief Calculates the adjugate (adjoint) of the matrix.
    * @return Adjugate of the matrix.
    * @note This method does not change the data of the matrix.
    */
    [[nodiscard]] mat<3, 3, T> adjoint() const;

    /**
    * @brief Calculates the inverse of the matrix.
    * @return Inverse of the matrix.
    * @note This method does not change the data of the matrix.
    */
    [[nodiscard]] mat<3, 3, T> invert() const;

    /**
    * @brief Calculates the transpose of the matrix.
    * @return Transpose of the matrix.
    * @note This method does not change the data of the matrix.
    */
    [[nodiscard]] mat<3, 3, T> transpose() const;
};

/**
* @brief Retrieves a constant pointer to the data of a mat<3, 3, T> matrix.
* @param m Reference to a mat<3, 3, T> matrix object.
* @return Constant pointer to the data of the matrix `m`.
*/
template<typename T>
T const* value_ptr(const mat<3, 3, T>& m) {
    return m.data();
}

/**
* @brief Calculates the determinant of a mat<3, 3, T>.
* @param m The mat<3, 3, T> object.
* @return Determinant of the matrix.
*/
template<typename T>
T determinant(const mat<3, 3, T>& m);

/**
* @brief Calculates the adjugate (adjoint) of a mat<3, 3, T>.
* @param m The mat<3, 3, T> object.
* @return Adjugate of the matrix.
*/
template<typename T>
mat<3, 3, T> adjoint(mat<3, 3, T>& m);

/**
* @brief Calculates the inverse of a mat<3, 3, T>.
* @param m The mat<3, 3, T> object.
* @return Inverse of the matrix.
*/
template<typename T>
mat<3, 3, T> invert(mat<3, 3, T>& m);

/**
* @brief Calculates the transpose of a mat<3, 3, T>.
* @param m The mat<3, 3, T> object.
* @return Transpose of the matrix.
*/
template<typename T>
mat<3, 3, T> transpose(mat<3, 3, T>& m);

} // namespace igm

#include "type_mat3x3.inl"

#endif // IGM_TYPE_MAT3x3_H