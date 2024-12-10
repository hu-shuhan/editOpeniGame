/**
* @file
* @brief    Related basic data structures for 4×4 matrix.
* @author   Sumzeek
* @date     4/12/2024
* @version  v1.0.0
* @par      Copyright(c): Hangzhou Dianzi University iGame Laboratory
* @par      History:
*	       v1.0.0: Sumzeek, 4/12/2024, first create this file\n
*/

#ifndef IGM_TYPE_MAT4x4_H
#define IGM_TYPE_MAT4x4_H

#include "common.h"
#include "type_vec4.h"

namespace igm
{

/**
* @brief 4×4 matrix class that supports basic matrix operations.
*/
template<typename T>
class mat<4, 4, T> {
public:
    // Column-major order storage of vectors
    vec<4, T> value[4];

    /**
    * @brief Default constructor.
    * Initializes matrix to identity matrix.
    */
    mat<4, 4, T>() : value{} { setToIdentity(); }

    /**
    * @brief Constructor initializing matrix with specified values.
    * @param x0, y0, z0, w0 Values for the first column vector.
    * @param x1, y1, z1, w1 Values for the second column vector.
    * @param x2, y2, z2, w2 Values for the third column vector.
    * @param x3, y3, z3, w3 Values for the fourth column vector.
    */
    mat<4, 4, T>(const T& x0, const T& y0, const T& z0, const T& w0,
                 const T& x1, const T& y1, const T& z1, const T& w1,
                 const T& x2, const T& y2, const T& z2, const T& w2,
                 const T& x3, const T& y3, const T& z3, const T& w3)
        : value{vec<4, T>(x0, y0, z0, w0), vec<4, T>(x1, y1, z1, w1),
                vec<4, T>(x2, y2, z2, w2), vec<4, T>(x3, y3, z3, w3)} {}

    /**
    * @brief Constructor initializing matrix with specified vectors.
    * @param v0 First column vector.
    * @param v1 Second column vector.
    * @param v2 Third column vector.
    * @param v3 Fourth column vector.
    */
    mat<4, 4, T>(const vec<4, T>& v0, const vec<4, T>& v1, const vec<4, T>& v2,
                 const vec<4, T>& v3)
        : value{v0, v1, v2, v3} {}

    /**
    * @brief Constructor initializing matrix to diagonal matrix with specified
    * diagonal value.
    * @param diagonal The value to set on the diagonal.
    */
    explicit mat<4, 4, T>(const T& diagonal) : value{} {
        this->value[0] = vec<4, T>(diagonal, 0, 0, 0);
        this->value[1] = vec<4, T>(0, diagonal, 0, 0);
        this->value[2] = vec<4, T>(0, 0, diagonal, 0);
        this->value[3] = vec<4, T>(0, 0, 0, diagonal);
    }

    /**
    * @brief Copy constructor.
    * @param other The matrix to copy.
    */
    mat<4, 4, T>(const mat<4, 4, T>& other)
        : value{other.value[0], other.value[1], other.value[2],
                other.value[3]} {}

    /**
    * @brief Output stream operator for mat<4, 4, T>.
    * @param os Output stream.
    * @param m Matrix to output.
    * @return Reference to the output stream.
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
    * @brief Assignment operator for mat<4, 4, T>.
    * @param other The matrix to assign.
    * @return Reference to the updated mat<4, 4, T> object.
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
    * @brief Addition operator for mat<4, 4, T>.
    * @param other The matrix to add.
    * @return Resultant mat<4, 4, T> object after addition.
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
    * @brief Addition assignment operator for mat<4, 4, T>.
    * @param other The matrix to add.
    * @return Reference to the updated mat<4, 4, T> object.
    */
    mat<4, 4, T>& operator+=(const mat<4, 4, T>& other) {
        this->value[0] += other[0];
        this->value[1] += other[1];
        this->value[2] += other[2];
        this->value[3] += other[3];
        return *this;
    }

    /**
    * @brief Subtraction operator for mat<4, 4, T>.
    * @param other The matrix to subtract.
    * @return Resultant mat<4, 4, T> object after subtraction.
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
    * @brief Subtraction assignment operator for mat<4, 4, T>.
    * @param other The matrix to subtract.
    * @return Reference to the updated mat<4, 4, T> object.
    */
    mat<4, 4, T>& operator-=(const mat<4, 4, T>& other) {
        this->value[0] -= other[0];
        this->value[1] -= other[1];
        this->value[2] -= other[2];
        this->value[3] -= other[3];
        return *this;
    }

    /**
    * @brief Scalar multiplication operator for mat<4, 4, T>.
    * @param scalar The scalar value to multiply.
    * @return Resultant mat<4, 4, T> object after scalar multiplication.
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
    * @brief Scalar multiplication assignment operator for mat<4, 4, T>.
    * @param scalar The scalar value to multiply.
    * @return Reference to the updated mat<4, 4, T> object.
    */
    mat<4, 4, T>& operator*=(const T& scalar) {
        this->value[0] *= scalar;
        this->value[1] *= scalar;
        this->value[2] *= scalar;
        this->value[3] *= scalar;
        return *this;
    }

    /**
    * @brief Matrix-vector multiplication operator for mat<4, 4, T> and vec<4, T>.
    *
    * This operator multiplies a 4x4 matrix by a 4-dimensional vector. The resulting
    * vector is computed as follows:
    *
    * result[0] = (m[0][0] * v[0]) + (m[1][0] * v[1]) + (m[2][0] * v[2]) + (m[3][0] * v[3])
    * result[1] = (m[0][1] * v[0]) + (m[1][1] * v[1]) + (m[2][1] * v[2]) + (m[3][1] * v[3])
    * result[2] = (m[0][2] * v[0]) + (m[1][2] * v[1]) + (m[2][2] * v[2]) + (m[3][2] * v[3])
    * result[3] = (m[0][3] * v[0]) + (m[1][3] * v[1]) + (m[2][3] * v[2]) + (m[3][3] * v[3])
    *
    * where m is the matrix and v is the vector.
    *
    * @param v The vec<4, T> object to multiply.
    * @return Resultant vec<4, T> object after matrix-vector multiplication.
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
    * @brief Vector multiplication operator for mat<4, 4, T>.
    * Multiplies a mat<4, 4, T> with a vec4 (as column vector).
    * @param v The vector to multiply.
    * @return Resultant vec4 after matrix-vector multiplication.
    */
    vec<4, T> operator*(const vec<4, T>& v) {
        vec<4, T> result(
                this->value[0].x * v.x + this->value[1].x * v.y +
                        this->value[2].x * v.z + this->value[3].x * v.w,
                this->value[0].y * v.x + this->value[1].y * v.y +
                        this->value[2].y * v.z + this->value[3].y * v.w,
                this->value[0].z * v.x + this->value[1].z * v.y +
                        this->value[2].z * v.z + this->value[3].z * v.w,
                this->value[0].w * v.x + this->value[1].w * v.y +
                        this->value[2].w * v.z + this->value[3].w * v.w);
        return result;
    }

    /**
    * @brief Matrix multiplication operator for mat<4, 4, T>.
    * Multiplies two mat<4, 4, T> matrices.
    * @param other The matrix to multiply.
    * @return Resultant mat<4, 4, T> object after matrix multiplication.
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
    * @brief Matrix multiplication assignment operator for mat<4, 4, T>.
    * Multiplies this mat<4, 4, T> with another mat<4, 4, T> and assigns the result.
    * @param other The matrix to multiply.
    * @return Reference to the updated mat<4, 4, T> object.
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
    * @brief Array subscript operator for accessing columns of the matrix.
    * @param index The index of the column to access (0-3).
    * @return Reference to the vec4 representing the column.
    * @throws std::out_of_range if index is out of valid range [0, 3].
    */
    vec<4, T>& operator[](int index) {
        if (index >= 0 && index < 4) return value[index];
        else
            throw std::out_of_range(
                    "igm Error: mat<4, 4, T> index out of range");
    }

    /**
    * @brief Const array subscript operator for accessing columns of the matrix.
    * @param index The index of the column to access (0-3).
    * @return Const reference to the vec4 representing the column.
    * @throws std::out_of_range if index is out of valid range [0, 3].
    */
    const vec<4, T>& operator[](int index) const {
        if (index >= 0 && index < 4) return value[index];
        else
            throw std::out_of_range(
                    "igm Error: mat<4, 4, T> index out of range");
    }

public:
    /**
    * @brief Returns a pointer to the first element of the matrix data.
    * Useful for interfacing with external C functions or APIs.
    * @return Pointer to const T data.
    */
    T const* data() const { return &value[0].x; }

    /**
    * @brief Sets the matrix to the identity matrix.
    * The identity matrix is a square matrix with 1s on the diagonal and 0s
    * elsewhere.
    */
    void setToIdentity();

    /**
    * @brief Calculates the determinant of the matrix.
    * @return Determinant value of the matrix.
    */
    [[nodiscard]] T determinant() const;

    /**
    * @brief Calculates the adjoint (or adjugate) matrix of the current matrix.
    * The adjoint matrix is the transpose of the cofactor matrix.
    * @return Adjacent matrix of the current matrix.
    * @note This method does not change the data of the matrix.
    */
    [[nodiscard]] mat<4, 4, T> adjoint() const;

    /**
    * @brief Calculates the inverse matrix of the current matrix.
    * If the matrix is not invertible, behavior is undefined.
    * @return Inverted matrix of the current matrix.
    * @note This method does not change the data of the matrix.
    */
    [[nodiscard]] mat<4, 4, T> invert() const;

    /**
    * @brief Calculates the transpose of the matrix.
    * The transpose of a matrix switches its rows with its columns.
    * @return Transposed matrix of the current matrix.
    * @note This method does not change the data of the matrix.
    */
    [[nodiscard]] mat<4, 4, T> transpose() const;
};

/**
* @brief Retrieves a constant pointer to the data of a mat<4, 4, T> matrix.
* @param m Reference to a mat<4, 4, T> matrix object.
* @return Constant pointer to the data of the matrix `m`.
*/
template<typename T>
T const* value_ptr(const mat<4, 4, T>& m) {
    return m.data();
}

/**
* @brief Calculates the determinant of a given mat<4, 4, T> matrix.
* @param m The matrix for which determinant is to be calculated.
* @return Determinant value of the matrix.
*/
template<typename T>
T determinant(const mat<4, 4, T>& m);

/**
* @brief Calculates the adjoint (or adjugate) matrix of a given mat<4, 4, T> matrix.
* The adjoint matrix is the transpose of the cofactor matrix.
* @param m The matrix for which adjoint matrix is to be calculated.
* @return Adjacent matrix of the provided matrix.
*/
template<typename T>
mat<4, 4, T> adjoint(mat<4, 4, T>& m);

/**
* @brief Calculates the inverse matrix of a given mat<4, 4, T> matrix.
* If the matrix is not invertible, behavior is undefined.
* @param m The matrix for which inverse matrix is to be calculated.
* @return Inverted matrix of the provided matrix.
*/
template<typename T>
mat<4, 4, T> invert(mat<4, 4, T>& m);

/**
* @brief Calculates the transpose of a given mat<4, 4, T> matrix.
* The transpose of a matrix switches its rows with its columns.
* @param m The matrix for which transpose matrix is to be calculated.
* @return Transposed matrix of the provided matrix.
*/
template<typename T>
mat<4, 4, T> transpose(mat<4, 4, T>& m);

} // namespace igm

#include "type_mat4x4.inl"

#endif // IGM_TYPE_MAT4x4_H
