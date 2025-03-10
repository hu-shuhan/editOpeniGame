#ifndef _Excepinfo_h
#define _Excepinfo_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

#include "CATBSTR.h"

typedef struct tagEXCEPINFO {
  unsigned short wCode;	       // An error code describing the error.
  unsigned short wReserved;    // Reserved
  CATBSTR bstrSource;          // Source of the exception.
  CATBSTR bstrDescription;     // Textual description of the error.
  CATBSTR bstrHelpFile;	       // Help file path.
  DWORD dwHelpContext; // Help context ID.
  void * pvReserved;           // Pointer to function that fills in Help and description info.
  HRESULT (* pfnDeferredFillIn) (struct tagEXCEPINFO *);
  HRESULT scode;               // A return value describing the error.
} EXCEPINFO;

#endif /* _WINDOWS_SOURCE */

#endif
