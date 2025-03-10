#ifndef VarDesc_h
#define VarDesc_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

#include "CATNTTypes.h"

typedef struct tagVARDESC {
 MEMBERID memid;
 CATLPOLESTR lpstrSchema;
 union {
   ULONG oInst;
   CATVariant *lpvarValue;
 };
 ELEMDESC elemdescVar;
 WORD wVarFlags;
 VARKIND varkind;
} VARDESC;

#endif /* _WINDOWS_SOURCE */

#endif
