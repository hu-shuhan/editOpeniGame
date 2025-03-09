//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */

#ifndef udd_Enums_h
#define udd_Enums_h

// Begin local includes
#include <omu_PrimEnum.h>
#include <omu_PrimTuple.h>
#include <SMABasPltFilterUtils.h>


// SECTION: Enums

enum  udd_XYSourceTypeEnm {
    udd_FROM_ODB = 0,
    udd_FROM_ASCII_FILE,
    udd_FROM_KEYBOARD,
    udd_FROM_OPERATION,
    udd_FROM_USER_DEFINED   
};

enum udd_XYOperations
{
    udd_ADD,
    udd_SUB,
    udd_MUL,
    udd_DIV,
    udd_MAX,
    udd_MIN,
    udd_RNG,
    udd_AVG,
    udd_POW,
    udd_COM,
    udd_SRSS
};

typedef omu_PrimEnum<udd_XYSourceTypeEnm,1> udd_xySourceTypeEnumConst;
typedef omu_PrimEnum<SMABasPlt_InterpolateEnum,2> udd_interpolateConst;
typedef omu_PrimEnum<SMABasPlt_EndConditionEnum,2> udd_endConditionConst;


#endif
