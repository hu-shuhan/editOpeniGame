//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef rgnC_ResEnum_h
#define rgnC_ResEnum_h

// Begin local includes
#include <omu_PrimEnum.h>

enum res_SetTypeEnm {
     res_SETTYPE_NODE,
     res_SETTYPE_ELEMENT,
     res_SETTYPE_SURFACE,
     res_SETTYPE_SURFACE_ELEMENT,
     res_SETTYPE_SURFACE_FACET,
     res_SETTYPE_SURFACE_EDGE,
     res_SETTYPE_SURFACE_NODE,
     res_dummy, // feel free to use for something. Needed a blank space
                // to correct an issue with a misplaced enum 
     res_SETTYPE_SURFACE_ANALYTICAL,
     res_NUM_SET_TYPE // Counter. Add all new types before
};

typedef omu_PrimEnum<res_SetTypeEnm,1> res_SetTypeEnmPE;

#endif

