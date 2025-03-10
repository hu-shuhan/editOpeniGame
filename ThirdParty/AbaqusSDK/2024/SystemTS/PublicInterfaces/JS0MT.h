#ifndef JS0MT_H
#define JS0MT_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0MT)
# define ExportedByJS0MT  DSYExport
#else  // __JS0MT
# define ExportedByJS0MT  DSYImport
#endif // __JS0MT

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#ifdef DSYINSTARCH /* Utilisation archive */
#undef  ExportedByJS0MT  
#define ExportedByJS0MT  
#endif

#endif
