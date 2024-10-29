#ifndef iGameVector_h
#define iGameVector_h

#include "iGameMacro.h"
#include "iGameType.h"
#include <cassert>
#include <cmath>
#include <iostream>

IGAME_NAMESPACE_BEGIN
template<class ScalarT, int dimension_t>
class Vector {
public:
    static constexpr int dimension = dimension_t;

    typedef ScalarT Scalar;
    typedef Vector<Scalar, dimension> Self;

    Vector() {}
    Vector(const Scalar& c0, const Scalar& c1);
    Vector(const Scalar& c0, const Scalar& c1, const Scalar& c2);
    Vector(const Scalar& c0, const Scalar& c1, const Scalar& c2,
           const Scalar& c3);
    Vector(const Scalar& c0, const Scalar& c1, const Scalar& c2,
           const Scalar& c3, const Scalar& c4);
    Vector(const Scalar& c0, const Scalar& c1, const Scalar& c2,
           const Scalar& c3, const Scalar& c4, const Scalar& c5);
    Vector(const Scalar& c0, const Scalar& c1, const Scalar& c2,
           const Scalar& c3, const Scalar& c4, const Scalar& c5,
           const Scalar& c6);
    Vector(const Self& o);
    Vector(const ScalarT p[dimension_t]) {
        for (int i = 0; i < dimension; ++i) { v_[i] = p[i]; }
    }

    template<class OtherScalarT, int other_dimension_t>
    Vector(const Vector<OtherScalarT, other_dimension_t>& src_vector) {
        for (int i = 0; i < dimension; ++i) {
            if (i < other_dimension_t) v_[i] = Scalar(src_vector[i]);
            else
                v_[i] = Scalar(0);
        }
    }

    inline Scalar& operator[](int i) {
        assert(0 <= i && i < dimension);
        return v_[i];
    }
    inline const Scalar& operator[](int i) const {
        assert(0 <= i && i < dimension);
        return v_[i];
    }

    Self operator-() const {
        Self ret;
        for (int i = 0; i < dimension; ++i) { ret[i] = -(*this)[i]; }
        return ret;
    }

    Self operator+(const Self& o) const {
        Self ret;
        for (int i = 0; i < dimension; ++i) { ret[i] = (*this)[i] + o[i]; }
        return ret;
    }

    Self operator-(const Self& o) const {
        Self ret;
        for (int i = 0; i < dimension; ++i) { ret[i] = (*this)[i] - o[i]; }
        return ret;
    }

    ScalarT operator*(const Self& o) const {
        ScalarT ret = 0.0;
        for (int i = 0; i < dimension; ++i) { ret += (*this)[i] * o[i]; }
        return ret;
    }

    Self& operator+=(const Self& o) {
        for (int i = 0; i < dimension; ++i) { (*this)[i] += o[i]; }
        return *this;
    }

    Self& operator+=(const double& k) {
        for (int i = 0; i < dimension; ++i) { (*this)[i] += k; }
        return *this;
    }

    Self& operator-=(const Self& o) {
        for (int i = 0; i < dimension; ++i) { (*this)[i] -= o[i]; }
        return *this;
    }

    Self& operator-=(const double& k) {
        for (int i = 0; i < dimension; ++i) { (*this)[i] -= k; }
        return *this;
    }

    Self& operator*=(const ScalarT& o) {
        for (int i = 0; i < dimension; ++i) { (*this)[i] *= o; }
        return *this;
    }

    Self operator*(const ScalarT& k) const {
        Self ret;
        for (int i = 0; i < dimension; ++i) { ret[i] = v_[i] * k; }
        return ret;
    }

    Self operator*=(const double& k) const {
        for (int i = 0; i < dimension; ++i) { v_[i] *= k; }
        return *this;
    }

    Self operator/(const double& k) const {
        Self ret;
        for (int i = 0; i < dimension; ++i) { ret[i] = v_[i] / k; }
        return ret;
    }

    Self operator/=(const double& k) {
        for (int i = 0; i < dimension; ++i) { v_[i] /= k; }
        return *this;
    }

    Self operator+(const Scalar& o) const {
        Self ret;
        for (int i = 0; i < dimension; ++i) { ret[i] = (*this)[i] + o; }
        return ret;
    }

    Self operator-(const Scalar& o) const {
        Self ret;
        for (int i = 0; i < dimension; ++i) { ret[i] = (*this)[i] - o; }
        return ret;
    }

    bool operator==(const Self& o) const {
        for (int i = 0; i < dimension; ++i) {
            if ((*this)[i] != o[i]) { return false; }
        }
        return true;
    }

    bool operator!=(const Self& x) const { return !((*this) == x); }

    bool operator<(const Self& x) const {
        for (int i = 0; i < dimension - 1; ++i) {
            if (v_[i] < x.v_[i]) { return true; }
            if (v_[i] > x.v_[i]) { return false; }
        }
        if (v_[dimension - 1] < x.v_[dimension - 1]) { return true; }
        return false;
    }
    inline void setZero() {
        for (int i = 0; i < dimension; ++i) { v_[i] = 0; }
    }

    inline double norm() const { return sqrt(this->squaredNorm()); }
    inline double length() const { return sqrt(this->squaredLength()); }

    inline double squaredNorm() const {
        double ret = 0;
        for (int i = 0; i < dimension; ++i) { ret += v_[i] * v_[i]; }
        return ret;
    }

    inline double squaredLength() const {
        double ret = 0;
        for (int i = 0; i < dimension; ++i) { ret += v_[i] * v_[i]; }
        return ret;
    }

    inline void normalize() {
        double magnitude = std::sqrt(this->squaredNorm());
        if (magnitude == 0) { return; }
        for (int i = 0; i < dimension; ++i) { (*this)[i] /= magnitude; }
    }
    inline Self scale(const Self& o) const {
        Self ret;
        for (int i = 0; i < dimension; ++i) {
            ret[i] = (*this)[i] * o[i];
        }
        return ret;
    }
    inline ScalarT dot(const Self& o) const {
        ScalarT ret = 0.0;
        for (int i = 0; i < dimension; ++i) { ret += (*this)[i] * o[i]; }
        return ret;
    }
    inline Self cross(const Self& v) const {

        if (dimension != 3) return *this;
        Self r;
        r[0] = (v_[1] * v[2]) - (v_[2] * v[1]);
        r[1] = (v_[2] * v[0]) - (v_[0] * v[2]);
        r[2] = (v_[0] * v[1]) - (v_[1] * v[0]);
        return r;
    }

    inline Self normalized() const {
        Self ret(*this);
        ret.normalize();
        return ret;
    }

    const Scalar& maxCoeff() const {
        return *std::max_element(v_, v_ + dimension);
    }

    const Scalar& minCoeff() const {
        return *std::min_element(v_, v_ + dimension);
    }

    Scalar* pointer() { return &(v_[0]); }
    const Scalar* pointer() const { return &(v_[0]); }
    size_t dim() { return dimension; }

private:
    Scalar v_[dimension];
};

template<class ScalarT, int dimension_t>
inline Vector<ScalarT, dimension_t>::Vector(const Scalar& c0,
                                            const Scalar& c1) {
    assert(dimension == 2);
    v_[0] = c0;
    v_[1] = c1;
}

template<class ScalarT, int dimension_t>
inline Vector<ScalarT, dimension_t>::Vector(const Scalar& c0, const Scalar& c1,
                                            const Scalar& c2) {
    assert(dimension == 3);
    v_[0] = c0;
    v_[1] = c1;
    v_[2] = c2;
}

template<class ScalarT, int dimension_t>
inline Vector<ScalarT, dimension_t>::Vector(const Scalar& c0, const Scalar& c1,
                                            const Scalar& c2,
                                            const Scalar& c3) {
    assert(dimension == 4);
    v_[0] = c0;
    v_[1] = c1;
    v_[2] = c2;
    v_[3] = c3;
}

template<class ScalarT, int dimension_t>
inline Vector<ScalarT, dimension_t>::Vector(const Scalar& c0, const Scalar& c1,
                                            const Scalar& c2, const Scalar& c3,
                                            const Scalar& c4) {
    assert(dimension == 5);
    v_[0] = c0;
    v_[1] = c1;
    v_[2] = c2;
    v_[3] = c3;
    v_[4] = c4;
}

template<class ScalarT, int dimension_t>
inline Vector<ScalarT, dimension_t>::Vector(const Scalar& c0, const Scalar& c1,
                                            const Scalar& c2, const Scalar& c3,
                                            const Scalar& c4,
                                            const Scalar& c5) {
    assert(dimension == 6);
    v_[0] = c0;
    v_[1] = c1;
    v_[2] = c2;
    v_[3] = c3;
    v_[4] = c4;
    v_[5] = c5;
}

template<class ScalarT, int dimension_t>
inline Vector<ScalarT, dimension_t>::Vector(const Scalar& c0, const Scalar& c1,
                                            const Scalar& c2, const Scalar& c3,
                                            const Scalar& c4, const Scalar& c5,
                                            const Scalar& c6) {
    assert(dimension == 7);
    v_[0] = c0;
    v_[1] = c1;
    v_[2] = c2;
    v_[3] = c3;
    v_[4] = c4;
    v_[5] = c5;
    v_[6] = c6;
}

template<class ScalarT, int dimension_t>
inline Vector<ScalarT, dimension_t>::Vector(const Self& o) {
    for (int i = 0; i < dimension; ++i) { (*this)[i] = o[i]; }
}

template<class ScalarT, int dimension_t>
Vector<ScalarT, dimension_t> operator*(const ScalarT& o,
                                       const Vector<ScalarT, dimension_t>& v) {
    return v * o;
}

template<class ScalarT, int dimension_t>
inline ScalarT DotProduct(const Vector<ScalarT, dimension_t>& o1,
                          const Vector<ScalarT, dimension_t>& o2) {
    ScalarT ret(0);
    for (int i = 0; i < dimension_t; ++i) { ret += o1[i] * o2[i]; }
    return ret;
}

template<class ScalarT>
inline Vector<ScalarT, 3> CrossProduct(const Vector<ScalarT, 3>& u,
                                       const Vector<ScalarT, 3>& v) {
    //static_assert(std::is_signed<ScalarT>::Scalar, "ScalarT must be a signed type. ");
    Vector<ScalarT, 3> r;
    r[0] = (u[1] * v[2]) - (u[2] * v[1]);
    r[1] = (u[2] * v[0]) - (u[0] * v[2]);
    r[2] = (u[0] * v[1]) - (u[1] * v[0]);
    return r;
}

template<class ScalarT, int dimension_t>
inline std::ostream& operator<<(std::ostream& out,
                                const Vector<ScalarT, dimension_t>& vec) {
    for (int i = 0; i < dimension_t - 1; ++i) { out << vec[i] << " "; }
    out << vec[dimension_t - 1];
    return out;
}

typedef Vector<float, 2> Vector2f;
typedef Vector<float, 3> Vector3f;
typedef Vector<float, 4> Vector4f;
typedef Vector<float, 5> Vector5f;
typedef Vector<float, 6> Vector6f;
typedef Vector<float, 7> Vector7f;

typedef Vector<double, 2> Vector2d;
typedef Vector<double, 3> Vector3d;
typedef Vector<double, 4> Vector4d;
typedef Vector<double, 5> Vector5d;
typedef Vector<double, 6> Vector6d;
typedef Vector<double, 7> Vector7d;

typedef Vector<int, 2> Vector2i;
typedef Vector<int, 3> Vector3i;
typedef Vector<int, 4> Vector4i;
typedef Vector<int, 5> Vector5i;
typedef Vector<int, 6> Vector6i;
typedef Vector<int, 7> Vector7i;

typedef Vector<uint32_t, 2> Vector2ui;
typedef Vector<uint32_t, 3> Vector3ui;
typedef Vector<uint32_t, 4> Vector4ui;
typedef Vector<uint32_t, 5> Vector5ui;
typedef Vector<uint32_t, 6> Vector6ui;
typedef Vector<uint32_t, 7> Vector7ui;

IGAME_NAMESPACE_END
#endif
