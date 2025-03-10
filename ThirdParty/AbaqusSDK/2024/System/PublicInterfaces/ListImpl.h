

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__ListImpl)
# define ExportedByListImpl   DSYExport
#else // __ListImpl
# define ExportedByListImpl   DSYImport
#endif  // __ListImpl

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"
