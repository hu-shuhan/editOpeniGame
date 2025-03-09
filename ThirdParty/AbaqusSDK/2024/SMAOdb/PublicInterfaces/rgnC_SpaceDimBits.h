//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// rgnC_SpaceDimBits.h

#ifndef rgnC_SpaceDimBits_h
#define rgnC_SpaceDimBits_h

// Begin local includes
#include <omi_types.h>

// bit-flags to indicate space info
typedef int                  rgnC_SpaceBits;
#define rgnC_UNKNOWN_SPACE  	(0)
#define rgnC_THREE_SPACE	(1<<0)
#define rgnC_TWO_SPACE		(1<<1)
#define rgnC_AXISYMM_SPACE	(1<<2)

// bit-flags to indicate dim info
typedef int                  rgnC_DimBits;
#define rgnC_UNKNOWN_DIM  	(0)
#define rgnC_THREE_DIM  	(1<<0)
#define rgnC_TWO_DIM		(1<<1)
#define rgnC_ONE_DIM    	(1<<2)
#define rgnC_ZERO_DIM           (1<<3)

#endif  // #ifdef rgnC_SpaceDimBits_h

