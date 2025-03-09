//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// omu_VarrayPtr.h
// Variable-length arrays of pointers.

#if !defined(omu_VarrayPtr_h)
#define omu_VarrayPtr_h

// Begin local includes
#include <omi_types.h>
#include <omu_types.h>
#include <omu_Varray.h>

// For now: obsolescent; use omu_Ptr instead.
typedef omu_Ptr Ptr;

typedef omu_Varray<omu_Ptr> omu_VarrayPtr;

#endif /* omu_VarrayPtr_h */
