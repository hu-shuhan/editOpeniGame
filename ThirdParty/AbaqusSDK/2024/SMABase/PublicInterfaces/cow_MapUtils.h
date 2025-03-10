//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: cow_MapUtils.h
// 
// Description:
//      utility functions that can be used with the cow_Map template.
//      this is included as function templates rather than member of the
//      map to reduce dependencies
//
//      Functions:
//          cow_List<key>   cow_Keys(const cow_Map<key,value> &);
//          cow_List<value> cow_Values(const cow_Map<key,value> &);
//     
//


#ifndef cow_MapUtils_h
#define cow_MapUtils_h

//
// Forward declarations
//
template<class Key, class Value> class cow_Map;

//
// Begin Local Includes
//
#include <mem_AllocationOperators.h>
#include <cow_List.h>
#include <mem_C_Allocation.h>

//
// Allocator Class definition
//
class cow_MapUtils_allocator 
{
public:
  cow_MapUtils_allocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p) {  
	mem_Free(__p);
  }
};

template<class Key, class Value>
class cow_MapUtils  : public mem_AllocationOperators
{
public:
    static cow_List<Key> cow_Keys(const cow_Map<Key,Value> &map);

    static cow_List<Value> cow_Values(const cow_Map<Key,Value> &map);
};

// The DECL macro must be used to disable implicit instantiation of this
// instance. More importantly, the .cpp file must include the instance
// declaration files of the return value (cow_List) instances to avoid
// a clash of implicit and explicit instantiation that will cause link
// errors.

#define COW_MAPUTILS_FWDL(Key, Value)

#ifdef SMA_NO_TGEN
#define COW_MAPUTILS_EXTL(Key,Value) \
extern template class cow_MapUtils<Key, Value>;
#else
#define COW_MAPUTILS_EXTL(Key,Value)
#endif

#define COW_MAPUTILS_DECL(Key, Value) \
COW_MAPUTILS_FWDL(Key, Value) \
COW_MAPUTILS_EXTL(Key, Value)

//
// Macro for instantiating the cow_MapUtils this goes into the .C file,
// don't forget to include the cow_MapUtils.T
//

#define COW_MAPUTILS_IMPL(Key,Value) \
template class cow_MapUtils<Key, Value>;


#endif  // #ifdef cow_MapUtils_h
