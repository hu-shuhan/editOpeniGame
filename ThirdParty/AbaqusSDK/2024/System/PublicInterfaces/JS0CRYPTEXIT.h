// COPYRIGHT DASSAULT SYSTEMES 2004
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0CRYPTEXIT)
# define ExportedByJS0CRYPTEXIT   DSYExport
#else // __JS0CRYPTEXIT
# define ExportedByJS0CRYPTEXIT   DSYImport
#endif  // __JS0CRYPTEXIT

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"
