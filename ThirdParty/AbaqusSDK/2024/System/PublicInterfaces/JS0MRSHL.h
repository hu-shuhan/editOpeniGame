#ifndef JS0MRSHL_H
#define JS0MRSHL_H

/*===========================================================================*/
/* COPYRIGHT DASSAULT SYSTEMES 1996                                          */
/*===========================================================================*/
/*                                                                           */
/*  JS0MRSHL                                                                 */
/*                                                                           */
/*  Usage Notes:                                                             */
/*  For Windows NT                                                           */
/*===========================================================================*/
/*  Creation September 1996                                                  */
/*===========================================================================*/
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0MRSHL)
# define ExportedByJS0MRSHL   DSYExport
#else // __JS0MRSHL
# define ExportedByJS0MRSHL   DSYImport
#endif  // __JS0MRSHL

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  // JS0MRSHL_H
