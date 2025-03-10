//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef cow_ListListDouble_h
#define cow_ListListDouble_h

// Begin local includes
#include <cow_ListDouble.h>
#include <mem_C_Allocation.h>

COW_LIST_DECL(cow_ListDouble, cow_ListListDouble);

//
// Allocator Class definition
//
class cow_ListListD_allocator 
{
public:
  cow_ListListD_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};

#endif  // #ifdef cow_ListListDouble_h


