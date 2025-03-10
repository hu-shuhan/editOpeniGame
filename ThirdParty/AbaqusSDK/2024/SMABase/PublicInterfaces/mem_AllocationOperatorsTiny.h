//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef MEM_AllocationOPERatorsTIny_h
#define MEM_AllocationOPERatorsTIny_h

// The class in this file implements the new and delete operators for all small
// ABAQUS classes with sizeof(class) <= 129. All ABAQUS classes with 
// sizeof(class) <= 129 must derive from mem_AllocationOperatorsTiny.
//
// Any classes with sizeof(class) > 128 must derive from mem_AllocationOperators.
//
// This class is implemented such that we get interface inheritance.
// Do NOT change this file without consulting with Dennis Flanagan.
//
// Allocations for built-in types need to use interfaces defined in 
// mem_AllocationOperatorsBuiltIn.h
//
// Declarations and implementation follow C++ Standard ISO/IEC 14882:1998(E)
// Applicaple sections 3.7.3, 5.3.4, 5.3.5, 12.5, 17.4.3.4, 18.4 

#include <stddef.h>  // For size_t

// Begin local includes
#include <mem_Definitions.h>
// End local includes

// Forward declarations

// namespace std { struct nothrow_t; }
// #define ABQ_NOTHROW std::nothrow_t

extern void* mem_OperatorNewTinyThrow(size_t);
extern void* mem_OperatorNewTinyNoThrow(size_t size, const HKS_NOTHROW& noThr);
extern void  mem_OperatorDeleteTiny(void* ptr);

extern "C" void mem_Free(void*);



class mem_AllocationOperatorsTiny
{
public:
    // Plain operator new. 
    // On failure to allocate, it will transfer control to the ABAQUS memory error handler.
    // If handler is not registered, it will throw an std::bad_alloc exception.

    static void* operator new(size_t sz) {
        return mem_OperatorNewTinyThrow(sz);
    }

    // Array new. 
    // When out of memory, will throw an std::bad_alloc exception or transfer control to the 
    // ABAQUS memory handler.
    // Note that we will return memory alligned according to the underlying operator alignment here.
    // There are a number of chances where the alignment can get messed up
    // 1.) The compiler can do the wrong thing
    // 2.) The user can replace the underlying allocator which can get the alignment wrong.
    // The default implementation returns 32-byte aligned memory, as required for the AVX instructions.

    static void* operator new[](size_t sz) {
        return mem_OperatorNewTinyThrow(sz);
    }

    // Plain operator new, will not throw on out of memory
    static void* operator new(size_t sz, const HKS_NOTHROW& noThr) throw() {
      return mem_OperatorNewTinyNoThrow(sz,noThr);
    }

    // Array new, will not throw on out of memory
    static void* operator new[](size_t sz, const HKS_NOTHROW& noThr) throw() { 
      return mem_OperatorNewTinyNoThrow(sz,noThr); 
    }

#if defined(SMA_TRACK_OPERATOR_NEW)
    // Plain delete
    static void operator delete(void* ptr) { mem_OperatorDeleteTiny(ptr); }
    static void operator delete(void* ptr, void*) { }
    // Array delete
    static void operator delete[](void* ptr) { mem_OperatorDeleteTiny(ptr); }
    static void operator delete[](void* ptr, void*) { }
#else
    // Plain delete
    static void operator delete(void* ptr) { mem_Free(ptr); }
    static void operator delete(void* ptr, void*) { }
    // Array delete
    static void operator delete[](void* ptr) { mem_Free(ptr); }
    static void operator delete[](void* ptr, void*) { }
#endif

    // In-place new
    static void* operator new  (size_t sz, void* ptr) { return ptr; }
    static void* operator new[](size_t sz, void* ptr) { return ptr; }
};

#endif  /* MEM_AllocationOPERatorsTIny_h */
