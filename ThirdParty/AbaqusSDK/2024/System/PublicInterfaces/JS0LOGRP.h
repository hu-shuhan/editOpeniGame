#ifndef _JS0LOGRP_H
#define _JS0LOGRP_H

// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/************************************************************************/
/* Defines                                                              */
/************************************************************************/
#if defined(__JS0LOGRP)
# define ExportedByJS0LOGRP   DSYExport
#else // __JS0LOGRP
# define ExportedByJS0LOGRP   DSYImport
#endif  // __JS0LOGRP

/************************************************************************/
/* Local includes                                                       */
/************************************************************************/
#include "DSYExport.h"

#endif  // _JS0LOGRP_H
