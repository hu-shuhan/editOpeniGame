#ifndef JS0SCBAK_H
#define JS0SCBAK_H


// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0SCBAK)
# define ExportedByJS0SCBAK   DSYExport
#else // __JS0SCBAK
# define ExportedByJS0SCBAK   DSYImport
#endif  // __JS0SCBAK

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  // JS0SCBAK_H
