#ifndef JS0COMP_H_
#define JS0COMP_H_

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0COMP)
# define ExportedByJS0COMP  DSYExport
#else  // __JS0COMP
# define ExportedByJS0COMP  DSYImport
#endif // __JS0COMP

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  // JS0COMP_H_
