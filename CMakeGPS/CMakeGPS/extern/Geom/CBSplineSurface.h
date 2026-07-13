#pragma once
#include "CGeomSurface.h"
#include "CPoint.h"
#include "nrbNurbs.h"
#include <memory>
#include <vector>
class CBSplineSurface : public CGeomSurface {
    friend class CAidFunction;
    friend class BladeExterior;
    friend class BladeInterior;
    friend class Film_hole;
    friend class Partition;
    friend class TrailingCrack;
    friend class Turb_column;

public:
    CBSplineSurface();
    virtual ~CBSplineSurface();

    CBSplineSurface(const CBSplineSurface& splineSurface);

    CBSplineSurface(const int& udegree, const int& vdegree,
                    const vector<vector<CPoint>>& controlPoints,
                    const vector<double>& uknots, const vector<double>& vknots,
                    const vector<int>& umultis, const vector<int>& vmultis,
                    bool uperiodic = false, bool vperiodic = false);

    CBSplineSurface(const int& udegree, const int& vdegree,
                    const vector<vector<CPoint>>& scalarPoints,
                    const vector<vector<CPoint>>& controlPoints,
                    const vector<double>& uknots, const vector<double>& vknots,
                    const vector<int>& umultis, const vector<int>& vmultis,
                    bool uperiodic = false, bool vperiodic = false);


    CBSplineSurface(const int& udegree, const int& vdegree,
                    const vector<vector<CPoint>>& controlPoints,
                    const vector<double>& uknots, const vector<double>& vknots,
                    const vector<int>& umultis, const vector<int>& vmultis,
                    const vector<vector<double>>& weights,
                    bool uperiodic = false, bool vperiodic = false);

public:
public:
public:
    vector<vector<CPoint>> getControlPoints() const;

    vector<vector<CPoint>> getScalarPoints() const;

    int getUDegree() const;

    int getVDegree() const;

    bool getUClosed() const;

    bool getVClosed() const;

    bool getUPeriodic() const;

    bool getVPeriodic() const;

public:
public:
    bool m_bCCW = true;      

protected:
private:
    bool createBSplineSurface();

public:
    int m_iTypeCoord;
    double m_dArea;
    bool m_bRational;
    bool m_bUPeriodic;
    bool m_bVPeriodic;
    vector<vector<CPoint>> m_ControlPoints;
    vector<vector<CPoint>> m_ScalarPoints;

    vector<double> m_UKnots;
    vector<double> m_VKnots;
    vector<vector<double>> m_Weights;
    vector<int> m_UMultis;
    vector<int> m_VMultis;

    int m_iUDegree;
    int m_iVDegree;
};
