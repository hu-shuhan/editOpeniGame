#ifndef JS0LIB0_INCLUDE
#define JS0LIB0_INCLUDE


/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/
// COPYRIGHT DASSAULT SYSTEMES 2000

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0LIB0)
# define ExportedByJS0LIB0  DSYExport
#else // __JS0LIB0
# define ExportedByJS0LIB0  DSYImport
#endif  // __JS0LIB0

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  // JS0LIB0_INCLUDE
