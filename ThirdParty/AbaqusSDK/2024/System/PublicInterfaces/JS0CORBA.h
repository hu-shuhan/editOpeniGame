#ifndef JS0CORBA_H_
#define JS0CORBA_H_

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0CORBA) || defined(__CATBBMagic)
# define ExportedByJS0CORBA   DSYExport
#else // __JS0CORBA || __CATBBMagic
# define ExportedByJS0CORBA   DSYImport
#endif  // __JS0CORBA || __CATBBMagic

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  // JS0CORBA_H_
