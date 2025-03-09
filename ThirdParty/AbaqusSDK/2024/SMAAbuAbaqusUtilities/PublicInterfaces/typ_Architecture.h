//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef typ_Architecture_h
#define typ_Architecture_h

// Begin local includes

#include <typ_ArchCompTable.h>

#define TYP_LIB		(0 << 8)
#define TYP_ROLE	(1 << 8)
#define TYP_MODULE	(2 << 8)
#define TYP_MOD_LIB     (3 << 8)

//
// 4 << 8; which is one beyond the last TYP_MOD_LIB entry allowed
// in typ_Architecture.txt
//

#define TYP_MAX_NUMBER_FACILITIES  (4 << 8)



#define TYP_MODULE_COM 0
#define TYP_MODULE_KER 1
#define TYP_MODULE_GUI 2
#define TYP_MODULE_FTR 3

#define TYP_ROLE_COM 0
#define TYP_ROLE_KER 1
#define TYP_ROLE_GUI 2


/////////////////////////////////////////////////////////////////////////
//
//	These two are handled specially.
//

#define TYP_FAC_typ	(TYP_LIB | 0x0000)	// 0
#define TYP_FAC_omni	(TYP_LIB | 0x0001)	// 1


#endif  // typ_Architecture_h
