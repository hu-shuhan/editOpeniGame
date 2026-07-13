#pragma once
#ifndef _CPOINT_H_
#define _CPOINT_H_
#include "CVector.h"
class CPoint : public CGeomPoint {
public:
    CPoint();

    CPoint(double p1, double p2, double p3, int typeCoord = 1,
           int typeAngle = 0);

    CPoint(const CPoint& point);

    virtual ~CPoint();

    bool setTypeCoord(int type);

    int getTypeCoord() const;

public:
    bool setX(double x);

    double getX() const;

    bool setY(double y);

    double getY() const;

    bool setZ(double z);

    double getZ() const;

    bool setXYZ(double x, double y, double z, int iTypeCoord = 1);

    bool getXYZ(double& x, double& y, double& z);

public:
    bool setR(double r);

    double getR(void);

    int getTypeAngle(void);

    bool setTheta(double theta, int typeAngle);

    double getTheta(void);

    bool setPolar(double r, double theta, double z, int typeAngle);

    bool getPolar(double& r, double& theta, double& z, int& typeAngle);

public:
    CPoint operator+(const CPoint& point);

    CPoint operator-(const CPoint& point);

    CPoint operator*(const CPoint& point);

    CPoint operator/(const CPoint& point);

    CPoint operator+(const CVector& vec);

    CPoint operator*(const double d);
    bool operator==(const CPoint& point);

public:
    double norm(void) const;

    bool neg(void);

    void normaliz();

    void rotate(CGeom& line, double angle, int typeAngle, CGeomPoint& point);

    void getPerpendicular(CGeomPoint& point);

    void getMidPoint(CPoint& pa, CPoint& pb, CPoint& mpoint);

    void getSymmetryPoint(CPoint& p, CPoint& sp, CPoint& newpoint);

    double getDistance(const CPoint& p);

public:
    double m_dX;  
    double m_dY;  
    double m_dZ;  
private:
    inline void computePolar();
    inline void computeCartesian();

private:
    double m_dR;       
    double m_dTheta;  
    int m_iTypeCoord;          
    int m_iTypeAngle;         
};

#endif  