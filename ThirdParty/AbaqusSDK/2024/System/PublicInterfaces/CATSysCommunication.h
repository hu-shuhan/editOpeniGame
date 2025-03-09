#ifndef CATSysCommunication_H
#define CATSysCommunication_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


// COPYRIGHT DASSAULT SYSTEMES 2000

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__CATSysCommunication)
# define ExportedByCATSysCommunication  DSYExport
#else // __CATSysCommunication
# define ExportedByCATSysCommunication  DSYImport
#endif  // __CATSysCommunication

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif
