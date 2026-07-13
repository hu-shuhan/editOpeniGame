#include "CBSplineSurface.h"
#include "DefineType.h"
CBSplineSurface::CBSplineSurface() {
    m_iUDegree = 0;
    m_iVDegree = 0;
    m_iTypeCoord = 1;
    m_dArea = 0;

    m_ControlPoints.clear();
    m_ScalarPoints.clear();

    m_UKnots.clear();
    m_VKnots.clear();
    m_UMultis.clear();
    m_VMultis.clear();
    m_Weights.clear();
    m_bUPeriodic = false;
    m_bVPeriodic = false;
    m_bRational = false;
}


CBSplineSurface::~CBSplineSurface() {
    m_ControlPoints.clear();
    m_ScalarPoints.clear();

    m_UKnots.clear();
    m_VKnots.clear();
    m_UMultis.clear();
    m_VMultis.clear();
    m_Weights.clear();
}

CBSplineSurface::CBSplineSurface(const CBSplineSurface& splineSurface) {
    m_iUDegree = splineSurface.m_iUDegree;
    m_iVDegree = splineSurface.m_iVDegree;
    m_iTypeCoord = 1;
    m_ControlPoints = splineSurface.m_ControlPoints;
    m_ScalarPoints = splineSurface.m_ScalarPoints;

    m_UKnots = splineSurface.m_UKnots;
    m_VKnots = splineSurface.m_VKnots;
    m_UMultis = splineSurface.m_UMultis;
    m_VMultis = splineSurface.m_VMultis;
    m_bRational = splineSurface.m_bRational;
    m_bUPeriodic = splineSurface.m_bUPeriodic;
    m_bVPeriodic = splineSurface.m_bVPeriodic;

    m_Weights = splineSurface.m_Weights;

    m_bCCW = splineSurface.m_bCCW;

    createBSplineSurface();
}

CBSplineSurface::CBSplineSurface(const int& udegree, const int& vdegree,
                                 const vector<vector<CPoint>>& scalarPoints,
                                 const vector<vector<CPoint>>& controlPoints,
                                 const vector<double>& uknots,
                                 const vector<double>& vknots,
                                 const vector<int>& umultis,
                                 const vector<int>& vmultis, bool uperiodic,
                                 bool vperiodic) {
    m_iUDegree = udegree;
    m_iVDegree = vdegree;
    m_iTypeCoord = 1;
    m_ControlPoints = controlPoints;
    m_ScalarPoints = scalarPoints;
    m_UKnots = uknots;
    m_VKnots = vknots;
    m_UMultis = umultis;
    m_VMultis = vmultis;
    m_bRational = false;
    m_bUPeriodic = uperiodic;
    m_bVPeriodic = vperiodic;

    createBSplineSurface();
}

CBSplineSurface::CBSplineSurface(const int& udegree, const int& vdegree,
                                 const vector<vector<CPoint>>& controlPoints,
                                 const vector<double>& uknots,
                                 const vector<double>& vknots,
                                 const vector<int>& umultis,
                                 const vector<int>& vmultis, bool uperiodic,
                                 bool vperiodic) {
    m_iUDegree = udegree;
    m_iVDegree = vdegree;
    m_iTypeCoord = 1;
    m_ControlPoints = controlPoints;
    m_UKnots = uknots;
    m_VKnots = vknots;
    m_UMultis = umultis;
    m_VMultis = vmultis;
    m_bRational = false;
    m_bUPeriodic = uperiodic;
    m_bVPeriodic = vperiodic;

    createBSplineSurface();
}

CBSplineSurface::CBSplineSurface(const int& udegree, const int& vdegree,
                                 const vector<vector<CPoint>>& controlPoints,
                                 const vector<double>& uknots,
                                 const vector<double>& vknots,
                                 const vector<int>& umultis,
                                 const vector<int>& vmultis,
                                 const vector<vector<double>>& weights,
                                 bool uperiodic, bool vperiodic) {
    m_iUDegree = udegree;
    m_iVDegree = vdegree;
    m_iTypeCoord = 1;
    m_ControlPoints = controlPoints;
    m_UKnots = uknots;
    m_VKnots = vknots;
    m_UMultis = umultis;
    m_VMultis = vmultis;
    m_Weights = weights;
    m_bRational = true;
    m_bUPeriodic = uperiodic;
    m_bVPeriodic = vperiodic;

    createBSplineSurface();
}

vector<vector<CPoint>> CBSplineSurface::getControlPoints() const {
    return m_ControlPoints;
}
vector<vector<CPoint>> CBSplineSurface::getScalarPoints() const {
    return m_ScalarPoints;
}
bool CBSplineSurface::createBSplineSurface() {
    if (m_ControlPoints.size() == 0 || m_ControlPoints[0].size() == 0 ||
        m_UKnots.size() == 0 || m_UMultis.size() == 0 || m_VKnots.size() == 0 ||
        m_VMultis.size() == 0) {
        return false;
    }
    size_t sizeUControlPoints = (m_ControlPoints).size();
    size_t sizeVControlPoints = (m_ControlPoints[0]).size();

    size_t sizeUKonts = (m_UKnots).size();
    size_t sizeUMultis = (m_UMultis).size();
    size_t sizeVKonts = (m_VKnots).size();
    size_t sizeVMultis = (m_VMultis).size();
    size_t sizeUWeights = 0, sizeVWeights = 0;




    if (m_bRational) {
    } else {
    }
}
