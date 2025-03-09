// COPYRIGHT DASSAULT SYSTEMES 2003
#ifndef CATERRPARAMS_INCLUDE
#define CATERRPARAMS_INCLUDE
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// Copyright DASSAULT SYSTEMS 1996
//-----------------------------------------------------------------------------
// Abstract:	Error parameters
//-----------------------------------------------------------------------------
// Usage:	Only used by CATError class
//-----------------------------------------------------------------------------
#include "JS0ERROR.h"

//
// Maximum number for each type of parameters
//
#define CATERROR_MAX_INT_PARAMS		12
#define CATERROR_MAX_REAL_PARAMS	12
#define CATERROR_MAX_CHARS_PARAMS	5

//
// A block of parameters
//
class ExportedByJS0ERROR CATErrParams {

  public:
    CATErrParams ();
    ~CATErrParams ();
    void Set (int numInt, const int *);
    void Set (int numReal, const double *);
    void Set (int numChars, const char **);

    int         Is[CATERROR_MAX_INT_PARAMS];
    double      Rs[CATERROR_MAX_REAL_PARAMS];
    char       *Cs[CATERROR_MAX_CHARS_PARAMS];
    int         numIs;
    int         numRs;
    int         numCs;
};

#endif
