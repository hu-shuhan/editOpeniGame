//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef thr_GlobalSingletonBase_h
#define thr_GlobalSingletonBase_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <thr_ThreadingModel.h>

// This file defines an abstract base class template. Do not instantiate
// this template directly. Use either the thr_GlobalSingleton derivative.


//---------------------------------------------------------------------
// THREADS IMPLEMENTATION
//---------------------------------------------------------------------
#if !defined HKS_NOTHREADS

class thr_Mutex;

template <class Class> class thr_GlobalSingletonBase  : public mem_AllocationOperators
{
    static Class*      instance;
    static thr_Mutex*  lock;
     
protected:
    // The constructor enforces that at most one instance is created.
    //
    thr_GlobalSingletonBase();
    
    // This Instance method is overridden in the derivative templates,
    // and return type is change from a pointer to a reference.
    //
    static Class* Instance();
    static void CreateSingletonLocks();
    
public:
    // The Finalize method should be called from within its
    // fac_initialize.C file.
    static void Finalize();
    static void LockBase();
    static void UnlockBase();
};

//-----------------------------------------------------------------
// NOTHREADS IMPLEMENTATION
//-----------------------------------------------------------------
#else //  HKS_NOTHREADS

// The following is for machines with no threads. This is provided
// to avoid forcing if def's for all threaded code. This singleton
// will behave like an omi_Singleton.

template <class Class> class thr_GlobalSingletonBase  : public mem_AllocationOperators
{
    static Class* instance;

protected:
    // The constructor enforces that at most one instance is created.
    //
    thr_GlobalSingletonBase();
    
    // This Instance method is overridden in the derivative templates,
    // and return type is change from a pointer to a reference.
    //
    static Class* Instance() { return instance; }
    static void CreateSingletonLocks() { }
    
public:
    // The Finalize method should be called from within its
    // fac_initialize.C file.
    //
    static void Finalize();
    static void LockBase()   { }
    static void UnlockBase() { }
};

#endif

#endif /* thr_GlobalSingletonBase_h */
