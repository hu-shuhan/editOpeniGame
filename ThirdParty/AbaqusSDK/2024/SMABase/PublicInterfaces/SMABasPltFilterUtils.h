//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */

#ifndef SMABasPltFilterUtils_h
#define SMABasPltFilterUtils_h

// Begin local includes
#include <BasicConstants.h>

// Forward Declarations
class omi_BlkBitVect;

enum SMABasPlt_InterpolateEnum
{
    SMABasPlt_LINEAR_INT,
    SMABasPlt_QUAD_INT,
    SMABasPlt_SPLINE_INT
};

enum SMABasPlt_EndConditionEnum
{
    SMABasPlt_ZERO,
    SMABasPlt_CONSTANT,
    SMABasPlt_MIRROR,
    SMABasPlt_REVERSE_MIRROR,
    SMABasPlt_TANGENTIAL
};

enum SMABasPlt_OperationEnum
{
    SMABasPlt_BUTTERWORTH,
    SMABasPlt_CHEBYSHEV_1,
    SMABasPlt_CHEBYSHEV_2
};

double SMABasPlt_getFilterTimeInc(const float *origX,
                                  const int origLen,
                                  const omi_BlkBitVect &dupLoc,
                                  const double tolerance,
                                  bool &requireInterp,
                                  double &errorLoc);

void SMABasPlt_filterInterpolate_lin(const float *origX,
                                     const float *origY,
                                     const int origLen,
                                     const omi_BlkBitVect &dupLoc,
                                     const double dt,
                                     const int extraFilterPts,
                                     float *&retX,
                                     float *&retY,
                                     const int retLen);

void SMABasPlt_filterInterpolate_sp(const float *origX,
                                    const float *origY,
                                    const int origLen,
                                    const omi_BlkBitVect &dupLoc,
                                    const double dt,
                                    const int extraFilterPts,
                                    const float deriv0,
                                    const float derivN,
                                    float *&retX,
                                    float *&retY,
                                    const int retLen);

void SMABasPlt_filterInterpolate_quad(const float *origX,
                                      const float *origY,
                                      const int origLen,
                                      const omi_BlkBitVect &dupLoc,
                                      const double dt,
                                      const int extraFilterPts,
                                      float *&retX,
                                      float *&retY,
                                      const int retLen);

void SMABasPlt_iirFilter(float *&retX,
                         float *&retY,
                         const int retLen,
                         const int extraFilterPts,
                         const float fcut,
                         const double dt,
                         const int nop,
                         const float ripple,
                         SMABasPlt_EndConditionEnum startC,
                         SMABasPlt_EndConditionEnum endC,
                         SMABasPlt_OperationEnum oper,
                         const bool backwardPass);

void SMABasPlt_saeFilter(float *&retX,
                         float *&retY,
                         const int retLen,
                         const int extraFilterPts,
                         const float fcut,
                         const double dt,
                         SMABasPlt_EndConditionEnum startC,
                         SMABasPlt_EndConditionEnum endC,
                         const bool backwardPass);

bool SMABasPlt_sbFilter_dep(float *&retY,
                            const int retLen,
                            const float fcut,
                            const double dtopt,
                            const int nop);

bool SMABasPlt_sbFilter(float *&retX,
                        float *&retY,
                        const int retLen,
                        const int extraFilterPts,
                        const float fcut,
                        const double dt,
                        const int nop,
                        SMABasPlt_EndConditionEnum startC,
                        SMABasPlt_EndConditionEnum endC,
                        const bool backwardPass);

#endif
