

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__CO0RCFLT)
# define ExportedByCO0RCFLT   DSYExport
#else // __CO0RCFLT
# define ExportedByCO0RCFLT   DSYImport
#endif  // __CO0RCFLT

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"
