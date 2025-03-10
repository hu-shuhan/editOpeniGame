#ifndef JS0MT00_H
#define JS0MT00_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000
#include "JS0MT.h"

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0MT00)
# define ExportedByJS0MT00  DSYExport
#else  // __JS0MT00
# define ExportedByJS0MT00  DSYImport
#endif // __JS0MT00

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#if defined(DSYINSTARCH) || defined (__CATSysDemon) /* Utilisation archive */
#undef  ExportedByJS0MT00  
#define ExportedByJS0MT00  
#endif

#endif
