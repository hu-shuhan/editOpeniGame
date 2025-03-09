//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */

#ifndef cow_ListFloat_h
#define cow_ListFloat_h

//
// Includes
//

// Begin local includes

#include <cow_List.h>
#include <mem_C_Allocation.h>

COW_LIST_ITER_BLT_DECL(float,cow_ListFloat);

//
// Allocator Class definition
//
class cow_ListFloat_allocator 
{
public:
  cow_ListFloat_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};

#endif  // #ifdef cow_ListFloat_h



