//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef SMABasOmiDefaultAllocator_h
#define SMABasOmiDefaultAllocator_h

#include <string.h> // for memset

// Begin local includes
#include <mem_C_Allocation.h>
// End  local includes

template <typename T>
class SMABasOmiAllocator: public mem_AllocationOperators
{
 public:

    typedef       T              value_type;

  	typedef       value_type*    pointer;
	typedef const value_type*    const_pointer;

	typedef       value_type&    reference;
	typedef const value_type&    const_reference;

	typedef            size_t    size_type;


   SMABasOmiAllocator() throw() { }

   SMABasOmiAllocator(const SMABasOmiAllocator& a) throw() { }

   template<typename T2>
   SMABasOmiAllocator(const SMABasOmiAllocator<T2>& a) throw() { }


  ~SMABasOmiAllocator() { }

   template<typename T2>
   struct rebind { 
   	  typedef SMABasOmiAllocator<T2> other; 
   };

   void deallocate(pointer ptr)
   {	
	  // ::operator delete(_Ptr);
	  mem_Free(ptr);
   }

   void deallocate(pointer ptr, size_type sz)
   {	
	  // ::operator delete(_Ptr);
	  mem_Free(ptr);
   }

   pointer allocate(size_type count)
   {
	  void* p = mem_Malloc(count*sizeof(T));
	  return pointer(p) ;
   }

   void construct(pointer ptr, const T& val) 
   { 
	  ::new(ptr) T(val); 
   }

   void destroy(pointer ptr) { ptr->~T(); }

};



class SMABasOmiDefaultAllocator
{
public:
   SMABasOmiDefaultAllocator() { }
  ~SMABasOmiDefaultAllocator() { }

  static void* allocate (size_t bytes) 
  {
      void* p = mem_Malloc(bytes);
	  memset(p, 0, bytes);
      return p ;
   }

  static void deallocate(void* ptr)
  {	
	  mem_Free(ptr);
  }

};


#endif // SMABasOmiDefaultAllocator_h
