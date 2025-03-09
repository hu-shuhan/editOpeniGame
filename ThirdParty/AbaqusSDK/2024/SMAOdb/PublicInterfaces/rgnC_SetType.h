//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// rgnC_SetType.h

#ifndef rgnC_SetType_h
#define rgnC_SetType_h

// Begin local includes
#include <omu_PrimEnum.h>

// Enumeration to specify the type of filter specified the procedures using  
// the region selection step
enum rgnC_SetType {
    rgnC_SET_GEOM,   // Geometry Set
    rgnC_SET_NODE,   // Node Set
    rgnC_SET_ELEM,   // Element Set
    rgnC_SURF_GEOM,  // Geometric Surface
    rgnC_SURF_MESH,   // Mesh Surface
    rgnC_NONE       // Invalid type
};

// bit-flags to indicate contents of set
typedef int                  rgnC_SetTypeBits;
#define rgnC_UNKNOWN_SET_TYPE  	        (0)
#define rgnC_GEOMETRY_SET_TYPE  	(1<<0)
#define rgnC_ELEMENT_SET_TYPE      	(1<<1)
#define rgnC_NODE_SET_TYPE    	        (1<<2)
// next indicates that set is a sided-set, i.e. is a surface
#define rgnC_SURF_SET_TYPE    	        (1<<3)
#define rgnC_SKIN_SET_TYPE              (1<<4)
#define rgnC_STRINGER_SET_TYPE          (1<<5)


typedef omu_PrimEnum<rgnC_SetType,1> rgnC_SetTypeConst;

#endif  // #ifdef rgnC_SetType_h
