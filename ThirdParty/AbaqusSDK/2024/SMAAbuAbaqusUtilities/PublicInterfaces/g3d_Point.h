//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: g3d_Point.h
// 
// Description:
//
//    This file contains the definition of the g3d_Point class.
//
// The objects of this class are points in homogeneous coordinates
//
//                         (x, y, z, w)
//
// where w is called the homogeneous coordinate. Typically the w coordinate
// is set to 1.0 for which the x,y and z coordinates then correspond to 3D
// space.
//
// These objects are core objects of 3d graphics. All geometry is specified
// via g3d_Points.
//
// Author: Bob Unnold
// 
// Creation date: Fri Oct 21 1994
//


#ifndef g3d_Point_h
#define g3d_Point_h

//
// SECTION: Includes
//

// begin local includes
#include <mem_AllocationOperators.h>

#include <omi_types.h>
#include <cow_COW.h>
#include <cow_List.h>
#include <g3d_Vector.h>
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

class g3d_Point  : public mem_AllocationOperators
{
    friend aio_ostream& operator<<(aio_ostream&, const g3d_Point&);

  public:

    // Default x=0.0, y=0.0, z=0.0, w=1.0.
    g3d_Point()
        : x(0.f), y(0.f), z(0.f), w(1.f) {}
    g3d_Point(float _x, float _y, float _z, float _w = 1.f)
        : x(_x), y(_y), z(_z), w(_w) {}
    g3d_Point(double _x, double _y, double _z, double _w = 1.0)
        : x( static_cast<float>(_x) ), y( static_cast<float>(_y) ), z( static_cast<float>(_z) ), w( static_cast<float>(_w) ) {}
    ~g3d_Point() {}

    // The equality tests are exact matches of the coordinates. No tolerance
    // test is made.
    bool operator==(const g3d_Point&) const;
    bool operator!=(const g3d_Point& obj) const { return !operator==(obj); }

    // Access methods
    float& X();     float GetX() const;
    float& Y();     float GetY() const;
    float& Z();     float GetZ() const;
    float& W();     float GetW() const;

    // Various utilities to set the coord of the point in one call.
    // Array elements correspond as follows 0->x, 1->y, 2->z, 3->w.
    // The "SetV3" methods set w = 1.0. 
    g3d_Point& Set(float x, float y, float z, float w = 1.f);
    g3d_Point& Set(double x, double y, double z, double w = 1.0);
    g3d_Point& SetV3(const float*);
    g3d_Point& SetV3(const double*);
    g3d_Point& SetV4(const float*);
    g3d_Point& SetV4(const double*);

    // This method performs a divide by w if w != 0.0. Otherwise the object
    // is not changed.
    g3d_Point& HNormalize();

    // Operations  (NOTE: Operations with vectors impose HNormalize)
    g3d_Point operator+(const g3d_Vector &v) const;
    void operator+=(const g3d_Vector &v);
    g3d_Point operator-(const g3d_Vector &v) const;
    void operator-=(const g3d_Vector &v);
    g3d_Point operator-();

    // needed for dynamic casting and repository
    typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

public: // Database interface
    g3d_Point(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    void DBWrite(const cls_WriteVisitor& wv) const;


  private:
    
    float x;
    float y;
    float z;
    float w;
};

COW_LIST_DECL( g3d_Point, g3d_ListPoint );

COW_COW_DECL(g3d_Point);

/////////////////////////////
// SECTION: Required inlines

inline float& g3d_Point::X() { return x; }
inline float& g3d_Point::Y() { return y; }
inline float& g3d_Point::Z() { return z; }
inline float& g3d_Point::W() { return w; }

inline float g3d_Point::GetX() const { return x; }
inline float g3d_Point::GetY() const { return y; }
inline float g3d_Point::GetZ() const { return z; }
inline float g3d_Point::GetW() const { return w; }

inline g3d_Point&
g3d_Point::Set(
    float xin,
    float yin,
    float zin,
    float win
)
{
    x = xin; 
    y = yin; 
    z = zin; 
    w = win;
    return *this;
}

inline g3d_Point&
g3d_Point::Set(
    double xin,
    double yin,
    double zin,
    double win
)
{
    x = (float) xin;
    y = (float) yin;
    z = (float) zin;
    w = (float) win;
    return *this;
}

inline g3d_Point&
g3d_Point::SetV3(
    const float *v
)
{
    x = v[0];
    y = v[1];
    z = v[2];
    w = 1.0;
    return *this;
}

inline g3d_Point&
g3d_Point::SetV3(
    const double *v
)
{
    x = (float) v[0];
    y = (float) v[1];
    z = (float) v[2];
    w = 1.f;
    return *this;
}

inline g3d_Point&
g3d_Point::SetV4(
    const float *v
)
{
    x = v[0];
    y = v[1];
    z = v[2];
    w = v[3];
    return *this;
}

inline g3d_Point&
g3d_Point::SetV4(
    const double *v
)
{
    x = (float) v[0];
    y = (float) v[1];
    z = (float) v[2];
    w = (float) v[3];
    return *this;
}

inline g3d_Point&
g3d_Point::HNormalize()
{
    if(  w != 0.f  &&  w != 1.f  )
    {
    	float rInvW = 1.f / w;
	x *= rInvW;
	y *= rInvW;
	z *= rInvW;
	w = 1.f;
    }
    return *this;
}


inline void
g3d_Point::operator+=(const g3d_Vector &v)
{ HNormalize(); x+=v.GetX(); y+=v.GetY(); z+=v.GetZ(); }

inline g3d_Point
g3d_Point::operator+(const g3d_Vector &v) const
{ g3d_Point out(*this); out += v; return out; }

inline void
g3d_Point::operator-=(const g3d_Vector &v)
{ HNormalize(); x-=v.GetX(); y-=v.GetY(); z-=v.GetZ(); }

inline g3d_Point
g3d_Point::operator-(const g3d_Vector &v) const
{ g3d_Point out(*this); out -= v; return out; }

inline g3d_Point
g3d_Point::operator-()
{ return g3d_Point(-x,-y,-z,w); }


#endif  // #ifdef g3d_Point_h
