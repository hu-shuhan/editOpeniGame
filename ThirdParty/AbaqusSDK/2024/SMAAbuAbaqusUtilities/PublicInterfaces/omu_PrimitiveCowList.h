//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: omu_PrimitiveCowList.h
// 
// Description:
//   A tuple object that is supposed to support the same interface as the
//   python tuple object.
//     
//

#ifndef omu_PrimitiveCowList_h
#define omu_PrimitiveCowList_h

//
// Begin Local Includes
#include <omu_Primitive.h>
#include <cow_ArrayCOW.h>
#include <cls_xcList.h>

// typedefs
COW_ARRAYCOW2_DECL(omu_Primitive,cow_Virtual);
CLS_xcLIST_ITER_DECL(omu_PrimitiveCOW,omu_Primitive,omu_PrimitiveCowList);
CLS_xcLIST_DECL(omu_PrimitiveCOW,omu_Primitive,omu_PrimitiveCowList);


#endif  // #ifdef omu_PrimitiveCowList_h
