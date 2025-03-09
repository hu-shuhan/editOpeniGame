#ifndef _ParamDesc_h
#define _ParamDesc_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

// COPYRIGHT DASSAULT SYSTEMES 2000

#ifndef _WINDOWS_SOURCE

typedef struct tagPARAMDESCEX *LPPARAMDESCEX;

typedef struct tagPARAMDESC {
 LPPARAMDESCEX pparamdescex;
 unsigned short wParamFlags;
} PARAMDESC;

#define	PARAMFLAG_NONE	( 0 )
#define	PARAMFLAG_FIN	( 0x1 )
#define	PARAMFLAG_FOUT	( 0x2 )
#define	PARAMFLAG_FLCID	( 0x4 )
#define	PARAMFLAG_FRETVAL	( 0x8 )
#define	PARAMFLAG_FOPT	( 0x10 )
#define	PARAMFLAG_FHASDEFAULT	( 0x20 )

#endif /* _WINDOWS_SOURCE */

#endif
