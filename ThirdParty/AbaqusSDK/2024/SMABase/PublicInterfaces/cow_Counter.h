//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef cow_Counter_h
#define cow_Counter_h

#include <stddef.h>  // for size_t

// cow_Counter is a private data structure which is maintained by the 
// cow_CountedHld class template. It holds a reference count.

// Begin local includes
#include <mem_AllocationOperatorsTiny.h>

class cow_Counter  : public mem_AllocationOperatorsTiny
{
    int references;

public:
    cow_Counter() : references(1) {}
    int References() const { return references; }
    void Increment() { references++; }

    // The Decrement method returns a bool to indicate whether the referent
    // and cow_Counter should be deleted. A true value is returned if the
    // the reference count is illegal (negative), unless NDEBUG is set, on
    // the assumption that an out-of-line Delete method will assert that
    // the value is legal.
    //
    bool Decrement() {
        --references;
#ifdef NDEBUG
        return references == 0;
#else
        return references <= 0;
#endif
    }

    // Since we allow derivation from this class, it may participate
    // in copy and assignment. These MUST be no-ops for this class,
    // so they MUST be implemented as below.
    //
    cow_Counter(const cow_Counter&) : references(1) {}
    const cow_Counter& operator=(const cow_Counter&) const { return *this; }

    // The following methods initialize/finalize the private allocator.
    static void Initialize();
    static void Finalize();
};

#endif // cow_Counter_h
