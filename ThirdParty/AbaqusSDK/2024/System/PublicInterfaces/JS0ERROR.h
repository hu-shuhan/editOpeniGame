#ifndef JS0ERROR_INCLUDE
#define JS0ERROR_INCLUDE

/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0ERROR)
# define ExportedByJS0ERROR   DSYExport
#else // __JS0ERROR
# define ExportedByJS0ERROR   DSYImport
#endif  // __JS0ERROR

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  // JS0ERROR_INCLUDE
