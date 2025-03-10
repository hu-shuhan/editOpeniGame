#ifndef ExportedByConnectionPts


// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__ConnectionPtsImpl)
# define ExportedByConnectionPts  DSYExport
#else // __ConnectionPtsImpl
# define ExportedByConnectionPts  DSYImport
#endif  // __ConnectionPtsImpl

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif
