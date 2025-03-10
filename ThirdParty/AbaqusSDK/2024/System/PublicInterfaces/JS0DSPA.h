

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0DSPA) || defined(__CATBBMagic) 
# define ExportedByJS0DSPA  DSYExport
#else // __JS0DSPA || __CATBBMagic ||  __SGInfra
# define ExportedByJS0DSPA  DSYImport
#endif  // __JS0DSPA || __CATBBMagic ||  __SGInfra

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"
