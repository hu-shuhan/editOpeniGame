#ifndef _ElemDesc_h
#define _ElemDesc_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

#ifndef CATMAINWIN

typedef struct  tagELEMDESC
    {
    PARAMDESC paramdesc;
    TYPEDESC tdesc;
    }	ELEMDESC;

#else /* CATMAINWIN */

#include "IdlDesc.h"

typedef struct tagELEMDESC {
 TYPEDESC tdesc;
 union {
   IDLDESC idldesc;
   PARAMDESC paramdesc;
 };
} ELEMDESC, * LPELEMDESC;

#endif /* CATMAINWIN */

#endif
#endif
