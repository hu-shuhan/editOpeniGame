

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__CO0LSTST)
# define ExportedByCO0LSTST   DSYExport
#else // __CO0LSTST
# define ExportedByCO0LSTST   DSYImport
#endif  // __CO0LSTST

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"
