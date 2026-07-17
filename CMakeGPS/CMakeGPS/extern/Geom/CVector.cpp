#include "CVector.h"
#include <cmath>

CVector::CVector() {
    m_dX = 0;
    m_dY = 0;
    m_dZ = 0;
    m_iTypeCoord = 1;
}

CVector::CVector(double x, double y, double z, int typeCoord) {
    m_dX = x;
    m_dY = y;
    m_dZ = z;
    m_iTypeCoord = typeCoord;
}

CVector::CVector(const CVector& vec) {
    const_cast<CVector&>(vec).getXYZ(m_dX, m_dY, m_dZ);
    m_iTypeCoord = 1;
    m_iTypeCoord = 1;
}


CVector::~CVector() {}

bool CVector::setX(double x) {
    m_dX = x;
    return true;
}

double CVector::getX() const { return m_dX; }

bool CVector::setY(double y) {
    m_dY = y;
    return true;
}

double CVector::getY() const { return m_dY; }

bool CVector::setZ(double z) {
    m_dZ = z;
    return true;
}

bool CVector::setTypeCoord(int type) {
    m_iTypeCoord = type;
    return true;
}

double CVector::getZ() const { return m_dZ; }

bool CVector::setXYZ(double x, double y, double z, int iTypeCoord) {
    m_dX = x;
    m_dY = y;
    m_dZ = z;
    m_iTypeCoord = iTypeCoord;
    return true;
}

bool CVector::getXYZ(double& x, double& y, double& z) {
    x = m_dX;
    y = m_dY;
    z = m_dZ;
    return true;
}

int CVector::getTypeCoord() { return m_iTypeCoord; }
CVector CVector::operator%(const CVector& point) {
    double x = m_dX / const_cast<CVector&>(point).getX();
    double y = m_dY / const_cast<CVector&>(point).getY();
    double z = m_dZ / const_cast<CVector&>(point).getZ();
    CVector finalPoint(x, y, z);
    return finalPoint;
}
void CVector::normalize() {
    double d = sqrt(m_dX * m_dX + m_dY * m_dY + m_dZ * m_dZ);
    m_dX = m_dX / d;
    m_dY = m_dY / d;
    m_dZ = m_dZ / d;
}

void CVector::getPerpendicular(CGeomPoint& vec) {
    if (fabs(m_dX) > 1e-8 || fabs(m_dY) > 1e-8) {
        vec.setXYZ(-m_dY, m_dX, 0);
    } else {
        vec.setXYZ(-m_dZ, 0, m_dX);
    }
}

CVector CVector::operator*(double t) {
    double x = m_dX * t;
    double y = m_dY * t;
    double z = m_dZ * t;
    CVector finalVector(x, y, z);
    return finalVector;
}

CVector CVector::operator/(int num) {
    double x = m_dX / num;
    double y = m_dY / num;
    double z = m_dZ / num;
    CVector finalVector(x, y, z);
    return finalVector;
    return CVector();
}

CVector CVector::operator+(const CVector& point) {
    double x = m_dX + const_cast<CVector&>(point).getX();
    double y = m_dY + const_cast<CVector&>(point).getY();
    double z = m_dZ + const_cast<CVector&>(point).getZ();
    CVector finalPoint(x, y, z);
    return finalPoint;
}

