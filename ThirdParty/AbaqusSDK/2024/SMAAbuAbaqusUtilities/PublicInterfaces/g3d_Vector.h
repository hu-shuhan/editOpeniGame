//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
//
// File Name: g3d_Vector.h
// 
// Description:
//
//    This file contains the definition of the g3d_Vector class.
//
// The objects of this class are vectors in R^3. 
//
//                     (x, y, z)
//
// They obey the usual vector operations. Vectors (unit vectors) are often
// used to specify normals to a surface for lighting calculations.
//
// Author: Bob Unnold
// 
// Creation date: Fri Oct 21 1994
//


#ifndef g3d_Vector_h
#define g3d_Vector_h

//
// SECTION: Includes
//

// begin local includes
#include <mem_AllocationOperators.h>

#include <omi_types.h>
#include <cow_COW.h>
//#include <cow_List.h>
#include <cls_xo1DArrayList.h>
#include <typ_typeTag.h>
#include <cls_IntCOW.h>



//
// SECTION: Forward declarations
//

class aio_ostream;
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;


//
// SECTION: Class definition
//

class g3d_Vector  : public mem_AllocationOperators
{
    // standard vector operations are defined here.
    friend aio_ostream& operator<<(aio_ostream&, const g3d_Vector&);
    friend float g3d_Dot(const g3d_Vector&, const g3d_Vector&);
    friend float g3d_TripleProduct(const g3d_Vector&, const g3d_Vector&, 
			     const g3d_Vector&);
    friend g3d_Vector g3d_Cross(const g3d_Vector&, const g3d_Vector&);
    friend g3d_Vector operator+(const g3d_Vector&, const g3d_Vector&);
    friend g3d_Vector operator-(const g3d_Vector&, const g3d_Vector&);
    friend g3d_Vector operator*(const g3d_Vector&, float);
    friend g3d_Vector operator*(float, const g3d_Vector&);
    friend g3d_Vector operator/(const g3d_Vector&, float);

  public:

    // By default the vector is initialized to the Zero vector
    g3d_Vector()
        : x(0.f), y(0.f), z(0.f) {}
    g3d_Vector(float _x, float _y, float _z)
        : x(_x), y(_y), z(_z) {}
    g3d_Vector(double _x, double _y, double _z)
      : x( static_cast<float>(_x) ), y( static_cast<float>(_y) ), z( static_cast<float>(_z) ) {}
    g3d_Vector(const g3d_Vector &copy)
        : x(copy.x), y(copy.y), z(copy.z) {}
    ~g3d_Vector() {}

    // Equality tests are exact matches of the coordinates. No tolerance test
    // is made.
    g3d_Vector& operator=(const g3d_Vector&);
    bool operator==(const g3d_Vector&) const;
    bool operator!=(const g3d_Vector& obj) const { return !operator==(obj); }

    // vector operations
    g3d_Vector& operator+=(const g3d_Vector&);
    g3d_Vector& operator-=(const g3d_Vector&);
    g3d_Vector& operator*=(float);
    // WARNING: The following does NOT check the arg against 0.0
    g3d_Vector& operator/=(float);
    g3d_Vector  operator-();

    // Access
    float& X();     float GetX() const;
    float& Y();     float GetY() const;
    float& Z();     float GetZ() const;

    // Setting the components in a single call.
    // Array elements correspond as follows 0->x, 1->y, 2->z.
    g3d_Vector& Set(float x, float y, float z);
    g3d_Vector& Set(double x, double y, double z);
    g3d_Vector& SetV3(const float*);
    g3d_Vector& SetV3(const double*);

    g3d_Vector& SetZero();
    // These next three set the vector to the i,j,k unit vectors
    g3d_Vector& SetI();
    g3d_Vector& SetJ();
    g3d_Vector& SetK();

    // The length is the euclidian length. Normalize turns the vector into
    // a unit vector if the length in not 0.0. Otherwise the object is not
    // changed.
    float Length() const;
    g3d_Vector& Normalize();

    float AngleWith(const g3d_Vector& vect, bool both_normmalized) const;
    float AngleWith(const g3d_Vector& vect, bool both_normmalized, 
		     const g3d_Vector& dir) const;

    // needed for dynamic casting and repository
    typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

public: // Database interface
    g3d_Vector(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    void DBWrite(const cls_WriteVisitor& wv) const;

  private:

    float x;
    float y;
    float z;
};

//COW_LIST_DECL( g3d_Vector, g3d_ListVector );
CLS_xo1DARRAYLIST_DECL( g3d_Vector, g3d_ListVector );

COW_COW_DECL(g3d_Vector);

////////////////////////////
// SECTION: required inlines


inline g3d_Vector&
g3d_Vector::operator=(
    const g3d_Vector& object
)
{
    if (&object == this){
      return *this;
    }

    x = object.x;
    y = object.y;
    z = object.z;
    return *this;
}

inline g3d_Vector&
g3d_Vector::operator+=(
    const g3d_Vector& object
)
{
    x += object.x;
    y += object.y;
    z += object.z;
    return *this;
}

inline g3d_Vector&
g3d_Vector::operator-=(
    const g3d_Vector& object
)
{
    x -= object.x;
    y -= object.y;
    z -= object.z;
    return *this;
}

inline g3d_Vector&
g3d_Vector::operator*=(
    float f
)
{
    x *= f;
    y *= f;
    z *= f;
    return *this;
}

inline g3d_Vector&
g3d_Vector::operator/=(
    float f
)
{
    // DNOTE: WARNING: does not check (f==0.0)
    float fInv = 1.f / f;
    x *= fInv;
    y *= fInv;
    z *= fInv;
    return *this;
}

inline g3d_Vector
g3d_Vector::operator-()
{
    return g3d_Vector(-x, -y, -z);
}


inline float& g3d_Vector::X() { return x; }
inline float& g3d_Vector::Y() { return y; }
inline float& g3d_Vector::Z() { return z; }

inline float g3d_Vector::GetX() const { return x; }
inline float g3d_Vector::GetY() const { return y; }
inline float g3d_Vector::GetZ() const { return z; }

inline g3d_Vector&
g3d_Vector::Set(
    float xin,
    float yin,
    float zin
)
{
    x = xin;
    y = yin;
    z = zin;
    return *this;
}

inline g3d_Vector&
g3d_Vector::Set(
    double xin,
    double yin,
    double zin
)
{
    x = (float) xin;
    y = (float) yin;
    z = (float) zin;
    return *this;
}

inline g3d_Vector&
g3d_Vector::SetV3(
    const float* v
)
{
    x = v[0];
    y = v[1];
    z = v[2];
    return *this;
}

inline g3d_Vector&
g3d_Vector::SetV3(
    const double* v
)
{
    x = (float) v[0];
    y = (float) v[1];
    z = (float) v[2];
    return *this;
}

inline g3d_Vector& g3d_Vector::SetZero() { x=0.0; y=0.0; z=0.0; return *this; }
inline g3d_Vector& g3d_Vector::SetI() { x=1.0; y=0.0; z=0.0; return *this; }
inline g3d_Vector& g3d_Vector::SetJ() { x=0.0; y=1.0; z=0.0; return *this; }
inline g3d_Vector& g3d_Vector::SetK() { x=0.0; y=0.0; z=1.0; return *this; }


inline float
g3d_Dot(
    const g3d_Vector& a,
    const g3d_Vector& b
)
{
    return (a.x*b.x + a.y*b.y + a.z*b.z);
}

#endif  // #ifdef g3d_Vector_h
