//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cow_ListTranslatedString_h
#define cow_ListTranslatedString_h

// Begin local includes

#include <cow_List.h>
#include <atr_String.h>
#include <mem_C_Allocation.h>

COW_LIST_ITER_DECL(atr_String, cow_ListTranslatedString);

//
// Allocator Class definition
//
class cow_ListTranslatedString_allocator 
{
public:
  cow_ListTranslatedString_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};

#endif // cow_ListString_h
