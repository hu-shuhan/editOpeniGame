//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */

#ifndef cow_ListBool_h
#define cow_ListBool_h

//
// Includes
//

// Begin local includes

#include <cow_List.h>
#include <mem_C_Allocation.h>

COW_LIST_ITER_BLT_DECL(bool,cow_ListBool);

//
// Allocator Class definition
//
class cow_ListBool_allocator 
{
public:
  cow_ListBool_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};


#endif  // #ifdef cow_ListBool_h

