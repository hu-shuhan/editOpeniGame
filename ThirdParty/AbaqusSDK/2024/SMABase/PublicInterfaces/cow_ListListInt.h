//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef cow_ListListInt_h
#define cow_ListListInt_h

// Begin local includes
#include <cow_ListInt.h>
#include <mem_C_Allocation.h>

COW_LIST_DECL(cow_ListInt, cow_ListListInt);

//
// Allocator Class definition
//
class cow_ListListI_allocator 
{
public:
  cow_ListListI_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};

#endif  // #ifdef cow_ListListInt_h


