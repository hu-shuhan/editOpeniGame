//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cow_MapUint2Uint_h
#define cow_MapUint2Uint_h

// Begin local includes

#include <cow_Map.h>
#include <omi_types.h>
#include <mem_C_Allocation.h>

COW_MAP_ITER_DECL(uint,uint,cow_MapUint2Uint);

//
// Allocator Class definition
//
class cow_MapUint2UI_allocator 
{
public:
  cow_MapUint2UI_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};

#endif // cow_MapUint2Uint_h


