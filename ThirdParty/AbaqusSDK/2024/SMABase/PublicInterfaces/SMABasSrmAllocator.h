//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// -*- mode: c++ -*- //
#ifndef SMABasSrmAllocator_h
#define SMABasSrmAllocator_h

#include <string.h>
#include <stdlib.h>

// Begin local includes
#include <mem_C_Allocation.h>
#include <mem_AllocationOperators.h>
#include <SMABasSrmResourceEnums.h>
#include <SMABasSrmForwarder.h>
// End local includes


template <enum srm_ResourceEnums srmResourceEnum> class  SMABasSrmAllocator;

// template<> class  SMABasSrmAllocator<e_res_sharedOperator>; // specialization
// template<> class  SMABasSrmAllocator<e_res_sharedSolution>; // specialization

///
/// An Allocator that performs all allocations/deallocations through an SRM Pool.
/// An Enum that identifies the SRM Pool to be used is passed as a template parameter.
///
/// m_pool is assigned in the Initialize() function

template <enum srm_ResourceEnums srmResourceEnum>
class  SMABasSrmAllocator : public mem_AllocationOperators
{
 public:
   SMABasSrmAllocator() { }
  ~SMABasSrmAllocator() { }

  static void* allocate (const size_t bytes) 
  {
    SMABasCamPool* pool =  GetPool();
    if (pool) {
        void* p = pool->Alloc(bytes);
        memset(p, 0, bytes);
        return p;
    }else{
        void* p = mem_Malloc(bytes);
        memset(p, 0, bytes);
        return p ;
    }
  }

  static void deallocate(void* ptr)
  { 
    SMABasCamPool* pool =  GetPool();
    if ( pool ) {
        pool->Free(ptr);
    }else{              
        mem_Free(ptr);
    }
  }

  static void Initialize();
  static void Finalize();

  // Debug Scaffolding

  static void            SetPool(SMABasCamPool* pool);
  static SMABasCamPool*  GetPool();

  void DebugPrint() const;

private:
  static SMABasCamPool* m_pool;

};


// Template instantiation macro

#define SRM_ALLOCATOR_IMPL(RESOURCE_ENUM)                               \
  template <> SMABasCamPool* SMABasSrmAllocator<RESOURCE_ENUM>::m_pool = 0;   \
  template <> void  SMABasSrmAllocator<RESOURCE_ENUM>::SetPool(SMABasCamPool* pool) { m_pool = pool; }  \
  template <> SMABasCamPool* SMABasSrmAllocator<RESOURCE_ENUM>::GetPool() {  return m_pool; } \
  template class SMABasSrmAllocator<RESOURCE_ENUM>; 



#endif // SMABasSrmAllocator_h
