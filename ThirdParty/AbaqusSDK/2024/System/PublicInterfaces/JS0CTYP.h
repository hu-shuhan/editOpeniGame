

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0CTYP)
# define ExportedByJS0CTYP  DSYExport
#else // __JS0CTYP
# define ExportedByJS0CTYP  DSYImport
#endif  // __JS0CTYP

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"
