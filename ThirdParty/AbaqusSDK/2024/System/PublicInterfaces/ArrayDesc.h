// COPYRIGHT DASSAULT SYSTEMES 2000
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/
#ifndef ArrayDesc_h
#define ArrayDesc_h

#ifndef _WINDOWS_SOURCE

typedef struct tagSAFEARRAYBOUND {
  ULONG cElements;
  LONG lLbound;
} SAFEARRAYBOUND;

typedef struct tagARRAYDESC {
  TYPEDESC tdescElem;
  unsigned short cDims;
  SAFEARRAYBOUND rgbounds[ 1 ];
} ARRAYDESC;

#endif /* _WINDOWS_SOURCE */

#endif
