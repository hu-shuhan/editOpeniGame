#ifndef cow_ListUint_h
#define cow_ListUint_h

// Begin local includes
#include <omi_types.h>
#include <cow_List.h>
#include <mem_C_Allocation.h>

COW_LIST_ITER_BLT_DECL(uint, cow_ListUint);


//
// Allocator Class definition
//
class cow_ListUI_allocator 
{
public:
  cow_ListUI_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};

#endif // cow_ListUint_h


