//-*- mode: c++ -*-//
//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasCowAllocator_h
#define SMABasCowAllocator_h

#include <stddef.h>

// Begin local includes
#include <mem_Definitions.h>
#include <SMABasMemPoolCow.h>
#include <mem_AllocationOperatorsTiny.h>
#include <mem_C_Allocation.h>
// End local inclues

// Forward declarations
// namespace std { struct nothrow_t; }
// #define ABQ_NOTHROW std::nothrow_t

extern void* mem_OperatorNewThrow(size_t);
extern void* mem_OperatorNewNoThrow(size_t, const HKS_NOTHROW&);
extern void  mem_OperatorDelete(void* ptr);

extern "C" void mem_Free(void*);

/// Regular Allocator for COW containers -- used by default in release builds

class SMABasCowAllocator 
{
public:
  SMABasCowAllocator() {} 

  static void* allocate(size_t __n) {  
	return mem_Malloc_Handled(__n); 
  }
  static void  deallocate(void* __p, size_t __n) {  
	mem_Free(__p);
  }

  static void* operator new(size_t sz) {   return mem_OperatorNewThrow(sz);   }
  static void* operator new[](size_t sz) { return mem_OperatorNewThrow(sz);   }

  // In-place new
  static void* operator new(size_t sz, void* ptr) {return ptr;}
  static void* operator new[](size_t sz, void* ptr) {return ptr;}

  static void operator delete(void* ptr) {     mem_Free(ptr);    }
  static void operator delete(void* ptr, void*) { }
  static void operator delete[](void* ptr) {   mem_Free(ptr);    }
  static void operator delete[](void* ptr, void*) { }
};


/// Tracking Allocator for COW containers
///
/// This is a drop-in replacement allocator which privides:
///   1) pool-based allocations which tie into the SIMULIA SRM system;
///   2) memory tracking and reporting (for debugging and scalability studies).
///
/// On failure to allocate, an exception of type HKS_BAD_ALLOC (== std::bad_alloc) is thrown.

class SMABasCowTrackingAllocator: public SMABasCowAllocator
{
public:
   SMABasCowTrackingAllocator(); 
  ~SMABasCowTrackingAllocator();
  
   SMABasCowTrackingAllocator& operator=( const SMABasCowTrackingAllocator& other);


   void*  allocate(size_t nbytes) const;
   void   deallocate(void* ptr) const;
   void   deallocate(void* ptr, size_t nbytes) const;

   size_t MemoryCurrent();    /// Return the current memory total, in bytes.
   size_t MemoryPeak();       /// Return the historic memory peak, in bytes.

   int    PoolID();     /// Return the ID of this pool.
   char*  PoolName();   /// Return the Name of this pool.

private:

   SMABasMemPoolCow&  m_pool; 
};


#endif



