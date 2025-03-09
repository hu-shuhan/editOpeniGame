#ifndef JS0LIB_INCLUDE
#define JS0LIB_INCLUDE

/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


/* COPYRIGHT DASSAULT SYSTEMES 2000 */

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#ifndef CATSYSV5_SYSTEMCUT
# if defined(__JS0LIB0) || defined(__JS0LIBSTDLIB)
#   define ExportedByJS0LIB   DSYExport
# else  /* __JS0LIB0 || __JS0LIBSTDLIB */
#   define ExportedByJS0LIB   DSYImport
# endif /* __JS0LIB0 || __JS0LIBSTDLIB */
#else /* CATSYSV5_SYSTEMCUT */
# if defined(__JS0LIB0)
#   define ExportedByJS0LIB   DSYExport
# else  /* __JS0LIB0 */
#   define ExportedByJS0LIB   DSYImport
# endif /* __JS0LIB0 */
#endif  /* CATSYSV5_SYSTEMCUT */

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  /* JS0LIB_INCLUDE */
