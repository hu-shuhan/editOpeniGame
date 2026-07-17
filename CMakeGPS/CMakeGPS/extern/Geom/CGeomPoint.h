#pragma once
#ifndef _CGEOMPOINT_H_
#define _CGEOMPOINT_H_
#include "CGeom.h"
#define PI  (acos(-1.0))

class CGeomPoint : public CGeom
{
public:
	CGeomPoint();
	virtual ~CGeomPoint();

	virtual double getX() const { return 0.0; };
	virtual double getY() const { return 0.0; };
	virtual double getZ() const { return 0.0; };
	virtual bool getXYZ(double &x, double &y, double &z) { return false; };
	virtual void getPerpendicular(CGeomPoint &) {};
	virtual bool setXYZ(double x, double y, double z, int iTypeCoord = 1) { return false; };
};
#endif  
