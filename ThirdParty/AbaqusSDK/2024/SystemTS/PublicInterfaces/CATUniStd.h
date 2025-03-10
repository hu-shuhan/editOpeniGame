#ifndef CATUniStd_H
#define CATUniStd_H
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


/* COPYRIGHT DASSAULT SYSTEMES 2000 */

/* ====================================================================== */
/* == SOME CONSTANTS DEFINITION TO PROVIDE MINIMUM UNIX/NT SOURCE      == */
/* == COMPATIBILITY                                                    == */
/* ====================================================================== */
#ifdef _WINDOWS_SOURCE
#ifndef F_OK
#define F_OK    00
#define X_OK    01
#define W_OK    02
#define R_OK    04
#endif
#ifndef SIGHUP
#define SIGHUP  1
#endif
#include <io.h>
#else
#include <unistd.h>
#endif
#endif
