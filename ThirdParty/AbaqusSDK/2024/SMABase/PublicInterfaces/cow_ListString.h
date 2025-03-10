//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cow_ListString_h
#define cow_ListString_h

// Begin local includes

#include <cow_List.h>
#include <cow_String.h>
#include <mem_C_Allocation.h>

#ifdef SMA_NO_TGEN
template <> int cow_List<cow_String>::SortCmp(const void* a, const void* b);
#endif
COW_LIST_ITER_DECL(cow_String, cow_ListString);


// In the future the following two methods will exits as members of 
// the cow_ListString class
// At that time the methods should be declared const

unsigned GetLengthShortestString( const cow_ListString & );
    // Determine the number of characters in the shortest string in the
    // cow_ListString
    // 0 if the list has no strings

unsigned GetLengthLongestString( const cow_ListString & );
    // Determine the number of characters in the longest string in the
    // cow_ListString


//
// Allocator Class definition
//
class cow_ListS_allocator 
{
public:
  cow_ListS_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};


#endif // cow_ListString_h



