#ifndef _TypeDesc_h
#define _TypeDesc_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

#ifndef CATMAINWIN

typedef struct  tagTYPEDESC
    {
    VARTYPE vt;
   union 
        {
       struct tagTYPEDESC *lptdesc;
       struct tagARRAYDESC *lpadesc;
       HREFTYPE hreftype;
        }	;
    }	TYPEDESC;

#else /* CATMAINWIN */

typedef struct tagTYPEDESC {
 union {
   struct tagTYPEDESC *lptdesc;
   struct tagARRAYDESC *lpadesc;
   HREFTYPE hreftype;
 } ;
 VARTYPE vt;
} TYPEDESC;

#endif /* CATMAINWIN */

#endif
#endif
