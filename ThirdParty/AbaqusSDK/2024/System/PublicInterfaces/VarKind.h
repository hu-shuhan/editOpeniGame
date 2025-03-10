#ifndef _VarKind_h
#define _VarKind_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

typedef enum tagVARKIND {	
	VAR_PERINSTANCE,
	VAR_STATIC,
	VAR_CONST,
	VAR_DISPATCH
}	VARKIND;

#endif /* _WINDOWS_SOURCE */

#endif
