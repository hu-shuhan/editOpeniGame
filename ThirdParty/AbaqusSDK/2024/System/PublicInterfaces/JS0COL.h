

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0COL)
# define ExportedByJS0COL   DSYExport
#else // __JS0COL
# define ExportedByJS0COL   DSYImport
#endif  // __JS0COL

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"
