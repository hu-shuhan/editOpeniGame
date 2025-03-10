//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef omi_SingletonBase_h
#define omi_SingletonBase_h

// This file defines an abstract base class template. Do not instantiate
// this template directly. Use either the omi_Singleton derivative, if
// possible, or the omi_SingletonExplicit derivative, if necessary.

// Begin local includes
#include <mem_AllocationOperators.h>
// End local includes

template <class Class> 
class omi_SingletonBase : public mem_AllocationOperators
{
    static Class* instance;

    // The Delete method must be specialized in the same location as
    // the instance.
    //
    static void Delete();

 protected:
    // The constructor enforces that at most one instance is created.
    //
    omi_SingletonBase();

    // This Instance method is overridden in the derivative templates,
    // and return type is change from a pointer to a reference.
    //
    static Class* Instance();

public:
    // The Finalize method should be called from within its
    // fac_initialize.C file.
    //
    static void Finalize();
};

#endif /* omi_SingletonBase_h */
