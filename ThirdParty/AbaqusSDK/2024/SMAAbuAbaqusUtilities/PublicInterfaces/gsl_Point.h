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
// File Name: gsl_Point.h
// 
// Description:
//
//    This file contains the defintion of the gsl_Point class.
//
// Author: Julio Carrera
// 
// Creation date: Thu May 20 1993
//


#ifndef gsl_Point_h
#define gsl_Point_h

//
// Includes
//


// Begin local includes
#include <mem_AllocationOperators.h>

#include <omi_types.h>
#include <typ_typeTag.h>
#include <cow_COW.h>
#include <gsl_TypeTable.h>
#include <cls_xo1DArrayList.h>
#include <cls_IntCOW.h>
#include <cls_Uid.h>

class aio_ostream;
class gsl_Matrix;
class gsl_StLine;
class gsl_Segment;
class gsl_Vector;
class gsl_Plane;
class cls_ReadVisitor;
class cls_WriteVisitor;
class cls_ClassRegistrar;

//
// Constants
//

//
// Class definition
//
class gsl_Point  : public mem_AllocationOperators
{
public:
// Constructors, Destructor

    gsl_Point(): _x(0.0),_y(0.0),_z(0.0){}

    gsl_Point(const double x, const double y, const double z)
      :_x(x),_y(y),_z(z){}

    /// At midpoint of 2 points
    gsl_Point(const gsl_Point& p1, const gsl_Point& p2);

    gsl_Point(const gsl_Point& p1, double coeff1, 
	      const gsl_Point& p2, double coeff2);

    // needed for messaging

// Operators 

    bool operator==(const gsl_Point&) const;
    bool operator!=(const gsl_Point&) const;
    bool operator>(const gsl_Point&) const;
    bool operator>=(const gsl_Point&) const;
    void operator-() { _x = -_x ; _y = -_y; _z = -_z; } 
  gsl_Point operator+(const gsl_Vector& vec) const;

  gsl_Point operator-(const gsl_Vector& vec) const;

  gsl_Vector operator-(const gsl_Point& point) const;

   double operator[](uint index) const
    {
	if (index == 0)return _x;
	else if (index == 1)return _y;
	else return _z;
    }

    double& operator[](uint index)
    {
	if (index == 0)return _x;
	else if (index == 1)return _y;
	else return _z;
    }


// Member functions
    void SetPos (const gsl_Point& point) {
        _x =point._x; _y =point._y; _z =point._z;
    }
    double& X () { return _x; }
    double& Y () { return _y; }
    double& Z () { return _z; }

    double GetX () const { return _x; }
    double GetY () const { return _y; }
    double GetZ () const { return _z; }

    double GetDistance(const gsl_Point&) const;
    double GetDistance(const gsl_StLine&) const;
    double GetDistance(const gsl_Segment&, gsl_Point& res) const;
    double GetDistance(const gsl_Plane&) const;
    double GetSignedDistance(const gsl_Plane&) const;
    double GetDistance(const gsl_Plane&, const gsl_Vector& dir) const;

    double SquaredDistance(const gsl_Point&) const;

    /// Get distance to a triangle
    double GetDistance(const gsl_Point&, const gsl_Point&, 
			const gsl_Point&) const;
    bool IsOn(const gsl_Point&) const;
    bool IsOn(const gsl_StLine&) const;
    bool IsOn(const gsl_Plane&) const;
    bool IsOn(const gsl_Point& pt, double& oDistance) const;

    gsl_Point PointAtDistInDirection(double iDistance, const gsl_Vector& iDir) const;

    // parm is the signed distance from PointOn() to result
    // which is equivalent to parametrization with Direction() being uint
    gsl_Point ProjectOn(const gsl_StLine&, double& parm) const;

    ///Project on segment along direction dir, returns FALSE if no intersecting
    bool RayIntersect(const gsl_Point& p1, const gsl_Point& p2,
                      const gsl_Vector& dir, // direction
                      gsl_Point& res, // result
                      double bary[2]) const; // barycentric coords of res

    bool RayIntersect(const gsl_Vector&, // direction
		      const gsl_Plane&,	//plane
		      gsl_Point&) const; // result

    gsl_Point ProjectOn(const gsl_Plane&) const;

    /// Returns TRUE if on segment
    bool ProjectOn(const gsl_Segment&, // segment
		   gsl_Point&, double& parm) const;    // result pnt and parm

    bool ProjectOn(const gsl_Segment&, double tol, 
                   gsl_Point&, double& parm) const;    // result pnt and parm

    /// Returns TRUE if on triangle, bary is barycentric coords of res
    bool ProjectOn(const gsl_Point& p1, const gsl_Point& p2, // segment
		   const gsl_Point& p3, gsl_Point& res, 
		   double bary[]) const;    

    void Transform (const gsl_Matrix&);
    void Draw () const;
    void Print (aio_ostream&, int = 0) const;

    typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId() {  return TYP_TIX_gsl_Point; }


public: // Database interface
    gsl_Point(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    static void InitializeMetadata(cls_ClassRegistrar& reg);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;

protected:

    double _x, _y, _z;

private:
    cls_Uid m_ClsUid;
    
public:
    // Finds barycentric coords of this on segment p1, p2
    bool Bary(const gsl_Point& p1, const gsl_Point& p2, double bary[2]) const;

    // Finds barycentric coords of this on triangle p1, p2, p3
    bool Bary(const gsl_Point& p1, const gsl_Point& p2, const gsl_Point& p3, 
	      double bary[3]) const;

};

CLS_xo1DARRAYLIST_DECL(gsl_Point, gsl_ArrayPoint);



COW_COW_DECL(gsl_Point);

#endif  // #ifdef Point_h
