#ifndef JS0STDLIB_INCLUDE
#define JS0STDLIB_INCLUDE
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/* COPYRIGHT DASSAULT SYSTEMES 2002 */

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0STDLIB)
# define ExportedByJS0STDLIB  DSYExport
#else /* __JS0STDLIB */
# define ExportedByJS0STDLIB  DSYImport
#endif  /* __JS0STDLIB */

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  /* JS0STDLIB_INCLUDE */
