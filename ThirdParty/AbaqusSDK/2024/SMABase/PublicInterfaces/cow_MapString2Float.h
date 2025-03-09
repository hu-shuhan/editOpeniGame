//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cow_MapString2Float_h
#define cow_MapString2Float_h

// Begin local includes

#include <cow_Map.h>
#include <cow_MapUtils.h>
#include <omi_types.h>
#include <cow_String.h>
#include <mem_C_Allocation.h>
#include <cow_ListFloat.h>
#include <cow_ListString.h>

COW_MAP_ITER_DECL(cow_String,float,cow_MapString2Float);
COW_MAPUTILS_DECL(cow_String,float);
typedef cow_ConstMapSortedIt<cow_String,float> cow_MapString2FloatSIT ;

//
// Allocator Class definition
//
class cow_MapString2F_allocator 
{
public:
  cow_MapString2F_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};

#endif // cow_MapString2Float_h


