#ifndef _CATSysTS_H
#define _CATSysTS_H

/* COPYRIGHT DASSAULT SYSTEMES 2000 */
/** @CAA2Required */
/************************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS   */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME   */
/************************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__CATSysTS) || defined(__CATBBMagic)
# define ExportedByCATSysTS   DSYExport
#else  /* __CATSysTS || __CATBBMagic */
# define ExportedByCATSysTS   DSYImport
#endif /* __CATSysTS || __CATBBMagic */

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

/* Utilisation archive */
#if defined(DSYINSTARCH) || defined (HTTPARCH) || defined(__mxUtil)
#undef  ExportedByCATSysTS
#define ExportedByCATSysTS
#endif

#endif  /* _CATSysTS_H */
