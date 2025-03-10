#ifndef JS0SETT_H_
#define JS0SETT_H_

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined (__JS0SETT)
#define ExportedByJS0SETT DSYExport
#else
#define ExportedByJS0SETT DSYImport
#endif

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  // JS0SETT_H_
