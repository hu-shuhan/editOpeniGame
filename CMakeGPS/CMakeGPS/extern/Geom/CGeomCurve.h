#pragma once
#ifndef _CGEOMCURVE_H_
#define _CGEOMCURVE_H_
#include "CGeom.h"
#include "CGeomPoint.h"

class CGeomCurve : public CGeom
{
public:
	CGeomCurve();
	virtual ~CGeomCurve();

	virtual void getNormal(CGeomPoint &) {}
	virtual void getPoint(CGeomPoint &) {}

	virtual int getCurveType() { return 0; }
};

#endif  