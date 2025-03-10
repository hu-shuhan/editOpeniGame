#ifndef _IdlDesc_h
#define _IdlDesc_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

typedef struct tagIDLDESC {
 unsigned long dwReserved;
 unsigned short wIDLFlags;
} IDLDESC;


#endif /* _WINDOWS_SOURCE */

#endif
