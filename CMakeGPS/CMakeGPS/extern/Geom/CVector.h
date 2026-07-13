#pragma once
#ifndef _CVECTOR_H_
#define _CVECTOR_H_
#include "CGeomPoint.h"
class CVector : public CGeomPoint {
public:
    CVector();

    CVector(double x, double y, double z, int typeCoord = 1);

    CVector(const CVector& vec);

    virtual ~CVector();

public:
    bool setX(double x);

    virtual double getX() const override;

    bool setY(double y);

    virtual double getY() const override;

    bool setZ(double z);

    bool setTypeCoord(int type);

    virtual double getZ() const override;

    bool setXYZ(double x, double y, double z, int iTypeCoord = 1);

    bool getXYZ(double& x, double& y, double& z);

    int getTypeCoord();

public:
    CVector operator%(const CVector& point);
    CVector operator*(double t);
    CVector operator/(int num);
    CVector operator+(const CVector& point);

public:
    void normalize();

    void getPerpendicular(CGeomPoint&);

public:

public:
private:
    double m_dX;       
    double m_dY;       
    double m_dZ;       
    int m_iTypeCoord;  
};
#endif  
