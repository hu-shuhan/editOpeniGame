//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cow_ListInt_h
#define cow_ListInt_h

// Begin local includes

#include <cow_List.h>
#include <mem_C_Allocation.h>

COW_LIST_ITER_BLT_DECL(int, cow_ListInt);

//
// Allocator Class definition
//
class cow_ListInt_allocator 
{
public:
  cow_ListInt_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};

#endif // cow_ListInt_h



