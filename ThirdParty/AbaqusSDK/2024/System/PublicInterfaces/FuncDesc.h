#ifndef _funcDesc_h
#define _funcDesc_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

// Use the header of System
#include "IUnknown.h"

#ifndef _WINDOWS_SOURCE
#include "IUnknown.h"
typedef LONG SCODE;

typedef struct tagFUNCDESC {
 MEMBERID memid;
 SCODE *lprgscode;
 ELEMDESC *lprgelemdescParam;
 FUNCKIND funckind;
 INVOKEKIND invkind;
 CALLCONV callconv;
 short cParams;
 short cParamsOpt;
 short oVft;
 short cScodes;
 ELEMDESC elemdescFunc;
 WORD wFuncFlags;
} FUNCDESC;

#endif /* _WINDOWS_SOURCE */

#endif
